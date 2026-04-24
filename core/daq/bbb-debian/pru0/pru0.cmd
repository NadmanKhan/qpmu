/* pru0.cmd — PRU0 linker script for AM335x (only regions we use) */

-cr

MEMORY
{
    PAGE 0:
    PRU_IMEM       : org = 0x00000000 len = 0x00002000                 /* 8 KB instruction RAM */

    PAGE 1:
    PRU_DMEM_0_1   : org = 0x00000000 len = 0x00002000 CREGISTER=24   /* 8 KB data RAM */

    PAGE 2:
    PRU_SHAREDMEM  : org = 0x00010000 len = 0x00003000 CREGISTER=28   /* 12 KB shared RAM */

    PRU_CFG        : org = 0x00026000 len = 0x00000044 CREGISTER=4
    PRU_INTC       : org = 0x00020000 len = 0x00001504 CREGISTER=0
}

SECTIONS {
    .text:_c_int00* > 0x0, PAGE 0
    .text           > PRU_IMEM,     PAGE 0

    .stack          > PRU_DMEM_0_1, PAGE 1
    .bss            > PRU_DMEM_0_1, PAGE 1
    .cio            > PRU_DMEM_0_1, PAGE 1
    .data           > PRU_DMEM_0_1, PAGE 1
    .switch         > PRU_DMEM_0_1, PAGE 1
    .sysmem         > PRU_DMEM_0_1, PAGE 1
    .cinit          > PRU_DMEM_0_1, PAGE 1
    .rodata         > PRU_DMEM_0_1, PAGE 1
    .rofardata      > PRU_DMEM_0_1, PAGE 1
    .farbss         > PRU_DMEM_0_1, PAGE 1
    .fardata        > PRU_DMEM_0_1, PAGE 1

    .resource_table > PRU_DMEM_0_1, PAGE 1
}
