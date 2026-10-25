#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo35.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
I=/data/data/com.termux/files/home/panvk-icd
proot-distro login ubuntu --shared-tmp -- bash -c "readelf -V /root/panvk/libvulkan_panfrost.so 2>/dev/null | grep -E 'GLIBC_' | sort -u | tail -8"
echo "== icd dir copy version needs =="
proot-distro login ubuntu --shared-tmp -- bash -c "cp /root/panvk/libvulkan_panfrost.so /root/panvk/p.so 2>/dev/null; readelf -d /root/panvk/libvulkan_panfrost.so 2>/dev/null | grep NEEDED"
echo "== mobox glibc version file check =="
strings /data/data/com.termux/files/usr/glibc/lib/libc.so.6 2>/dev/null | grep -E "GNU C Library" | head -1
echo "== does Mobox env have libz etc for panvk? =="
ls /data/data/com.termux/files/usr/glibc/lib/libz* /data/data/com.termux/files/usr/glibc/lib/libzstd* 2>&1 | head -4
