#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo62.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export VK_DRIVER_FILES=$I/panvk_icd.json
export WSI_DEBUG=all MESA_DEBUG=1

cd $WP/drive_c/test
$BOX $WINE vktri.exe > wine-vk.txt 2>&1
echo "rc=$?"
echo "== WSI debug lines =="
grep -iE 'WSI|wsi|swapchain|assert|err' wine-vk.txt | head -25
echo "ALL_DONE $(date)"
