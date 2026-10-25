#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo15.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
U=$R/usr/lib/aarch64-linux-gnu
I=/data/data/com.termux/files/home/panvk-icd
LD=$U/ld-linux-aarch64.so.1
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json

echo "== 1: stage rootfs real libs =="
cp $U/libc.so.6 $I/libc.so
cp $U/libm.so.6 $I/libm.so.6
cp $U/ld-linux-aarch64.so.1 $I/ld-linux-aarch64.so.1
for l in libxcb.so.1 libXau.so.6 libXdmcp.so.6 libX11.so.6 libxkbcommon.so.0 libgcc_s.so.1 libdrm.so.2; do cp $U/$l $I/ 2>/dev/null; done
ls $I

echo "== 2: patchelf box64 + wine (rootfs interpreter) =="
cp $G/bin/box64 $I/box64
cp $G/wine-9.3-vanilla-wow64/bin/wine $I/wine
patchelf --set-interpreter $LD --set-rpath "$I:$U:$R/lib/aarch64-linux-gnu" $I/box64
patchelf --set-interpreter $LD --set-rpath "$I:$U:$R/lib/aarch64-linux-gnu" $I/wine
patchelf --print-interpreter $I/box64

echo "== 3: box64 --version (direct!) =="
$I/box64 --version 2>&1 | tail -2
echo "rc1=$?"
echo "== 4: wine --version =="
$I/box64 $I/wine --version 2>&1 | tail -2
echo "rc2=$?"
echo "ALL_DONE $(date)"
