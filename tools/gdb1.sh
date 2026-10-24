#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gdb1.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 DISPLAY=:0
echo "step 1: install gdb"
proot-distro login ubuntu --shared-tmp -- bash -c "export DEBIAN_FRONTEND=noninteractive; apt install -y gdb 2>&1 | tail -1"
echo "step 2: gdb wine notepad"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; cd /root/.wine/drive_c/test && gdb -batch -ex run -ex 'info registers pc' -ex bt -ex 'x/4i \$pc' --args box64 /opt/wine/bin/wine notepad 2>&1 | tail -25"
echo "ALL_DONE $(date)"
