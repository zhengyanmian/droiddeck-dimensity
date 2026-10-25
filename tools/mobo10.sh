#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo10.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=/data/data/com.termux/files/home/panvk-icd/panvk_icd.json
export WINEDEBUG=+vulkan
cd $WP/drive_c/test
export LD_LIBRARY_PATH=/data/data/com.termux/files/home/panvk-icd:$R/usr/lib/aarch64-linux-gnu:$R/lib/aarch64-linux-gnu:$G/lib/x86_64-linux-gnu:$G/lib
timeout 120 $BOX $WINE d3d11tri.exe > vklog.txt 2>&1
echo "rc=$?"
echo "=== FULL vklog (first 50 lines) ==="
head -50 vklog.txt
echo "ALL_DONE $(date)"
