#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo64.txt 2>&1
echo "START $(date)"
proot-distro login ubuntu --shared-tmp -- bash -c "apt update 2>&1 | tail -1; DEBIAN_FRONTEND=noninteractive apt install -y libx11-dev libxcb1-dev libxcb-dri3-dev libx11-xcb-dev libxcb-keysyms-dev libxshmfence-dev libxcb-randr-dev libxcb-shm-dev libxcb-xfixes-dev libxcb-present-dev libxcb-sync-dev zlib1g-dev libexpat1-dev 2>&1 | tail -2; echo APT_RC=\$?"
echo "== current build platforms config =="
proot-distro login ubuntu --shared-tmp -- bash -c "grep -E 'platforms|android' /root/build/panvk-kbase-android/build/linux-glibc-cross/meson-info/intro-buildoptions.json 2>/dev/null | head -3; meson configure /root/build/panvk-kbase-android/build/linux-glibc-cross 2>/dev/null | grep -iE 'platform|wsi' | head -4"
echo "ALL_DONE $(date)"
