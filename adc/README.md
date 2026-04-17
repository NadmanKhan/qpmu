# pru-mcp3208-experiments

To build, download the PRU software support package, from
<https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/>
and extract to `/usr/share/ti/pru-software-support-package`. It can be
extracted to another path as long as the `PRU_SSP` environment variable points
to it and is exported.

Aditionally, the PRU code generation tools must be downloaded from
<https://www.ti.com/tool/download/PRU-CGT-2-1/> and installed in
`/usr/share/ti/cgt-pru`. If it is installed in a different path, the
`PRU_CGT` environment variable must be exported and point to it.

For cross-compilation, export `CC` to the appropriate cross-compiler, like
`export CC=armv7a-hardfloat-linux-gnueabihf-gcc`.

To compile, simply run `make`. To load the firmware into the PRUs, run
`make deploy` in the BeagleBone Black. It also configures the relevant pins.
The ARM Host program, `host_rpmsg_mcp3208` reads the data and prints the first
4 readings of each buffer into the STDOUT, together with the timestamp and
time difference between consecutive messages.

## Pinout

- P9_28: Chip Select
- P9_29: SPI MISO
- P9_30: SPI MOSI
- P9_31: SPI Clock

## Notes by Nadman for the Debian 13.4 (Trixie) image of BBB

- The README is a little outdated:
  - The expected location for SSP is `/usr/lib/ti/pru-software-support-package` instead of `/usr/share/ti/pru-software-support-package`.
- The PRU code generation tools (CGT) package is available at the correct location
- The PRU software support package (SSP) needs to be resolved this way:
  - Clone the repository:

```bash
sudo git clone https://git.ti.com/git/pru-software-support-package/pru-software-support-package.git /usr/lib/ti/pru-software-support-package
```

    - Copy the linker command scripts into expected locations. **Important:** Use the
      `PRU_RPMsg_Echo_Interrupt0` (or `Interrupt1`) example's `AM335x_PRU.cmd`, **not** the
      `PRU_Halt` example — the Halt example lacks the `.resource_table` section mapping
      that the remoteproc driver needs to discover the RPMsg resource table.

```bash
cp /usr/lib/ti/pru-software-support-package/examples/am335x/PRU_RPMsg_Echo_Interrupt0/AM335x_PRU.cmd adc/pru0_mcp3208_comm/
cp /usr/lib/ti/pru-software-support-package/examples/am335x/PRU_RPMsg_Echo_Interrupt1/AM335x_PRU.cmd adc/pru1_mcp3208_rpmsg/
```

      Alternatively, manually add this line to the `SECTIONS` block in both `AM335x_PRU.cmd` files:

```
	.resource_table > PRU_DMEM_0_1, PAGE 1
```
