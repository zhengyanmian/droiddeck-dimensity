#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr7.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json
echo "== A: grun box64 --version =="
grun $R/usr/local/bin/box64 --version 2>&1 | tail -3
echo "rcA=$?"
echo "== B: grun box64 wine --version =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version 2>&1 | tail -3
echo "rcB=$?"
echo "== C: grun box64 wine cmd =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | grep -E "WINE_ALIVE" | head -1
echo "rcC=$?"
echo "== D: grun box64 wine wineboot =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine wineboot -u 2>&1 | tail -3
echo "rcD=$?"
echo "ALL_DONE $(date)"
