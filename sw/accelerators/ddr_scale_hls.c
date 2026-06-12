#include "../alveo.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*
 * DDR scaling HLS accelerator tutorial for the Alveo XDMA + HBICAP template.
 *
 * Expected hardware inside the configured A1 region:
 *
 *   - ddr_scale_accel HLS IP connected to the AXI-Lite control path.
 *   - ddr_scale_accel/m_axi_gmem connected to the shell-side DDR/full-AXI path.
 *
 * HLS function implemented in hardware:
 *
 *   void ddr_scale_accel(const uint32_t *in_ddr,
 *                        uint32_t *out_ddr,
 *                        uint32_t length,
 *                        uint32_t scale)
 *   {
 *       for (uint32_t i = 0; i < length; ++i)
 *           out_ddr[i] = in_ddr[i] * scale;
 *   }
 *
 * This tutorial demonstrates how to:
 *
 *   1. Reconfigure the A1 region from a .bin bitstream.
 *   2. Move input/output buffers through XDMA using alveo_write()/alveo_read().
 *   3. Program and start the HLS IP through AXI-Lite registers.
 *   4. Poll ap_done and verify the scaled output buffer.
 *
 * To adapt this file to another HLS design, modify mainly:
 *
 *   - HLS_CTRL_BASE_ADDR
 *   - INPUT_DDR_ADDR / OUTPUT_DDR_ADDR
 *   - the HLS register offsets below, if your generated HLS driver reports
 *     different offsets.
 */

/* -------------------------------------------------------------------------- */
/* User design configuration                                                  */
/* -------------------------------------------------------------------------- */

/* AXI-Lite base address of ddr_scale_accel/s_axi_control in the Vivado map. */
#define HLS_CTRL_BASE_ADDR     0x42000000ULL

/*
 * Full-AXI/DDR addresses used by the HLS m_axi_gmem port.
 *
 * These addresses must be visible both to:
 *
 *   - the host XDMA data path used by alveo_write()/alveo_read(), and
 *   - the HLS IP m_axi_gmem master port.
 *
 * Keep them far enough apart so input and output buffers do not overlap.
 */
#define INPUT_DDR_ADDR         0x00000000ULL
#define OUTPUT_DDR_ADDR        0x00010000ULL

/* Default test size and scale factor. They can be overridden from argv. */
#define DEFAULT_TEST_WORDS     1024U
#define DEFAULT_SCALE_VALUE    7U

/* XDMA examples commonly use page-aligned host buffers. */
#define BUFFER_ALIGNMENT       4096U

/* Poll limit for waiting on HLS ap_done. */
#define HLS_POLL_MAX           100000000UL

/* -------------------------------------------------------------------------- */
/* ddr_scale_accel HLS AXI-Lite register map                                  */
/* -------------------------------------------------------------------------- */

/* Standard Vitis HLS ap_ctrl_hs control register. */
#define HLS_REG_AP_CTRL        0x00U
#define HLS_REG_GIE            0x04U
#define HLS_REG_IER            0x08U
#define HLS_REG_ISR            0x0CU

/* ap_ctrl bits. */
#define HLS_AP_START           0x00000001U
#define HLS_AP_DONE            0x00000002U
#define HLS_AP_IDLE            0x00000004U
#define HLS_AP_READY           0x00000008U

/*
 * Argument registers for this exact HLS top function.
 *
 * For the function:
 *
 *   ddr_scale_accel(in_ddr, out_ddr, length, scale)
 *
 * Vitis HLS normally assigns the following offsets:
 *
 *   0x10/0x14 : in_ddr  64-bit base address
 *   0x1c/0x20 : out_ddr 64-bit base address
 *   0x28      : length  32-bit number of uint32_t words
 *   0x30      : scale   32-bit multiplication factor
 *
 * If your HLS export generated a different xddr_scale_accel_hw.h, use that
 * generated header as the source of truth and update these constants.
 */
#define HLS_REG_IN_DDR         0x10U
#define HLS_REG_OUT_DDR        0x1CU
#define HLS_REG_LENGTH         0x28U
#define HLS_REG_SCALE          0x30U

/* -------------------------------------------------------------------------- */
/* Small AXI-Lite helper functions                                            */
/* -------------------------------------------------------------------------- */

