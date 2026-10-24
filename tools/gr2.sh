#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/gr2.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
H=/data/data/com.termux/files/home
for U in "https://raw.githubusercontent.com/sabamdarif/termux-glibc/main/install.sh" "https://gh-proxy.com/https://raw.githubusercontent.com/sabamdarif/termux-glibc/main/install.sh" "https://ghfast.top/https://raw.githubusercontent.com/sabamdarif/termux-glibc/main/install.sh"; do
  curl -sfL -m 60 -o $H/gr-install.sh "$U" && echo "fetched from $U" && break
done
ls -la $H/gr-install.sh 2>&1
head -3 $H/gr-install.sh 2>&1
echo "== running installer =="
bash $H/gr-install.sh 2>&1 | tail -6
echo "== verify =="
command -v grun 2>&1
command -v glibc-run 2>&1
echo "ALL_DONE $(date)"
