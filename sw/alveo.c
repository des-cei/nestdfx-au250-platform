#include "alveo.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Internal helpers keep the public API small.  The user of the library should
 * not need to know about mmap offsets, XDMA device names, chunk sizes, or the
 * exact HBICAP register programming sequence.
 */

static int report_errno(const char *context)
{
    int err = errno ? errno : EIO;
    fprintf(stderr, "[alveo] %s: %s\n", context, strerror(err));
    return -err;
}

static int validate_axil_addr(uint64_t axil_addr)
{
    if (axil_addr < ALVEO_AXIL_BASE_ADDR) {
        fprintf(stderr,
                "[alveo] AXI-Lite address 0x%" PRIx64
                " is below configured AXI-Lite base 0x%" PRIx64 "\n",
                axil_addr, (uint64_t)ALVEO_AXIL_BASE_ADDR);
        return -EINVAL;
    }

    if ((axil_addr & 0x3ULL) != 0) {
        fprintf(stderr,
                "[alveo] AXI-Lite address 0x%" PRIx64
                " is not 32-bit aligned\n",
                axil_addr);
        return -EINVAL;
    }

    return 0;
}


/*
 * Map the page that contains a 32-bit AXI-Lite register, access it once, and
 * unmap it.  This is intentionally simple.  Register access is not expected to
 * be performance-critical in this template.
 */
static int axil_access32(uint64_t axil_addr, uint32_t *value, int is_write)
{
    int ret = validate_axil_addr(axil_addr);
    if (ret < 0)
        return ret;

    if (value == NULL)
        return -EINVAL;

    long page_size_long = sysconf(_SC_PAGESIZE);
    if (page_size_long <= 0)
        return -EINVAL;

    const uint64_t page_size = (uint64_t)page_size_long;
    const uint64_t user_offset = ALVEO_AXIL_USER_OFFSET(axil_addr);
    const uint64_t map_offset = user_offset & ~(page_size - 1ULL);
    const uint64_t page_delta = user_offset - map_offset;

    int fd = open(ALVEO_XDMA_USER_DEV, O_RDWR | O_SYNC);
    if (fd < 0)
        return report_errno("open(" ALVEO_XDMA_USER_DEV ")");

    void *map = mmap(NULL,
                     (size_t)page_size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd,
                     (off_t)map_offset);
    if (map == MAP_FAILED) {
        ret = report_errno("mmap(/dev/xdma0_user)");
        close(fd);
        return ret;
    }

    volatile uint32_t *reg =
        (volatile uint32_t *)(void *)((unsigned char *)map + page_delta);

    if (is_write)
        *reg = *value;
    else
        *value = *reg;

    if (munmap(map, (size_t)page_size) != 0)
        ret = report_errno("munmap(/dev/xdma0_user)");

    if (close(fd) != 0 && ret == 0)
        ret = report_errno("close(" ALVEO_XDMA_USER_DEV ")");

    return ret;
}

int alveo_reg_write32(uint64_t axil_addr, uint32_t value)
{
    return axil_access32(axil_addr, &value, 1);
}

int alveo_reg_read32(uint64_t axil_addr, uint32_t *value)
{
    return axil_access32(axil_addr, value, 0);
}

/*
 * Transfer data through one XDMA character device.  The offset is the Vivado
 * full-AXI address and is intentionally not translated.
 */
