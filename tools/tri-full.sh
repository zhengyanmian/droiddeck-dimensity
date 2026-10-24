#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/tri-full.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 DISPLAY=:0
nohup proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; box64 /opt/wine/bin/wineserver -p" > /dev/null 2>&1 &
sleep 5
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine VK_DRIVER_FILES=/root/panvk/panvk_icd.json; mkdir -p /root/.wine/drive_c/test; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe 2>&1"
echo "ALL_DONE $(date)"
