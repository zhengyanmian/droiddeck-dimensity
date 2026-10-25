#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/morning1.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
P=/data/data/com.termux/files/usr
R=$P/var/lib/proot-distro/containers/ubuntu/rootfs
export WINEPREFIX=$R/root/.wine DISPLAY=:0 VK_DRIVER_FILES=/root/panvk/panvk_icd.json
export WINEDLLOVERRIDES="d3d11,dxgi,d3d9,d3d10,d3d10core=n"
echo "== A: dynarec OFF + wine cmd =="
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 BOX64_DYNAREC=0; box64 /opt/wine/bin/wine cmd /c echo WINE_ALIVE > /root/ma.txt 2>&1; echo a-rc=\$?; grep WINE_ALIVE /root/ma.txt | head -1"
echo "== B: dynarec OFF + d3d11tri =="
proot-distro login ubuntu --shared-tmp -- bash -c "export PROOT_NO_SECCOMP=1 BOX64_DYNAREC=0; mkdir -p /root/.wine/drive_c/test; cp /sdcard/Download/panvk-x11/d3d11tri.exe /root/.wine/drive_c/test/; cd /root/.wine/drive_c/test && timeout 600 box64 /opt/wine/bin/wine d3d11tri.exe > /root/mb.txt 2>&1; echo b-rc=\$?; tail -12 /root/mb.txt"
echo "ALL_DONE $(date)"
