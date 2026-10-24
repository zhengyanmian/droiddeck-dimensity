#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr9.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json BOX64_LOG=1
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version 2>&1 | grep -A3 "could not load ntdll" | fold -w 110
echo "ALL_DONE $(date)"
