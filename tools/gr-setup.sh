#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
apt update 2>&1 | tail -1
apt install -y glibc-runner 2>&1 | tail -4
which grun && grun --help 2>&1 | head -5
echo "ALL_DONE $(date)"
