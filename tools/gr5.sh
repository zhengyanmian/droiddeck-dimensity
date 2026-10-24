#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr5.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
P=/data/data/com.termux/files/usr
echo "== 0: glibc-runner layout =="
ls $P/glibc/lib/ld-linux-aarch64.so.1 2>&1
command -v patchelf 2>&1
pkg install -y patchelf 2>&1 | tail -1
echo "== 1: copy wine + patchelf =="
mkdir -p /data/data/com.termux/files/home/panvk-bin
cp $R/opt/wine/bin/wine /data/data/com.termux/files/home/panvk-bin/wine
patchelf --set-interpreter $P/glibc/lib/ld-linux-aarch64.so.1 --set-rpath "$P/glibc/lib:$R/usr/lib/aarch64-linux-gnu:$R/lib/aarch64-linux-gnu" /data/data/com.termux/files/home/panvk-bin/wine
echo "patched:"
patchelf --print-interpreter /data/data/com.termux/files/home/panvk-bin/wine 2>&1
echo "== 2: run wine --version =="
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=$R/root/panvk/panvk_icd.json
/data/data/com.termux/files/home/panvk-bin/wine --version 2>&1 | tail -4
echo "rc1=$?"
echo "== 3: wine cmd =="
/data/data/com.termux/files/home/panvk-bin/wine cmd /c echo WINE_ALIVE 2>&1 | tail -3
echo "rc2=$?"
echo "ALL_DONE $(date)"
