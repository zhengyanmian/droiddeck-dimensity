#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo-run.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1

echo "== 1: panvk icd dir =="
mkdir -p /data/data/com.termux/files/home/panvk-icd
cp $R/root/panvk/libvulkan_panfrost.so /data/data/com.termux/files/home/panvk-icd/ 2>/dev/null
cp $R/usr/lib/aarch64-linux-gnu/libdrm.so.2 /data/data/com.termux/files/home/panvk-icd/ 2>/dev/null
cat > /data/data/com.termux/files/home/panvk-icd/panvk_icd.json << EOF
{
  "file_format_version": "1.0.0",
  "ICD": {
    "library_path": "/data/data/com.termux/files/home/panvk-icd/libvulkan_panfrost.so",
    "api_version": "1.3.0"
  }
}
EOF
ls -la /data/data/com.termux/files/home/panvk-icd/

echo "== 2: X server =="
pkill -f termux-x11 2>/dev/null; sleep 1
nohup termux-x11 :0 > /dev/null 2>&1 &
sleep 5
ls /tmp/.X11-unix/ 2>/dev/null || ls $PREFIX/tmp/.X11-unix/ 2>/dev/null

echo "== 3: wineboot new prefix =="
export WINEPREFIX=$WP DISPLAY=:0
export BOX64_LD_LIBRARY_PATH=$G/lib/x86_64-linux-gnu:$G/lib
$BOX $WINE wineboot -i 2>&1 | tail -3
echo "wineboot-rc=$?"
ls $WP/drive_c/windows/system32 2>/dev/null | head -3

echo "== 4: install DXVK dlls =="
cd $G/opt/libs/d3d
7z x -y dxvk-async-2.0.7z -o/tmp/dxvk > /dev/null 2>&1
cp /tmp/dxvk/system32/d3d11.dll /tmp/dxvk/system32/dxgi.dll /tmp/dxvk/system32/d3d9.dll /tmp/dxvk/system32/d3d10core.dll $WP/drive_c/windows/system32/ 2>&1 && echo dxvk-ok

echo "== 5: D3D11 TRIANGLE TEST =="
mkdir -p $WP/drive_c/test
cp /sdcard/Download/panvk-x11/d3d11tri.exe $WP/drive_c/test/
cd $WP/drive_c/test
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
export VK_DRIVER_FILES=/data/data/com.termux/files/home/panvk-icd/panvk_icd.json
export LD_LIBRARY_PATH=/data/data/com.termux/files/home/panvk-icd
timeout 300 $BOX $WINE d3d11tri.exe 2>&1 | tail -14
echo "tri-rc=$?"

echo "ALL_DONE $(date)"
