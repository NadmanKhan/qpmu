/* pru0_main.c — MCP3208 SPI bit-bang → PRUSS shared RAM
 *
 * Continuously samples NUM_CHANNELS of the MCP3208 ADC via bit-banged SPI,
 * accumulates NUM_SCANS scans into a double-buffer in PRUSS shared RAM, then
 * signals the ARM host by incrementing a sequence counter (see shared_buffer.h).
 *
 * Pins (PRU0 R30/R31):
 *   P9_31  mcasp0_aclkx  mode 5  R30[0]  CLK   (output)
 *   P9_29  mcasp0_fsx    mode 6  R31[1]  MISO  (input)
 *   P9_30  mcasp0_axr0   mode 5  R30[2]  MOSI  (output)
 *   P9_28  mcasp0_ahclkr mode 5  R30[3]  CS~   (output, active-low)
 *
 * MCP3208 timing (DS21298E, conservative for 3.3 V Vdd):
 *   Tsucs ≥ 100 ns → 20 cycles @ 200 MHz (5 ns/cycle)
 *   Thi/Tlo ≥ 200 ns → 40 cycles each
 *   Tcsh  ≥ 500 ns → 100 cycles
 */

#include <stdint.h>
#include <pru_cfg.h>
#include <pru_ctrl.h>

#include "resource_table_empty.h"
#include "../common/shared_buffer.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;

#define SCLK  (1u << 0)
#define MOSI  (1u << 2)
#define CS    (1u << 3)
#define MISO  (1u << 1)

#define T_SUCS  20
#define T_HI    40
#define T_LO    40
#define T_CSH  100

/* Single-ended channel control nibbles: SGL=1, D2:D0 = channel number */
static const uint8_t CTRL[NUM_CHANNELS] = {
    0b1000, 0b1001, 0b1010, 0b1011, 0b1100, 0b1101,
};

/* PRUSS shared RAM at CREGISTER 28 */
static volatile SharedMem *shmem = (volatile SharedMem *)0x10000;

/* ── SPI ─────────────────────────────────────────────────────────────────── */

static uint16_t mcp3208_read(uint8_t ctrl)
{
    uint8_t  i;
    uint16_t result = 0;

    __R30 &= ~CS;               /* CS low (assert)  */
    __R30 |=  MOSI;             /* start bit = HIGH */

    /* leading setup clock */
    __R30 &= ~SCLK; __delay_cycles(T_SUCS);
    __R30 |=  SCLK; __delay_cycles(T_HI);

    /* 4 control bits, MSB first: SGL + D2 D1 D0 */
    for (i = 0x8; i; i >>= 1) {
        __R30 &= ~SCLK;
        if (ctrl & i) __R30 |= MOSI; else __R30 &= ~MOSI;
        __delay_cycles(T_LO);
        __R30 |= SCLK; __delay_cycles(T_HI);
    }

    /* null/sample clock */
    __R30 &= ~SCLK; __delay_cycles(T_LO);
    __R30 |=  SCLK; __delay_cycles(T_HI);

    /* 13 clocks: null bit + B11..B0, MSB first */
    for (i = 0; i < 13; i++) {
        __R30 &= ~SCLK; __delay_cycles(T_LO);
        __R30 |=  SCLK;
        result = (uint16_t)((result << 1) | ((__R31 & MISO) ? 1u : 0u));
        __delay_cycles(T_HI);
    }

    __R30 |= CS;                /* CS high (deassert) */
    return result & 0x0FFF;
}

/* ── Timestamp (PRU0 cycle counter, 200 MHz = 5 ns/cycle) ───────────────── */

static uint64_t ext_cycles;

static uint64_t now_ns(void)
{
    uint32_t cyc = PRU0_CTRL.CYCLE;
    if (cyc & (1u << 31)) {     /* approaching overflow: extend and reset */
        PRU0_CTRL.CTRL_bit.CTR_EN = 0;
        ext_cycles += cyc;
        PRU0_CTRL.CYCLE = 4;
        PRU0_CTRL.CTRL_bit.CTR_EN = 1;
        cyc = PRU0_CTRL.CYCLE;
    }
    return (ext_cycles + cyc) * 5;
}

/* ── Main ────────────────────────────────────────────────────────────────── */

void main(void)
{
    uint32_t seq = 0;
    int i;

    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0; /* enable OCP master port for shared RAM */

    __R30 |= CS;                /* deassert CS */
    __R30 &= ~SCLK;            /* clock idle low */

    ext_cycles = 0;
    PRU0_CTRL.CYCLE = 0;
    PRU0_CTRL.CTRL_bit.CTR_EN = 1;

    shmem->seq = 0;

    for (;;) {
        seq++;
        volatile Buffer *b = &shmem->buf[seq & 1];

        b->timestamp_ns = now_ns();
        for (i = 0; i < SAMPLES_PER_BUF; i++) {
            b->data[i] = mcp3208_read(CTRL[i % NUM_CHANNELS]);
            __delay_cycles(T_CSH);
        }

        shmem->seq = seq;   /* publish: buf[seq & 1] is now complete */
    }
}
