#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo4.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
export VK_DRIVER_FILES=/data/data/com.termux/files/home/panvk-icd/panvk_icd.json

echo "== 1: extract DXVK to HOME =="
mkdir -p /data/data/com.termux/files/home/dxvk
cd $G/opt/libs/d3d && 7z x -y dxvk-async-2.0.7z -o/data/data/com.termux/files/home/dxvk > /dev/null 2>&1
DXV=$(find /data/data/com.termux/files/home/dxvk -name "d3d11.dll" | head -1 | xargs dirname)
echo "DXV=$DXV"; ls $DXV | head -6

echo "== 2: install dlls =="
cp $DXV/d3d11.dll $DXV/dxgi.dll $DXV/d3d9.dll $DXV/d3d10core.dll $WP/drive_c/windows/system32/ && echo dxvk-ok
ls $WP/drive_c/windows/system32/d3d11.dll

echo "== 3: X server check/start =="
ls /tmp/.X11-unix/ 2>/dev/null || (nohup termux-x11 :0 > /dev/null 2>&1 & sleep 6; ls /tmp/.X11-unix/ 2>/dev/null)

echo "== 4: D3D11 TRIANGLE =="
cd $WP/drive_c/test
rm -f log.txt
timeout 300 $BOX $WINE d3d11tri.exe 2>&1 | tail -8
echo "tri-rc=$?"
echo "== log.txt =="
cat log.txt 2>/dev/null

echo "ALL_DONE $(date)"
