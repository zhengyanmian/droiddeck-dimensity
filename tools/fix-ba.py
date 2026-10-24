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

with open(path, "w") as f:
    f.writelines(lines)

print("FIXED. Line 54 now:")
print(lines[53])
