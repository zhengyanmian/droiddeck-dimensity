#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/noproot4.txt 2>&1
echo "START $(date)"
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
U=$R/usr/lib/aarch64-linux-gnu
G=$R/root/glibclib
cp $U/libc.so.6 $G/libc.so
file $G/libc.so 2>/dev/null || head -c 4 $G/libc.so | od -A x -t x1
echo "== box64 --version =="
$G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 --version
echo "rc1=$?"
echo "== wine --version =="
WINEPREFIX=$R/root/.wine DISPLAY=:0 $G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version
echo "rc2=$?"
echo "ALL_DONE $(date)"
