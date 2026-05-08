#pragma once

#if defined(_WIN32)
#include <memory>
#include <vector>

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <d2d1.h>
#include <dwrite.h>

#include "modules/ui/domain/RenderTypes.hpp"
#include "core/event/api/IEventBus.hpp"
#include "core/event/events/ExitApplicationRequestedEvent.hpp"
#include "HitTestManager.hpp"

namespace omc::infra
{
    // Expuesto en el header para que DrawVertices pueda usarlo como param
    struct Vertex {
        float    x, y;
        float    r, g, b, a;
        float    u, v;
        uint32_t mode; // 0 = color sólido, 1 = textura
    };

    class Win32Renderer
    {
    public:
        explicit Win32Renderer(omc::event::EventBus& eventBus);
        ~Win32Renderer();

        bool init();
        void update();
        void render(const std::vector<omc::ui::DrawCommand>& drawCommands);

        struct Impl;

    private:
        std::unique_ptr<Impl> m_pimpl;
        omc::event::EventBus& eventBus;

        bool CreateOverlayWindow(int width, int height);
        bool InitD3D(int width, int height);
        bool InitPipeline();
        bool InitBlendState();
        bool InitSamplerState();
        bool InitDirectWrite();
        bool CreateDynamicVertexBuffer(size_t maxVertices);

        void CreateOrUpdateTexture(int mediaId, const uint8_t* rgba, int texW, int texH);
        void DrawVertices(const std::vector<Vertex>& vertices);
        void RenderText(const std::vector<omc::ui::TextCmd>& texts, int screenW, int screenH);

        void CleanupD3D();
        void UpdateClickThrough(HWND hwnd, bool interactive);
        void onExit(const omc::event::ExitApplicationRequestedEvent&);
    };
}
#endif
