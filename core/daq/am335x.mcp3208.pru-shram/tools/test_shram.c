/*
 * test_shram.c - standalone test for PRU shared RAM data flow.
 *
 * Reads sample frames from PRUSS shared RAM and prints seq, timestamp, status,
 * heartbeat, first-scan channel values, and averaged channel values. Times out
 * if PRU stops producing data. Optional --expect CH:MIN:MAX checks averaged
 * values against known hardware readings.
 *
 * Build:  gcc -O2 -o test_shram test_shram.c
 * Run:    sudo ./test_shram
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "../shared/buffer.h"

static const char *status_name(uint32_t status)
{
    switch (status) {
    case QPMU_PRU_STATUS_BOOTING:
        return "booting";
    case QPMU_PRU_STATUS_FILLING:
        return "filling";
    case QPMU_PRU_STATUS_PUBLISHED:
        return "published";
    default:
        return "unknown";
    }
}

static uint64_t monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void print_state(const volatile Shared_State *st, const char *prefix)
{
    printf("%s magic=0x%08" PRIx32 " seq=%" PRIu32 " heartbeat=%" PRIu32
           " status=%" PRIu32 "(%s) sample=%" PRIu32 "\n",
           prefix, st->magic, st->seq, st->heartbeat, st->status, status_name(st->status),
           st->sample_index);
}

static int parse_int_arg(const char *value, const char *name)
{
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 0) {
        fprintf(stderr, "Invalid %s: %s\n", name, value);
        exit(2);
    }
    return (int)parsed;
}

static void parse_expect_arg(const char *value, int enabled[NUM_CHANNELS],
                             int min_adc[NUM_CHANNELS], int max_adc[NUM_CHANNELS])
{
    int ch;
    int lo;
    int hi;

    if (sscanf(value, "%d:%d:%d", &ch, &lo, &hi) != 3 || ch < 0 || ch >= NUM_CHANNELS
        || lo < 0 || hi > 4095 || lo > hi) {
        fprintf(stderr, "Invalid --expect %s; use CH:MIN:MAX with ADC counts 0-4095\n", value);
        exit(2);
    }

    enabled[ch] = 1;
    min_adc[ch] = lo;
    max_adc[ch] = hi;
}

int main(int argc, char **argv)
{
    int clear = 0;
    int frames = 10;
    int timeout_ms = 2000;
    int expect_enabled[NUM_CHANNELS] = { 0 };
    int expect_min[NUM_CHANNELS] = { 0 };
    int expect_max[NUM_CHANNELS] = { 0 };
    int failed_expectation = 0;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--clear") == 0) {
            clear = 1;
        } else if (strcmp(argv[i], "--frames") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "--frames requires a value\n");
                return 2;
            }
            frames = parse_int_arg(argv[i], "--frames");
        } else if (strcmp(argv[i], "--timeout-ms") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "--timeout-ms requires a value\n");
                return 2;
            }
            timeout_ms = parse_int_arg(argv[i], "--timeout-ms");
        } else if (strcmp(argv[i], "--expect") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "--expect requires CH:MIN:MAX\n");
                return 2;
            }
            parse_expect_arg(argv[i], expect_enabled, expect_min, expect_max);
        } else {
            fprintf(stderr,
                    "Usage: %s [--clear] [--frames N] [--timeout-ms N]"
                    " [--expect CH:MIN:MAX]...\n",
                    argv[0]);
            return 2;
        }
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    volatile void *base =
            mmap(NULL, PRUSS_SHARED_RAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                 PRUSS_SHARED_RAM_PHYS);
    close(fd);
    if (base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    volatile Shared_State *st = (volatile Shared_State *)base;
    if (clear) {
        memset((void *)st, 0, sizeof(*st));
        msync((void *)st, sizeof(*st), MS_SYNC);
        print_state(st, "Cleared shared RAM:");
        if (frames == 0) {
            munmap((void *)base, PRUSS_SHARED_RAM_SIZE);
            return 0;
        }
    }

    {
        uint64_t deadline = monotonic_ns() + (uint64_t)timeout_ms * 1000000ULL;
        while (st->magic != QPMU_PRU_MAGIC && monotonic_ns() < deadline) {
            usleep(1000);
        }
    }

    print_state(st, "Initial state:");
    if (st->magic != QPMU_PRU_MAGIC) {
        printf("TIMEOUT: QPMU PRU firmware did not publish magic 0x%08" PRIx32 "\n",
               (uint32_t)QPMU_PRU_MAGIC);
        munmap((void *)base, PRUSS_SHARED_RAM_SIZE);
        return 1;
    }

    uint32_t last = st->seq;

    for (i = 0; i < frames; i++) {
        uint32_t seq;
        uint32_t start_heartbeat = st->heartbeat;
        uint64_t deadline = monotonic_ns() + (uint64_t)timeout_ms * 1000000ULL;

        while ((seq = st->seq) == last && monotonic_ns() < deadline) {
            usleep(1000);
        }
        if (seq == last) {
            printf("TIMEOUT: PRU did not publish a new frame (seq stuck at %" PRIu32
                   ", heartbeat %" PRIu32 "->%" PRIu32 ")\n",
                   last, start_heartbeat, st->heartbeat);
            print_state(st, "Final state:");
            break;
        }

        volatile Sample_Buffer *buf = &st->buf[seq & 1];
        uint64_t ts;
        uint32_t avg[NUM_CHANNELS];
        memcpy(&ts, (void *)&buf->timestamp_ns, sizeof(ts));

        printf("seq=%" PRIu32 " heartbeat=%" PRIu32 " status=%s sample=%" PRIu32
               " ts=%" PRIu64 "ns  first=%u,%u,%u,%u,%u,%u",
               seq, st->heartbeat, status_name(st->status), st->sample_index, ts,
               buf->samples[0], buf->samples[1], buf->samples[2], buf->samples[3],
               buf->samples[4], buf->samples[5]);

        printf(" avg=");
        for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
            uint32_t sum = 0;
            for (int scan = 0; scan < NUM_SCANS; ++scan)
                sum += buf->samples[scan * NUM_CHANNELS + ch];
            avg[ch] = sum / NUM_SCANS;
            printf("%s%" PRIu32, ch == 0 ? "" : ",", avg[ch]);
        }
        printf("\n");

        for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
            if (!expect_enabled[ch])
                continue;
            if ((int)avg[ch] < expect_min[ch] || (int)avg[ch] > expect_max[ch]) {
                fprintf(stderr, "ADC ch%d expected %d-%d, got %" PRIu32 "\n",
                        ch, expect_min[ch], expect_max[ch], avg[ch]);
                failed_expectation = 1;
            }
        }

        last = seq;
    }

    munmap((void *)base, PRUSS_SHARED_RAM_SIZE);
    return failed_expectation ? 1 : 0;
}
