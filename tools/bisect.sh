#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/bisect.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 DISPLAY=:0
nohup proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; box64 /opt/wine/bin/wineserver -p" > /dev/null 2>&1 &
sleep 5
echo "TEST-A: wine notepad (GUI path, no DXVK involved)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; timeout 20 box64 /opt/wine/bin/wine notepad 2>&1 | tail -5; echo notepad-rc=\$?"
echo "TEST-B: d3d11tri WITHOUT dxvk dlls (builtin d3d11)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; mkdir -p /root/dxvkbackup; mv /root/.wine/drive_c/windows/system32/d3d11.dll /root/.wine/drive_c/windows/system32/dxgi.dll /root/dxvkbackup/ 2>/dev/null; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -6; echo tri-rc=\$?"
echo "TEST-C: restore dxvk"
proot-distro login ubuntu --shared-tmp -- bash -c "mv /root/dxvkbackup/*.dll /root/.wine/drive_c/windows/system32/ 2>/dev/null; ls /root/.wine/drive_c/windows/system32/d3d11.dll && echo restored"
echo "TEST-D: core dumps?"
proot-distro login ubuntu --shared-tmp -- bash -c "ls -la /root/.wine/drive_c/test/core* /root/core* 2>/dev/null; cat /proc/sys/kernel/core_pattern"
echo "ALL_DONE $(date)"
