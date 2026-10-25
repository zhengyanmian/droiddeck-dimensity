#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/vkdump.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
WP=$G/opt/prefix/wp1
cat $WP/drive_c/test/vklog.txt 2>/dev/null | head -30
