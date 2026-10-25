#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe-mobo3.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
echo "== WINEPREFIX in scripts =="
grep -rn "WINEPREFIX=" $G/opt/scripts/mobox $G/opt/scripts/mobox_menu $G/opt/scripts/prefix-settings 2>/dev/null | head -5
echo "== box64 tarball contents =="
tar tJf $G/opt/box/mar3.tar.xz 2>/dev/null | head -8
echo "== extract box64 =="
tar xJf $G/opt/box/mar3.tar.xz -C $G/ && echo box64-extracted
ls $G/usr/local/bin/box64 2>/dev/null || find $G -name "box64" -maxdepth 4 2>/dev/null | head -3
echo "== dxvk 7z listing =="
7z l $G/opt/libs/d3d/dxvk-async-2.0.7z 2>/dev/null | grep -E "d3d11|dxgi" | head -4
echo "== mesa dir (GL for wined3d?) =="
ls $G/opt/prefix/mesa 2>/dev/null | head -5
