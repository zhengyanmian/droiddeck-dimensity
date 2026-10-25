#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo17.txt 2>&1
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

echo "== 0: glibc version =="
$G/lib/libc.so.6 2>&1 | head -1
echo "== 1: X server =="
nohup termux-x11 :0 > /dev/null 2>&1 &
sleep 8
ls /tmp/.X11-unix/ 2>&1
ls /data/data/com.termux/files/usr/tmp/.X11-unix/ 2>&1

echo "== 2: D3D11 TRIANGLE =="
cd $WP/drive_c/test
rm -f log.txt
timeout 240 $BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== wine-tri.txt (vulkan trace + errors) =="
grep -iE 'vulkan|icd|driver|mali|panfrost|panvk|surface|instance|adapter|FAIL|SUCCESS|frame|err:' wine-tri.txt | head -25
echo "== log.txt (exe file log) =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
