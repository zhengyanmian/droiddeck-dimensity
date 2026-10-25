#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo16.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json

$I/box64 --version > /sdcard/Download/bv.txt 2>&1
echo "box64-version-rc=$?"
cat /sdcard/Download/bv.txt
echo "== wine --version =="
$I/box64 $I/wine --version > /sdcard/Download/wv.txt 2>&1
echo "rc=$?"
cat /sdcard/Download/wv.txt
echo "== wineboot =="
$I/box64 $I/wine wineboot -u > /sdcard/Download/wb.txt 2>&1
echo "rc=$?"
tail -3 /sdcard/Download/wb.txt
ls $WINEPREFIX/drive_c/windows/system32/d3d11.dll 2>/dev/null && echo DXVK_OK
echo "== d3d11tri =="
mkdir -p $WINEPREFIX/drive_c/test
cp /sdcard/Download/panvk-x11/d3d11tri.exe $WINEPREFIX/drive_c/test/
cd $WINEPREFIX/drive_c/test
$I/box64 $I/wine d3d11tri.exe > /sdcard/Download/tri2.txt 2>&1
echo "rc=$?"
head -20 /sdcard/Download/tri2.txt
echo "ALL_DONE $(date)"
