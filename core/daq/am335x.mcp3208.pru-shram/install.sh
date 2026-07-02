#!/usr/bin/env bash
#
# install.sh - install QPMU ADC + GPS support on BeagleBone Black.
#
# Run from the repo root:  sudo ./core/daq/am335x.mcp3208.pru-shram/install.sh
#
set -euo pipefail

# -- Paths --------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

OVERLAY_DIR="$SCRIPT_DIR/overlay"
CONFIG_DIR="$SCRIPT_DIR/config"
PRU0_DIR="$SCRIPT_DIR/pru0"
SHARED_DIR="$SCRIPT_DIR/shared"

PRU_CGT="${PRU_CGT:-/usr/share/ti/cgt-pru}"
PRU_SSP="${PRU_SSP:-$REPO_ROOT/external/pru-software-support-package}"

BUILD_DIR="/tmp/qpmu-am335x-mcp3208-pru-shram-build"

# -- Helpers ------------------------------------------------------------------

info()  { printf '\033[1;34m>> %s\033[0m\n' "$*"; }
ok()    { printf '\033[1;32m   OK: %s\033[0m\n' "$*"; }
fail()  { printf '\033[1;31m   FAIL: %s\033[0m\n' "$*"; exit 1; }
warn()  { printf '\033[1;33m   WARN: %s\033[0m\n' "$*"; }

require_root() {
    [[ $EUID -eq 0 ]] || fail "must run as root (sudo)"
}

set_uenv_line() {
    local file="$1" key="$2" value="$3"

    if grep -q "^${key}=" "$file"; then
        sed -i "s|^${key}=.*|${key}=${value}|" "$file"
    else
        echo "${key}=${value}" >> "$file"
    fi
}

# -- Phase 0: preflight ------------------------------------------------------

phase0_preflight() {
    info "Phase 0: preflight checks"

    require_root

    KERNEL="$(uname -r)"
    [[ "$KERNEL" == 6.* ]] || warn "expected kernel 6.x, got $KERNEL"
    ok "kernel $KERNEL"

    if [[ -f /etc/debian_version ]]; then
        DEBIAN="$(cat /etc/debian_version)"
        ok "Debian $DEBIAN"
    fi

    # DT include path
    DT_INCLUDE=""
    for candidate in /opt/source/dtb-*/include; do
        if [[ -f "$candidate/dt-bindings/pinctrl/am33xx.h" ]]; then
            DT_INCLUDE="$candidate"
            break
        fi
    done
    [[ -n "$DT_INCLUDE" ]] || fail "cannot find dt-bindings/pinctrl/am33xx.h - set DT_INCLUDE"
    ok "DT includes: $DT_INCLUDE"

    # PRU toolchain
    [[ -x "$PRU_CGT/bin/clpru" ]] || fail "clpru not found at $PRU_CGT/bin/clpru"
    ok "PRU CGT: $PRU_CGT"

    mkdir -p "$BUILD_DIR"
}

# -- Phase 1: overlays -------------------------------------------------------

compile_overlay() {
    local src="$1" name="$2"
    local pp="$BUILD_DIR/${name}.pp.dts"
    local out="$BUILD_DIR/${name}.dtbo"

    cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
        -I"$DT_INCLUDE" "$src" -o "$pp"
    dtc -I dts -O dtb -@ -o "$out" "$pp" 2>/dev/null
    [[ -s "$out" ]] || fail "overlay compile failed: $name"
    ok "compiled $name"
}

phase1_overlays() {
    info "Phase 1: compile and install device tree overlays"

    local overlay_dest="/lib/firmware"
    mkdir -p "$overlay_dest"

    compile_overlay "$OVERLAY_DIR/BB-PRU-MCP3208-00A0.dtso" "BB-PRU-MCP3208-00A0"
    compile_overlay "$OVERLAY_DIR/BB-UART1-00A0.dtso"       "BB-UART1-00A0"
    compile_overlay "$OVERLAY_DIR/BB-PPS-00A0.dtso"         "BB-PPS-00A0"

    cp "$BUILD_DIR"/BB-PRU-MCP3208-00A0.dtbo "$overlay_dest/"
    cp "$BUILD_DIR"/BB-UART1-00A0.dtbo       "$overlay_dest/"
    cp "$BUILD_DIR"/BB-PPS-00A0.dtbo         "$overlay_dest/"
    ok "overlays installed to $overlay_dest"

    # Update uEnv.txt if needed
    local uenv="/boot/uEnv.txt"
    if [[ -f "$uenv" ]]; then
        cp "$uenv" "${uenv}.backup.$(date +%Y%m%d%H%M%S)"

        # Ensure overlays enabled
        set_uenv_line "$uenv" enable_uboot_overlays 1

        # Disable audio (frees mcasp0 pins for PRU)
        set_uenv_line "$uenv" disable_uboot_overlay_audio 1

        # Comment out old PRU overlay
        sed -i 's|^uboot_overlay_pru=.*|#&|' "$uenv"

        # Add our overlays if not present
        set_uenv_line "$uenv" uboot_overlay_addr4 /lib/firmware/BB-PRU-MCP3208-00A0.dtbo
        set_uenv_line "$uenv" uboot_overlay_addr5 /lib/firmware/BB-UART1-00A0.dtbo
        set_uenv_line "$uenv" uboot_overlay_addr6 /lib/firmware/BB-PPS-00A0.dtbo

        ok "uEnv.txt updated (backup saved)"
    else
        warn "no /boot/uEnv.txt found - configure overlays manually"
    fi
}

