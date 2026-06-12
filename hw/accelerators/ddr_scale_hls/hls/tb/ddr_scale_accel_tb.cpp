#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/ddr_scale_accel.h"

static int run_case(uint32_t length, uint32_t scale)
{
    uint32_t *in_buf = (uint32_t *)malloc((size_t)length * sizeof(uint32_t));
    uint32_t *out_buf = (uint32_t *)malloc((size_t)length * sizeof(uint32_t));

    if (in_buf == NULL || out_buf == NULL) {
        free(in_buf);
        free(out_buf);
        return 1;
    }

    for (uint32_t i = 0; i < length; ++i) {
        in_buf[i] = 0x1000U + i;
        out_buf[i] = 0;
    }

    ddr_scale_accel(in_buf, out_buf, length, scale);

    for (uint32_t i = 0; i < length; ++i) {
        uint32_t expected = in_buf[i] * scale;
        if (out_buf[i] != expected) {
            printf("FAIL: idx=%u expected=0x%08x got=0x%08x\n",
                   i,
                   expected,
                   out_buf[i]);
            free(in_buf);
            free(out_buf);
            return 1;
        }
    }

    free(in_buf);
    free(out_buf);
    return 0;
}

int main(void)
{
    if (run_case(256U, 1U) != 0)
        return 1;

    if (run_case(1024U, 3U) != 0)
        return 1;

    if (run_case(4096U, 7U) != 0)
        return 1;

    printf("PASS: ddr_scale_accel C testbench completed successfully.\n");
    return 0;
}
