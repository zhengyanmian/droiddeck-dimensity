#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo30.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
unset LD_LIBRARY_PATH
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json
export BOX64_LOG=2

cd $WP/drive_c/test
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1 &
sleep 8
echo "== libvulkan.so.1 resolution =="
grep -E 'libvulkan\.so\.1' wine-tri.txt | grep -vE "closing|dlclose" | head -15
echo "ALL_DONE $(date)"
