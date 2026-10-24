#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe2.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/installed-rootfs/ubuntu
ls $R/ | head -8
ls $R/lib/ld-linux-aarch64.so.1 2>&1
ls $R/usr/local/bin/box64 2>&1
ls $R/opt/wine/bin/wine 2>&1
