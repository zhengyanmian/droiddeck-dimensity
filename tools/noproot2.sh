#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/noproot2.txt 2>&1
echo "START $(date)"
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
echo "P0: check files"
ls -la $R/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1
ls -la $R/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1
ls -la $R/usr/local/bin/box64
find $R -name "ld-linux-aarch64*" -maxdepth 4 2>/dev/null
echo "P1: run box64"
$R/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1 --library-path "$R/usr/lib/aarch64-linux-gnu:$R/lib/aarch64-linux-gnu" $R/usr/local/bin/box64 --version
echo "rc1=$?"
echo "P2: wine --version"
WINEPREFIX=$R/root/.wine DISPLAY=:0 $R/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1 --library-path "$R/usr/lib/aarch64-linux-gnu:$R/lib/aarch64-linux-gnu" $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version
echo "rc2=$?"
echo "ALL_DONE $(date)"
