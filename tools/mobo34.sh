#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo34.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
proot-distro login ubuntu --shared-tmp -- bash -c "readelf -d /root/panvk/libvulkan_panfrost.so | grep NEEDED; echo ===; ldd /root/panvk/libvulkan_panfrost.so 2>&1 | grep -E 'not found' | head -8"
