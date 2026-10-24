#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/winediag.txt 2>&1
echo "DIAG1: wine --version (no prefix needed)"
proot-distro login ubuntu --shared-tmp -- bash -c "box64 /opt/wine/bin/wine --version; echo rc=\$?"
echo "DIAG2: which box64 / wine binary type"
proot-distro login ubuntu --shared-tmp -- bash -c "which box64; file /opt/wine/bin/wine 2>/dev/null || head -c 20 /opt/wine/bin/wine | od -A x -t x1 | head -2"
echo "DIAG3: wineboot with err logging"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0 WINEDEBUG=err+all; box64 /opt/wine/bin/wine wineboot -i 2>&1 | tail -15"
echo "DIAG4: prefix after boot"
proot-distro login ubuntu --shared-tmp -- bash -c "ls /root/.wine/ 2>&1; ls /root/.wine/drive_c 2>&1 | head -5"
echo "ALL_DONE $(date)"
