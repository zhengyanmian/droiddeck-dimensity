#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo7.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=/data/data/com.termux/files/home/panvk-icd/panvk_icd.json
export WINEDEBUG=+vulkan
cd $WP/drive_c/test
timeout 120 $BOX $WINE d3d11tri.exe > vklog.txt 2>&1
echo "rc=$?"
grep -iE 'vulkan|icd|driver|mali|panfrost|panvk|surface|instance|loader' vklog.txt | head -30
echo "ALL_DONE $(date)"
