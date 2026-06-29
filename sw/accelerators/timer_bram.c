#include "../alveo.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*
 * Alveo reconfigurable-region template example
 * --------------------------------------------
 *
 * This example assumes that the bitstream loaded into the A1 region contains:
 *
 *   1. An AXI Timer connected to the AXI-Lite path.
 *   2. An AXI BRAM Controller connected to the full-AXI/XDMA data path.
 *
 * The example demonstrates how to:
 *
 *   1. Reconfigure the A1 region from a .bin bitstream.
 *   2. Access AXI-Lite registers using alveo_reg_read32/write32().
 *   3. Transfer data using alveo_write()/alveo_read().
 *   4. Measure a CMS power rail around the DMA workload using alveo_power_start/stop().
 *
 * To adapt this file to another reconfigurable design, modify mainly the
 * address definitions in the "User design configuration" section.
 */

/* -------------------------------------------------------------------------- */
/* User design configuration                                                  */
/* -------------------------------------------------------------------------- */

/* AXI-Lite address of the AXI Timer, as shown in the Vivado address map. */
#define TIMER_BASE_ADDR      0x42000000ULL

/* Full-AXI address of the BRAM controller, as shown in the Vivado address map. */
#define BRAM_BASE_ADDR       0x80000000ULL

/* Number of 32-bit words used in the BRAM transfer test. */
#define TEST_WORDS           1024U

/* XDMA examples commonly use page-aligned host buffers. */
#define BUFFER_ALIGNMENT     4096U

/* -------------------------------------------------------------------------- */
/* AXI Timer register map                                                     */
/* -------------------------------------------------------------------------- */

/*
 * AXI Timer register offsets, in bytes.
 *
 * In this example, timer 0 and timer 1 are used in cascade mode to create
 * a 64-bit timer:
 *
 *   timer 0 -> lower 32 bits
 *   timer 1 -> upper 32 bits
 */
#define TIMER_TCSR0          0x00U   /* Control/status register, timer 0 */
#define TIMER_TLR0           0x04U   /* Load register, timer 0 */
#define TIMER_TCR0           0x08U   /* Counter register, timer 0 */

#define TIMER_TCSR1          0x10U   /* Control/status register, timer 1 */
#define TIMER_TLR1           0x14U   /* Load register, timer 1 */
#define TIMER_TCR1           0x18U   /* Counter register, timer 1 */

/*
 * Timer configuration values.
 *
 * These values follow the original working timer sequence:
 *
 *   1. Stop both timer channels.
 *   2. Load both counters with zero.
 *   3. Start timer 0 in cascade mode.
 */
#define TIMER_LOAD_VALUE     0x00000020U
#define TIMER_START_VALUE    0x000008D0U

/* -------------------------------------------------------------------------- */
/* Small AXI-Lite helper functions.                                           */
/* User should leverage alveo_reg_write/read calls to build their own.        */
/* -------------------------------------------------------------------------- */

static int timer_write(uint32_t offset, uint32_t value)
{
    return alveo_reg_write32(TIMER_BASE_ADDR + offset, value);
}

static int timer_read_reg(uint32_t offset, uint32_t *value)
{
    return alveo_reg_read32(TIMER_BASE_ADDR + offset, value);
}

static int timer_start(void)
{
    /* Stop both timer channels before changing their configuration. */
    if (timer_write(TIMER_TCSR0, 0) < 0)
        return -1;

    if (timer_write(TIMER_TCSR1, 0) < 0)
        return -1;

    /* Load both counters with zero. */
    if (timer_write(TIMER_TLR0, 0) < 0)
        return -1;

    if (timer_write(TIMER_TLR1, 0) < 0)
        return -1;

    /* Copy the load-register values into the actual counter registers. */
    if (timer_write(TIMER_TCSR0, TIMER_LOAD_VALUE) < 0)
        return -1;

    if (timer_write(TIMER_TCSR1, TIMER_LOAD_VALUE) < 0)
        return -1;

    /*
     * Start the cascaded 64-bit timer.
     *
     * In this setup, timer 0 controls the cascaded pair.
     */
    if (timer_write(TIMER_TCSR1, 0) < 0)
        return -1;

    if (timer_write(TIMER_TCSR0, TIMER_START_VALUE) < 0)
        return -1;

    return 0;
}

