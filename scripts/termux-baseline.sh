#!/data/data/com.termux/files/usr/bin/bash
# droiddeck-dimensity Phase 2 baseline - run INSIDE Termux on the phone
# Usage: bash /sdcard/droiddeck/termux-baseline.sh
set -e
OUT="$HOME/baseline-report.txt"
echo "=== termux baseline $(date) ===" | tee "$OUT"

echo "--- uname ---" | tee -a "$OUT"
uname -a | tee -a "$OUT"

echo "--- /dev/mali0 visibility inside Termux ---" | tee -a "$OUT"
ls -l /dev/mali0 2>&1 | tee -a "$OUT" || true
test -w /dev/mali0 && echo "mali0 WRITABLE" | tee -a "$OUT" || echo "mali0 NOT writable" | tee -a "$OUT"

echo "--- /dev/dri inside Termux ---" | tee -a "$OUT"
ls -l /dev/dri 2>&1 | tee -a "$OUT" || echo "no /dev/dri access"

echo "--- vendor vulkaninfo (system Mali driver, bionic in-process) ---" | tee -a "$OUT"
pkg install -y vulkan-tools >/dev/null 2>&1 || true
vulkaninfo --summary 2>&1 | tee -a "$OUT" || echo "vulkaninfo failed" | tee -a "$OUT"

echo "=== done, report at \$HOME/baseline-report.txt ===" | tee -a "$OUT"
