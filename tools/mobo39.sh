#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo39.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
WP=$G/opt/prefix/wp1
F=$WP/drive_c/test/wine-tri.txt
echo "== full DXVK device block =="
grep -A 12 "Mali-G1-Ultra MC12:" $F | head -40
echo "== adapter/dxgi lines =="
grep -iE 'adapter|luid|dxgi' $F | head -8
