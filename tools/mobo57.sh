#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo57.txt 2>&1
echo "START $(date)"
G=/data/data/com.termux/files/usr/glibc
echo "== mobox X start logic =="
grep -rn "termux-x11\|XDG_RUNTIME\|X11-unix" $G/opt/scripts/mobox $G/opt/scripts/mobox_menu 2>/dev/null | head -10
echo "== binary check =="
command -v termux-x11 2>&1
ls /data/data/com.termux/files/usr/bin/ | grep -i "x11\|xserver" | head -4
echo "== try foreground start 5s =="
timeout 5 termux-x11 :0 2>&1 | head -6
echo "rc=$?"
