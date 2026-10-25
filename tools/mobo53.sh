#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo53.txt 2>&1
echo "START $(date)"
mkdir -p /data/data/com.termux/files/home/dl
cd /data/data/com.termux/files/home/dl
P="https://github.com/doitsujin/dxvk/releases/download/v1.10.3/dxvk-1.10.3.tar.gz"
for m in "" "https://gh-proxy.com/" "https://ghfast.top/"; do curl -sfL --retry 2 -m 240 -o dxvk-1.10.3.tar.gz "${m}${P}" && break; done
ls -la dxvk-1.10.3.tar.gz
md5sum dxvk-1.10.3.tar.gz
cp dxvk-1.10.3.tar.gz /sdcard/Download/ 2>/dev/null
echo "ALL_DONE $(date)"
