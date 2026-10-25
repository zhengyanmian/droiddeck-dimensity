#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo46.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
proot-distro login ubuntu --shared-tmp -- bash -c "sed -i 's/typedef int VkResult; typedef unsigned int U32; typedef void\* Handle;/typedef int VkResult; typedef unsigned int U32; typedef void* Handle;/' /root/ve3.c; sed -i 's/VkResult (\*ci)(void \*, void \*) = dlsym(h, \"vkCreateInstance\");/VkResult (*ci)(const void *, const void *, void *) = dlsym(h, \"vkCreateInstance\");/' /root/ve3.c; gcc /root/ve3.c -o /root/ve3 -ldl && echo compiled"
echo "== run features probe =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve3 /root/panvk/libvulkan.so.1 2>&1 | head -60"
