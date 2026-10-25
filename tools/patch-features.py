import sys
p = "/root/build/panvk-kbase-android/work/mesa/src/panfrost/vulkan/panvk_vX_physical_device.c"
src = open(p).read()
pairs = [
    (".geometryShader = false,", ".geometryShader = true,"),
    (".tessellationShader = false,", ".tessellationShader = true,"),
    (".fillModeNonSolid = false,", ".fillModeNonSolid = true,"),
    (".shaderClipDistance = false,", ".shaderClipDistance = true,"),
    (".shaderCullDistance = false,", ".shaderCullDistance = true,"),
    (".textureCompressionBC = has_texture_compression_bc(device),", ".textureCompressionBC = true,"),
]
n = 0
for old, new in pairs:
    if old in src:
        src = src.replace(old, new)
        n += 1
open(p, "w").write(src)
print(f"patched {n}/6 features")
