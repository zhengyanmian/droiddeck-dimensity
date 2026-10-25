#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo44.txt 2>&1
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

echo "== install DXVK 1.10.3 (no sparse requirement) =="
rm -rf /data/data/com.termux/files/home/dxvk1103
mkdir -p /data/data/com.termux/files/home/dxvk1103
cd /data/data/com.termux/files/home/dxvk1103
7z x -y $G/opt/libs/d3d/dxvk-async-1.10.3.7z > /dev/null 2>&1
DXV=$(find /data/data/com.termux/files/home/dxvk1103 -type d -name "x64" | head -1)
echo "DXV=$DXV"; ls $DXV/d3d11.dll 2>&1
cp $DXV/d3d11.dll $DXV/dxgi.dll $DXV/d3d9.dll $DXV/d3d10core.dll $DXV/d3d10.dll $WP/drive_c/windows/system32/ && echo dlls-ok

echo "== run d3d11tri =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'Mali|Driver:|Vulkan:|Memory|adapter|FAIL|SUCCESS|frame|feature|err:' wine-tri.txt | head -25
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
