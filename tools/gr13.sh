#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr13.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=/root/panvk/panvk_icd.json
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
proot-distro login ubuntu --shared-tmp -- bash -c "mkdir -p /root/.wine/drive_c/test && cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/ && cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe > /root/tri.txt 2>&1; echo exe-rc=\$? >> /root/tri.txt; cp /root/tri.txt /sdcard/Download/tri.txt"
echo "launched, waiting 120s for 180 frames..."
sleep 120
echo "=== tri.txt from sdcard ==="
head -40 /sdcard/Download/tri.txt
echo "ALL_DONE $(date)"
