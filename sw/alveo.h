#ifndef ALVEO_H
#define ALVEO_H

/*
 * Minimal Alveo XDMA/HBICAP helper library.
 *
 * This library is intentionally small and source-only.  It is meant for
 * tutorial/template designs where a userspace application needs to:
 *
 *   1. Reconfigure the A1/static reconfigurable region with a .bin file.
 *   2. Access AXI-Lite registers through the XDMA user BAR.
 *   3. Move bulk data through XDMA H2C/C2H over the full AXI bus.
 *   4. Sample selected power rails through the Alveo CMS AXI-Lite register window.
 *
 * The public API deliberately exposes only the operations a normal user of the
 * template needs.  Device names and address-map constants are kept below so a
 * developer can adapt the file to a different Alveo shell or Vivado design.
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/* -------------------------------------------------------------------------- */
/* XDMA device files                                                          */
/* -------------------------------------------------------------------------- */

/* AXI-Lite register access path.  This is the XDMA user BAR. */
#define ALVEO_XDMA_USER_DEV          "/dev/xdma0_user"

/* Full AXI data movement path used by normal application buffers. */
#define ALVEO_XDMA_H2C_DATA_DEV      "/dev/xdma0_h2c_0"
#define ALVEO_XDMA_C2H_DATA_DEV      "/dev/xdma0_c2h_0"

/* Full AXI path used to stream a bitstream into the HBICAP input address. */
#define ALVEO_XDMA_H2C_RECONFIG_DEV  "/dev/xdma0_h2c_1"

/* Maximum chunk used for a single read()/write() call to an XDMA device. */
#define ALVEO_RW_MAX_SIZE            0x7ffff000UL

/* -------------------------------------------------------------------------- */
/* Address-map constants                                                      */
/* -------------------------------------------------------------------------- */

/*
 * AXI-Lite address space, as shown in Vivado.
 *
 * A user of alveo_reg_read32()/alveo_reg_write32() passes Vivado AXI-Lite
 * addresses, for example 0x42000000 for the AXI Timer in the tutorial design.
 * The library internally converts them to /dev/xdma0_user mmap offsets by
 * subtracting ALVEO_AXIL_BASE_ADDR.
 */
#define ALVEO_AXIL_BASE_ADDR         0x40000000ULL

#define ALVEO_AXIL_USER_OFFSET(addr) ((uint64_t)(addr) - ALVEO_AXIL_BASE_ADDR)

/*
 * Shell-to-A1 gates used during static/A1-region reconfiguration.
 *
 * These blocks protect the AXI interfaces that cross from the static shell into
 * the A1 reconfigurable region.  For this specific template all of them are
 * asserted before reconfiguration and deasserted afterwards.
 */
#define ALVEO_DFX_SHUTDOWN_0_ADDR    0x40000000ULL
#define ALVEO_DFX_SHUTDOWN_1_ADDR    0x40001000ULL
#define ALVEO_DFX_DECOUPLER_0_ADDR   0x40002000ULL
#define ALVEO_DFX_SHUTDOWN_2_ADDR    0x40003000ULL

/*
 * Alveo CMS control/sensor register window.
 */
#define ALVEO_CMS_CTRL_ADDR     0x41000000ULL
#define ALVEO_CMS_CTRL_MAP_SIZE 0x40000U

/* CMS internal byte offsets inside ALVEO_CMS_CTRL_ADDR. */
#define ALVEO_CMS_RESET_BYTE_OFFSET  0x20000U
#define ALVEO_CMS_SENSOR_BYTE_OFFSET 0x28000U

/* CMS sensor register word offsets from ALVEO_CMS_SENSOR_BYTE_OFFSET. */
#define ALVEO_CMS_12V_PEX_VOLTAGE_WORD_OFFSET    10U
#define ALVEO_CMS_12V_PEX_CURRENT_WORD_OFFSET    52U
#define ALVEO_CMS_12V_AUX_VOLTAGE_WORD_OFFSET    19U
#define ALVEO_CMS_12V_AUX_CURRENT_WORD_OFFSET    55U
#define ALVEO_CMS_3V3_PEX_VOLTAGE_WORD_OFFSET    13U
#define ALVEO_CMS_3V3_PEX_CURRENT_WORD_OFFSET    160U
#define ALVEO_CMS_3V3_AUX_VOLTAGE_WORD_OFFSET    16U
#define ALVEO_CMS_3V3_AUX_CURRENT_WORD_OFFSET    190U
#define ALVEO_CMS_VCCINT_VOLTAGE_WORD_OFFSET     58U
#define ALVEO_CMS_VCCINT_CURRENT_WORD_OFFSET     61U

/*
 * The CMS voltage/current registers are used as mV and mA values. Their
 * product is therefore microwatts. The library stores power samples in mW so
 * examples can print watts without losing too much precision.
 */
#define ALVEO_CMS_POWER_UW_PER_MW 1000U
#define ALVEO_POWER_MW_PER_W 1000U

