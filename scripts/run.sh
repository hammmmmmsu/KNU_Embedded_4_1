#!/bin/bash
# run.sh — 드라이버 적재 + 앱 빌드 + 실행 (한번에)
# Usage: sudo bash scripts/run.sh
set -e
GAME_DIR="$(cd "$(dirname "$0")/.." && pwd)"
bash "$GAME_DIR/scripts/setup_device.sh"
cd "$GAME_DIR"
make
echo ""
echo "=== Starting bomb_game ==="
sudo ./bomb_game
