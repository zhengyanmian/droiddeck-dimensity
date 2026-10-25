#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo36.txt 2>&1
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

echo "== 1: stage 24.04 libs (glibc<=2.38) =="
cp /sdcard/Download/panvk-libs/*.so* $I/
ls $I | wc -l

echo "== 2: patchelf rpath on panvk ICD =="
patchelf --set-rpath $I $I/libvulkan_panfrost.so
echo "== 3: replace Mobox loader =="
cp $I/libvulkan.so.1 $G/lib/libvulkan.so.1
echo "== 4: run d3d11tri =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'surface|instance|mali|panfrost|panvk|adapter|FAIL|SUCCESS|frame|device' wine-tri.txt | head -25
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
