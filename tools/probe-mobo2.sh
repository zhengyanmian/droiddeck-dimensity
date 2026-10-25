#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/probe-mobo2.txt 2>&1
G=/data/data/com.termux/files/usr/glibc
echo "== box dir =="
ls $G/opt/box/ 2>&1
find $G/opt/box -name "box64" 2>/dev/null | head -3
echo "== wine bin =="
ls $G/wine-9.3-vanilla-wow64/bin/ 2>&1
echo "== prefix =="
ls $G/opt/prefix/ 2>&1
echo "== extract dxvk 2.0 =="
cd $G/opt/libs/d3d && 7z x -y dxvk-async-2.0.7z -o/tmp/dxvk >/dev/null 2>&1; ls /tmp/dxvk/x64 2>/dev/null | head -8
echo "== install dxvk dlls into prefix =="
PFX=$(find $G/opt/prefix -maxdepth 3 -name "drive_c" -type d 2>/dev/null | head -1)
echo "PFX=$PFX"
if [ -n "$PFX" ]; then
  SDIR=$(dirname $PFX)/windows/system32
  cp /tmp/dxvk/x64/d3d11.dll /tmp/dxvk/x64/dxgi.dll /tmp/dxvk/x64/d3d9.dll /tmp/dxvk/x64/d3d10core.dll $SDIR/ 2>&1 && echo dlls-ok
fi
echo "ALL_PROBED"
