#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/noproot3.txt 2>&1
echo "START $(date)"
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
U=$R/usr/lib/aarch64-linux-gnu
mkdir -p $R/root/glibclib
for l in libc.so libc.so.6 libm.so.6 libm.so.6 libdl.so.2 libpthread.so.0 librt.so.1 libresolv.so.2 libstdc++.so.6 libgcc_s.so.1 ld-linux-aarch64.so.1; do cp -L $U/$l $R/root/glibclib/ 2>/dev/null; done
ls $R/root/glibclib
echo "== box64 --version =="
$R/root/glibclib/ld-linux-aarch64.so.1 --library-path $R/root/glibclib $R/usr/local/bin/box64 --version
echo "rc1=$?"
echo "== wine --version =="
WINEPREFIX=$R/root/.wine DISPLAY=:0 $R/root/glibclib/ld-linux-aarch64.so.1 --library-path $R/root/glibclib $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version
echo "rc2=$?"
echo "ALL_DONE $(date)"
