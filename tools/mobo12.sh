#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo12.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
U=$R/usr/lib/aarch64-linux-gnu
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json
export WINEDEBUG=+vulkan

echo "== 1: install full loader =="
cp /sdcard/Download/libvulkan1_1.3.275.0-1build1_arm64.deb /tmp/lv.deb 2>/dev/null || cp /sdcard/Download/libvulkan1_1.3.275.0-1build1_arm64.deb /tmp/lv.deb
mkdir -p /tmp/lvx && cd /tmp/lvx && ar x /tmp/lv.deb && tar xJf data.tar.xz 2>/dev/null || tar xf data.tar.zst 2>/dev/null
find /tmp/lvx -name "libvulkan.so*" | head -3
cp $(find /tmp/lvx -name "libvulkan.so.1*" | head -1) $I/libvulkan.so.1
ls -la $I/libvulkan.so.1

echo "== 2: stage libs (all real ELFs) =="
cp $U/libc.so.6 $I/libc.so
for l in libm.so.6 libxcb.so.1 libXau.so.6 libXdmcp.so.6 libX11.so.6 libxkbcommon.so.0 libgcc_s.so.1 libdrm.so.2; do cp $U/$l $I/ 2>/dev/null; done
ls $I

echo "== 3: run d3d11tri =="
cd $WP/drive_c/test
export LD_LIBRARY_PATH=$I:$G/lib/x86_64-linux-gnu:$G/lib
timeout 120 $BOX $WINE d3d11tri.exe > vklog.txt 2>&1
echo "rc=$?"
echo "=== key lines ==="
grep -iE 'surface|instance|mali|panfrost|panvk|FAIL|adapter|SUCCESS|frame|err:' vklog.txt | head -25
echo "ALL_DONE $(date)"
