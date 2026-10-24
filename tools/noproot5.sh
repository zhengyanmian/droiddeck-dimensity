#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/noproot5.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
G=$R/root/glibclib
echo "== box64 --version =="
$G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 --version
echo "rc1=$?"
echo "== wine --version =="
WINEPREFIX=$R/root/.wine DISPLAY=:0 $G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version
echo "rc2=$?"
echo "== wineboot =="
$G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 $R/opt/wine/bin/wine wineboot -i 2>&1 | tail -3
echo "rc3=$?"
ls $WINEPREFIX/drive_c/windows/system32 2>/dev/null | head -3 && echo PREFIX_OK
echo "== wine cmd =="
$G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 $R/opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | tail -2
echo "== d3d11tri =="
mkdir -p $WINEPREFIX/drive_c/test
cp /sdcard/Download/panvk-x11/d3d11tri.exe $WINEPREFIX/drive_c/test/
cd $WINEPREFIX/drive_c/test
$G/ld-linux-aarch64.so.1 --library-path $G $R/usr/local/bin/box64 $R/opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -12
echo "ALL_DONE $(date)"
