#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo58.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
WP=$G/opt/prefix/wp1
F=$WP/drive_c/test/wine-tri.txt
echo "== swapchain/surface related =="
grep -iE 'swapchain|surface|present|xcb|x11' $F | head -20
echo "== full tail =="
tail -12 $F
