// d3d11tri.cpp — minimal D3D11 clear+triangle test for DXVK-on-PanVK.
// Renders 180 frames of a rotating-color clear + green triangle, then exits 0.
// Build: x86_64-w64-mingw32-g++ -O2 d3d11tri.cpp -o d3d11tri.exe -ld3d11 -ldxgi
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <cstdio>

struct Vtx { float x, y, z; float r, g, b; };
static Vtx g_verts[3] = {
    {  0.0f,  0.6f, 0.5f,   0.1f, 0.9f, 0.1f },
    {  0.6f, -0.4f, 0.5f,   0.1f, 0.9f, 0.1f },
    { -0.6f, -0.4f, 0.5f,   0.1f, 0.9f, 0.1f },
};

static const char* g_vs = R"(
struct VIn { float4 pos : POSITION; float4 col : COLOR; };
struct VOut { float4 pos : SV_POSITION; float4 col : COLOR; };
VOut main(VIn i) { VOut o; o.pos = i.pos; o.col = i.col; return o; }
)";
static const char* g_ps = R"(
struct PIn { float4 pos : SV_POSITION; float4 col : COLOR; };
float4 main(PIn i) : SV_Target { return i.col; }
)";

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    freopen("C:\\test\\log.txt", "w", stdout);
    AllocConsole();
    IDXGIFactory1 *factory = NULL;
    CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&factory);
    if (!factory) { printf("FAIL CreateDXGIFactory1\n"); return 1; }
    IDXGIAdapter1 *ad = NULL;
    factory->EnumAdapters1(0, &ad);
    DXGI_ADAPTER_DESC1 desc; ad->GetDesc1(&desc);
    wprintf(L"[ok] adapter: %s\n", desc.Description);

    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device *dev = NULL; ID3D11DeviceContext *ctx = NULL;
    HRESULT hr = D3D11CreateDevice(ad, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
                                   &fl, 1, D3D11_SDK_VERSION, &dev, NULL, &ctx);
    if (FAILED(hr)) { printf("FAIL D3D11CreateDevice hr=0x%08lx\n", (unsigned long)hr); return 1; }
    printf("[ok] D3D11 device @ FL 11_0\n");

    HWND wnd = CreateWindowExA(0, "STATIC", "d3d11tri", WS_OVERLAPPEDWINDOW,
                               50, 50, 800, 600, NULL, NULL, hInst, NULL);
    IDXGISwapChain *sc = NULL;
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = wnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = factory->CreateSwapChain(dev, &sd, &sc);
    if (FAILED(hr)) { printf("FAIL CreateSwapChain hr=0x%08lx\n", (unsigned long)hr); return 1; }
    ID3D11Texture2D *bb = NULL;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&bb);
    ID3D11RenderTargetView *rtv = NULL;
    dev->CreateRenderTargetView(bb, NULL, &rtv);
    D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0, 1 };
    ctx->RSSetViewports(1, &vp);

    ID3D11Buffer *vbuf = NULL;
    D3D11_BUFFER_DESC bd = { sizeof(g_verts), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0 };
    D3D11_SUBRESOURCE_DATA init = { g_verts, 0, 0 };
    dev->CreateBuffer(&bd, &init, &vbuf);

    // compile shaders from inline HLSL via runtime compilation is heavy; use
    // precompiled bytecode instead: fall back to position-only passthrough by
    // building shaders with D3DCompile at runtime.
    ID3DBlob *vsb = NULL, *psb = NULL, *err = NULL;
    hr = D3DCompile(g_vs, strlen(g_vs), "vs", NULL, NULL, "main", "vs_5_0", 0, 0, &vsb, &err);
    if (FAILED(hr)) { printf("FAIL compile vs\n"); return 1; }
    hr = D3DCompile(g_ps, strlen(g_ps), "ps", NULL, NULL, "main", "ps_5_0", 0, 0, &psb, &err);
    if (FAILED(hr)) { printf("FAIL compile ps\n"); return 1; }
    ID3D11VertexShader *vs = NULL; ID3D11PixelShader *ps = NULL;
    dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), NULL, &vs);
    dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), NULL, &ps);
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout *il = NULL;
    dev->CreateInputLayout(ied, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), &il);

    for (int f = 0; f < 180; f++) {
        float c[4] = { 0.9f * (f % 60) / 60.0f, 0.1f, 0.9f - 0.9f * (f % 60) / 60.0f, 1.0f };
        ctx->ClearRenderTargetView(rtv, c);
        ctx->OMSetRenderTargets(1, &rtv, NULL);
        ctx->IASetInputLayout(il);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT stride = sizeof(Vtx), offset = 0;
        ctx->IASetVertexBuffers(0, 1, &vbuf, &stride, &offset);
        ctx->VSSetShader(vs, NULL, 0);
        ctx->PSSetShader(ps, NULL, 0);
        ctx->Draw(3, 0);
        sc->Present(1, 0);
        if (f % 60 == 0) printf("[frame %d] ok\n", f);
    }
    printf("D3D11TRI_SUCCESS (180 frames)\n");
    return 0;
}
