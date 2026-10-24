#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr8.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json
echo "== full ntdll error =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version 2>&1 | head -12
echo "== ntdll deps =="
proot-distro login ubuntu --shared-tmp -- bash -c "readelf -d /opt/wine/lib/wine/x86_64-unix/ntdll.so 2>/dev/null | grep NEEDED | head -8; file /opt/wine/lib/wine/x86_64-unix/ntdll.so | cut -d, -f1-2"
echo "ALL_DONE $(date)"
