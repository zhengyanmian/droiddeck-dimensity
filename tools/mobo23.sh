#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo23.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
U=$R/usr/lib/aarch64-linux-gnu
I=/data/data/com.termux/files/home/panvk-icd
LD=$U/ld-linux-aarch64.so.1

echo "== patchelf probe =="
ls -la /data/data/com.termux/files/home/ve 2>&1
patchelf --set-interpreter $LD --set-rpath "$I:$U" /data/data/com.termux/files/home/ve 2>&1
echo "== probe A: LD=$I (full loader first) =="
(
  export LD_LIBRARY_PATH=$I:$G/lib/x86_64-linux-gnu
  /data/data/com.termux/files/home/ve A
) 2>&1 | head -12
echo "== probe B: Mobox loader only =="
(
  export LD_LIBRARY_PATH=$G/lib/x86_64-linux-gnu:$G/lib
  /data/data/com.termux/files/home/ve B
) 2>&1 | head -12
echo "ALL_DONE $(date)"
