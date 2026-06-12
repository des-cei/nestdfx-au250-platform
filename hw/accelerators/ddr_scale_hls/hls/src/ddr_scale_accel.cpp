#include "ddr_scale_accel.h"

void ddr_scale_accel(const uint32_t *in_ddr,
                     uint32_t *out_ddr,
                     uint32_t length,
                     uint32_t scale)
{
#pragma HLS INTERFACE m_axi port=in_ddr offset=slave bundle=gmem max_read_burst_length=64 num_read_outstanding=16
#pragma HLS INTERFACE m_axi port=out_ddr offset=slave bundle=gmem max_write_burst_length=64 num_write_outstanding=16

#pragma HLS INTERFACE s_axilite port=in_ddr bundle=control
#pragma HLS INTERFACE s_axilite port=out_ddr bundle=control
#pragma HLS INTERFACE s_axilite port=length bundle=control
#pragma HLS INTERFACE s_axilite port=scale bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    for (uint32_t i = 0; i < length; ++i) {
#pragma HLS PIPELINE II=1
        out_ddr[i] = in_ddr[i] * scale;
    }
}