static ssize_t xdma_transfer(const char *device,
                             int is_write,
                             uint64_t hw_addr,
                             void *buffer,
                             uint64_t size)
{
    if (buffer == NULL && size != 0)
        return -EINVAL;

    if (size == 0)
        return 0;

    int fd = open(device, O_RDWR);
    if (fd < 0)
        return report_errno(device);

    uint64_t count = 0;
    unsigned char *buf = (unsigned char *)buffer;

    while (count < size) {
        uint64_t remaining = size - count;
        size_t chunk = remaining > ALVEO_RW_MAX_SIZE
                         ? (size_t)ALVEO_RW_MAX_SIZE
                         : (size_t)remaining;
        uint64_t addr = hw_addr + count;

        off_t off = (off_t)addr;
        if ((uint64_t)off != addr) {
            close(fd);
            return -EINVAL;
        }

        if (lseek(fd, off, SEEK_SET) != off) {
            int ret = report_errno("lseek(XDMA)");
            close(fd);
            return ret;
        }

        ssize_t rc = is_write
                       ? write(fd, buf + count, chunk)
                       : read(fd, buf + count, chunk);

        if (rc < 0) {
            int ret = report_errno(is_write ? "write(XDMA)" : "read(XDMA)");
            close(fd);
            return ret;
        }

        if (rc == 0) {
            fprintf(stderr, "[alveo] XDMA transfer stopped after 0 bytes\n");
            close(fd);
            return -EIO;
        }

        count += (uint64_t)rc;

        if ((size_t)rc != chunk) {
            fprintf(stderr,
                    "[alveo] short XDMA %s at 0x%" PRIx64
                    ": got %zd bytes, expected %zu bytes\n",
                    is_write ? "write" : "read", addr, rc, chunk);
            close(fd);
            return -EIO;
        }
    }

    if (close(fd) != 0)
        return report_errno("close(XDMA)");

    if (count > (uint64_t)LONG_MAX)
        return -EOVERFLOW;

    return (ssize_t)count;
}

ssize_t alveo_write(uint64_t hw_addr, const void *buffer, uint64_t size)
{
    /* Cast is safe because xdma_transfer() does not modify the buffer when
     * is_write is true.  The helper uses one implementation for both paths. */
    return xdma_transfer(ALVEO_XDMA_H2C_DATA_DEV, 1, hw_addr, (void *)buffer, size);
}

ssize_t alveo_read(uint64_t hw_addr, void *buffer, uint64_t size)
{
    return xdma_transfer(ALVEO_XDMA_C2H_DATA_DEV, 0, hw_addr, buffer, size);
}

static int load_binary_file(const char *path, unsigned char **data, size_t *size)
{
    if (path == NULL || data == NULL || size == NULL)
        return -EINVAL;

    *data = NULL;
    *size = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return report_errno("open(bitstream)");

    struct stat st;
    if (fstat(fd, &st) != 0) {
        int ret = report_errno("fstat(bitstream)");
        close(fd);
        return ret;
    }

    if (st.st_size <= 0) {
        fprintf(stderr, "[alveo] bitstream file is empty: %s\n", path);
        close(fd);
        return -EINVAL;
    }

    if ((st.st_size % 4) != 0) {
        fprintf(stderr,
                "[alveo] bitstream size must be a multiple of 4 bytes: %s\n",
                path);
        close(fd);
        return -EINVAL;
    }

    unsigned char *buf = malloc((size_t)st.st_size);
    if (buf == NULL) {
        close(fd);
        return -ENOMEM;
    }

    size_t done = 0;
    while (done < (size_t)st.st_size) {
        ssize_t rc = read(fd, buf + done, (size_t)st.st_size - done);
        if (rc < 0) {
            int ret = report_errno("read(bitstream)");
            free(buf);
            close(fd);
            return ret;
        }
        if (rc == 0) {
            fprintf(stderr, "[alveo] unexpected EOF while reading bitstream\n");
            free(buf);
            close(fd);
            return -EIO;
        }
        done += (size_t)rc;
    }

    if (close(fd) != 0) {
        int ret = report_errno("close(bitstream)");
        free(buf);
        return ret;
    }

    *data = buf;
    *size = done;
    return 0;
}

static int set_reconfiguration_gates(uint32_t value)
{
    const uint64_t gate_addrs[] = {
        ALVEO_DFX_SHUTDOWN_0_ADDR,
        ALVEO_DFX_SHUTDOWN_1_ADDR,
        ALVEO_DFX_DECOUPLER_0_ADDR,
        ALVEO_DFX_SHUTDOWN_2_ADDR,
    };

    for (size_t i = 0; i < sizeof(gate_addrs) / sizeof(gate_addrs[0]); ++i) {
        int ret = alveo_reg_write32(gate_addrs[i], value);
        if (ret < 0) {
            fprintf(stderr,
                    "[alveo] failed to %s reconfiguration gate at 0x%" PRIx64 "\n",
                    value ? "assert" : "deassert", gate_addrs[i]);
            return ret;
        }
    }

    return 0;
}

