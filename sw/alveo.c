#include "alveo.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
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

/* -------------------------------------------------------------------------- */
/* Alveo CMS power sampling */
/* -------------------------------------------------------------------------- */

static int power_fd = -1;
static void *power_cms_map = MAP_FAILED;
static pthread_t power_thread;
static int power_thread_created = 0;
static int power_stop_requested = 0;
static uint32_t power_samples[ALVEO_POWER_MAX_SAMPLES];
static size_t power_num_samples = 0;
static alveo_power_rail_t power_rail = ALVEO_POWER_DEFAULT_RAIL;
static pthread_mutex_t power_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t power_cond = PTHREAD_COND_INITIALIZER;

static volatile uint32_t *cms_word_ptr(uint32_t byte_offset)
{
    return (volatile uint32_t *)(void *)((unsigned char *)power_cms_map + byte_offset);
}

static int map_cms_control(void)
{
    if (power_cms_map != MAP_FAILED)
        return 0;

    power_fd = open(ALVEO_XDMA_USER_DEV, O_RDWR | O_SYNC);
    if (power_fd < 0)
        return report_errno("open(" ALVEO_XDMA_USER_DEV ")");

    const uint64_t cms_user_offset = ALVEO_AXIL_USER_OFFSET(ALVEO_CMS_CTRL_ADDR);

    power_cms_map = mmap(NULL,
                         ALVEO_CMS_CTRL_MAP_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         power_fd,
                         (off_t)cms_user_offset);
    if (power_cms_map == MAP_FAILED) {
        int ret = report_errno("mmap(CMS control)");
        close(power_fd);
        power_fd = -1;
        return ret;
    }

    return 0;
}

static void unmap_cms_control(void)
{
    if (power_cms_map != MAP_FAILED) {
        (void)munmap(power_cms_map, ALVEO_CMS_CTRL_MAP_SIZE);
        power_cms_map = MAP_FAILED;
    }

    if (power_fd >= 0) {
        (void)close(power_fd);
        power_fd = -1;
    }
}

static void cms_write_reset(uint32_t value)
{
    volatile uint32_t *reset_reg = cms_word_ptr(ALVEO_CMS_RESET_BYTE_OFFSET);
    *reset_reg = value;
    __sync_synchronize();
}

static uint32_t cms_read_power_pair_mw(uint32_t voltage_word, uint32_t current_word)
{
    volatile uint32_t *sensor_regs = cms_word_ptr(ALVEO_CMS_SENSOR_BYTE_OFFSET);
    const uint32_t voltage_mv = sensor_regs[voltage_word];
    const uint32_t current_ma = sensor_regs[current_word];

    /*
     * The CMS registers are interpreted as mV and mA. Their product is uW.
     * Keep the public sample buffer in mW so applications can print W with a
     * decimal conversion while still using compact integer samples internally.
     */
    const uint64_t sample_uw = (uint64_t)voltage_mv * (uint64_t)current_ma;
    const uint64_t sample_mw = sample_uw / ALVEO_CMS_POWER_UW_PER_MW;

    if (sample_mw > UINT32_MAX)
        return UINT32_MAX;

    return (uint32_t)sample_mw;
}

