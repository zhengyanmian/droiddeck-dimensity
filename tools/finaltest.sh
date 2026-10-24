#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/finaltest.txt 2>&1
echo "START $(date)"
echo "== A: glibc xcb swapchain 90f =="
proot-distro login ubuntu --shared-tmp -- bash -c "cp /sdcard/Download/panvk-x11/libvulkan_panfrost_glibc.so /root/panvk/ && cd /root/panvk && LD_LIBRARY_PATH=/root/panvk DISPLAY=:0 ./xcb-vk-glibc ./libvulkan_panfrost_glibc.so 90 2>&1 | tail -3"
echo "== B: wineboot (system heap) =="
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine wineboot -u 2>&1 | tail -2; ls /root/.wine/drive_c/windows/system32/d3d11.dll 2>/dev/null && echo DXVK_DLL_PRESENT"
echo "== C: wine cmd =="
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine cmd /c echo WINE_ALIVE 2>&1 | grep -E 'WINE_ALIVE' | head -1"
echo "== D: D3D11 triangle (box64+wine+DXVK+PanVK) =="
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/ 2>/dev/null; mkdir -p /root/.wine/drive_c/test; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/; cd /root/.wine/drive_c/test && box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | tail -12"
echo "ALL_DONE $(date)"
