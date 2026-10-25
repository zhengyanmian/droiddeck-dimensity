#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo52.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
I=/data/data/com.termux/files/home/panvk-icd
echo "== stage everything into container =="
cp $I/libvulkan_panfrost.so $R/root/panvk/libvulkan_panfrost.so
cp $I/libvulkan.so.1 $R/root/panvk/libvulkan.so.1
cp $I/libdrm.so.2 $I/libxcb.so.1 $I/libX11.so.6 $I/libXau.so.6 $I/libXdmcp.so.6 $I/libX11-xcb.so.1 $I/libxcb-dri3.so.0 $I/libxcb-present.so.0 $I/libxcb-xfixes.so.0 $I/libxcb-sync.so.1 $I/libxcb-randr.so.0 $I/libxcb-shm.so.0 $I/libxshmfence.so.1 $I/libxcb-keysyms.so.1 $I/libexpat.so.1 $I/libz.so.1 $R/root/panvk/ 2>/dev/null
cp $I/libc.so $I/libm.so.6 $I/libgcc_s.so.1 $R/root/panvk/ 2>/dev/null
cat > $R/root/panvk/panvk_icd.json << "EOF"
{
  "file_format_version": "1.0.0",
  "ICD": {
    "library_path": "/root/panvk/libvulkan_panfrost.so",
    "api_version": "1.3.0"
  }
}
EOF
ls $R/root/panvk/
echo "== run features probe =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve5 /root/panvk/libvulkan.so.1 2>&1 | head -20"
