#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo2.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
BOX=$G/bin/box64
WINE=$G/wine-9.3-vanilla-wow64/bin/wine
WP=$G/opt/prefix/wp1
rm -rf $WP
export WINEPREFIX=$WP DISPLAY=:0
export WINEDEBUG=warn+all,err+all
echo "== wineboot with logging (timeout 150s) =="
timeout 150 $BOX $WINE wineboot -i > /sdcard/Download/wineboot.log 2>&1
echo "wb-rc=$?"
ls $WP/drive_c/windows/system32 2>/dev/null | head -3 && echo PREFIX_CREATED
tail -20 /sdcard/Download/wineboot.log
echo "ALL_DONE $(date)"
