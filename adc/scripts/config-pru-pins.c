/*
 * config-pru-pins.c
 *
 * Configure P9_28-P9_31 pad mux registers for PRU0 bit-banged SPI (MCP3208).
 *
 * The pru_rproc kernel driver does not support pinctrl bindings, so pin
 * configuration cannot be done via device tree alone. This program writes
 * directly to the AM335x pad configuration registers via /dev/mem.
 *
 * Must be run as root (sudo).
 *
 * Register addresses (AM335x TRM, Table 9-10):
 *   P9_31  mcasp0_aclkx  0x44E10990  mode 5 -> pr1_pru0_pru_r30[0]  output  CLK
 *   P9_29  mcasp0_fsx    0x44E10994  mode 6 -> pr1_pru0_pru_r31[1]  input   MISO
 *   P9_30  mcasp0_axr0   0x44E10998  mode 5 -> pr1_pru0_pru_r30[2]  output  MOSI
 *   P9_28  mcasp0_ahclkr 0x44E1099C  mode 5 -> pr1_pru0_pru_r30[3]  output  CS
 *
 * Pad config register bit fields (function-mask = 0x7F):
 *   [2:0]  mux mode
 *   [3]    pull type      (0=pull-down,   1=pull-up)
 *   [4]    pull disable   (0=pull enabled, 1=pull disabled)
 *   [5]    input enable   (0=output only,  1=input sampled)
 *   [6]    slew control   (0=fast,         1=slow)
 *
 * Values used:
 *   PIN_OUTPUT = pull disabled (bit4=1)                    = 0x08 | mode
 *   PIN_INPUT  = pull disabled (bit4=1) + input (bit5=1)   = 0x28 | mode
 *
 * Pin modes verified against:
 *   dtb-5.10-ti/src/arm/am335x-bonegreen-wireless-common-univ.dtsi
 *   dtb-5.10-ti/src/arm/am335x-boneblack-pruswuart.dts
 *   dtb-6.18.x/src/arm/ti/omap/am335x-pocketbeagle.dts
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define PADCONF_BASE  0x44E10000U
#define PAGE_SIZE     0x1000U
#define FUNC_MASK     0x7FU

struct pin_cfg {
    uint32_t    addr;
    uint8_t     target;   /* 7-bit pad config value */
    const char *desc;
};

static const struct pin_cfg pins[] = {
    { 0x44E10990, 0x08 | 5, "P9_31 (CLK,  R30[0]): output mode 5" },
    { 0x44E10994, 0x28 | 6, "P9_29 (MISO, R31[1]): input  mode 6" },
    { 0x44E10998, 0x08 | 5, "P9_30 (MOSI, R30[2]): output mode 5" },
    { 0x44E1099C, 0x08 | 5, "P9_28 (CS,   R30[3]): output mode 5" },
};

int main(void)
{
    /* O_SYNC: prevents write buffering for hardware register access */
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem (must be run as root)");
        return 1;
    }

    volatile uint32_t *regs = mmap(NULL, PAGE_SIZE,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   fd, PADCONF_BASE);
    if (regs == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    int ok = 1;
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        uint32_t idx     = (pins[i].addr - PADCONF_BASE) / sizeof(uint32_t);
        uint8_t  before  = regs[idx] & FUNC_MASK;
        regs[idx]        = (regs[idx] & ~FUNC_MASK) | (pins[i].target & FUNC_MASK);
        uint8_t  after   = regs[idx] & FUNC_MASK;
        int      passed  = (after == (pins[i].target & FUNC_MASK));
        if (!passed) ok = 0;
        printf("  %s\n    0x%08X: 0x%02X -> 0x%02X  %s\n",
               pins[i].desc, pins[i].addr, before, after,
               passed ? "OK" : "FAILED");
    }

    munmap((void *)regs, PAGE_SIZE);
    close(fd);

    if (!ok) {
        fprintf(stderr,
            "\nWARNING: one or more pin writes failed.\n"
            "If mcasp0 still owns P9_28/P9_29/P9_31, install the DT overlay,\n"
            "update /boot/uEnv.txt, and reboot (see adc/README.md).\n");
        return 1;
    }

    puts("Done.");
    return 0;
}
