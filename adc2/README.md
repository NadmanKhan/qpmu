# adc2 — PRU-based MCP3208 ADC for BeagleBone Black (Debian 13 / kernel 6.19)

## Architecture

PRU0 bit-bangs SPI to sample 6 channels of the MCP3208 ADC, accumulates
30 scans per buffer into a double-buffer in PRUSS shared RAM, and signals
completion by incrementing a sequence counter. The ARM host maps the shared
RAM read-only via `/dev/mem` and polls the counter.

```
MCP3208 ──SPI──► PRU0 ──shared RAM──► host program
                       (0x4A310000)    (sudo ./host_mcp3208)
```

No RPMsg, no kernel driver beyond the basic remoteproc loader.

> **Why not RPMsg?** The `pru_rproc` driver on kernel 6.19.11-bone14 does not
> implement `rproc->ops->kick`, which the virtio transport requires. Any
> firmware that includes a virtio vdev resource table entry will fail to start
> with `.kick method not defined`.

## Wiring

| BBB pin | MCP3208    | PRU0 register |
|---------|------------|---------------|
| P9_31   | CLK  (13)  | R30[0]        |
| P9_29   | DOUT (12)  | R31[1]        |
| P9_30   | DIN  (11)  | R30[2]        |
| P9_28   | CS~  (10)  | R30[3]        |

## Files

```
common/shared_buffer.h   shared data structures (PRU firmware + host)
pru0/pru0_main.c         PRU0 firmware
pru0/resource_table_empty.h
pru0/AM335x_PRU.cmd      PRU linker script (from SSP PRU_RPMsg_Echo_Interrupt0)
pru0/Makefile
overlay/BB-PRU-MCP3208-00A0.dtso   DT overlay: disable mcasp0, set PRU0 pins
host/host_mcp3208.c
Makefile
```

## Setup (one-time, requires reboot)

```bash
make -C adc2
sudo make -C adc2 install
```

Add to `/boot/uEnv.txt`:
```
disable_uboot_overlay_audio=1
uboot_overlay_addr4=BB-PRU-MCP3208-00A0.dtbo
#uboot_overlay_pru=AM335X-PRU-UIO-00A0.dtbo
```

```bash
sudo reboot
```

## Usage (after reboot)

```bash
sudo make -C adc2 deploy
adc2/host/host_mcp3208
```
