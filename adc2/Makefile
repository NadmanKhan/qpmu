PRU_CGT     ?= /usr/share/ti/cgt-pru
DT_INCLUDE  := /opt/source/dtb-6.18.x/include
OVERLAY_DIR := /boot/dtbs/$(shell uname -r)/overlays
RPROC0      := /sys/class/remoteproc/remoteproc1

OVERLAY_SRC := overlay/BB-PRU-MCP3208-00A0.dtso
OVERLAY_BIN := overlay/BB-PRU-MCP3208-00A0.dtbo

all: pru0 $(OVERLAY_BIN) host/host_mcp3208

pru0:
	$(MAKE) -C pru0 PRU_CGT=$(PRU_CGT)

$(OVERLAY_BIN): $(OVERLAY_SRC)
	cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
	    -I$(DT_INCLUDE) $< | dtc -I dts -O dtb -o $@ -@

host/host_mcp3208: host/host_mcp3208.c common/shared_buffer.h
	$(CC) -O2 -I. -o $@ $<

# ── install: copies overlay to /boot (requires reboot to take effect) ──────
# Edit /boot/uEnv.txt after running:
#   disable_uboot_overlay_audio=1
#   uboot_overlay_addr4=BB-PRU-MCP3208-00A0.dtbo
#   #uboot_overlay_pru=AM335X-PRU-UIO-00A0.dtbo

install: all
	cp pru0/gen/pru0.out /lib/firmware/pru0_mcp3208
	cp $(OVERLAY_BIN) $(OVERLAY_DIR)/

# ── deploy: (re)start PRU0; installs firmware first ────────────────────────

deploy: all
	cp pru0/gen/pru0.out /lib/firmware/pru0_mcp3208
	-echo stop  > $(RPROC0)/state
	echo pru0_mcp3208 > $(RPROC0)/firmware
	echo start  > $(RPROC0)/state

clean:
	$(MAKE) -C pru0 clean
	rm -f $(OVERLAY_BIN) host/host_mcp3208

.PHONY: all pru0 install deploy clean
