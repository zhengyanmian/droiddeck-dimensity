#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo40.txt 2>&1
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

echo "== download dxvk 2.7 (mirrors) =="
mkdir -p /data/data/com.termux/files/home/dl
cd /data/data/com.termux/files/home/dl
P="https://github.com/doitsujin/dxvk/releases/download/v2.7/dxvk-2.7.tar.gz"
for m in "" "https://gh-proxy.com/" "https://ghfast.top/"; do curl -sfL --retry 2 -m 240 -o dxvk.tar.gz "${m}${P}" && break; done
ls -la dxvk.tar.gz

echo "== extract =="
mkdir -p /data/data/com.termux/files/home/dxvk27
cd /data/data/com.termux/files/home/dxvk27
tar xzf ../dl/dxvk.tar.gz 2>&1 | tail -1
DXV=$(find /data/data/com.termux/files/home/dxvk27 -type d -name "x64" | head -1)
echo "DXV=$DXV"; ls $DXV/d3d11.dll 2>&1

echo "== install dlls =="
cp $DXV/d3d11.dll $DXV/dxgi.dll $WP/drive_c/windows/system32/ && echo dxvk27-ok

echo "== run d3d11tri =="
cd $WP/drive_c/test
rm -f log.txt
$BOX $WINE d3d11tri.exe > wine-tri.txt 2>&1
echo "tri-rc=$?"
echo "== key lines =="
grep -iE 'Mali|Driver:|Vulkan:|Memory|adapter|FAIL|SUCCESS|frame|feature|err:' wine-tri.txt | head -25
echo "== log.txt =="
cat log.txt 2>/dev/null
echo "ALL_DONE $(date)"