/*
 * Map the complete HBICAP control window once.  This is only used internally by
 * alveo_reconfigure(), where status polling would be unnecessarily slow if each
 * poll opened and unmapped /dev/xdma0_user separately.
 */
static volatile uint32_t *map_hbicap_control(int *fd_out, void **map_out)
{
    if (fd_out == NULL || map_out == NULL)
        return NULL;

    *fd_out = -1;
    *map_out = MAP_FAILED;

    int fd = open(ALVEO_XDMA_USER_DEV, O_RDWR | O_SYNC);
    if (fd < 0) {
        report_errno("open(" ALVEO_XDMA_USER_DEV ")");
        return NULL;
    }

    const uint64_t hbicap_user_offset = ALVEO_AXIL_USER_OFFSET(ALVEO_HBICAP_CTRL_ADDR);
    const size_t map_size = 0x10000U; /* HBICAP control range from address map. */

    void *map = mmap(NULL,
                     map_size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd,
                     (off_t)hbicap_user_offset);
    if (map == MAP_FAILED) {
        report_errno("mmap(HBICAP control)");
        close(fd);
        return NULL;
    }

    *fd_out = fd;
    *map_out = map;
    return (volatile uint32_t *)map;
}

static void unmap_hbicap_control(int fd, void *map)
{
    if (map != MAP_FAILED && map != NULL)
        (void)munmap(map, 0x10000U);
    if (fd >= 0)
        (void)close(fd);
}

int alveo_reconfigure(const char *bitstream_path)
{
    unsigned char *bitstream = NULL;
    size_t bitstream_size = 0;

    int ret = load_binary_file(bitstream_path, &bitstream, &bitstream_size);
    if (ret < 0)
        return ret;

    /*
     * This is a static/A1-region reconfiguration flow.  The four gates protect
     * every shell-to-A1 interface shown in the design address map.  Do this as
     * late as possible so the region is decoupled only during the actual load.
     */
    ret = set_reconfiguration_gates(1U);
    if (ret < 0) {
        free(bitstream);
        return ret;
    }

    int hbicap_fd = -1;
    void *hbicap_map = MAP_FAILED;
    volatile uint32_t *hbicap = map_hbicap_control(&hbicap_fd, &hbicap_map);
    if (hbicap == NULL) {
        (void)set_reconfiguration_gates(0U);
        free(bitstream);
        return -EIO;
    }

    const uint32_t nwords = (uint32_t)(bitstream_size / sizeof(uint32_t));

    hbicap[ALVEO_HBICAP_CTRL_WORD_OFFSET] = ALVEO_HBICAP_CTRL_START_VALUE;
    hbicap[ALVEO_HBICAP_SIZE_WORD_OFFSET] = nwords;

    /* Make sure the control writes reach the device before the bitstream DMA. */
    __sync_synchronize();

    ssize_t wr = xdma_transfer(ALVEO_XDMA_H2C_RECONFIG_DEV,
                               1,
                               ALVEO_HBICAP_DATA_ADDR,
                               bitstream,
                               bitstream_size);
    if (wr < 0) {
        ret = (int)wr;
        goto out_release;
    }

    if ((size_t)wr != bitstream_size) {
        fprintf(stderr,
                "[alveo] reconfiguration transfer size mismatch: %zd/%zu\n",
                wr, bitstream_size);
        ret = -EIO;
        goto out_release;
    }

    for (unsigned long poll = 0; poll < ALVEO_HBICAP_POLL_MAX; ++poll) {
        uint32_t status = hbicap[ALVEO_HBICAP_STATUS_WORD_OFFSET];
        if ((status & ALVEO_HBICAP_STATUS_DONE_MASK) != 0U) {
            ret = 0;
            goto out_release;
        }
    }

    fprintf(stderr, "[alveo] timeout while waiting for HBICAP completion\n");
    ret = -ETIMEDOUT;

out_release:
    unmap_hbicap_control(hbicap_fd, hbicap_map);

    /* Always try to reconnect the shell and A1 region before returning. */
    int gate_ret = set_reconfiguration_gates(0U);
    if (ret == 0 && gate_ret < 0)
        ret = gate_ret;

    free(bitstream);
    return ret;
}
