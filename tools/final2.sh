#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/final2.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0 VK_DRIVER_FILES=/root/panvk/panvk_icd.json VK_ICD_FILENAMES=/root/panvk/panvk_icd.json
echo "step 1: icd json present?"
proot-distro login ubuntu --shared-tmp -- bash -c "cat /root/panvk/panvk_icd.json 2>/dev/null || cp /sdcard/Download/panvk-x11/panvk_icd.json /root/panvk/ 2>/dev/null; cat /root/panvk/panvk_icd.json"
echo "step 2: wine cmd"
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | grep WINE_ALIVE; echo cmd-rc=\$?"
echo "step 3: D3D11 TRIANGLE"
proot-distro login ubuntu --shared-tmp -- bash -c "mkdir -p /root/.wine/drive_c/test && cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/ && cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -14"
echo "ALL_DONE $(date)"
