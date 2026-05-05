#include "Win32Renderer.hpp"

namespace omc::infra
{
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };

    Win32Renderer::Win32Renderer(omc::event::EventBus& eventBus)
        : eventBus(eventBus), m_pimpl(std::make_unique<Impl>())
    {
        eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const auto& e) {
            onExit(e);
            });
	}

    Win32Renderer::~Win32Renderer() = default;

    // =========================================================
    // Globals
    // =========================================================

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

        IDCompositionDevice* g_dcompDevice = nullptr;
        IDCompositionTarget* g_dcompTarget = nullptr;
        IDCompositionVisual* g_dcompVisual = nullptr;

        HitTestManager g_hitTest;
	};

    const char* g_vsCode = R"(
        struct VS_INPUT {
            float2 pos : POSITION;
            float4 col : COLOR;
        };

        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
        };

        PS_INPUT main(VS_INPUT input)
        {
            PS_INPUT o;
            o.pos = float4(input.pos, 0.0f, 1.0f);
            o.col = input.col;
            return o;
        }
        )";

    const char* g_psCode = R"(
        float4 main(float4 pos : SV_POSITION, float4 col : COLOR) : SV_TARGET
        {
            return col;
        }
        )";

    // =========================================================
    // Hit Test (base)
    // =========================================================

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_NCHITTEST:
        {
            return HTCLIENT;
        }
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // =========================================================
    // DX11 Setup
    // =========================================================

    bool Win32Renderer::InitPipeline()
    {
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        HRESULT hr = D3DCompile(g_vsCode, strlen(g_vsCode), nullptr, nullptr, nullptr,
            "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);

        if (FAILED(hr)) return false;

        hr = D3DCompile(g_psCode, strlen(g_psCode), nullptr, nullptr, nullptr,
            "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);

        if (FAILED(hr)) return false;

        if (FAILED(m_pimpl->g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pimpl->g_vs))) return false;
        if (FAILED(m_pimpl->g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pimpl->g_ps))) return false;
        

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        if (FAILED(m_pimpl->g_device->CreateInputLayout(
            layout, 2,
            vsBlob->GetBufferPointer(),
            vsBlob->GetBufferSize(),
            &m_pimpl->g_inputLayout
        ))) return false;

        vsBlob->Release();
        psBlob->Release();

        return true;
    }

    bool Win32Renderer::InitD3D(int width, int height)
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &m_pimpl->g_device,
            nullptr,
            &m_pimpl->g_context
        );

        if (FAILED(hr)) {
            MessageBoxA(nullptr, "D3D11CreateDevice failed", "Error", MB_OK);
            return false;
        }

        // =========================================================
        // DXGI Factory
        // =========================================================

        IDXGIDevice* dxgiDevice = nullptr;
        m_pimpl->g_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));

        IDXGIAdapter* adapter = nullptr;
        dxgiDevice->GetAdapter(&adapter);

        IDXGIFactory2* factory = nullptr;
        adapter->GetParent(IID_PPV_ARGS(&factory));

        // =========================================================
        // SwapChain (alpha real)
        // =========================================================

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

        hr = factory->CreateSwapChainForComposition(
            m_pimpl->g_device,
            &scd,
            nullptr,
            &swapChain1
        );

        if (FAILED(hr)) {
            MessageBoxA(nullptr, "CreateSwapChainForComposition failed", "Error", MB_OK);
            return false;
        }

        swapChain1->QueryInterface(IID_PPV_ARGS(&m_pimpl->g_swapChain));

        // =========================================================
        // Render Target
        // =========================================================

        ID3D11Texture2D* backBuffer = nullptr;
        m_pimpl->g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

        if (FAILED(hr)) {
            MessageBoxA(nullptr, "GetBuffer failed", "Error", MB_OK);
            return false;
		}

        if (!backBuffer) {
            MessageBoxA(nullptr, "GetBuffer returned null", "Error", MB_OK);
            return false;
        }

        hr = m_pimpl->g_device->CreateRenderTargetView(backBuffer, nullptr, &m_pimpl->g_rtv);
        backBuffer->Release();

        if (FAILED(hr)) {
            MessageBoxA(nullptr, "CreateRenderTargetView failed", "Error", MB_OK);
            return false;
        }

        // =========================================================
        // Viewport
        // =========================================================

        D3D11_VIEWPORT vp{};
        vp.Width = (FLOAT)width;
        vp.Height = (FLOAT)height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;

        m_pimpl->g_context->RSSetViewports(1, &vp);

        // =========================================================
        // DirectComposition
        // =========================================================

        hr = DCompositionCreateDevice(
            dxgiDevice,
            __uuidof(IDCompositionDevice),
            (void**)&m_pimpl->g_dcompDevice
        );

        if (FAILED(hr)) {
            MessageBoxA(nullptr, "DCompositionCreateDevice failed", "Error", MB_OK);
            return false;
        }

        m_pimpl->g_dcompDevice->CreateTargetForHwnd(m_pimpl->g_hwnd, TRUE, &m_pimpl->g_dcompTarget);
        m_pimpl->g_dcompDevice->CreateVisual(&m_pimpl->g_dcompVisual);

        m_pimpl->g_dcompVisual->SetContent(m_pimpl->g_swapChain);
        m_pimpl->g_dcompTarget->SetRoot(m_pimpl->g_dcompVisual);
        m_pimpl->g_dcompDevice->Commit();

        // =========================================================
        // Cleanup temporales
        // =========================================================

        dxgiDevice->Release();
        adapter->Release();
        factory->Release();
        swapChain1->Release();

        return true;
    }

    void Win32Renderer::CleanupD3D()
    {
        if (m_pimpl->g_rtv) m_pimpl->g_rtv->Release();
        if (m_pimpl->g_swapChain) m_pimpl->g_swapChain->Release();
        if (m_pimpl->g_context) m_pimpl->g_context->Release();
        if (m_pimpl->g_device) m_pimpl->g_device->Release();
    }

    // =========================================================
    // Window Setup
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
            wc.lpszClassName,
            "Overlay Media Controller",
            WS_POPUP,
            0, 0, width, height,
            nullptr, nullptr, wc.hInstance, nullptr
        );

        if (!m_pimpl->g_hwnd) return false;

        ShowWindow(m_pimpl->g_hwnd, SW_SHOW);

        return true;
    }

    void Win32Renderer::UpdateClickThrough(HWND hwnd, bool interactive)
    {
        // Caché de estado: previene llamadas redundantes a la API de Windows
        static bool s_isInteractive = true;
        if (s_isInteractive == interactive) return;

        s_isInteractive = interactive;

        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        if (interactive) {
            printf("Setting window to interactive mode\n");
            exStyle &= ~WS_EX_TRANSPARENT; // Captura el input
        }
        else {
            printf("Setting window to click-through mode\n");
            exStyle |= WS_EX_TRANSPARENT;  // Deja pasar los clicks
        }

        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    bool Win32Renderer::CreateDynamicVertexBuffer(size_t maxVertices)
    {
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * maxVertices);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        return SUCCEEDED(m_pimpl->g_device->CreateBuffer(&desc, nullptr, &m_pimpl->g_vertexBuffer));
    }

    void AddRect(std::vector<Vertex>& vertices, const omc::ui::RectCmd& cmd, int screenW, int screenH)
    {
        float x = cmd.rect.position.x;
        float y = cmd.rect.position.y;
        float w = cmd.rect.size.x;
        float h = cmd.rect.size.y;

        float r = cmd.color.r / 255.0f;
        float g = cmd.color.g / 255.0f;
        float b = cmd.color.b / 255.0f;
        float a = cmd.color.a / 255.0f;

        auto toNDC = [&](float px, float py) {
            return Vertex{
                (px / screenW) * 2.0f - 1.0f,
                1.0f - (py / screenH) * 2.0f,
                r, g, b, a
            };
            };

        Vertex v0 = toNDC(x, y);
        Vertex v1 = toNDC(x + w, y);
        Vertex v2 = toNDC(x, y + h);
        Vertex v3 = toNDC(x + w, y + h);

        vertices.insert(vertices.end(), { v0, v1, v2, v2, v1, v3 });
    }

    // =========================================================
    // Win32Renderer Implementation
    // =========================================================

    bool Win32Renderer::init()
    {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        if (!CreateOverlayWindow(width, height)) {
            MessageBoxA(nullptr, "CreateOverlayWindow failed", "Error", MB_OK);
            return false;
        }

        if (!InitD3D(width, height)) {
            MessageBoxA(nullptr, "InitD3D failed", "Error", MB_OK);
            return false;
        }

        if (!InitPipeline()) {
            MessageBoxA(nullptr, "InitPipeline failed", "Error", MB_OK);
            return false;
		}

        if (!CreateDynamicVertexBuffer(10000)) {
            MessageBoxA(nullptr, "CreateDynamicVertexBuffer failed", "Error", MB_OK);
			return false;
        }

        return true;
    }

    void Win32Renderer::update()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_pimpl->g_hwnd, &pt);

        bool interactive = m_pimpl->g_hitTest.isInteractive(pt.x, pt.y);

        UpdateClickThrough(m_pimpl->g_hwnd, interactive);
    }

    void Win32Renderer::render(const std::vector<omc::ui::DrawCommand>& drawCommands)
    {
        float clearColor[4] = { 0, 0, 0, 0 };

        m_pimpl->g_context->OMSetRenderTargets(1, &m_pimpl->g_rtv, nullptr);
        m_pimpl->g_context->ClearRenderTargetView(m_pimpl->g_rtv, clearColor);

        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        std::vector<Vertex> vertices;

        // ordenar por zIndex
        std::vector<omc::ui::RectCmd> rects;

        for (const auto& draw : drawCommands) {
            if (auto r = std::get_if<omc::ui::RectCmd>(&draw)) {
                rects.push_back(*r);
            }
        }

        std::sort(rects.begin(), rects.end(), [](auto& a, auto& b) {
            return a.zIndex < b.zIndex;
            });

        for (const auto& r : rects) {
            AddRect(vertices, r, width, height);
        }

        if (vertices.empty()) {
            m_pimpl->g_swapChain->Present(1, 0);
            return;
        }

        std::vector<HitRegion> regions;

        for (const auto& draw : drawCommands) {
            if (auto r = std::get_if<omc::ui::RectCmd>(&draw)) {
                regions.push_back({
                    (int)r->rect.position.x,
                    (int)r->rect.position.y,
                    (int)r->rect.size.x,
                    (int)r->rect.size.y,
                    r->color.a > 10 // o cualquier regla
                    });
            }
        }

        m_pimpl->g_hitTest.setRegions(std::move(regions));

        // subir datos al GPU
        D3D11_MAPPED_SUBRESOURCE mapped{};
        m_pimpl->g_context->Map(m_pimpl->g_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
        m_pimpl->g_context->Unmap(m_pimpl->g_vertexBuffer, 0);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        m_pimpl->g_context->IASetVertexBuffers(0, 1, &m_pimpl->g_vertexBuffer, &stride, &offset);
        m_pimpl->g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pimpl->g_context->IASetInputLayout(m_pimpl->g_inputLayout);

        m_pimpl->g_context->VSSetShader(m_pimpl->g_vs, nullptr, 0);
        m_pimpl->g_context->PSSetShader(m_pimpl->g_ps, nullptr, 0);

        m_pimpl->g_context->Draw(static_cast<UINT>(vertices.size()), 0);

        m_pimpl->g_swapChain->Present(1, 0);
    }

    void Win32Renderer::onExit(const omc::event::ExitApplicationRequestedEvent&)
    {
        CleanupD3D();

        if (m_pimpl->g_hwnd)
        {
            DestroyWindow(m_pimpl->g_hwnd);
            m_pimpl->g_hwnd = nullptr;
        }
    }

} // namespace omc::infra