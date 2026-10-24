#!/data/data/com.termux/files/usr/bin/bash
# box64-setup2.sh — build box64 from local tarball (no network needed).
exec > /sdcard/Download/box64-status.txt 2>&1
echo "START $(date)"
echo "step 1/5: unpack source"
proot-distro login ubuntu --shared-tmp -- bash -c "rm -rf /root/box64 && mkdir -p /root/box64 && tar xzf /sdcard/Download/box64src.tar.gz -C /root/box64 && echo unpack-ok"
echo "step 2/5: cmake"
proot-distro login ubuntu --shared-tmp -- bash -c "cmake -S /root/box64 -B /root/box64/build -DARM_DYNAREC=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_DYNAREC=aarch64 && echo cmake-ok"
echo "step 3/5: make -j8 (long)"
proot-distro login ubuntu --shared-tmp -- bash -c "make -C /root/box64/build -j8 && echo make-ok"
echo "step 4/5: install"
proot-distro login ubuntu --shared-tmp -- bash -c "make -C /root/box64/build install > /dev/null 2>&1 && echo install-ok"
echo "step 5/5: verify"
proot-distro login ubuntu --shared-tmp -- bash -c "box64 --version; echo verify-rc=\$?"
echo "ALL_DONE $(date)"
