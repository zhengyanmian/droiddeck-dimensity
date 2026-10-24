#!/data/data/com.termux/files/usr/bin/bash
# wine-setup.sh — DroidDeck: install Wine (Kron4ek amd64) into proot Ubuntu.
exec > /sdcard/Download/wine-status.txt 2>&1
echo "START $(date)"
echo "step 1/7: curl + runtime deps"
proot-distro login ubuntu --shared-tmp -- bash -c "export DEBIAN_FRONTEND=noninteractive; apt install -y curl libgnutls30 libxrender1 libxrandr2 libxi6 libxxf86vm1 libxcursor1 libxinerama1 libxkbcommon0 libwayland-client0 libasound2 libcups2 libkrb5-3 libgssapi-krb5-2 libv4l-0 libpcap0.8 libodbc2 libnetapi0 samba-common libsdl2-2.0-0; echo deps-ok"
echo "step 2/7: query latest Wine release"
proot-distro login ubuntu --shared-tmp -- bash -c "curl -sfL --retry 3 https://api.github.com/repos/Kron4ek/Wine-Builds/releases/latest -o /tmp/rel.json && grep -o '\"browser_download_url\": \"[^\"]*amd64.tar.xz\"' /tmp/rel.json | head -2"
echo "step 3/7: download Wine amd64 (resume+retry)"
proot-distro login ubuntu --shared-tmp -- bash -c 'mkdir -p /opt/dl && cd /opt/dl; URL=$(grep -o "\"browser_download_url\": \"[^\"]*amd64.tar.xz\"" /tmp/rel.json | head -1 | cut -d\" -f4); echo "URL=$URL"; for i in 1 2 3 4 5 6; do curl -fL -C - --retry 3 -o wine.tar.xz "$URL" && break; sleep 5; done; ls -la wine.tar.xz'
echo "step 4/7: extract"
proot-distro login ubuntu --shared-tmp -- bash -c "mkdir -p /opt/wine && cd /opt/dl && tar xJf wine.tar.xz -C /opt/wine --strip-components=1 && ls /opt/wine/bin | head -5 && echo extract-ok"
echo "step 5/7: wineboot init (box64)"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine64 wineboot -i 2>&1 | tail -5; ls /root/.wine/drive_c >/dev/null && echo prefix-ok"
echo "step 6/7: wine cmd test"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine64 cmd /c echo WINE_IS_ALIVE 2>&1 | tail -3"
echo "step 7/7: verify"
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine DISPLAY=:0; box64 /opt/wine/bin/wine64 --version; echo verify-rc=\$?"
echo "ALL_DONE $(date)"
