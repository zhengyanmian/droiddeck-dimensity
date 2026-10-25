#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo11.txt 2>&1
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

echo "== stage real ELFs into icd dir =="
mkdir -p $I
cp $U/libc.so.6 $I/libc.so
for l in libm.so.6 libvulkan.so.1 libxcb.so.1 libXau.so.6 libXdmcp.so.6 libX11.so.6 libxkbcommon.so.0 libgcc_s.so.1; do cp $U/$l $I/ 2>/dev/null; done
ls $I
file $I/libc.so 2>/dev/null || head -c 4 $I/libc.so | od -A x -t x1

echo "== run =="
cd $WP/drive_c/test
export LD_LIBRARY_PATH=$I:$U:$R/lib/aarch64-linux-gnu:$G/lib/x86_64-linux-gnu:$G/lib
timeout 120 $BOX $WINE d3d11tri.exe > vklog.txt 2>&1
echo "rc=$?"
unset LD_LIBRARY_PATH
echo "=== vklog key lines ==="
grep -iE 'surface|instance|driver|mali|panfrost|panvk|FAIL|adapter|SUCCESS|frame|err:' vklog.txt | head -25
echo "ALL_DONE $(date)"
