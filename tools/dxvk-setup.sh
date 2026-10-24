#!/data/data/com.termux/files/usr/bin/bash
# dxvk-d3d11-setup.sh — install DXVK into the wine prefix + run the D3D11 test.
exec > /sdcard/Download/dxvk-status.txt 2>&1
echo "START $(date)"
echo "step 1/5: download DXVK"
proot-distro login ubuntu --shared-tmp -- bash -c 'mkdir -p /opt/dl && cd /opt/dl; P="https://github.com/doitsujin/dxvk/releases/download/v2.7/dxvk-2.7.tar.gz"; for m in "" "https://gh-proxy.com/" "https://ghfast.top/"; do curl -sfL --retry 2 -m 300 -o dxvk.tar.gz "${m}${P}" && break; done; ls -la dxvk.tar.gz'
echo "step 2/5: extract"
proot-distro login ubuntu --shared-tmp -- bash -c "cd /opt/dl && tar xzf dxvk.tar.gz && ls dxvk-2.7/x64/ | head -8 && echo extract-ok"
echo "step 3/5: install dlls into prefix"
proot-distro login ubuntu --shared-tmp -- bash -c "cp /opt/dl/dxvk-2.7/x64/d3d11.dll /opt/dl/dxvk-2.7/x64/dxgi.dll /opt/dl/dxvk-2.7/x64/d3d9.dll /opt/dl/dxvk-2.7/x64/d3d10core.dll /opt/dl/dxvk-2.7/x64/d3d10.dll /root/.wine/drive_c/windows/system32/ && echo dlls-ok"
echo "step 4/5: set native overrides"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; for d in d3d11 dxgi d3d9 d3d10 d3d10core; do box64 /opt/wine/bin/wine64 reg add 'HKCU\\\\Software\\\\Wine\\\\DllOverrides' /v \$d /t REG_SZ /s native /f 2>/dev/null; done; echo overrides-ok"
echo "step 5/5: run D3D11 triangle test (box64+wine+DXVK+PanVK)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/; cd /root/.wine/drive_c && box64 /opt/wine/bin/wine64 d3d11tri.exe 2>&1 | tail -8"
echo "ALL_DONE $(date)"
