#!/usr/bin/env bash
# DroidDeck-Dimensity Phase 1 device diagnostics
# Collects ground-truth GPU/driver info from a connected Android phone via adb.
# Output: docs/diag-report.txt (next to this script's project root)

set -u
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$PROJ/docs/diag-report.txt"
mkdir -p "$PROJ/docs"

section() { printf '\n========== %s ==========\n' "$1" >> "$OUT"; }
run() { printf '\n$ %s\n' "$1" >> "$OUT"; adb shell "$1" 2>&1 >> "$OUT"; }

: > "$OUT"
echo "DroidDeck-Dimensity diag report - $(date '+%Y-%m-%d %H:%M:%S')" >> "$OUT"

section "DEVICE"
adb devices -l 2>&1 >> "$OUT"
for p in ro.product.brand ro.product.model ro.product.manufacturer \
         ro.build.version.release ro.build.version.sdk \
         ro.board.platform ro.hardware ro.soc.manufacturer ro.soc.model \
         ro.hardware.vulkan ro.hardware.egl; do
  printf '%s = ' "$p" >> "$OUT"; adb shell getprop "$p" 2>&1 >> "$OUT"
done

section "ROOT CHECK"
run "id"
run "which su || echo no-su-found"

section "KERNEL"
run "cat /proc/version"
run "uname -a"

section "MALI KBASE NODE"
run "ls -l /dev/mali* 2>&1"
run "test -w /dev/mali0 && echo mali0-WRITABLE || echo mali0-NOT-writable"
run "cat /proc/gpuinfo 2>/dev/null || echo no-proc-gpuinfo"

section "DRM RENDER NODES"
run "ls -l /dev/dri/ 2>&1"

section "GPU / VULKAN RELATED PROPS"
adb shell getprop 2>&1 | grep -iE "mali|gpu|vulkan|graphics|gralloc" >> "$OUT" || true

section "VULKAN FEATURES"
run "pm list features 2>/dev/null | grep -i vulkan || echo no-vulkan-feature-listed"

section "DUMPSYS GPU (first 80 lines)"
run "dumpsys gpu 2>/dev/null | head -n 80"

section "GLES RENDERER (SurfaceFlinger)"
run "dumpsys SurfaceFlinger 2>/dev/null | grep -iE 'GLES|OpenGL|renderer' | head -n 10"

section "DONE"
echo ""
echo "Report written to: $OUT"
