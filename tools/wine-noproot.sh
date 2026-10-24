#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/noproot.txt 2>&1
echo "START $(date)"
ROOTF=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
LD=$ROOTF/lib/ld-linux-aarch64.so.1
LP="$ROOTF/usr/lib/aarch64-linux-gnu:$ROOTF/lib/aarch64-linux-gnu:$ROOTF/usr/lib:$ROOTF/lib"
B64=$ROOTF/usr/local/bin/box64
export BOX64_LOG=0 WINEPREFIX=$ROOTF/root/.wine DISPLAY=:0
export HOME=$ROOTF/root
echo "== 1: box64 via ld.so (no proot) =="
$LD --library-path "$LP" $B64 --version; echo rc=$?
echo "== 2: wine --version =="
$LD --library-path "$LP" $B64 $ROOTF/opt/wine/bin/wine --version; echo rc=$?
echo "== 3: wineboot =="
$LD --library-path "$LP" $B64 $ROOTF/opt/wine/bin/wine wineboot -i 2>&1 | tail -3; echo rc=$?
ls $WINEPREFIX/drive_c/windows/system32 2>/dev/null | head -3
echo "== 4: wine cmd =="
$LD --library-path "$LP" $B64 $ROOTF/opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | tail -3
echo "== 5: d3d11tri =="
mkdir -p $WINEPREFIX/drive_c/test
cp /sdcard/Download/panvk-x11/d3d11tri.exe $WINEPREFIX/drive_c/test/
cd $WINEPREFIX/drive_c/test
$LD --library-path "$LP" $B64 $ROOTF/opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -12
echo "ALL_DONE $(date)"
