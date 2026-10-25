#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo42.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
WP=$G/opt/prefix/wp1
echo "== full wine-tri tail =="
tail -25 $WP/drive_c/test/wine-tri.txt
echo "== dxvk dll version check =="
ls -la $WP/drive_c/windows/system32/d3d11.dll $WP/drive_c/windows/system32/dxgi.dll
