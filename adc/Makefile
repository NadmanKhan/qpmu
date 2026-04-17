TOPTARGETS := all clean install deploy
SUBDIRS := pru0_mcp3208_comm/ pru1_mcp3208_rpmsg/

OVERLAY_SRC := BB-PRU-MCP3208-00A0.dtso
OVERLAY_BIN := BB-PRU-MCP3208-00A0.dtbo
OVERLAY_DIR := /boot/dtbs/$(shell uname -r)/overlays
DT_INCLUDE  := /opt/source/dtb-6.18.x/include

PIN_CFG_SRC := scripts/config-pru-pins.c
PIN_CFG_BIN := scripts/config-pru-pins

all: host_rpmsg_mcp3208 $(OVERLAY_BIN) $(PIN_CFG_BIN)
$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

host_rpmsg_mcp3208: host_rpmsg_mcp3208.c
	$(CC) -o $@ $^

$(OVERLAY_BIN): $(OVERLAY_SRC)
	cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -I$(DT_INCLUDE) $< | \
		dtc -I dts -O dtb -o $@ -@

# Installs the DT overlay to /boot so it is loaded at next boot.
# After running this, edit /boot/uEnv.txt (see adc/README.md) and reboot.
$(PIN_CFG_BIN): $(PIN_CFG_SRC)
	$(CC) -O2 -o $@ $^

install-overlay: $(OVERLAY_BIN)
	cp $(OVERLAY_BIN) $(OVERLAY_DIR)/$(OVERLAY_BIN)

deploy: install-overlay

clean:
	rm -f host_rpmsg_mcp3208 $(OVERLAY_BIN) $(PIN_CFG_BIN)

.PHONY: $(TOPTARGETS) $(SUBDIRS) install-overlay
