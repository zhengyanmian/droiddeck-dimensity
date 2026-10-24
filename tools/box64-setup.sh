#!/data/data/com.termux/files/usr/bin/bash
# box64-setup.sh — DroidDeck: full box64 build inside proot Ubuntu.
# All output -> /sdcard/Download/box64-status.txt (polled via adb).
exec > /sdcard/Download/box64-status.txt 2>&1
echo "START $(date)"
echo "step 1/6: dpkg fix"
proot-distro login ubuntu --shared-tmp -- bash -c "export DEBIAN_FRONTEND=noninteractive; dpkg --configure -a; echo dpkg-ok"
echo "step 2/6: apt deps"
proot-distro login ubuntu --shared-tmp -- bash -c "export DEBIAN_FRONTEND=noninteractive; apt install -y tzdata cmake python3 git build-essential; echo apt-ok"
echo "step 3/6: clone box64"
proot-distro login ubuntu --shared-tmp -- bash -c "rm -rf /root/box64; git clone --depth 1 https://github.com/ptitSeb/box64 /root/box64 && echo clone-ok"
echo "step 4/6: cmake"
proot-distro login ubuntu --shared-tmp -- bash -c "cmake -S /root/box64 -B /root/box64/build -DARM_DYNAREC=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_DYNAREC=aarch64 && echo cmake-ok"
echo "step 5/6: make -j8 (long)"
proot-distro login ubuntu --shared-tmp -- bash -c "make -C /root/box64/build -j8 && echo make-ok"
echo "step 6/6: install + verify"
proot-distro login ubuntu --shared-tmp -- bash -c "make -C /root/box64/build install >/dev/null 2>&1; box64 --version; echo verify-rc=\$?"
echo "ALL_DONE $(date)"
cp /sdcard/Download/box64-status.txt /sdcard/Download/box64-status-final.txt 2>/dev/null