static uint32_t cms_read_power_sample(alveo_power_rail_t rail)
{
    uint64_t total_mw;

    switch (rail) {
    case ALVEO_POWER_RAIL_VCCINT:
        return cms_read_power_pair_mw(ALVEO_CMS_VCCINT_VOLTAGE_WORD_OFFSET,
                                      ALVEO_CMS_VCCINT_CURRENT_WORD_OFFSET);

    case ALVEO_POWER_RAIL_12V_PEX:
        return cms_read_power_pair_mw(ALVEO_CMS_12V_PEX_VOLTAGE_WORD_OFFSET,
                                      ALVEO_CMS_12V_PEX_CURRENT_WORD_OFFSET);

    case ALVEO_POWER_RAIL_12V_AUX:
        return cms_read_power_pair_mw(ALVEO_CMS_12V_AUX_VOLTAGE_WORD_OFFSET,
                                      ALVEO_CMS_12V_AUX_CURRENT_WORD_OFFSET);

    case ALVEO_POWER_RAIL_3V3_PEX:
        return cms_read_power_pair_mw(ALVEO_CMS_3V3_PEX_VOLTAGE_WORD_OFFSET,
                                      ALVEO_CMS_3V3_PEX_CURRENT_WORD_OFFSET);

    case ALVEO_POWER_RAIL_3V3_AUX:
        return cms_read_power_pair_mw(ALVEO_CMS_3V3_AUX_VOLTAGE_WORD_OFFSET,
                                      ALVEO_CMS_3V3_AUX_CURRENT_WORD_OFFSET);

    case ALVEO_POWER_RAIL_CARD_TOTAL:
        total_mw = 0;
        total_mw += cms_read_power_pair_mw(ALVEO_CMS_12V_PEX_VOLTAGE_WORD_OFFSET,
                                           ALVEO_CMS_12V_PEX_CURRENT_WORD_OFFSET);
        total_mw += cms_read_power_pair_mw(ALVEO_CMS_12V_AUX_VOLTAGE_WORD_OFFSET,
                                           ALVEO_CMS_12V_AUX_CURRENT_WORD_OFFSET);
        total_mw += cms_read_power_pair_mw(ALVEO_CMS_3V3_PEX_VOLTAGE_WORD_OFFSET,
                                           ALVEO_CMS_3V3_PEX_CURRENT_WORD_OFFSET);
        total_mw += cms_read_power_pair_mw(ALVEO_CMS_3V3_AUX_VOLTAGE_WORD_OFFSET,
                                           ALVEO_CMS_3V3_AUX_CURRENT_WORD_OFFSET);

        if (total_mw > UINT32_MAX)
            return UINT32_MAX;

        return (uint32_t)total_mw;
    }

    return 0;
}

static int power_wait_sample_period(void)
{
    struct timespec timeout;

    if (clock_gettime(CLOCK_REALTIME, &timeout) != 0)
        return 0;

    timeout.tv_sec += ALVEO_POWER_SAMPLE_PERIOD_US / 1000000U;
    timeout.tv_nsec += (long)(ALVEO_POWER_SAMPLE_PERIOD_US % 1000000U) * 1000L;
    if (timeout.tv_nsec >= 1000000000L) {
        timeout.tv_sec += 1;
        timeout.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&power_lock);
    while (!power_stop_requested) {
        int rc = pthread_cond_timedwait(&power_cond, &power_lock, &timeout);
        if (rc == ETIMEDOUT)
            break;
        if (rc != 0)
            break;
    }

    int stop = power_stop_requested || power_num_samples >= ALVEO_POWER_MAX_SAMPLES;
    pthread_mutex_unlock(&power_lock);

    return stop;
}

static void *power_sample_thread(void *arg)
{
    (void)arg;

    for (;;) {
        pthread_mutex_lock(&power_lock);
        int stop = power_stop_requested || power_num_samples >= ALVEO_POWER_MAX_SAMPLES;
        pthread_mutex_unlock(&power_lock);

        if (stop)
            break;

        alveo_power_rail_t rail;

        pthread_mutex_lock(&power_lock);
        rail = power_rail;
        pthread_mutex_unlock(&power_lock);

        uint32_t sample = cms_read_power_sample(rail);

        pthread_mutex_lock(&power_lock);
        if (!power_stop_requested && power_num_samples < ALVEO_POWER_MAX_SAMPLES)
            power_samples[power_num_samples++] = sample;
        stop = power_stop_requested || power_num_samples >= ALVEO_POWER_MAX_SAMPLES;
        pthread_mutex_unlock(&power_lock);

        if (stop)
            break;

        if (power_wait_sample_period())
            break;
    }

    return NULL;
}

int alveo_power_start(void)
{
    int ret;

    pthread_mutex_lock(&power_lock);
    if (power_thread_created) {
        pthread_mutex_unlock(&power_lock);
        fprintf(stderr, "[alveo] power monitor is already running\n");
        return -EBUSY;
    }

    memset(power_samples, 0, sizeof(power_samples));
    power_num_samples = 0;
    power_stop_requested = 0;
    pthread_mutex_unlock(&power_lock);

    ret = map_cms_control();
    if (ret < 0)
        return ret;

    cms_write_reset(ALVEO_CMS_RESET_START_VALUE);

    ret = pthread_create(&power_thread, NULL, power_sample_thread, NULL);
    if (ret != 0) {
        errno = ret;
        ret = report_errno("pthread_create(power monitor)");
        cms_write_reset(ALVEO_CMS_RESET_STOP_VALUE);
        unmap_cms_control();
        return ret;
    }

    pthread_mutex_lock(&power_lock);
    power_thread_created = 1;
    pthread_mutex_unlock(&power_lock);

    return 0;
}

