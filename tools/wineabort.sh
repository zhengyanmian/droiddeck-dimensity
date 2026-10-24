#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/wineabort.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 WINEPREFIX=/root/.wine DISPLAY=:0
echo "A: wine --version (baseline works?)"
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine --version; echo rc=\$?"
echo "B: wine cmd /c echo (minimal)"
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine cmd /c echo HELLO 2>&1 | tail -6; echo rc=\$?"
echo "C: BOX64_LOG=1 wine d3d11tri (full abort msg)"
proot-distro login ubuntu --shared-tmp -- bash -c "cd /root/.wine/drive_c/test && BOX64_LOG=1 box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | head -30"
echo "ALL_DONE $(date)"
