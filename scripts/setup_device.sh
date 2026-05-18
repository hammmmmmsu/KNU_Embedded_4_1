#!/bin/bash
# setup_device.sh — FPGA 드라이버 전체 적재 + /dev 엔트리 생성
# Usage: sudo bash scripts/setup_device.sh

HOME_DIR="/home/pi0207"
GAME_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# ── helpers ───────────────────────────────────────────────
load_if_not() {
    local mod="$1"
    local ko="$2"
    if lsmod | grep -q "^${mod} "; then
        echo "  [SKIP] ${mod} already loaded"
    else
        echo "  [LOAD] ${mod}"
        insmod "$ko" || echo "  [WARN] insmod ${mod} failed"
    fi
}

make_dev() {
    local name="$1"
    local major="$2"
    if [ ! -e "/dev/${name}" ]; then
        mknod "/dev/${name}" c "$major" 0
        chmod 666 "/dev/${name}"
        echo "  [DEV]  created /dev/${name}  (major ${major})"
    else
        echo "  [SKIP] /dev/${name} already exists"
    fi
}

# ── 1. FPGA interface (반드시 가장 먼저) ─────────────────
echo "=== [1] FPGA interface driver ==="
load_if_not fpga_interface_driver "${HOME_DIR}/fpga_interface_driver.ko"

# ── 2. 기존 테스트 완료된 드라이버들 ──────────────────────
echo ""
echo "=== [2] Device drivers ==="
load_if_not fpga_led          "${HOME_DIR}/led_driver.ko"
load_if_not fpga_fnd          "${HOME_DIR}/try/fnd_driver.ko"
load_if_not fpga_push_switch  "${HOME_DIR}/try/push_driver.ko"
load_if_not fpga_dip_switch   "${HOME_DIR}/dip_driver.ko"
load_if_not itr_count_driver  "${HOME_DIR}/itr_shift_count_lab/itr_count_driver.ko"

# ── 3. DOT driver — 새 버전(10-byte raw 패턴 지원) ────────
echo ""
echo "=== [3] DOT matrix driver (new version) ==="
if lsmod | grep -q "^fpga_dot "; then
    echo "  [SKIP] fpga_dot already loaded"
else
    NEW_DOT="${GAME_DIR}/drivers/fpga_dot.ko"

    # .ko 없으면 지금 빌드
    if [ ! -f "${NEW_DOT}" ]; then
        echo "  [BUILD] Compiling dot_driver.ko..."
        make -C /lib/modules/$(uname -r)/build \
             M="${GAME_DIR}/drivers" modules 2>&1 | tail -3
    fi

    if [ -f "${NEW_DOT}" ]; then
        echo "  [LOAD]  fpga_dot (bomb-game new version)"
        insmod "${NEW_DOT}"
    else
        echo "  [WARN]  Build failed — falling back to ~/try/dot_driver.ko"
        echo "  [WARN]  폭탄 애니메이션이 작동하지 않을 수 있습니다."
        insmod "${HOME_DIR}/try/dot_driver.ko"
    fi
fi

# ── 4. /dev 엔트리 생성 ───────────────────────────────────
echo ""
echo "=== [4] /dev entries ==="
make_dev fpga_led          260
make_dev fpga_fnd          261
make_dev fpga_dot          262
make_dev fpga_push_switch  265
make_dev fpga_dip_switch   266

INT_MAJOR="$(awk '$2 == "itr_count_dev" { print $1 }' /proc/devices)"
if [ -n "${INT_MAJOR}" ]; then
    if [ -e "/dev/fpga_interrupt" ]; then
        rm -f /dev/fpga_interrupt
    fi
    make_dev fpga_interrupt "${INT_MAJOR}"
else
    echo "  [WARN] itr_count_dev major not found"
fi

# ── 5. 결과 확인 ─────────────────────────────────────────
echo ""
echo "=== 완료 ==="
ls -la /dev/fpga_* 2>/dev/null || echo "[WARN] /dev/fpga_* 없음"
echo ""
echo "실행: cd ~/bomb-game && sudo ./bomb_game"
