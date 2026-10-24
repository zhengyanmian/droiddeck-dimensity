#!/bin/bash
# Build panvk-host.apk entirely on WSL: NDK clang + build-tools aapt/zipalign/apksigner.
# Usage: bash build.sh   (run from anywhere; paths are absolute)
set -euo pipefail

SRC=/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/host-apk
DRIVER=/root/build/panvk-kbase-android/dist/android-android-g720-v12-csf/libvulkan_panfrost.so
LIBDRM=/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/.downloads/libdrm.so
NDK=/opt/android-ndk-r28b
BT=/root/build/sdk/bt/android-14
AJAR=/root/build/sdk/plat/android-34-ext10/android.jar
WORK=/root/build/panvk-host
TC=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin
CC=$TC/aarch64-linux-android29-clang

mkdir -p "$WORK"
cd "$WORK"

echo "== 1. shaders -> spv -> host_spv.h =="
glslangValidator -V --target-env vulkan1.0 "$SRC/shaders/host.vert" -o host.vert.spv >/dev/null
glslangValidator -V --target-env vulkan1.0 "$SRC/shaders/host.frag" -o host.frag.spv >/dev/null
python3 - <<'EOF'
def emit(path, name, out):
    data = open(path, 'rb').read()
    words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
    with open(out, 'a') as f:
        f.write(f"static const uint32_t {name}[] = {{\n")
        for i in range(0, len(words), 6):
            f.write("  " + ",".join(f"0x{w:08x}" for w in words[i:i+6]) + ",\n")
        f.write("};\n")
        f.write(f"static const unsigned int {name}_len = sizeof({name});\n\n")

open("host_spv.h", "w").write("/* generated from host.vert/host.frag, do not edit */\n#include <stdint.h>\n\n")
emit("host.vert.spv", "host_vert_spv", "host_spv.h")
emit("host.frag.spv", "host_frag_spv", "host_spv.h")
EOF
wc -c host_spv.h

echo "== 2. compile native lib =="
$CC -O2 -shared -fPIC -Wall -Wno-unused-variable -Wno-unused-function \
    -I"$WORK" -I"$NDK/sources/android/native_app_glue" \
    "$SRC/jni/panvk_host.c" "$NDK/sources/android/native_app_glue/android_native_app_glue.c" \
    -o libpanvk_host.so \
    -landroid -lEGL -lGLESv2 -llog -ldl
ls -la libpanvk_host.so

echo "== 3. stage libs =="
rm -rf stage && mkdir -p stage/lib/arm64-v8a
cp libpanvk_host.so stage/lib/arm64-v8a/
cp "$DRIVER" stage/lib/arm64-v8a/
cp "$LIBDRM" stage/lib/arm64-v8a/
ls -la stage/lib/arm64-v8a/

echo "== 4. package =="
rm -f base.apk aligned.apk panvk-host.apk
"$BT/aapt" package -f -M "$SRC/AndroidManifest.xml" -I "$AJAR" -F base.apk
( cd stage && zip -q ../base.apk lib/arm64-v8a/libpanvk_host.so lib/arm64-v8a/libvulkan_panfrost.so lib/arm64-v8a/libdrm.so )
"$BT/zipalign" -f 4 base.apk aligned.apk

echo "== 5. sign =="
if [ ! -f debug.keystore ]; then
  keytool -genkeypair -keystore debug.keystore -alias androiddebugkey \
    -storepass android -keypass android \
    -dname "CN=Android Debug,O=Android,C=US" -keyalg RSA -keysize 2048 -validity 10000
fi
"$BT/apksigner" sign --ks debug.keystore --ks-pass pass:android --key-pass pass:android \
  --out panvk-host.apk aligned.apk
"$BT/apksigner" verify --print-certs panvk-host.apk | head -3

echo "== done =="
ls -la panvk-host.apk
