#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr3.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
pkg install -y glibc-repo 2>&1 | tail -2
apt update 2>&1 | tail -1
pkg install -y glibc-runner 2>&1 | tail -3
command -v grun 2>&1
grun --help 2>&1 | head -5
echo "ALL_DONE $(date)"
