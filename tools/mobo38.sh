#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo38.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
WP=$G/opt/prefix/wp1
F=$WP/drive_c/test/wine-tri.txt
echo "== mesa/kbase/panvk driver logs =="
grep -iE 'kbase|MESA|panvk|mali|dma|MEM_|import' $F | head -20
echo "== physical device lines =="
grep -iE 'physical|EnumeratePhysical|CreateDevice|queue' $F | head -12
echo "== total lines =="
wc -l $F
