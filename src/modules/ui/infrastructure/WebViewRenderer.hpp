#pragma once

#if defined(_WIN32)

#include <Windows.h>
#include <dcomp.h>
#include <wrl.h>
#include <WebView2.h>
#include <windowsx.h>

#include <functional>
#include <string>
#include <memory>

namespace omc::infra
{
    class WebViewRenderer : public std::enable_shared_from_this<WebViewRenderer>
    {
    public:
        struct InitParams {
            HWND parentHwnd = nullptr;
            RECT initialBounds{};
            Microsoft::WRL::ComPtr<IDCompositionDevice> dcompDevice;
            Microsoft::WRL::ComPtr<IDCompositionVisual> rootVisual;
            std::wstring userDataFolder;
            std::function<void(HRESULT)> onReady;
        };

        WebViewRenderer();
        ~WebViewRenderer();

        bool Initialize(const InitParams& params);
        void Update();
        void Resize(const RECT& boundsPx);
        void Navigate(const std::wstring& url);

        bool HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        bool HandleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        void Shutdown();
        bool IsReady() const noexcept;

    private:
        bool EnsureTransparentBackground();
        bool UpdateVisualTreeAttachment();
        POINT ClientToWebViewPoint(LPARAM lParam) const;
        void MoveFocusToWebView();
        void RemoveVisualFromTree();

        HWND m_parentHwnd = nullptr;
        RECT m_bounds{};
        bool m_initialized = false;
        bool m_shuttingDown = false;
        bool m_visualAttached = false;

        Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcompDevice;
        Microsoft::WRL::ComPtr<IDCompositionVisual> m_rootVisual;
        Microsoft::WRL::ComPtr<IDCompositionVisual> m_webViewVisual;

        Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_environment;
        Microsoft::WRL::ComPtr<ICoreWebView2CompositionController> m_compositionController;
        Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
        Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
    };
}

#endif
