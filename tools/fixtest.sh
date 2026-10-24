#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/fixtest.txt 2>&1
echo "TEST-A: PROOT_NO_SECCOMP=1 + wineboot"
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine wineboot -i 2>&1 | tail -4; ls /root/.wine/drive_c 2>/dev/null | head -4 && echo PREFIX_OK"
echo "TEST-B: + BOX64_DYNAREC=0 (if A failed)"
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 BOX64_DYNAREC=0 WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine wineboot -i 2>&1 | tail -4; ls /root/.wine/drive_c 2>/dev/null | head -4 && echo PREFIX_OK"
echo "ALL_DONE $(date)"
