#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo13.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
U=$R/usr/lib/aarch64-linux-gnu
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json

echo "== 1: extract loader deb (dpkg -x) =="
rm -rf /data/data/com.termux/files/home/lvx
dpkg -x /sdcard/Download/libvulkan1_1.3.275.0-1build1_arm64.deb /data/data/com.termux/files/home/lvx
find /data/data/com.termux/files/home/lvx -name "libvulkan.so*" 2>&1
LV=$(find /data/data/com.termux/files/home/lvx -name "libvulkan.so.1*" | head -1)
cp "$LV" $I/libvulkan.so.1 && echo loader-staged

echo "== 2: stage all real ELFs =="
cp $U/libc.so.6 $I/libc.so
for l in libm.so.6 libxcb.so.1 libXau.so.6 libXdmcp.so.6 libX11.so.6 libxkbcommon.so.0 libgcc_s.so.1 libdrm.so.2; do cp $U/$l $I/ 2>/dev/null; done
ls $I

echo "== 3: run d3d11tri (clean scoped env) =="
cd $WP/drive_c/test
rm -f log.txt
(
  export LD_LIBRARY_PATH=$I:$G/lib/x86_64-linux-gnu
  timeout 240 $BOX $WINE d3d11tri.exe
) > /data/data/com.termux/files/home/tri-out.txt 2>&1
echo "rc=$?"
echo "=== tri-out ==="
head -25 /data/data/com.termux/files/home/tri-out.txt
echo "=== log.txt ==="
head -15 $WP/drive_c/test/log.txt 2>/dev/null
echo "ALL_DONE $(date)"
