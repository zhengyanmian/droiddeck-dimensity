#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/morning3.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0 VK_DRIVER_FILES=/root/panvk/panvk_icd.json
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
echo "== A: grun persistent wineserver (NO proot) =="
nohup grun $R/opt/wine/bin/wineserver -p > /sdcard/Download/ws-grun.txt 2>&1 &
sleep 10
head -5 /sdcard/Download/ws-grun.txt
echo "== B: grun wine --version (client, no proot) =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version 2>&1 | tail -3
echo "rcB=$?"
echo "== C: grun wine cmd =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine cmd /c echo WINE_ALIVE > /sdcard/Download/mc.txt 2>&1
echo "rcC=$?"
grep -E "WINE_ALIVE|error" /sdcard/Download/mc.txt | head -3
echo "== D: grun wine d3d11tri =="
mkdir -p $WINEPREFIX/drive_c/test
cp /sdcard/Download/panvk-x11/d3d11tri.exe $WINEPREFIX/drive_c/test/
cd $WINEPREFIX/drive_c/test
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine d3d11tri.exe > /sdcard/Download/md.txt 2>&1
echo "rcD=$?"
tail -15 /sdcard/Download/md.txt
echo "ALL_DONE $(date)"
