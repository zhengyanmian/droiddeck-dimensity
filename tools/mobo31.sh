#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo31.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
I=/data/data/com.termux/files/home/panvk-icd

echo "== stage full loader + libs into container =="
cp $I/libvulkan.so.1 $R/root/panvk/libvulkan.so.1
cp $I/libc.so $R/root/panvk/libc.so 2>/dev/null
cp $I/libm.so.6 $R/root/panvk/ 2>/dev/null
cp $I/libxcb.so.1 $I/libXau.so.6 $I/libXdmcp.so.6 $I/libX11.so.6 $I/libxkbcommon.so.0 $I/libdrm.so.2 $R/root/panvk/ 2>/dev/null
ls $R/root/panvk/

echo "== probe inside proot with full loader =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve P"
echo "== probe without VK env =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; /root/ve Q"
echo "ALL_DONE $(date)"
