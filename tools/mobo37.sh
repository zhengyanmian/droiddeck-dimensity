#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo37.txt 2>&1
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
rm -f log.txt
cp /sdcard/Download/panvk-x11/d3d11tri.exe .
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'adapter|try|FAIL|SUCCESS|frame|device|err:.*vulkan' wine-tri.txt | head -20
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
