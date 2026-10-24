#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/final3.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 DISPLAY=:0
echo "step 1: start persistent wineserver (background proot session)"
nohup proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; box64 /opt/wine/bin/wineserver -p" > /sdcard/Download/ws.txt 2>&1 &
sleep 6
echo "wineserver started, rc check:"
tail -2 /sdcard/Download/ws.txt
echo "step 2: wine cmd (connects to running wineserver)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; box64 /opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | tail -4; echo cmd-rc=\$?"
echo "step 3: D3D11 TRIANGLE"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine VK_DRIVER_FILES=/root/panvk/panvk_icd.json; mkdir -p /root/.wine/drive_c/test; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -14"
echo "ALL_DONE $(date)"
