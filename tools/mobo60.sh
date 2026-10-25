#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo60.txt 2>&1
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
export MESA_DEBUG=1

echo "== kill stale processes =="
pkill -9 -f box64 2>/dev/null; pkill -9 -f wineserver 2>/dev/null; pkill -9 -f d3d11tri 2>/dev/null
sleep 2
echo "== memory before =="
grep -E "MemAvailable|MemFree" /proc/meminfo
echo "== run =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'Mali|adapter|FAIL|SUCCESS|frame|Using feature|assert|Killed' wine-tri.txt | head -15
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "== memory after =="
grep -E "MemAvailable" /proc/meminfo
echo "ALL_DONE $(date)"
