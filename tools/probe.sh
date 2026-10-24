#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe.txt 2>&1
ls -d /data/data/com.termux/files/usr/var/lib/proot-distro/installed-rootfs/* 2>&1
find /data/data/com.termux/files/usr/var/lib/proot-distro -maxdepth 3 -name "ld-linux*" 2>/dev/null | head -3
find /data/data/com.termux/files -maxdepth 6 -type d -name "ubuntu" 2>/dev/null | head -5
