/* PRU ↔ host shared memory layout.
 *
 * PRUSS shared RAM: physical 0x4A310000, PRU-local 0x10000 (CREGISTER 28).
 * Protocol: PRU fills buf[seq & 1], increments seq. Host polls seq.
 */
#ifndef PRU_SHARED_BUFFER_H
#define PRU_SHARED_BUFFER_H

#include <stdint.h>

#define NUM_CHANNELS    6
#define NUM_SCANS       30
#define SAMPLES_PER_BUF (NUM_CHANNELS * NUM_SCANS)

/* __packed__: TI clpru and ARM GCC may pad differently without it. */
typedef struct __attribute__((__packed__)) {
    uint64_t timestamp_ns;
    uint16_t samples[SAMPLES_PER_BUF];
} Sample_Buffer;

typedef struct __attribute__((__packed__)) {
    uint32_t seq;
    uint32_t _pad; /* 8-byte alignment for buf[] */
    Sample_Buffer buf[2];
} Shared_State;

#define PRUSS_SHARED_RAM_PHYS 0x4A310000U /* AM335x TRM §4.3.1 */
#define PRUSS_SHARED_RAM_SIZE 0x3000U     /* 12 KB */

#endif
