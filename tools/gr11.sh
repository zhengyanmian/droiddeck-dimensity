#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr11.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0
export VK_DRIVER_FILES=/root/panvk/panvk_icd.json
echo "== check X server =="
proot-distro login ubuntu --shared-tmp -- bash -c "ls /tmp/.X11-unix/ 2>&1"
echo "== d3d11tri full =="
proot-distro login ubuntu --shared-tmp -- bash -c "mkdir -p /root/.wine/drive_c/test && cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/ && cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe > /root/tri.txt 2>&1; echo rc=\$?"
proot-distro login ubuntu --shared-tmp -- bash -c "cat /root/tri.txt | head -30"
echo "ALL_DONE $(date)"
