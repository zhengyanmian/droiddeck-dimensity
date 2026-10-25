#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo3.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
export VK_DRIVER_FILES=/data/data/com.termux/files/home/panvk-icd/panvk_icd.json

echo "== 1: panvk icd dir (fix .so) =="
mkdir -p /data/data/com.termux/files/home/panvk-icd
cp /sdcard/Download/panvk-x11/libvulkan_panfrost_glibc.so /data/data/com.termux/files/home/panvk-icd/libvulkan_panfrost.so
cp $R/usr/lib/aarch64-linux-gnu/libdrm.so.2 /data/data/com.termux/files/home/panvk-icd/ 2>/dev/null
ls -la /data/data/com.termux/files/home/panvk-icd/

echo "== 2: X server =="
nohup termux-x11 :0 > /dev/null 2>&1 &
sleep 6
ls /tmp/.X11-unix/ 2>/dev/null

echo "== 3: DXVK dlls =="
mkdir -p /tmp/dxvk
cd $G/opt/libs/d3d && 7z x -y dxvk-async-2.0.7z -o/tmp/dxvk > /dev/null 2>&1
find /tmp/dxvk -name "d3d11.dll" | head -2
DXV=$(find /tmp/dxvk -name "d3d11.dll" | head -1 | xargs dirname)
cp $DXV/d3d11.dll $DXV/dxgi.dll $DXV/d3d9.dll $DXV/d3d10core.dll $WP/drive_c/windows/system32/ && echo dxvk-ok

echo "== 4: D3D11 TRIANGLE =="
mkdir -p $WP/drive_c/test
cp /sdcard/Download/panvk-x11/d3d11tri.exe $WP/drive_c/test/
cd $WP/drive_c/test
rm -f log.txt
timeout 300 $BOX $WINE d3d11tri.exe 2>&1 | tail -6
echo "tri-rc=$?"
echo "== log.txt (exe file log) =="
cat log.txt 2>/dev/null

echo "ALL_DONE $(date)"
