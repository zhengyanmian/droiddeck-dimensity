import io
p = "C:/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/alloc-probe.c"
src = open(p).read()
cut = src.find("    /* --- compositor-style readback")
src = src[:cut] + "    printf(\"DONE\\n\");\n    return 0;\n}\n"
open(p, "w").write(src)
print("tail fixed")
