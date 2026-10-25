#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/vkdump2.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
WP=$G/opt/prefix/wp1
head -30 $WP/drive_c/test/vklog.txt 2>&1
echo "=== loader check ==="
ls /data/data/com.termux/files/home/panvk-icd/libvulkan.so.1 2>&1
proot-distro login ubuntu --shared-tmp -- bash -c "apt install -y libvulkan1 2>&1 | tail -1; ls -la /usr/lib/aarch64-linux-gnu/libvulkan.so.1"
