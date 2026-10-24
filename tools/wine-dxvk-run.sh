#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/dxvk-status.txt 2>&1
echo "START $(date)"
export WINEPREFIX=/root/.wine
export DISPLAY=:0
W="box64 /opt/wine/bin/wine"
echo "step 1/6: wineboot init"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine wineboot -i 2>&1 | tail -3; ls /root/.wine/drive_c/windows/system32 >/dev/null && echo prefix-ok"
echo "step 2/6: cmd test"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine cmd /c echo WINE_IS_ALIVE 2>&1 | tail -2"
echo "step 3/6: download DXVK"
proot-distro login ubuntu --shared-tmp -- bash -c 'mkdir -p /opt/dl && cd /opt/dl; P="https://github.com/doitsujin/dxvk/releases/download/v2.7/dxvk-2.7.tar.gz"; for m in "" "https://gh-proxy.com/" "https://ghfast.top/"; do curl -sfL --retry 2 -m 300 -o dxvk.tar.gz "${m}${P}" && break; done; ls -la dxvk.tar.gz'
echo "step 4/6: extract + install dlls"
proot-distro login ubuntu --shared-tmp -- bash -c "cd /opt/dl && tar xzf dxvk.tar.gz && cp dxvk-2.7/x64/d3d11.dll dxvk-2.7/x64/dxgi.dll dxvk-2.7/x64/d3d9.dll dxvk-2.7/x64/d3d10core.dll dxvk-2.7/x64/d3d10.dll /root/.wine/drive_c/windows/system32/ && echo dlls-ok"
echo "step 5/6: dll overrides"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; for d in d3d11 dxgi d3d9 d3d10 d3d10core; do box64 /opt/wine/bin/wine reg add 'HKCU\\Software\\Wine\\DllOverrides' /v \$d /t REG_SZ /d native /f 2>/dev/null | tail -1; done; echo overrides-ok"
echo "step 6/6: D3D11 TRIANGLE TEST"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/; cd /root/.wine/drive_c && box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -10"
echo "ALL_DONE $(date)"
