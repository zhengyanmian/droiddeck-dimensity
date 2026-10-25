#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo14.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
U=$R/usr/lib/aarch64-linux-gnu
I=/data/data/com.termux/files/home/panvk-icd
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export WINEDLLOVERRIDES="d3d11,dxgi=n"
export VK_DRIVER_FILES=$I/panvk_icd.json
export BOX64_LD_LIBRARY_PATH=$I:$U:$G/lib/x86_64-linux-gnu:$G/lib

echo "== grun libgcc check =="
ls $G/lib/libgcc_s.so.1 $G/lib/libc.so.6 2>&1
echo "== grun box64 wine --version (with BOX64_LD_LIBRARY_PATH) =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine --version > /sdcard/Download/nv.txt 2>&1
echo "rc=$?"
head -8 /sdcard/Download/nv.txt
echo "== grun box64 wine cmd =="
grun $R/usr/local/bin/box64 $R/opt/wine/bin/wine cmd /c echo WINE_ALIVE > /sdcard/Download/nc.txt 2>&1
echo "rc=$?"
grep -E "WINE_ALIVE|err" /sdcard/Download/nc.txt | head -3
echo "ALL_DONE $(date)"
