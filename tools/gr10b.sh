#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr10b.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0 VK_DRIVER_FILES=/root/panvk/panvk_icd.json
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version > /sdcard/Download/ntdll-err.txt 2>&1
echo "captured rc=$?"
echo "ALL_DONE $(date)"
