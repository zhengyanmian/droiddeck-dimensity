#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe4.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
echo "== rootfs panvk dir =="
ls -la $R/root/panvk/ 2>&1
echo "== wineboot alive? =="
ps -ef 2>/dev/null | grep -E "wineboot|wineserver|box64" | grep -v grep | head -5
echo "== prefix state =="
ls /data/data/com.termux/files/usr/glibc/opt/prefix/wp1/ 2>&1 | head -6
