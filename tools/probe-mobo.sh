#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe-mobo.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
echo "== opt tree =="
ls $G/opt/
echo "== wine bins =="
find $G/opt -maxdepth 4 -name "wine" -o -maxdepth 4 -name "wine64" 2>/dev/null | head -5
echo "== box64 =="
find $G/opt -maxdepth 4 -name "box64" 2>/dev/null | head -3
echo "== dxvk =="
find $G/opt -maxdepth 5 -iname "*dxvk*" -o -maxdepth 5 -name "d3d11.dll" 2>/dev/null | head -8
echo "== x86_64 lib dir =="
ls $G/lib/x86_64-linux-gnu 2>/dev/null | head -8
echo "== vulkan loader =="
find $G -name "libvulkan*" 2>/dev/null | head -4
echo "== x86_64 libvulkan =="
find $G -path "*x86_64*" -name "libvulkan*" 2>/dev/null | head -4