int alveo_power_stop(void)
{
    int ret = 0;
    int should_join;

    pthread_mutex_lock(&power_lock);
    should_join = power_thread_created;
    power_stop_requested = 1;
    pthread_cond_signal(&power_cond);
    pthread_mutex_unlock(&power_lock);

    if (should_join) {
        int rc = pthread_join(power_thread, NULL);
        if (rc != 0) {
            errno = rc;
            ret = report_errno("pthread_join(power monitor)");
        }

        pthread_mutex_lock(&power_lock);
        power_thread_created = 0;
        pthread_mutex_unlock(&power_lock);
    }

    if (power_cms_map != MAP_FAILED)
        cms_write_reset(ALVEO_CMS_RESET_STOP_VALUE);

    unmap_cms_control();

    return ret;
}

static int power_rail_is_valid(alveo_power_rail_t rail)
{
    switch (rail) {
    case ALVEO_POWER_RAIL_VCCINT:
    case ALVEO_POWER_RAIL_12V_PEX:
    case ALVEO_POWER_RAIL_12V_AUX:
    case ALVEO_POWER_RAIL_3V3_PEX:
    case ALVEO_POWER_RAIL_3V3_AUX:
    case ALVEO_POWER_RAIL_CARD_TOTAL:
        return 1;
    }

    return 0;
}

int alveo_power_set_rail(alveo_power_rail_t rail)
{
    if (!power_rail_is_valid(rail))
        return -EINVAL;

    pthread_mutex_lock(&power_lock);
    if (power_thread_created) {
        pthread_mutex_unlock(&power_lock);
        fprintf(stderr, "[alveo] cannot change power rail while monitoring is running\n");
        return -EBUSY;
    }

    power_rail = rail;
    pthread_mutex_unlock(&power_lock);

    return 0;
}

alveo_power_rail_t alveo_power_get_rail(void)
{
    alveo_power_rail_t rail;

    pthread_mutex_lock(&power_lock);
    rail = power_rail;
    pthread_mutex_unlock(&power_lock);

    return rail;
}

const char *alveo_power_get_rail_name(alveo_power_rail_t rail)
{
    switch (rail) {
    case ALVEO_POWER_RAIL_VCCINT:
        return "VCCINT";
    case ALVEO_POWER_RAIL_12V_PEX:
        return "12V_PEX";
    case ALVEO_POWER_RAIL_12V_AUX:
        return "12V_AUX";
    case ALVEO_POWER_RAIL_3V3_PEX:
        return "3V3_PEX";
    case ALVEO_POWER_RAIL_3V3_AUX:
        return "3V3_AUX";
    case ALVEO_POWER_RAIL_CARD_TOTAL:
        return "CARD_TOTAL";
    }

    return "UNKNOWN";
}

size_t alveo_power_get_num_samples(void)
{
    size_t count;

    pthread_mutex_lock(&power_lock);
    count = power_num_samples;
    pthread_mutex_unlock(&power_lock);

    return count;
}

const uint32_t *alveo_power_get_samples(void)
{
    return power_samples;
}

uint32_t alveo_power_get_average(void)
{
    uint64_t sum = 0;
    size_t count;

    pthread_mutex_lock(&power_lock);
    count = power_num_samples;
    for (size_t i = 0; i < count; ++i)
        sum += power_samples[i];
    pthread_mutex_unlock(&power_lock);

    if (count == 0)
        return 0;

    return (uint32_t)(sum / count);
}

uint32_t alveo_power_get_max(void)
{
    uint32_t max = 0;

    pthread_mutex_lock(&power_lock);
    for (size_t i = 0; i < power_num_samples; ++i) {
        if (power_samples[i] > max)
            max = power_samples[i];
    }
    pthread_mutex_unlock(&power_lock);

    return max;
}

/* -------------------------------------------------------------------------- */
/* HBICAP reconfiguration */
/* -------------------------------------------------------------------------- */

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
