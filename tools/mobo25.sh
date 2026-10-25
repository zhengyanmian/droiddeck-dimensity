#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo25.txt 2>&1
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

cd $WP/drive_c/test
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1 &
WPID=$!
sleep 6
echo "== process maps: libvulkan =="
for p in $(pidof box64); do echo "PID $p:"; grep -i vulkan /proc/$p/maps 2>/dev/null | awk '{print $6}' | sort -u; done
sleep 8
echo "== key lines =="
grep -iE 'surface|instance|mali|panfrost|panvk|adapter|FAIL|SUCCESS|frame' wine-tri.txt | head -20
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
