#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/turbox64.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
pkg install -y box64 2>&1 | tail -2
which box64; box64 --version
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
echo "== termux-box64 runs glibc wine --version =="
box64 $R/opt/wine/bin/wine --version 2>&1 | tail -4; echo rc=$?
echo "== wine cmd =="
box64 $R/opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | tail -4; echo rc=$?
echo "ALL_DONE $(date)"
