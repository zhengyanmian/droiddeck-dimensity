#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo54.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
I=/data/data/com.termux/files/home/panvk-icd
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
export WINEPREFIX=$WP DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json

echo "== 1: patch panvk features reporting =="
proot-distro login ubuntu --shared-tmp -- bash -c "cd /root/build/panvk-kbase-android/work/mesa/src/panfrost/vulkan; sed -i 's/\.geometryShader = false,/.geometryShader = true,/; s/\.tessellationShader = false,/.tessellationShader = true,/; s/\.fillModeNonSolid = false,/.fillModeNonSolid = true,/; s/\.shaderClipDistance = false,/.shaderClipDistance = true,/; s/\.shaderCullDistance = false,/.shaderCullDistance = true,/; s/\.textureCompressionBC = has_texture_compression_bc(device),/.textureCompressionBC = true,/' panvk_vX_physical_device.c; grep -cE 'geometryShader = true|tessellationShader = true|fillModeNonSolid = true' panvk_vX_physical_device.c"
echo "== 2: rebuild glibc driver =="
proot-distro login ubuntu --shared-tmp -- bash -c "cd /root/build/panvk-kbase-android && ninja -C build/linux-glibc-cross 2>&1 | tail -2"
proot-distro login ubuntu --shared-tmp -- bash -c "cp build/linux-glibc-cross/src/panfrost/vulkan/libvulkan_panfrost.so /root/panvk/libvulkan_panfrost.so && cp build/linux-glibc-cross/src/panfrost/vulkan/libvulkan_panfrost.so /opt/dl/ && echo so-updated"
cp $R/opt/dl/libvulkan_panfrost.so $I/libvulkan_panfrost.so
patchelf --set-rpath $I $I/libvulkan_panfrost.so
cp $I/libvulkan_panfrost.so $R/root/panvk/libvulkan_panfrost.so

echo "== 3: extract DXVK 1.10.3 dlls =="
rm -rf /data/data/com.termux/files/home/dxvk1103
mkdir -p /data/data/com.termux/files/home/dxvk1103
cd /data/data/com.termux/files/home/dxvk1103
7z x -y $G/opt/libs/d3d/dxvk-async-1.10.3.7z > /dev/null 2>&1
7z l $G/opt/libs/d3d/dxvk-async-1.10.3.7z 2>/dev/null | grep d3d11.dll | head -2
DXV=$(find /data/data/com.termux/files/home/dxvk1103 -type d -name "x64" | head -1)
echo "DXV=$DXV"
if [ -z "$DXV" ]; then DXV=$(find /data/data/com.termux/files/home/dxvk1103 -name "d3d11.dll" | head -1 | xargs dirname); echo "DXV2=$DXV"; fi
cp $DXV/d3d11.dll $DXV/dxgi.dll $WP/drive_c/windows/system32/ && echo dlls-ok

echo "== 4: run d3d11tri =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'Mali|Driver:|Vulkan:|adapter|FAIL|SUCCESS|frame|feature|err:' wine-tri.txt | head -20
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