/* Values used by the current CMS control register sequence. */
#define ALVEO_CMS_RESET_START_VALUE 0x00000001U
#define ALVEO_CMS_RESET_STOP_VALUE  0x00000000U

/* Default power sampling configuration. */
#define ALVEO_POWER_SAMPLE_PERIOD_US 120000U
#define ALVEO_POWER_MAX_SAMPLES      4096U

/*
 * HBICAP control registers are accessed through the AXI-Lite path.
 * The data stream itself is sent through the full AXI XDMA H2C reconfig path.
 */
#define ALVEO_HBICAP_CTRL_ADDR       0x41040000ULL
#define ALVEO_HBICAP_DATA_ADDR       0x40000000ULL

/* Original design used uint32_t word offsets from the HBICAP control base. */
#define ALVEO_HBICAP_SIZE_WORD_OFFSET    0x0042U
#define ALVEO_HBICAP_CTRL_WORD_OFFSET    0x0043U
#define ALVEO_HBICAP_STATUS_WORD_OFFSET  0x0044U

/* Values used by the existing HBICAP controller in the template design. */
#define ALVEO_HBICAP_CTRL_START_VALUE    0x0000000CU
#define ALVEO_HBICAP_STATUS_DONE_MASK    0x00000001U
#define ALVEO_HBICAP_POLL_MAX            100000000UL

/* -------------------------------------------------------------------------- */
/* Power rails                                                                */
/* -------------------------------------------------------------------------- */

typedef enum {
    ALVEO_POWER_RAIL_VCCINT = 0,
    ALVEO_POWER_RAIL_12V_PEX,
    ALVEO_POWER_RAIL_12V_AUX,
    ALVEO_POWER_RAIL_3V3_PEX,
    ALVEO_POWER_RAIL_3V3_AUX,
    ALVEO_POWER_RAIL_CARD_TOTAL,
} alveo_power_rail_t;

#define ALVEO_POWER_DEFAULT_RAIL ALVEO_POWER_RAIL_VCCINT

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/*
 * Reconfigure the A1/static reconfigurable region using a raw .bin bitstream.
 *
 * This function internally asserts all shell-to-A1 gates, configures HBICAP,
 * streams the bitstream through XDMA, waits for completion, and releases the
 * gates again.
 *
 * Important: this implementation is for the static/A1-region reconfiguration
 * flow.  A future nested-DFX flow inside the A1 region will probably need a
 * separate function/path, because it may use HBICAP without asserting these
 * outer shell-to-A1 gates.
 *
 * Returns 0 on success or a negative errno-style value on failure.
 */
int alveo_reconfigure(const char *bitstream_path);

/*
 * Write/read one 32-bit AXI-Lite register.
 *
 * axil_addr is the Vivado AXI-Lite address, not the mmap offset.  For example,
 * if the AXI Timer is mapped at 0x42000000 in Vivado, call:
 *
 *     alveo_reg_write32(0x42000000, value);
 *
 * The library maps the corresponding /dev/xdma0_user page internally.
 */
int alveo_reg_write32(uint64_t axil_addr, uint32_t value);
int alveo_reg_read32(uint64_t axil_addr, uint32_t *value);

/*
 * Move data through the full AXI XDMA path.
 *
 * hw_addr is the Vivado full-AXI address and is used directly as the XDMA
 * offset.  Unlike AXI-Lite register access, no address translation is applied.
 */
ssize_t alveo_write(uint64_t hw_addr, const void *buffer, uint64_t size);
ssize_t alveo_read(uint64_t hw_addr, void *buffer, uint64_t size);

/*
 * Start/stop CMS-based power-rail sampling.
 *
 * The implementation maps ALVEO_CMS_CTRL_ADDR through /dev/xdma0_user, starts a
 * small sampling thread, and stores CMS-derived power samples in mW in an
 * internal buffer. By default, the sampled rail is ALVEO_POWER_RAIL_VCCINT.
 *
 * Use alveo_power_get_*() after alveo_power_stop() to inspect the collected
 * samples. All functions return 0 on success or a negative errno-style value on
 * failure unless otherwise stated.
 */
int alveo_power_start(void);
int alveo_power_stop(void);

/* Select/read the rail used by the next power measurement. */
int alveo_power_set_rail(alveo_power_rail_t rail);
alveo_power_rail_t alveo_power_get_rail(void);
const char *alveo_power_get_rail_name(alveo_power_rail_t rail);

/* Number of samples collected by the most recent alveo_power_start/stop pair. */
size_t alveo_power_get_num_samples(void);

/* Pointer to the internal sample buffer. Valid until the next power start. */
const uint32_t *alveo_power_get_samples(void);

/* Simple statistics over the collected samples, in mW. Return 0 if there are none. */
uint32_t alveo_power_get_average(void);
uint32_t alveo_power_get_max(void);

#endif /* ALVEO_H */
