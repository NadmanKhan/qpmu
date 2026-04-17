/* shared_buffer.h — shared memory layout for PRU0 firmware and ARM host
 *
 * Lives in PRUSS shared RAM at physical 0x4A310000 (12 KB).
 * PRU local address: CREGISTER 28 = 0x10000.
 *
 * Protocol:
 *   PRU fills buf[seq & 1], then writes seq (incrementing it).
 *   Host polls seq; when it changes, buf[seq & 1] has new data.
 */
#ifndef SHARED_BUFFER_H
#define SHARED_BUFFER_H

#include <stdint.h>

#define NUM_CHANNELS     6
#define NUM_SCANS        30
#define SAMPLES_PER_BUF  (NUM_CHANNELS * NUM_SCANS)

typedef struct {
    uint64_t timestamp_ns;
    uint16_t data[SAMPLES_PER_BUF];
} Buffer;

typedef struct {
    uint32_t seq;   /* buf[seq & 1] is complete when seq changes */
    uint32_t _pad;  /* keeps buf[] at 8-byte offset              */
    Buffer   buf[2];
} SharedMem;

#define PRUSS_SHARED_PHYS  0x4A310000U  /* AM335x TRM §4.3.1 */

#endif
