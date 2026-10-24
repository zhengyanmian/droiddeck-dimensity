#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe3.txt 2>&1
C=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu
ls $C/
ls $C/rootfs/ 2>&1 | head -8
ls $C/rootfs/lib/ld-linux-aarch64.so.1 2>&1
ls $C/rootfs/usr/local/bin/box64 2>&1
ls $C/rootfs/opt/wine/bin/wine 2>&1