static int timer_read(uint64_t *ticks)
{
    uint32_t low = 0;
    uint32_t high = 0;

    if (ticks == NULL)
        return -1;

    if (timer_read_reg(TIMER_TCR0, &low) < 0)
        return -1;

    if (timer_read_reg(TIMER_TCR1, &high) < 0)
        return -1;

    *ticks = ((uint64_t)high << 32) | low;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* DMA buffer and test-pattern helpers                                        */
/* -------------------------------------------------------------------------- */

static int allocate_dma_buffer(uint32_t **buffer, size_t bytes)
{
    int ret;

    /*
     * posix_memalign() is used because XDMA transfers commonly use
     * page-aligned host buffers. This is a safe default for this template.
     */
    ret = posix_memalign((void **)buffer, BUFFER_ALIGNMENT, bytes);
    if (ret != 0) {
        *buffer = NULL;
        return -1;
    }

    return 0;
}

static void fill_test_pattern(uint32_t *buffer)
{
    for (uint32_t i = 0; i < TEST_WORDS; ++i)
        buffer[i] = 0xA5A50000U | i;
}

static int verify_test_pattern(const uint32_t *expected, const uint32_t *observed)
{
    for (uint32_t i = 0; i < TEST_WORDS; ++i) {
        if (observed[i] != expected[i]) {
            printf("ERROR: mismatch at word %u: expected 0x%08" PRIx32
                   ", got 0x%08" PRIx32 "\n",
                   i, expected[i], observed[i]);
            return -1;
        }
    }

    return 0;
}

static void print_power_summary(void)
{
    size_t nsamples = alveo_power_get_num_samples();

    printf(" Power rail    : %s\n",
           alveo_power_get_rail_name(alveo_power_get_rail()));
    printf(" Power samples : %zu\n", nsamples);

    if (nsamples == 0) {
        printf(" Average power : n/a\n");
        printf(" Maximum power : n/a\n");
        return;
    }

    printf(" Average power : %.3f W\n",
           (double)alveo_power_get_average() / (double)ALVEO_POWER_MW_PER_W);
    printf(" Maximum power : %.3f W\n",
           (double)alveo_power_get_max() / (double)ALVEO_POWER_MW_PER_W);
}

/* -------------------------------------------------------------------------- */
/* Main tutorial flow                                                         */
/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *bitstream_path;
    const size_t transfer_size = TEST_WORDS * sizeof(uint32_t);

    uint32_t *tx_buffer = NULL;
    uint32_t *rx_buffer = NULL;

    uint64_t timer_before = 0;
    uint64_t timer_after = 0;
    int power_started = 0;
    int status = EXIT_FAILURE;

    if (argc != 2) {
        printf("Usage: %s <a1_region_bitstream.bin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    bitstream_path = argv[1];

    printf("Alveo template tutorial\n");
    printf("  Bitstream : %s\n", bitstream_path);
    printf("  Timer     : 0x%08" PRIx64 "  AXI-Lite\n", (uint64_t)TIMER_BASE_ADDR);
    printf("  BRAM      : 0x%08" PRIx64 "  full AXI\n", (uint64_t)BRAM_BASE_ADDR);
    printf("  Transfer  : %u words (%zu bytes)\n\n", TEST_WORDS, transfer_size);

    /* ------------------------------------------------------------------ */
    /* 1. Reconfigure the A1 region.                                      */
    /* ------------------------------------------------------------------ */

    printf("[1] Reconfiguring the A1 region...\n");

    if (alveo_reconfigure(bitstream_path) < 0) {
        printf("ERROR: reconfiguration failed.\n");
        goto cleanup;
    }

    /*
     * Note:
     * alveo_reconfigure() checks that the configuration transaction finished.
     * It does not prove that the bitstream contains this exact timer/BRAM
     * design. The accesses below act as a simple design-level validation.
     */

    /* ------------------------------------------------------------------ */
    /* 2. Allocate host buffers and prepare the data to send.              */
    /* ------------------------------------------------------------------ */

    printf("[2] Allocating DMA buffers...\n");

    if (allocate_dma_buffer(&tx_buffer, transfer_size) < 0) {
        printf("ERROR: could not allocate TX buffer.\n");
        goto cleanup;
    }

    if (allocate_dma_buffer(&rx_buffer, transfer_size) < 0) {
        printf("ERROR: could not allocate RX buffer.\n");
        goto cleanup;
    }

    fill_test_pattern(tx_buffer);
    memset(rx_buffer, 0, transfer_size);

    /* ------------------------------------------------------------------ */
    /* 3. Start CMS power monitoring outside the timer window.             */
    /* ------------------------------------------------------------------ */

    printf("[3] Starting CMS power monitoring...\n");
    if (alveo_power_start() < 0) {
        printf("ERROR: could not start CMS power monitoring.\n");
        goto cleanup;
    }
    power_started = 1;

    /*
     * Start/read the hardware timer after power monitoring has started, and
     * read it again before stopping power monitoring.  This avoids adding the
     * CMS thread stop latency, which can be close to one sample period, to the
     * timer measurement.
     */
    printf("[4] Starting the AXI Timer through AXI-Lite...\n");

    if (timer_start() < 0) {
        printf("ERROR: could not configure the AXI Timer.\n");
        goto cleanup;
    }

    if (timer_read(&timer_before) < 0) {
        printf("ERROR: could not read the AXI Timer.\n");
        goto cleanup;
    }

    printf("    Timer before DMA: %" PRIu64 "\n", timer_before);

    printf("[5] Writing data to BRAM through XDMA...\n");

    if (alveo_write(BRAM_BASE_ADDR, tx_buffer, transfer_size) !=
        (ssize_t)transfer_size) {
        printf("ERROR: XDMA write to BRAM failed.\n");
        goto cleanup;
    }

    printf("[6] Reading data back from BRAM through XDMA...\n");

    if (alveo_read(BRAM_BASE_ADDR, rx_buffer, transfer_size) !=
        (ssize_t)transfer_size) {
        printf("ERROR: XDMA read from BRAM failed.\n");
        goto cleanup;
    }

    if (timer_read(&timer_after) < 0) {
        printf("ERROR: could not read the AXI Timer after DMA.\n");
        goto cleanup;
    }

    printf("    Timer after DMA : %" PRIu64 "\n", timer_after);

    if (alveo_power_stop() < 0) {
        printf("ERROR: could not stop CMS power monitoring.\n");
        goto cleanup;
    }
    power_started = 0;

    /* ------------------------------------------------------------------ */
    /* 4. Verify that the received data matches the transmitted data.      */
    /* ------------------------------------------------------------------ */

    printf("[7] Verifying BRAM contents...\n");

    if (verify_test_pattern(tx_buffer, rx_buffer) < 0)
        goto cleanup;

    printf("\nSuccess.\n");
    printf("  AXI-Lite path works: timer configured and read correctly.\n");
    printf("  Full AXI path works: BRAM write/readback matched.\n");
    printf("  CMS power path works: power-rail samples were collected.\n");
    print_power_summary();

    status = EXIT_SUCCESS;

cleanup:
    if (power_started)
        (void)alveo_power_stop();

    free(tx_buffer);
    free(rx_buffer);

    return status;
}
