#!/usr/bin/env python3
p = "/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/d3d11tri.cpp"
src = open(p).read()
old = "    setvbuf(stdout, NULL, _IONBF, 0);"
new = '    setvbuf(stdout, NULL, _IONBF, 0);\n    freopen("C:\\\\test\\\\log.txt", "w", stdout);'
assert old in src
src = src.replace(old, new)
open(p, "w").write(src)
print("freopen added")
