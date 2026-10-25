#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo61.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export VK_DRIVER_FILES=$I/panvk_icd.json

ls /tmp/.X11-unix/ 2>&1
cd $WP/drive_c/test
cp /sdcard/Download/panvk-x11/vktri.exe .
rm -f vklog.txt
$BOX $WINE vktri.exe > wine-vk.txt 2>&1
echo "rc=$?"
echo "== key lines =="
grep -iE 'ok|FAIL|VKTRI|fail|assert|err:' wine-vk.txt | head -20
echo "ALL_DONE $(date)"
