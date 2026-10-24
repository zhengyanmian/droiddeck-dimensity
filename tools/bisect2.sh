#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/bisect2.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 DISPLAY=:0
nohup proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; box64 /opt/wine/bin/wineserver -p" > /dev/null 2>&1 &
sleep 5
echo "TEST-A: notepad (GUI)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; timeout 15 box64 /opt/wine/bin/wine notepad > /root/na.txt 2>&1; echo na-rc=\$?; tail -3 /root/na.txt"
echo "TEST-B: d3d11tri WITH dxvk dlls"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine VK_DRIVER_FILES=/root/panvk/panvk_icd.json; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe > /root/tb.txt 2>&1; echo tb-rc=\$?; tail -10 /root/tb.txt"
echo "TEST-C: d3d11tri WITHOUT dxvk (builtin)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; mkdir -p /root/dxvkbk; mv /root/.wine/drive_c/windows/system32/d3d11.dll /root/.wine/drive_c/windows/system32/dxgi.dll /root/dxvkbk/ 2>/dev/null; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe > /root/tc.txt 2>&1; echo tc-rc=\$?; tail -10 /root/tc.txt; mv /root/dxvkbk/*.dll /root/.wine/drive_c/windows/system32/ 2>/dev/null"
echo "ALL_DONE $(date)"
