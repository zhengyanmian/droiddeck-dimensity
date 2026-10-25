#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo24.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json
export WINEDEBUG=+vulkan

echo "== patchelf box64 rpath: $I first =="
patchelf --set-rpath "$I:$G/lib:$G/lib/x86_64-linux-gnu" $BOX
patchelf --print-rpath $BOX
echo "== run =="
(
  export LD_LIBRARY_PATH=$I:$G/lib/x86_64-linux-gnu
  cd $WP/drive_c/test
  $BOX $WINE d3d11tri.exe
) > $WP/drive_c/test/wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'surface|instance|mali|panfrost|panvk|adapter|FAIL|SUCCESS|frame|device' $WP/drive_c/test/wine-tri.txt | head -25
echo "== log.txt =="
cat $WP/drive_c/test/log.txt 2>/dev/null
echo "ALL_DONE $(date)"
