path = "/root/build/panvk-kbase-android/scripts/build-android.sh"
shims = "/root/build/panvk-kbase-android/shims"

with open(path) as f:
    lines = f.readlines()

new54 = (
    '  -Dcpp_link_args="['
    "'-L" + shims + "', '-Wl,--allow-shlib-undefined', '-static-libstdc++'"
    ']"'
    ' >>"$LOG" 2>&1 || { tail -n 20 "$LOG"; echo "BUILD-FAIL: meson setup (log $LOG)" >&2; exit 1; }\n'
)

lines[53] = new54

# Fix 2 (2026-10-24): -Dplatforms=android,x11 pulls glibc X11 libs (libxcb & co.)
# as DT_NEEDED into the ICD — unbootable on Android. Pure android WSI instead.
if "-Dplatforms=android,x11" in lines[48]:
    lines[48] = lines[48].replace("-Dplatforms=android,x11", "-Dplatforms=android")
    print("FIXED platforms: android,x11 -> android")
else:
    print("platforms line already patched or moved; line 49 = " + lines[48])

with open(path, "w") as f:
    f.writelines(lines)

print("FIXED. Line 54 now:")
print(lines[53])
