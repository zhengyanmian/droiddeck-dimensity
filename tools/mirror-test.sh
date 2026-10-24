#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mirror-test.txt 2>&1
proot-distro login ubuntu --shared-tmp -- bash -c '
P="https://github.com/Kron4ek/Wine-Builds/releases/download/11.20/wine-11.20-amd64.tar.xz"
for m in "" "https://gh-proxy.com/" "https://ghfast.top/" "https://gh.llkk.cc/" "https://github.moeyy.xyz/"; do
  echo "== mirror[$m]"
  curl -sfI -m 20 -L "${m}${P}" 2>&1 | head -4
done
echo PROBE_DONE'
