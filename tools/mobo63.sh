#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo63.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export VK_DRIVER_FILES=$I/panvk_icd.json

echo "== deploy instrumented driver =="
cp /sdcard/Download/panvk-x11/libvulkan_panfrost_glibc.so $I/libvulkan_panfrost.so
patchelf --set-rpath $I $I/libvulkan_panfrost.so
echo "== run vktri (minimal vulkan, instrumented) =="
cd $WP/drive_c/test
rm -f vklog.txt
$BOX $WINE vktri.exe > wine-vk.txt 2>&1
echo "rc=$?"
echo "== WSI trace =="
grep -E 'X11WSI|WSICOMMON|import OK|VKTRI|assert' wine-vk.txt | head -30
echo "ALL_DONE $(date)"
