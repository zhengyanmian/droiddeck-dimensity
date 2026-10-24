#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/wineerr.txt 2>&1
proot-distro login ubuntu --shared-tmp -- bash -c "echo ===na===; tail -8 /root/na.txt 2>/dev/null; echo ===tb===; tail -8 /root/tb.txt 2>/dev/null; echo ===tc===; tail -8 /root/tc.txt 2>/dev/null"
