#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/winebin.txt 2>&1
proot-distro login ubuntu --shared-tmp -- bash -c "ls -la /opt/wine/bin/ ; ls /opt/wine/ ; ls /opt/dl/"
