#!/data/data/com.termux/files/usr/bin/bash
# DroidDeck: non-interactive Mobox Termux-side installer (wow64 wine)
exec > /sdcard/Download/moboinst.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
cd /data/data/com.termux/files/home

echo "== 1: storage symlink =="
mkdir -p ~/storage
ln -sfn /sdcard ~/storage/shared
ls ~/storage/shared/ | head -2

echo "== 2: packages =="
pkg install -y x11-repo 2>&1 | tail -1
pkg install -y pulseaudio xwayland wget patchelf p7zip xorg-xrandr ncurses-utils hashdeep termux-am 2>&1 | tail -2
pkg install -y termux-x11-nightly 2>&1 | tail -1

echo "== 3: glibc env =="
mkdir -p $PREFIX/glibc/opt/package-manager/installed
echo "PROJECT_ID=54240888" > $PREFIX/glibc/opt/package-manager/token

echo "== 4: fetch package-manager from gitlab =="
wget -q --retry-connrefused --tries=3 --timeout=30 "https://gitlab.com/api/v4/projects/54240888/repository/files/package-manager/raw?ref=main" -O $PREFIX/glibc/opt/package-manager/package-manager
ls -la $PREFIX/glibc/opt/package-manager/package-manager
head -3 $PREFIX/glibc/opt/package-manager/package-manager

echo "== 5: sync-all =="
. $PREFIX/glibc/opt/package-manager/package-manager
sync-all 2>&1 | tail -3

echo "== 6: sync wine wow64 =="
sync-package wine-9.3-vanilla-wow64 2>&1 | tail -3

echo "== 7: mobox script =="
ls $PREFIX/glibc/opt/scripts/ 2>&1
ln -sf $PREFIX/glibc/opt/scripts/mobox $PREFIX/bin/mobox 2>&1

echo "ALL_DONE $(date)"
