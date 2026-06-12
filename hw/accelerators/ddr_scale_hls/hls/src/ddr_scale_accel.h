#ifndef DDR_SCALE_ACCEL_H
#define DDR_SCALE_ACCEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DDR pointer model accelerator:
 * - AXI4-Lite controls length/scale and start via ap_ctrl_hs.
 * - AXI4 master reads input data from DDR and writes output data to DDR.
 *
 * The same AXI master bundle is used for both input and output pointers so the
 * generated IP can map to a single shell-facing AXI4 master connection.
 */
void ddr_scale_accel(const uint32_t *in_ddr,
                     uint32_t *out_ddr,
                     uint32_t length,
                     uint32_t scale);

#ifdef __cplusplus
}
#endif

#endif
