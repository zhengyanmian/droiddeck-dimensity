#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo59.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json
export MESA_DEBUG=1 MESA_LOG_LEVEL=debug

cd $WP/drive_c/test
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== mesa errors around swapchain =="
grep -iE 'MESA: (error|warning)|wsi|swapchain|present|queue' wine-tri.txt | tail -25
echo "== assert context =="
grep -B 3 -A 1 "wassert" wine-tri.txt | head -10
