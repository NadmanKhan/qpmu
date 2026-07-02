/*
 * Configure P9_28-P9_31 pad mux registers for PRU0 bit-banged MCP3208 SPI.
 *
 * The boot overlay should own these pins. This tool verifies the registers and
 * attempts a direct fallback write on images that permit it.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PADCONF_BASE 0x44E10000U
#define PAGE_SIZE 0x1000U
#define FUNC_MASK 0x7FU

struct Pin_Config
{
    uint32_t addr;
    uint8_t target;
    const char *desc;
};

static const struct Pin_Config pins[] = {
    { 0x44E10990U, 0x08U | 5U, "P9_31 SCLK  R30[0] output mode 5" },
    { 0x44E10994U, 0x28U | 6U, "P9_29 MISO  R31[1] input  mode 6" },
    { 0x44E10998U, 0x08U | 5U, "P9_30 MOSI  R30[2] output mode 5" },
    { 0x44E1099CU, 0x08U | 5U, "P9_28 CS    R30[3] output mode 5" },
};

int main(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    volatile uint32_t *regs =
            mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PADCONF_BASE);
    close(fd);
    if (regs == MAP_FAILED) {
        perror("mmap padconf");
        return 1;
    }

    int ok = 1;
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        uint32_t idx = (pins[i].addr - PADCONF_BASE) / sizeof(uint32_t);
        uint8_t before = regs[idx] & FUNC_MASK;
        regs[idx] = (regs[idx] & ~FUNC_MASK) | (pins[i].target & FUNC_MASK);
        uint8_t after = regs[idx] & FUNC_MASK;
        int matched = after == (pins[i].target & FUNC_MASK);

        printf("%s: 0x%02x -> 0x%02x expected 0x%02x %s\n",
               pins[i].desc, before, after, pins[i].target, matched ? "OK" : "FAIL");
        if (!matched)
            ok = 0;
    }

    munmap((void *)regs, PAGE_SIZE);
    return ok ? 0 : 1;
}
