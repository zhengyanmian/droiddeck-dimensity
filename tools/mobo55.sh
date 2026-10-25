#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo55.txt 2>&1
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

echo "== deploy patched driver =="
cp /sdcard/Download/panvk-x11/libvulkan_panfrost_glibc.so $I/libvulkan_panfrost.so
patchelf --set-rpath $I $I/libvulkan_panfrost.so
ls -la $I/libvulkan_panfrost.so
echo "== run d3d11tri (DXVK 1.10.3 + patched panvk) =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'Mali|Driver:|Vulkan:|adapter|FAIL|SUCCESS|frame|feature|err:' wine-tri.txt | head -25
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
