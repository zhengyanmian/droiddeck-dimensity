#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/dxvk-run.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0
echo "A: prefix state"
proot-distro login ubuntu --shared-tmp -- bash -c "ls /root/.wine/drive_c/windows/system32/ | head -8; ls /root/.wine/drive_c/*.exe 2>/dev/null"
echo "B: dll overrides (single command per call)"
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine reg add 'HKCU\\Software\\Wine\\DllOverrides' /v d3d11 /t REG_SZ /d native /f" 2>&1 | tail -2
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine reg add 'HKCU\\Software\\Wine\\DllOverrides' /v dxgi /t REG_SZ /d native /f" 2>&1 | tail -2
echo "C: copy exe"
proot-distro login ubuntu --shared-tmp -- bash -c "mkdir -p /root/.wine/drive_c/test && cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/ && ls -la /root/.wine/drive_c/test/"
echo "D: run d3d11tri"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe" 2>&1 | tail -15
echo "ALL_DONE $(date)"