# -- Phase 2: PRU firmware ---------------------------------------------------

phase2_pru() {
    info "Phase 2: build and deploy PRU0 firmware"

    local obj="$BUILD_DIR/pru0_main.object"
    local out="$BUILD_DIR/pru0.out"

    "$PRU_CGT/bin/clpru" \
        --include_path="$PRU_CGT/include" \
        --include_path="$PRU_SSP/include" \
        --include_path="$PRU_SSP/include/am335x" \
        --include_path="$SHARED_DIR" \
        -v3 -O2 --display_error_number --endian=little --hardware_mac=on \
        --obj_directory="$BUILD_DIR" --pp_directory="$BUILD_DIR" \
        --asm_directory="$BUILD_DIR" \
        -fe "$obj" "$PRU0_DIR/pru0_main.c"
    ok "PRU0 compiled"

    "$PRU_CGT/bin/clpru" -v3 -z \
        -i"$PRU_CGT/lib" -i"$PRU_CGT/include" \
        --reread_libs --warn_sections \
        --stack_size=0x100 --heap_size=0x100 \
        -o "$out" "$obj" \
        -m"$BUILD_DIR/pru0.map" \
        "$PRU0_DIR/pru0.cmd" \
        --library=libc.a
    ok "PRU0 linked"

    cp "$out" /lib/firmware/pru0_mcp3208
    ok "PRU0 firmware installed"
}

# -- Phase 3: GPS / PPS config -----------------------------------------------

phase3_gps() {
    info "Phase 3: configure gpsd + chrony for GPS/PPS time sync"

    # Install packages if missing
    for pkg in gpsd gpsd-clients pps-tools chrony; do
        if ! dpkg -s "$pkg" >/dev/null 2>&1; then
            apt-get install -y "$pkg"
        fi
    done
    ok "packages installed"

    # Stop competing time services
    systemctl stop ntp 2>/dev/null || true
    systemctl disable ntp 2>/dev/null || true
    systemctl stop systemd-timesyncd 2>/dev/null || true
    systemctl disable systemd-timesyncd 2>/dev/null || true

    # Install gpsd config
    cp "$CONFIG_DIR/gpsd.conf" /etc/default/gpsd
    ok "gpsd config installed"

    # Append chrony GPS refclocks (if not already present)
    local chrony_conf="/etc/chrony/chrony.conf"
    if [[ -f "$chrony_conf" ]] && ! grep -q 'refid PPS' "$chrony_conf"; then
        printf '\n' >> "$chrony_conf"
        cat "$CONFIG_DIR/chrony-gps.conf" >> "$chrony_conf"
        ok "chrony GPS config appended"
    else
        ok "chrony GPS config already present"
    fi

    # Restart services
    systemctl restart gpsd
    systemctl restart chrony
    ok "gpsd and chrony restarted"
}

# -- Summary ------------------------------------------------------------------

summary() {
    info "Setup complete. Checklist:"
    echo ""
    echo "  [ ] Reboot to load overlays if this was the first install:  sudo reboot"
    echo "  [ ] After reboot, start PRU0 and verify pinmux:  sudo $SCRIPT_DIR/start-pru.sh"
    echo "  [ ] Verify ADC ranges:  sudo $SCRIPT_DIR/start-pru.sh --expect 0:MIN:MAX"
    echo "  [ ] Test GPS:  cgps -s"
    echo "  [ ] Test PPS:  sudo ppstest /dev/pps0"
    echo "  [ ] Verify chrony:  chronyc sources -v"
    echo ""
    echo "  GPS device paths in /etc/default/gpsd may need adjustment."
    echo "  See config/gpsd.conf comments for details."
}

# -- Main ---------------------------------------------------------------------

phase0_preflight
phase1_overlays
phase2_pru
phase3_gps
summary