static int hls_write32(uint32_t offset, uint32_t value)
{
    return alveo_reg_write32(HLS_CTRL_BASE_ADDR + offset, value);
}

static int hls_read32(uint32_t offset, uint32_t *value)
{
    return alveo_reg_read32(HLS_CTRL_BASE_ADDR + offset, value);
}

static int hls_write64(uint32_t offset, uint64_t value)
{
    if (hls_write32(offset, (uint32_t)(value & 0xffffffffULL)) < 0)
        return -1;

    if (hls_write32(offset + 4U, (uint32_t)(value >> 32)) < 0)
        return -1;

    return 0;
}

static int hls_start(void)
{
    return hls_write32(HLS_REG_AP_CTRL, HLS_AP_START);
}

static int hls_wait_done(void)
{
    for (unsigned long poll = 0; poll < HLS_POLL_MAX; ++poll) {
        uint32_t ctrl = 0;

        if (hls_read32(HLS_REG_AP_CTRL, &ctrl) < 0)
            return -1;

        if ((ctrl & HLS_AP_DONE) != 0U)
            return 0;
    }

    printf("ERROR: timeout while waiting for ddr_scale_accel ap_done.\n");
    return -1;
}

static int hls_configure(uint64_t in_addr,
                         uint64_t out_addr,
                         uint32_t length,
                         uint32_t scale)
{
    if (hls_write64(HLS_REG_IN_DDR, in_addr) < 0)
        return -1;

    if (hls_write64(HLS_REG_OUT_DDR, out_addr) < 0)
        return -1;

    if (hls_write32(HLS_REG_LENGTH, length) < 0)
        return -1;

    if (hls_write32(HLS_REG_SCALE, scale) < 0)
        return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */
/* DMA buffer and test-pattern helpers                                        */
/* -------------------------------------------------------------------------- */

static int allocate_dma_buffer(uint32_t **buffer, size_t bytes)
{
    int ret;

    ret = posix_memalign((void **)buffer, BUFFER_ALIGNMENT, bytes);
    if (ret != 0) {
        *buffer = NULL;
        return -1;
    }

    return 0;
}

static void fill_input_pattern(uint32_t *buffer, uint32_t words)
{
    for (uint32_t i = 0; i < words; ++i)
        buffer[i] = i + 1U;
}

static int verify_scaled_output(const uint32_t *input,
                                const uint32_t *output,
                                uint32_t words,
                                uint32_t scale)
{
    for (uint32_t i = 0; i < words; ++i) {
        const uint32_t expected = input[i] * scale;

        if (output[i] != expected) {
            printf("ERROR: mismatch at word %u: input 0x%08" PRIx32
                   ", scale %" PRIu32
                   ", expected 0x%08" PRIx32 ", got 0x%08" PRIx32 "\n",
                   i, input[i], scale, expected, output[i]);
            return -1;
        }
    }

    return 0;
}

static int parse_u32_arg(const char *text, uint32_t *value)
{
    char *end = NULL;
    unsigned long parsed;

    if (text == NULL || value == NULL)
        return -1;

    parsed = strtoul(text, &end, 0);
    if (*text == '\0' || *end != '\0' || parsed > 0xffffffffUL)
        return -1;

    *value = (uint32_t)parsed;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Main tutorial flow                                                         */
/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *bitstream_path;
    uint32_t test_words = DEFAULT_TEST_WORDS;
    uint32_t scale = DEFAULT_SCALE_VALUE;
    size_t transfer_size;
    uint32_t *input_buffer = NULL;
    uint32_t *output_buffer = NULL;
    int status = EXIT_FAILURE;

    if (argc < 2 || argc > 4) {
        printf("Usage: %s <a1_region_bitstream.bin> [scale] [words]\n", argv[0]);
        return EXIT_FAILURE;
    }

    bitstream_path = argv[1];

    if (argc >= 3 && parse_u32_arg(argv[2], &scale) < 0) {
        printf("ERROR: invalid scale value: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    if (argc >= 4 && parse_u32_arg(argv[3], &test_words) < 0) {
        printf("ERROR: invalid word count: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    if (test_words == 0U) {
        printf("ERROR: word count must be greater than zero.\n");
        return EXIT_FAILURE;
    }

    transfer_size = (size_t)test_words * sizeof(uint32_t);

    printf("Alveo DDR-scale HLS tutorial\n");
    printf("  Bitstream       : %s\n", bitstream_path);
    printf("  HLS AXI-Lite    : 0x%08" PRIx64 "\n", (uint64_t)HLS_CTRL_BASE_ADDR);
    printf("  Input DDR addr  : 0x%08" PRIx64 "\n", (uint64_t)INPUT_DDR_ADDR);
    printf("  Output DDR addr : 0x%08" PRIx64 "\n", (uint64_t)OUTPUT_DDR_ADDR);
    printf("  Transfer        : %" PRIu32 " words (%zu bytes)\n", test_words, transfer_size);
    printf("  Scale           : %" PRIu32 "\n\n", scale);

    /* ------------------------------------------------------------------ */
    /* 1. Reconfigure the A1 region.                                      */
    /* ------------------------------------------------------------------ */

    printf("[1] Reconfiguring the A1 region...\n");
    if (alveo_reconfigure(bitstream_path) < 0) {
        printf("ERROR: reconfiguration failed.\n");
        goto end;
    }

    /* ------------------------------------------------------------------ */
    /* 2. Allocate host buffers and prepare the input pattern.             */
    /* ------------------------------------------------------------------ */

    printf("[2] Allocating DMA buffers...\n");
    if (allocate_dma_buffer(&input_buffer, transfer_size) < 0) {
        printf("ERROR: could not allocate input buffer.\n");
        goto end;
    }

    if (allocate_dma_buffer(&output_buffer, transfer_size) < 0) {
        printf("ERROR: could not allocate output buffer.\n");
        goto cleanup_input;
    }

    fill_input_pattern(input_buffer, test_words);
    memset(output_buffer, 0, transfer_size);

    /* ------------------------------------------------------------------ */
    /* 3. Initialize input/output DDR memory through XDMA.                 */
    /* ------------------------------------------------------------------ */

    printf("[3] Writing input buffer to DDR through XDMA...\n");
    if (alveo_write(INPUT_DDR_ADDR, input_buffer, transfer_size) !=
        (ssize_t)transfer_size) {
        printf("ERROR: XDMA write to input DDR address failed.\n");
        goto cleanup;
    }

    printf("[4] Clearing output buffer in DDR through XDMA...\n");
    if (alveo_write(OUTPUT_DDR_ADDR, output_buffer, transfer_size) !=
        (ssize_t)transfer_size) {
        printf("ERROR: XDMA write to output DDR address failed.\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 4. Configure and start the HLS accelerator through AXI-Lite.        */
    /* ------------------------------------------------------------------ */

    printf("[5] Configuring ddr_scale_accel through AXI-Lite...\n");
    if (hls_configure(INPUT_DDR_ADDR, OUTPUT_DDR_ADDR, test_words, scale) < 0) {
        printf("ERROR: could not configure ddr_scale_accel registers.\n");
        goto cleanup;
    }

    printf("[6] Starting ddr_scale_accel...\n");
    if (hls_start() < 0) {
        printf("ERROR: could not start ddr_scale_accel.\n");
        goto cleanup;
    }

    if (hls_wait_done() < 0) {
        printf("ERROR: ddr_scale_accel did not complete.\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 5. Read the output DDR buffer back through XDMA.                    */
    /* ------------------------------------------------------------------ */

    printf("[7] Reading output buffer from DDR through XDMA...\n");
    memset(output_buffer, 0, transfer_size);

    if (alveo_read(OUTPUT_DDR_ADDR, output_buffer, transfer_size) !=
        (ssize_t)transfer_size) {
        printf("ERROR: XDMA read from output DDR address failed.\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 6. Verify the scaled output.                                        */
    /* ------------------------------------------------------------------ */

    printf("[8] Verifying scaled output...\n");
    if (verify_scaled_output(input_buffer, output_buffer, test_words, scale) < 0)
        goto cleanup;

    printf("\nSuccess.\n");
    printf("  Reconfiguration path works: A1 bitstream loaded.\n");
    printf("  AXI-Lite path works: HLS control registers programmed and ap_done seen.\n");
    printf("  Full AXI path works: DDR input/output buffers matched expected scaling.\n");

    status = EXIT_SUCCESS;

cleanup:
    free(output_buffer);
cleanup_input:
    free(input_buffer);
end:
    return status;
}
