import re
p = T = "/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/d3d11tri.cpp"
src = open(p).read()
old = """    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device *dev = NULL; ID3D11DeviceContext *ctx = NULL;
    HRESULT hr = D3D11CreateDevice(ad, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
                                   &fl, 1, D3D11_SDK_VERSION, &dev, NULL, &ctx);
    if (FAILED(hr)) { printf("FAIL D3D11CreateDevice hr=0x%08lx\\n", (unsigned long)hr); return 1; }
    printf("[ok] D3D11 device @ FL 11_0\\n");"""
new = """    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device *dev = NULL; ID3D11DeviceContext *ctx = NULL;
    HRESULT hr = D3D11CreateDevice(ad, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
                                   &fl, 1, D3D11_SDK_VERSION, &dev, NULL, &ctx);
    printf("[try1 adapter+UNKNOWN 11_0] hr=0x%08lx\\n", (unsigned long)hr);
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(ad, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
                               NULL, 0, D3D11_SDK_VERSION, &dev, NULL, &ctx);
        printf("[try2 adapter+UNKNOWN default] hr=0x%08lx\\n", (unsigned long)hr);
    }
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                               &fl, 1, D3D11_SDK_VERSION, &dev, NULL, &ctx);
        printf("[try3 NULL+HARDWARE 11_0] hr=0x%08lx\\n", (unsigned long)hr);
    }
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                               NULL, 0, D3D11_SDK_VERSION, &dev, NULL, &ctx);
        printf("[try4 NULL+HARDWARE default] hr=0x%08lx\\n", (unsigned long)hr);
    }
    if (FAILED(hr)) { printf("FAIL D3D11CreateDevice all variants\\n"); return 1; }
    printf("[ok] D3D11 device created\\n");"""
assert old in src
src = src.replace(old, new)
open(p, "w").write(src)
print("variants added")
