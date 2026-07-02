/* pru0_main.c - MCP3208 SPI bit-bang -> PRUSS shared RAM
 *
 * Samples NUM_CHANNELS channels of the MCP3208 via bit-banged SPI, fills
 * double-buffered shared RAM, and signals the host via sequence counter.
 *
 * Pin mapping (PRU0 R30/R31):
 *   P9_31  mcasp0_aclkx   mode 5  R30[0]  SCLK  (output)
 *   P9_29  mcasp0_fsx     mode 6  R31[1]  MISO  (input)
 *   P9_30  mcasp0_axr0    mode 5  R30[2]  MOSI  (output)
 *   P9_28  mcasp0_ahclkr  mode 5  R30[3]  CS    (output, active-low)
 *
 * MCP3208 timing at 3.3 V (DS21298E, conservative):
 *   Tsucs >= 100 ns  ->  20 cycles @ 200 MHz
 *   Thi   >= 200 ns  ->  40 cycles
 *   Tlo   >= 200 ns  ->  40 cycles
 *   Tcsh  >= 500 ns  -> 100 cycles
 */

#include <stdint.h>
#include <pru_cfg.h>
#include <pru_ctrl.h>

#include "../shared/buffer.h"
#include "resource_table_empty.h"

/* -- GPIO pin masks (bit positions in R30/R31) ----------------------------- */

#ifdef __TI_COMPILER_VERSION__
volatile register uint32_t __R30;
volatile register uint32_t __R31;
#endif

#define PIN_SCLK (1u << 0) /* R30[0] output */
#define PIN_MISO (1u << 1) /* R31[1] input  */
#define PIN_MOSI (1u << 2) /* R30[2] output */
#define PIN_CS (1u << 3) /* R30[3] output */

/* -- MCP3208 SPI timing (PRU cycles, 200 MHz = 5 ns/cycle) ----------------- */

#define DELAY_CS_SETUP 20 /* Tsucs: CS assert to first clock */
#define DELAY_CLK_HIGH 40 /* Thi:   clock high half-period   */
#define DELAY_CLK_LOW 40 /* Tlo:   clock low half-period    */
#define DELAY_CS_HOLD 100 /* Tcsh:  CS deassert hold time    */

/* Single-ended control nibbles per channel: start=1, SGL=1, D2:D0 */
static const uint8_t channel_ctrl[NUM_CHANNELS] = {
    0b1000, 0b1001, 0b1010, 0b1011, 0b1100, 0b1101,
};

/* -- Shared memory --------------------------------------------------------- */

static volatile Shared_State *shared = (volatile Shared_State *)0x10000;

/* -- SPI transaction ------------------------------------------------------- */

static uint16_t mcp3208_read(uint8_t ctrl)
{
    uint8_t i;
    uint16_t result = 0;

    __R30 &= ~PIN_CS;
    __R30 |= PIN_MOSI; /* start bit */

    __R30 &= ~PIN_SCLK;
    __delay_cycles(DELAY_CS_SETUP);
    __R30 |= PIN_SCLK;
    __delay_cycles(DELAY_CLK_HIGH);

    /* 4 control bits (MSB first): SGL + D2 D1 D0 */
    for (i = 0x8; i; i >>= 1) {
        __R30 &= ~PIN_SCLK;
        if (ctrl & i)
            __R30 |= PIN_MOSI;
        else
            __R30 &= ~PIN_MOSI;
        __delay_cycles(DELAY_CLK_LOW);
        __R30 |= PIN_SCLK;
        __delay_cycles(DELAY_CLK_HIGH);
    }

    /* Null/sample clock */
    __R30 &= ~PIN_SCLK;
    __delay_cycles(DELAY_CLK_LOW);
    __R30 |= PIN_SCLK;
    __delay_cycles(DELAY_CLK_HIGH);

    /* 13 clocks: null bit + B11..B0 (MSB first) */
    for (i = 0; i < 13; i++) {
        __R30 &= ~PIN_SCLK;
        __delay_cycles(DELAY_CLK_LOW);
        __R30 |= PIN_SCLK;
        result = (uint16_t)((result << 1) | ((__R31 & PIN_MISO) ? 1u : 0u));
        __delay_cycles(DELAY_CLK_HIGH);
    }

    __R30 |= PIN_CS;
    return result & 0x0FFF;
}

/* -- 64-bit timestamp from PRU0 cycle counter (200 MHz = 5 ns/cycle) ------- */

static uint64_t accumulated_cycles;

static uint64_t timestamp_ns(void)
{
    uint32_t cycles = PRU0_CTRL.CYCLE;
    if (cycles & (1u << 31)) {
        PRU0_CTRL.CTRL_bit.CTR_EN = 0;
        accumulated_cycles += cycles;
        PRU0_CTRL.CYCLE = 4;
        PRU0_CTRL.CTRL_bit.CTR_EN = 1;
        cycles = PRU0_CTRL.CYCLE;
    }
    return (accumulated_cycles + cycles) * 5;
}

/* -- Entry point ----------------------------------------------------------- */

int main(void)
{
    uint32_t seq = 0;
    volatile Sample_Buffer *buf;
    int i;

    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0; /* enable OCP master port */

    __R30 |= PIN_CS;
    __R30 &= ~PIN_SCLK;

    accumulated_cycles = 0;
    PRU0_CTRL.CYCLE = 0;
    PRU0_CTRL.CTRL_bit.CTR_EN = 1;

    shared->magic = QPMU_PRU_MAGIC;
    shared->heartbeat = 0;
    shared->status = QPMU_PRU_STATUS_BOOTING;
    shared->sample_index = 0;
    shared->seq = 0;

    for (;;) {
        ++seq;
        ++shared->heartbeat;
        shared->status = QPMU_PRU_STATUS_FILLING;
        shared->sample_index = 0;
        buf = &shared->buf[seq & 1];

        buf->timestamp_ns = timestamp_ns();
        for (i = 0; i < SAMPLES_PER_BUF; i++) {
            shared->sample_index = (uint32_t)i;
            buf->samples[i] = mcp3208_read(channel_ctrl[i % NUM_CHANNELS]);
            __delay_cycles(DELAY_CS_HOLD);
        }

        shared->status = QPMU_PRU_STATUS_PUBLISHED;
        shared->seq = seq;
        ++shared->heartbeat;
    }

    return 0;
}
