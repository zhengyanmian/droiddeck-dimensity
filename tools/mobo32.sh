#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo32.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
echo "== strings check: loader files =="
proot-distro login ubuntu --shared-tmp -- bash -c "strings /root/panvk/libvulkan.so.1 | grep -c VK_KHR_surface; strings /root/panvk/libvulkan.so.1 | grep VK_KHR | head -8; md5sum /root/panvk/libvulkan.so.1"
echo "== also check our icd dir copy =="
strings /data/data/com.termux/files/home/panvk-icd/libvulkan.so.1 2>/dev/null | grep -c VK_KHR_surface
echo "== rootfs has real libvulkan? =="
proot-distro login ubuntu --shared-tmp -- bash -c "dpkg -l | grep -i vulkan; strings /usr/lib/aarch64-linux-gnu/libvulkan.so.1 2>/dev/null | grep -c VK_KHR_surface"
