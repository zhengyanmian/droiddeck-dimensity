#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo49.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
proot-distro login ubuntu --shared-tmp -- bash -c "find /root/build -name 'vulkan_core.h' 2>/dev/null | head -3; ls /root/build/panvk-kbase-android/work/mesa/include/ 2>/dev/null"
