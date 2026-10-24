#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/tri2.txt 2>&1
echo "START $(date)"
export PROOT_NO_SECCOMP=1 DISPLAY=:0
nohup proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine; box64 /opt/wine/bin/wineserver -p" > /dev/null 2>&1 &
sleep 5
proot-distro login ubuntu --shared-tmp -- bash -c "export WINEPREFIX=/root/.wine VK_DRIVER_FILES=/root/panvk/panvk_icd.json; cat > /root/panvk/panvk_icd.json << 'EOF'
{
  \"file_format_version\": \"1.0.0\",
  \"ICD\": {
    \"library_path\": \"/root/panvk/libvulkan_panfrost.so\",
    \"api_version\": \"1.3.0\"
  }
}
EOF
cat /root/panvk/panvk_icd.json; cd /root/.wine/drive_c/test && WINEDEBUG=+loaddll box64 /opt/wine/bin/wine d3d11tri.exe 2>&1 | grep -E 'Loaded|loaddll|err:|assert|Aborted|FAIL|ok|adapter|SUCCESS' | head -25"
echo "ALL_DONE $(date)"
