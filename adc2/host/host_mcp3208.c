/* host_mcp3208.c — read MCP3208 ADC samples from PRU0 via shared RAM
 *
 * PRU0 fills double-buffers in PRUSS shared RAM and signals completion by
 * incrementing a sequence counter. This program maps that memory read-only
 * via /dev/mem and prints one scan per buffer as it arrives.
 *
 * Requires membership in the `kmem` group (the `pmu` user already is).
 * Usage:  ./host_mcp3208
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../common/shared_buffer.h"

#define MAP_LEN  0x3000U    /* 12 KB — full PRUSS shared RAM */

int main(void)
{
    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "open /dev/mem: %s\n", strerror(errno));
        return 1;
    }

    const volatile SharedMem *shm = mmap(NULL, MAP_LEN,
                                         PROT_READ, MAP_SHARED,
                                         fd, PRUSS_SHARED_PHYS);
    close(fd);

    if (shm == MAP_FAILED) {
        fprintf(stderr, "mmap: %s\n", strerror(errno));
        return 1;
    }

    uint32_t last_seq = shm->seq;
    uint64_t prev_ts  = 0;
    int      i;

    for (;;) {
        uint32_t seq;
        while ((seq = shm->seq) == last_seq)
            ;

        const volatile Buffer *buf = &shm->buf[seq & 1];
        uint64_t ts = buf->timestamp_ns;

        for (i = 0; i < NUM_CHANNELS; i++)
            printf("ch%d=%4" PRIu16 "  ", i, buf->data[i]);
        printf("ts=%" PRIu64 " ns  dt=%" PRIu64 " ns\n", ts, ts - prev_ts);

        prev_ts  = ts;
        last_seq = seq;
    }
}
