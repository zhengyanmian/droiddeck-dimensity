#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr10.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json BOX64_LOG=1
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine --version > /root/err.txt 2>&1; echo rc=\$?"
proot-distro login ubuntu --shared-tmp -- bash -c "fold -w 100 /root/err.txt | head -20"
echo "ALL_DONE $(date)"
