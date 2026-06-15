/*
 * test_shram.c — standalone test for PRU shared RAM data flow.
 *
 * Reads 10 sample frames from PRUSS shared RAM and prints seq, timestamp,
 * and first-scan channel values. Times out if PRU stops producing data.
 *
 * Build:  gcc -O2 -o test_shram test_shram.c
 * Run:    sudo ./test_shram
 */

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../shared/buffer.h"

int main(void)
{
    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    volatile void *base =
            mmap(NULL, PRUSS_SHARED_RAM_SIZE, PROT_READ, MAP_SHARED, fd, PRUSS_SHARED_RAM_PHYS);
    close(fd);
    if (base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    volatile Shared_State *st = (volatile Shared_State *)base;
    uint32_t last = st->seq;
    printf("Initial seq = %u\n", last);

    for (int i = 0; i < 10; i++) {
        uint32_t seq;
        int spins = 0;
        while ((seq = st->seq) == last && ++spins < 100000000)
            ;
        if (spins >= 100000000) {
            printf("TIMEOUT — PRU not producing data (seq stuck at %u)\n", last);
            break;
        }

        volatile Sample_Buffer *buf = &st->buf[seq & 1];
        uint64_t ts;
        memcpy(&ts, (void *)&buf->timestamp_ns, sizeof(ts));

        printf("seq=%u ts=%" PRIu64 "ns  ch0=%u ch1=%u ch2=%u ch3=%u ch4=%u ch5=%u\n", seq, ts,
               buf->samples[0], buf->samples[1], buf->samples[2], buf->samples[3],
               buf->samples[4], buf->samples[5]);
        last = seq;
    }

    munmap((void *)base, PRUSS_SHARED_RAM_SIZE);
    return 0;
}
