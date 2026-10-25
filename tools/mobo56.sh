#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo56.txt 2>&1
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

echo "== start X server =="
nohup termux-x11 :0 > /dev/null 2>&1 &
sleep 8
ls /tmp/.X11-unix/ 2>&1

echo "== run d3d11tri (device FL9_2 + X swapchain) =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'Mali|Driver:|adapter|FAIL|SUCCESS|frame|feature|Using|err:.*vulkan|assert' wine-tri.txt | head -20
echo "== log.txt (exe file log) =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
