#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr4.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json
echo "== 1: grun wine --version =="
grun $R/opt/wine/bin/wine --version 2>&1 | tail -4
echo "rc1=$?"
echo "== 2: grun wineboot =="
grun $R/opt/wine/bin/wine wineboot -u 2>&1 | tail -3
echo "rc2=$?"
ls $WINEPREFIX/drive_c/windows/system32/d3d11.dll 2>/dev/null && echo DXVK_PRESENT
echo "== 3: grun wine cmd =="
grun $R/opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | grep -E "WINE_ALIVE" | head -1
echo "rc3=$?"
echo "ALL_DONE $(date)"
