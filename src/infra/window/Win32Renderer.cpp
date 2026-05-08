#include "Win32Renderer.hpp"

#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_set>

#include "WebViewRenderer.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace omc::infra
{
    // =========================================================
    // Shaders — UV + modo + premultiplicación para DComp
    // =========================================================
    const char* g_vsCode = R"(
        struct VS_INPUT {
            float2 pos  : POSITION;
            float4 col  : COLOR;
            float2 uv   : TEXCOORD;
            uint   mode : BLENDINDICES;
        };
        struct PS_INPUT {
            float4 pos  : SV_POSITION;
            float4 col  : COLOR;
            float2 uv   : TEXCOORD;
            uint   mode : BLENDINDICES;
        };
        PS_INPUT main(VS_INPUT i) {
            PS_INPUT o;
            o.pos  = float4(i.pos, 0.0f, 1.0f);
            o.col  = i.col;
            o.uv   = i.uv;
            o.mode = i.mode;
            return o;
        }
    )";

    const char* g_psCode = R"(
        Texture2D    g_tex     : register(t0);
        SamplerState g_sampler : register(s0);

        struct PS_INPUT {
            float4 pos  : SV_POSITION;
            float4 col  : COLOR;
            float2 uv   : TEXCOORD;
            uint   mode : BLENDINDICES;
        };

        float4 main(PS_INPUT i) : SV_TARGET {
            float4 c;
            if (i.mode == 1u) {
                c = g_tex.Sample(g_sampler, i.uv);
                c.rgb *= c.a;   // premultiply para DirectComposition
            } else {
                c = i.col;      // ya viene premultiplicado desde CPU
            }
            return c;
        }
    )";

    // =========================================================
    // Impl
    // =========================================================
    struct TextureEntry {
        ID3D11Texture2D* tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0;
        int height = 0;

        void release() {
            if (srv) { srv->Release(); srv = nullptr; }
            if (tex) { tex->Release(); tex = nullptr; }
            width = height = 0;
        }
    };

    struct Win32Renderer::Impl {
        HWND g_hwnd = nullptr;

        ID3D11Device* g_device = nullptr;
        ID3D11DeviceContext* g_context = nullptr;
        IDXGISwapChain* g_swapChain = nullptr;
        ID3D11RenderTargetView* g_rtv = nullptr;

        ID3D11VertexShader* g_vs = nullptr;
        ID3D11PixelShader* g_ps = nullptr;
        ID3D11InputLayout* g_inputLayout = nullptr;
        ID3D11Buffer* g_vertexBuffer = nullptr;
        ID3D11BlendState* g_blendState = nullptr;
        ID3D11SamplerState* g_sampler = nullptr;

        IDCompositionDevice* g_dcompDevice = nullptr;
        IDCompositionTarget* g_dcompTarget = nullptr;
        IDCompositionVisual* g_rootVisual = nullptr;
        IDCompositionVisual* g_swapChainVisual = nullptr;

        ID2D1Factory* g_d2dFactory = nullptr;
        IDWriteFactory* g_dwriteFactory = nullptr;
        IDWriteTextFormat* g_defaultFont = nullptr;

        std::unordered_map<int, TextureEntry> g_textureCache;
        size_t g_vertexBufferCapacity = 0;

        HitTestManager g_hitTest;

        std::unordered_map<int, std::unique_ptr<WebViewRenderer>> webViews;
        std::unordered_map<int, std::string>                      webViewUrls;

        int surfaceWidth = 0;
        int surfaceHeight = 0;
    };

    // =========================================================
    // WndProc
    // =========================================================
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto* self = reinterpret_cast<Win32Renderer::Impl*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (self) {
            for (auto& [id, wv] : self->webViews) {
                if (!wv) continue;
                if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
                    wv->HandleMouseMessage(msg, wParam, lParam);
                else if (msg == WM_KEYDOWN || msg == WM_KEYUP ||
                    msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_CHAR)
                    wv->HandleKeyboardMessage(msg, wParam, lParam);
            }
        }

        switch (msg) {
        case WM_DESTROY:  PostQuitMessage(0); return 0;
        case WM_NCHITTEST: return HTCLIENT;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // =========================================================
    // Constructor / Destructor
    // =========================================================
    Win32Renderer::Win32Renderer(omc::event::EventBus& eventBus)
        : eventBus(eventBus), m_pimpl(std::make_unique<Impl>())
    {
        eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const auto& e) {
            onExit(e);
            });
    }

    Win32Renderer::~Win32Renderer() = default;

    // =========================================================
    // Helpers de vértices
    // =========================================================
    static Vertex makeVert(float px, float py, float r, float g, float b, float a,
        float u, float v, uint32_t mode, int sw, int sh)
    {
        return Vertex{
            (px / sw) * 2.0f - 1.0f,
            1.0f - (py / sh) * 2.0f,
            r, g, b, a, u, v, mode
        };
    }

    static void AddColorRect(std::vector<Vertex>& verts,
        const omc::ui::RectCmd& cmd, int sw, int sh)
    {
        float x = cmd.rect.position.x, y = cmd.rect.position.y;
        float w = cmd.rect.size.x, h = cmd.rect.size.y;

        // Premultiply alpha para DirectComposition
        float a = cmd.color.a / 255.0f;
        float r = (cmd.color.r / 255.0f) * a;
        float g = (cmd.color.g / 255.0f) * a;
        float b = (cmd.color.b / 255.0f) * a;

        Vertex v0 = makeVert(x, y, r, g, b, a, 0, 0, 0, sw, sh);
        Vertex v1 = makeVert(x + w, y, r, g, b, a, 1, 0, 0, sw, sh);
        Vertex v2 = makeVert(x, y + h, r, g, b, a, 0, 1, 0, sw, sh);
        Vertex v3 = makeVert(x + w, y + h, r, g, b, a, 1, 1, 0, sw, sh);

        verts.insert(verts.end(), { v0, v1, v2, v2, v1, v3 });
    }

    static void AddTexturedRect(std::vector<Vertex>& verts,
        const omc::ui::ImageCmd& cmd, int sw, int sh)
    {
        float x = cmd.position.x, y = cmd.position.y;
        float w = cmd.size.x, h = cmd.size.y;

        // color blanco — el shader usa la textura directamente
        Vertex v0 = makeVert(x, y, 1, 1, 1, 1, 0, 0, 1, sw, sh);
        Vertex v1 = makeVert(x + w, y, 1, 1, 1, 1, 1, 0, 1, sw, sh);
        Vertex v2 = makeVert(x, y + h, 1, 1, 1, 1, 0, 1, 1, sw, sh);
        Vertex v3 = makeVert(x + w, y + h, 1, 1, 1, 1, 1, 1, 1, sw, sh);

        verts.insert(verts.end(), { v0, v1, v2, v2, v1, v3 });
    }

    // =========================================================
    // InitPipeline
    // =========================================================
    bool Win32Renderer::InitPipeline()
    {
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        auto compileShader = [&](const char* src, const char* target, ID3DBlob*& out) -> bool {
            HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                "main", target, 0, 0, &out, &errorBlob);
            if (FAILED(hr)) {
                if (errorBlob) {
                    OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
                    errorBlob->Release();
                }
                return false;
            }
            return true;
            };

        if (!compileShader(g_vsCode, "vs_4_0", vsBlob)) return false;
        if (!compileShader(g_psCode, "ps_4_0", psBlob)) { vsBlob->Release(); return false; }

        bool ok = true;
        ok &= SUCCEEDED(m_pimpl->g_device->CreateVertexShader(
            vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pimpl->g_vs));
        ok &= SUCCEEDED(m_pimpl->g_device->CreatePixelShader(
            psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pimpl->g_ps));

        if (ok) {
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION",     0, DXGI_FORMAT_R32G32_FLOAT,          0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,          0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDINDICES", 0, DXGI_FORMAT_R32_UINT,              0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            ok &= SUCCEEDED(m_pimpl->g_device->CreateInputLayout(
                layout, _countof(layout),
                vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                &m_pimpl->g_inputLayout));
        }

        vsBlob->Release();
        psBlob->Release();
        return ok;
    }

    bool Win32Renderer::InitBlendState()
    {
        // Premultiplied alpha — requerido por DXGI_ALPHA_MODE_PREMULTIPLIED
        D3D11_BLEND_DESC desc{};
        auto& rt = desc.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D11_BLEND_ONE;
        rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ONE;
        rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        return SUCCEEDED(m_pimpl->g_device->CreateBlendState(&desc, &m_pimpl->g_blendState));
    }

    bool Win32Renderer::InitSamplerState()
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        return SUCCEEDED(m_pimpl->g_device->CreateSamplerState(&desc, &m_pimpl->g_sampler));
    }

    bool Win32Renderer::InitDirectWrite()
    {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pimpl->g_d2dFactory);
        if (FAILED(hr)) return false;

        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pimpl->g_dwriteFactory)
        );
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_dwriteFactory->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            14.0f, L"en-us",
            &m_pimpl->g_defaultFont
        );
        return SUCCEEDED(hr);
    }

    // =========================================================
    // InitD3D
    // =========================================================
    bool Win32Renderer::InitD3D(int width, int height)
    {
        m_pimpl->surfaceWidth = width;
        m_pimpl->surfaceHeight = height;
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            flags, nullptr, 0, D3D11_SDK_VERSION,
            &m_pimpl->g_device, nullptr, &m_pimpl->g_context
        );
        if (FAILED(hr)) { MessageBoxA(nullptr, "D3D11CreateDevice failed", "Error", MB_OK); return false; }

        IDXGIDevice* dxgiDevice = nullptr;
        IDXGIAdapter* adapter = nullptr;
        IDXGIFactory2* factory = nullptr;

        m_pimpl->g_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        dxgiDevice->GetAdapter(&adapter);
        adapter->GetParent(IID_PPV_ARGS(&factory));

        DXGI_SWAP_CHAIN_DESC1 scd{};
        scd.Width = width;
        scd.Height = height;
        scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.BufferCount = 2;
        scd.SampleDesc.Count = 1;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

        IDXGISwapChain1* swapChain1 = nullptr;
        hr = factory->CreateSwapChainForComposition(m_pimpl->g_device, &scd, nullptr, &swapChain1);
        if (FAILED(hr)) { MessageBoxA(nullptr, "CreateSwapChainForComposition failed", "Error", MB_OK); return false; }

        swapChain1->QueryInterface(IID_PPV_ARGS(&m_pimpl->g_swapChain));

        ID3D11Texture2D* backBuffer = nullptr;
        m_pimpl->g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (!backBuffer) { MessageBoxA(nullptr, "GetBuffer failed", "Error", MB_OK); return false; }

        hr = m_pimpl->g_device->CreateRenderTargetView(backBuffer, nullptr, &m_pimpl->g_rtv);
        backBuffer->Release();
        if (FAILED(hr)) { MessageBoxA(nullptr, "CreateRenderTargetView failed", "Error", MB_OK); return false; }

        D3D11_VIEWPORT vp{};
        vp.Width = (FLOAT)width;
        vp.Height = (FLOAT)height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_pimpl->g_context->RSSetViewports(1, &vp);

        hr = DCompositionCreateDevice(dxgiDevice, __uuidof(IDCompositionDevice),
            reinterpret_cast<void**>(&m_pimpl->g_dcompDevice));
        if (FAILED(hr)) { MessageBoxA(nullptr, "DCompositionCreateDevice failed", "Error", MB_OK); return false; }

        hr = m_pimpl->g_dcompDevice->CreateTargetForHwnd(m_pimpl->g_hwnd, TRUE, &m_pimpl->g_dcompTarget);
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_dcompDevice->CreateVisual(&m_pimpl->g_rootVisual);
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_dcompDevice->CreateVisual(&m_pimpl->g_swapChainVisual);
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_swapChainVisual->SetContent(m_pimpl->g_swapChain);
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_rootVisual->AddVisual(m_pimpl->g_swapChainVisual, TRUE, nullptr);
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_dcompTarget->SetRoot(m_pimpl->g_rootVisual);
        if (FAILED(hr)) return false;

        hr = m_pimpl->g_dcompDevice->Commit();
        if (FAILED(hr)) return false;

        dxgiDevice->Release();
        adapter->Release();
        factory->Release();
        swapChain1->Release();

        return true;
    }

    void Win32Renderer::CleanupD3D()
    {
        for (auto& [id, entry] : m_pimpl->g_textureCache) entry.release();
        m_pimpl->g_textureCache.clear();

        auto safeRelease = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };

        safeRelease(m_pimpl->g_defaultFont);
        safeRelease(m_pimpl->g_dwriteFactory);
        safeRelease(m_pimpl->g_d2dFactory);
        safeRelease(m_pimpl->g_sampler);
        safeRelease(m_pimpl->g_blendState);
        safeRelease(m_pimpl->g_inputLayout);
        safeRelease(m_pimpl->g_vs);
        safeRelease(m_pimpl->g_ps);
        safeRelease(m_pimpl->g_vertexBuffer);
        safeRelease(m_pimpl->g_swapChainVisual);
        safeRelease(m_pimpl->g_rootVisual);
        safeRelease(m_pimpl->g_dcompTarget);
        safeRelease(m_pimpl->g_dcompDevice);
        safeRelease(m_pimpl->g_rtv);
        safeRelease(m_pimpl->g_swapChain);
        safeRelease(m_pimpl->g_context);
        safeRelease(m_pimpl->g_device);
    }

    static void UpdateViewport(ID3D11DeviceContext* context, int width, int height)
    {
        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<FLOAT>(width);
        vp.Height = static_cast<FLOAT>(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);
    }

    static void ResizeSwapChainIfNeeded(Win32Renderer::Impl& impl, int width, int height)
    {
        if (!impl.g_swapChain || !impl.g_device || !impl.g_context || !impl.g_rtv) return;
        if (width <= 0 || height <= 0) return;
        if (width == impl.surfaceWidth && height == impl.surfaceHeight) return;

        impl.g_context->OMSetRenderTargets(0, nullptr, nullptr);
        impl.g_rtv->Release();
        impl.g_rtv = nullptr;

        if (FAILED(impl.g_swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0))) {
            return;
        }

        ID3D11Texture2D* backBuffer = nullptr;
        if (FAILED(impl.g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) return;
        if (FAILED(impl.g_device->CreateRenderTargetView(backBuffer, nullptr, &impl.g_rtv))) {
            backBuffer->Release();
            return;
        }
        backBuffer->Release();

        impl.surfaceWidth = width;
        impl.surfaceHeight = height;
        UpdateViewport(impl.g_context, width, height);
    }

    // =========================================================
    // Window
    // =========================================================
    bool Win32Renderer::CreateOverlayWindow(int width, int height)
    {
        WNDCLASS wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "OMCOverlayClass";
        RegisterClass(&wc);

        m_pimpl->g_hwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED,
            wc.lpszClassName, "Overlay Media Controller",
            WS_POPUP, 0, 0, width, height,
            nullptr, nullptr, wc.hInstance, nullptr
        );

        if (!m_pimpl->g_hwnd) return false;
        SetWindowLongPtr(m_pimpl->g_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m_pimpl.get()));
        ShowWindow(m_pimpl->g_hwnd, SW_SHOW);
        return true;
    }

    void Win32Renderer::UpdateClickThrough(HWND hwnd, bool interactive)
    {
        static bool s_isInteractive = true;
        if (s_isInteractive == interactive) return;
        s_isInteractive = interactive;

        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        exStyle = interactive ? (exStyle & ~WS_EX_TRANSPARENT) : (exStyle | WS_EX_TRANSPARENT);
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    // =========================================================
    // Vertex Buffer
    // =========================================================
    bool Win32Renderer::CreateDynamicVertexBuffer(size_t maxVertices)
    {
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * maxVertices);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        bool ok = SUCCEEDED(m_pimpl->g_device->CreateBuffer(&desc, nullptr, &m_pimpl->g_vertexBuffer));
        if (ok) m_pimpl->g_vertexBufferCapacity = maxVertices;
        return ok;
    }

    void Win32Renderer::DrawVertices(const std::vector<Vertex>& vertices)
    {
        if (vertices.empty()) return;

        // Auto-grow si se necesita más capacidad
        if (vertices.size() > m_pimpl->g_vertexBufferCapacity) {
            if (m_pimpl->g_vertexBuffer) { m_pimpl->g_vertexBuffer->Release(); m_pimpl->g_vertexBuffer = nullptr; }
            CreateDynamicVertexBuffer(vertices.size() * 2);
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        m_pimpl->g_context->Map(m_pimpl->g_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
        m_pimpl->g_context->Unmap(m_pimpl->g_vertexBuffer, 0);

        UINT stride = sizeof(Vertex), offset = 0;
        m_pimpl->g_context->IASetVertexBuffers(0, 1, &m_pimpl->g_vertexBuffer, &stride, &offset);
        m_pimpl->g_context->Draw(static_cast<UINT>(vertices.size()), 0);
    }

    // =========================================================
    // Texture management
    // =========================================================
    void Win32Renderer::CreateOrUpdateTexture(int mediaId, const uint8_t* rgba, int texW, int texH)
    {
        if (!rgba || texW <= 0 || texH <= 0) return;

        auto& entry = m_pimpl->g_textureCache[mediaId];

        // Recrear si las dimensiones cambiaron (resolución de video)
        if (entry.width != texW || entry.height != texH) {
            entry.release();

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = static_cast<UINT>(texW);
            desc.Height = static_cast<UINT>(texH);
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            if (FAILED(m_pimpl->g_device->CreateTexture2D(&desc, nullptr, &entry.tex))) return;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            if (FAILED(m_pimpl->g_device->CreateShaderResourceView(entry.tex, &srvDesc, &entry.srv))) {
                entry.release();
                return;
            }

            entry.width = texW;
            entry.height = texH;
        }

        // Subir datos del frame actual — row-by-row por el RowPitch variable del driver
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(m_pimpl->g_context->Map(entry.tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            auto* dst = static_cast<uint8_t*>(mapped.pData);
            const int srcStride = texW * 4;
            for (int row = 0; row < texH; ++row)
                std::memcpy(dst + row * mapped.RowPitch, rgba + row * srcStride, srcStride);
            m_pimpl->g_context->Unmap(entry.tex, 0);
        }
    }

    // =========================================================
    // Text rendering via DirectWrite + D2D sobre DXGI surface
    // =========================================================
    void Win32Renderer::RenderText(const std::vector<omc::ui::TextCmd>& texts, int screenW, int screenH)
    {
        if (!m_pimpl->g_d2dFactory || !m_pimpl->g_defaultFont) return;

        // Liberar la RT de D3D11 antes de que D2D use la misma superficie
        m_pimpl->g_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_pimpl->g_context->Flush();

        IDXGISurface* dxgiSurface = nullptr;
        if (FAILED(m_pimpl->g_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface)))) {
            m_pimpl->g_context->OMSetRenderTargets(1, &m_pimpl->g_rtv, nullptr);
            return;
        }

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        ID2D1RenderTarget* d2dRT = nullptr;
        HRESULT hr = m_pimpl->g_d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface, &rtProps, &d2dRT);
        dxgiSurface->Release();

        if (FAILED(hr)) {
            m_pimpl->g_context->OMSetRenderTargets(1, &m_pimpl->g_rtv, nullptr);
            return;
        }

        d2dRT->BeginDraw();

        // Ordenar por zIndex para respetar el orden de capas
        auto sorted = texts;
        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            return a.zIndex < b.zIndex;
            });

        for (const auto& text : sorted) {
            if (text.text.empty()) continue;

            std::wstring wtext(text.text.begin(), text.text.end());

            ID2D1SolidColorBrush* brush = nullptr;
            d2dRT->CreateSolidColorBrush(
                D2D1::ColorF(
                    text.color.r / 255.0f,
                    text.color.g / 255.0f,
                    text.color.b / 255.0f,
                    text.color.a / 255.0f
                ),
                &brush
            );

            if (brush) {
                D2D1_RECT_F rect = D2D1::RectF(
                    text.position.x,
                    text.position.y,
                    static_cast<float>(screenW),
                    static_cast<float>(screenH)
                );
                d2dRT->DrawText(wtext.c_str(), static_cast<UINT32>(wtext.size()),
                    m_pimpl->g_defaultFont, rect, brush);
                brush->Release();
            }
        }

        d2dRT->EndDraw();
        d2dRT->Release();

        // Rebind D3D11 RT para próximos frames
        m_pimpl->g_context->OMSetRenderTargets(1, &m_pimpl->g_rtv, nullptr);
    }

    // =========================================================
    // init
    // =========================================================
    bool Win32Renderer::init()
    {
        const int width = GetSystemMetrics(SM_CXSCREEN);
        const int height = GetSystemMetrics(SM_CYSCREEN);

        if (!CreateOverlayWindow(width, height)) { MessageBoxA(nullptr, "CreateOverlayWindow failed", "Error", MB_OK); return false; }
        if (!InitD3D(width, height)) { MessageBoxA(nullptr, "InitD3D failed", "Error", MB_OK); return false; }
        if (!InitPipeline()) { MessageBoxA(nullptr, "InitPipeline failed", "Error", MB_OK); return false; }
        if (!InitBlendState()) { MessageBoxA(nullptr, "InitBlendState failed", "Error", MB_OK); return false; }
        if (!InitSamplerState()) { MessageBoxA(nullptr, "InitSamplerState failed", "Error", MB_OK); return false; }
        if (!InitDirectWrite()) { MessageBoxA(nullptr, "InitDirectWrite failed", "Error", MB_OK); return false; }
        if (!CreateDynamicVertexBuffer(10000)) { MessageBoxA(nullptr, "CreateVertexBuffer failed", "Error", MB_OK); return false; }

        // WebView instances are created on-demand in render() — nothing to do here.
        return true;
    }

    // =========================================================
    // update
    // =========================================================
    void Win32Renderer::update()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_pimpl->g_hwnd, &pt);

        for (auto& [id, wv] : m_pimpl->webViews)
            if (wv) wv->Update();

        UpdateClickThrough(m_pimpl->g_hwnd,
            m_pimpl->g_hitTest.isInteractive(pt.x, pt.y));
    }

    // =========================================================
    // render
    // =========================================================
    void Win32Renderer::render(const std::vector<omc::ui::DrawCommand>& drawCommands)
    {
        const int sw = GetSystemMetrics(SM_CXSCREEN);
        const int sh = GetSystemMetrics(SM_CYSCREEN);

        ResizeSwapChainIfNeeded(*m_pimpl, sw, sh);

        // --- 1. Clear ---
        float clearColor[4] = { 0, 0, 0, 0 };
        m_pimpl->g_context->OMSetRenderTargets(1, &m_pimpl->g_rtv, nullptr);
        m_pimpl->g_context->ClearRenderTargetView(m_pimpl->g_rtv, clearColor);

        // --- 2. Classify commands ---
        std::vector<omc::ui::RectCmd>    rects;
        std::vector<omc::ui::ImageCmd>   images;
        std::vector<omc::ui::TextCmd>    texts;
        std::vector<omc::ui::WebViewCmd> webViews;

        for (const auto& cmd : drawCommands) {
            if (auto* r = std::get_if<omc::ui::RectCmd>(&cmd)) rects.push_back(*r);
            else if (auto* i = std::get_if<omc::ui::ImageCmd>(&cmd)) images.push_back(*i);
            else if (auto* t = std::get_if<omc::ui::TextCmd>(&cmd)) texts.push_back(*t);
            else if (auto* w = std::get_if<omc::ui::WebViewCmd>(&cmd)) webViews.push_back(*w);
        }

        auto byZ = [](const auto& a, const auto& b) { return a.zIndex < b.zIndex; };
        std::sort(rects.begin(), rects.end(), byZ);
        std::sort(images.begin(), images.end(), byZ);
        std::sort(webViews.begin(), webViews.end(), byZ);

        // 3. Estado del pipeline común
        const float blendFactor[4] = {};
        m_pimpl->g_context->OMSetBlendState(m_pimpl->g_blendState, blendFactor, 0xFFFFFFFF);
        m_pimpl->g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pimpl->g_context->IASetInputLayout(m_pimpl->g_inputLayout);
        m_pimpl->g_context->VSSetShader(m_pimpl->g_vs, nullptr, 0);
        m_pimpl->g_context->PSSetShader(m_pimpl->g_ps, nullptr, 0);
        m_pimpl->g_context->PSSetSamplers(0, 1, &m_pimpl->g_sampler);

        // 4. Batch de rects sólidos (un solo draw call)
        if (!rects.empty()) {
            std::vector<Vertex> verts;
            verts.reserve(rects.size() * 6);
            for (const auto& r : rects) AddColorRect(verts, r, sw, sh);
            DrawVertices(verts);
        }

        // 5. Imágenes — un draw call por textura (bind → draw → unbind)
        for (const auto& img : images) {
            if (!img.data || img.data->empty()) continue;

            // frameWidth/frameHeight = dimensiones reales de los píxeles del frame
            CreateOrUpdateTexture(img.imageId, img.data->data(), img.frameWidth, img.frameHeight);

            auto it = m_pimpl->g_textureCache.find(img.imageId);
            if (it == m_pimpl->g_textureCache.end() || !it->second.srv) continue;

            m_pimpl->g_context->PSSetShaderResources(0, 1, &it->second.srv);

            std::vector<Vertex> verts;
            AddTexturedRect(verts, img, sw, sh);
            DrawVertices(verts);

            ID3D11ShaderResourceView* nullSRV = nullptr;
            m_pimpl->g_context->PSSetShaderResources(0, 1, &nullSRV);
        }

        // 7. Texto via DirectWrite (última pasada antes del Present)
        if (!texts.empty()) {
            RenderText(texts, sw, sh);
        }

        std::unordered_set<int> activeIds;

        for (const auto& wvCmd : webViews) {
            activeIds.insert(wvCmd.windowId);

            auto& wv = m_pimpl->webViews[wvCmd.windowId];

            // First time we see this ID: create the renderer
            if (!wv) {
                wv = std::make_unique<WebViewRenderer>();

                const std::string url = wvCmd.url;
                WebViewRenderer* rawWv = wv.get();

                WebViewRenderer::InitParams p{};
                p.parentHwnd = m_pimpl->g_hwnd;
                p.dcompDevice = m_pimpl->g_dcompDevice;
                p.rootVisual = m_pimpl->g_rootVisual;
                p.initialBounds = RECT{
                    static_cast<LONG>(wvCmd.rect.position.x),
                    static_cast<LONG>(wvCmd.rect.position.y),
                    static_cast<LONG>(wvCmd.rect.position.x + wvCmd.rect.size.x),
                    static_cast<LONG>(wvCmd.rect.position.y + wvCmd.rect.size.y)
                };
                p.onReady = [rawWv, url](HRESULT hr) {
                    if (SUCCEEDED(hr))
                        rawWv->Navigate(std::wstring(url.begin(), url.end()));
                    };

                m_pimpl->webViewUrls[wvCmd.windowId] = url;
                wv->Initialize(p);   // ← una sola vez, con el callback correcto
            }
            else {
                // Resize every frame to track window drag / resize
                RECT bounds{
                    static_cast<LONG>(wvCmd.rect.position.x),
                    static_cast<LONG>(wvCmd.rect.position.y),
                    static_cast<LONG>(wvCmd.rect.position.x + wvCmd.rect.size.x),
                    static_cast<LONG>(wvCmd.rect.position.y + wvCmd.rect.size.y)
                };
                wv->Resize(bounds);

                // Re-navigate only when the URL actually changes
                auto& cachedUrl = m_pimpl->webViewUrls[wvCmd.windowId];
                if (wv->IsReady() && wvCmd.url != cachedUrl) {
                    cachedUrl = wvCmd.url;
                    wv->Navigate(std::wstring(wvCmd.url.begin(), wvCmd.url.end()));
                }
            }
        }

        // Destroy renderers for windows that are no longer in the command list
        for (auto it = m_pimpl->webViews.begin(); it != m_pimpl->webViews.end(); ) {
            if (activeIds.find(it->first) == activeIds.end()) {
                if (it->second) it->second->Shutdown();
                m_pimpl->webViewUrls.erase(it->first);
                it = m_pimpl->webViews.erase(it);
            }
            else {
                ++it;
            }
        }

        // 8. Hit testing
        std::vector<HitRegion> regions;
        for (const auto& cmd : drawCommands) {
            if (auto r = std::get_if<omc::ui::RectCmd>(&cmd)) {
                regions.push_back({
                    (int)r->rect.position.x, (int)r->rect.position.y,
                    (int)r->rect.size.x,     (int)r->rect.size.y,
                    r->color.a > 10
                    });
            }
        }
        m_pimpl->g_hitTest.setRegions(std::move(regions));

        // 9. Present
        m_pimpl->g_swapChain->Present(1, 0);
    }

    void Win32Renderer::onExit(const omc::event::ExitApplicationRequestedEvent&)
    {
        for (auto& [id, wv] : m_pimpl->webViews)
            if (wv) { wv->Shutdown(); }
        m_pimpl->webViews.clear();
        m_pimpl->webViewUrls.clear();

        CleanupD3D();
        if (m_pimpl->g_hwnd) {
            DestroyWindow(m_pimpl->g_hwnd);
            m_pimpl->g_hwnd = nullptr;
        }
    }

} // namespace omc::infra
