#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr6.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
W=/data/data/com.termux/files/home/panvk-bin/wine
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json
ls -la $P/glibc/lib/ld-linux-aarch64.so.1
chmod +x $P/glibc/lib/ld-linux-aarch64.so.1
echo "== A: direct ld exec =="
$P/glibc/lib/ld-linux-aarch64.so.1 $W --version 2>&1 | tail -4
echo "rcA=$?"
echo "== B: grun patched wine =="
grun $W --version 2>&1 | tail -4
echo "rcB=$?"
echo "ALL_DONE $(date)"
