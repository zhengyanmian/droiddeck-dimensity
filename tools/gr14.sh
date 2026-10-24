#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr14.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=/root/panvk/panvk_icd.json
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
proot-distro login ubuntu --shared-tmp -- bash -c "mkdir -p /root/.wine/drive_c/test && cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/ && cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe > /root/tri.txt 2>&1; echo exe-rc=\$? >> /root/tri.txt; cp /root/tri.txt /root/.wine/drive_c/test/log.txt /sdcard/Download/ 2>/dev/null"
sleep 3
echo "=== log.txt (exe file log) ==="
cat /sdcard/Download/log.txt 2>/dev/null
echo "=== tri.txt (stderr) ==="
head -20 /sdcard/Download/tri.txt 2>/dev/null
echo "ALL_DONE $(date)"
