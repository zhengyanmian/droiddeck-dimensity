#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo41.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
echo "== archive check =="
ls -la $R/opt/dl/dxvk-2.7.tar.gz 2>&1
tar tzf $R/opt/dl/dxvk-2.7.tar.gz 2>&1 | head -5
echo "tar-list-rc=$?"
echo "== md5 =="
md5sum $R/opt/dl/dxvk-2.7.tar.gz 2>&1
