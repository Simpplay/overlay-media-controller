#if defined(_WIN32)

#include "WebViewRenderer.hpp"

#include <WebView2EnvironmentOptions.h>

namespace omc::infra
{
    using Microsoft::WRL::ComPtr;

    static bool IsKeyMessage(UINT msg)
    {
        return msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_CHAR;
    }

    WebViewRenderer::WebViewRenderer() = default;
    WebViewRenderer::~WebViewRenderer() { Shutdown(); }

    bool WebViewRenderer::Initialize(const InitParams& params)
    {
        if (m_initialized) return true;
        if (!params.parentHwnd || !params.dcompDevice || !params.rootVisual) return false;

        m_parentHwnd = params.parentHwnd;
        m_bounds = params.initialBounds;
        m_dcompDevice = params.dcompDevice;
        m_rootVisual = params.rootVisual;

        HRESULT hr = m_dcompDevice->CreateVisual(&m_webViewVisual);
        if (FAILED(hr)) return false;

        auto weakThis = weak_from_this();

        hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            params.userDataFolder.empty() ? nullptr : params.userDataFolder.c_str(),
            nullptr,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [weakThis, onReady = params.onReady](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    auto self = weakThis.lock();
                    if (!self || self->m_shuttingDown) return E_ABORT;

                    if (FAILED(result) || !env) {
                        if (onReady) onReady(result);
                        return result;
                    }

                    self->m_environment = env;
                    ComPtr<ICoreWebView2Environment3> env3;
                    HRESULT castHr = self->m_environment.As(&env3);

                    if (FAILED(castHr) || !env3) {
                        if (onReady) onReady(castHr);
                        return castHr;
                    }

                    return env3->CreateCoreWebView2CompositionController(
                        self->m_parentHwnd,
                        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
                            [weakThis, onReady](HRESULT controllerResult, ICoreWebView2CompositionController* compositionController) -> HRESULT {
                                auto self2 = weakThis.lock();
                                if (!self2 || self2->m_shuttingDown) return E_ABORT;

                                if (FAILED(controllerResult) || !compositionController) {
                                    if (onReady) onReady(controllerResult);
                                    return controllerResult;
                                }

                                self2->m_compositionController = compositionController;
                                self2->m_compositionController.As(&self2->m_controller);

                                HRESULT webViewHr = self2->m_controller->get_CoreWebView2(&self2->m_webView);
                                if (FAILED(webViewHr)) {
                                    if (onReady) onReady(webViewHr);
                                    return webViewHr;
                                }

                                webViewHr = self2->m_compositionController->put_RootVisualTarget(self2->m_webViewVisual.Get());
                                if (FAILED(webViewHr)) {
                                    if (onReady) onReady(webViewHr);
                                    return webViewHr;
                                }

                                self2->EnsureTransparentBackground();
                                self2->Resize(self2->m_bounds);
                                self2->UpdateVisualTreeAttachment();
                                self2->m_controller->put_IsVisible(TRUE);
                                self2->m_initialized = true;

                                if (onReady) onReady(S_OK);
                                return S_OK;
                            }).Get());
                }).Get());

        return SUCCEEDED(hr);
    }

    bool WebViewRenderer::EnsureTransparentBackground()
    {
        if (!m_controller || !m_webView) return false;

        COREWEBVIEW2_COLOR transparent{ 0, 0, 0, 0 };
        ComPtr<ICoreWebView2Controller2> controller2;
        HRESULT hr = m_controller.As(&controller2);

        if (SUCCEEDED(hr) && controller2) {
            controller2->put_DefaultBackgroundColor(transparent);
        }

        const wchar_t* cssInjection = LR"JS(
            (() => {
                const style = document.createElement('style');
                style.textContent = 'html,body{background:transparent !important;}';
                document.documentElement.appendChild(style);
            })();
        )JS";

        EventRegistrationToken token{};
        return SUCCEEDED(m_webView->AddScriptToExecuteOnDocumentCreated(cssInjection, nullptr));
    }

    bool WebViewRenderer::UpdateVisualTreeAttachment()
    {
        if (m_visualAttached || !m_rootVisual || !m_webViewVisual) return m_visualAttached;
        const HRESULT hr = m_rootVisual->AddVisual(m_webViewVisual.Get(), FALSE, nullptr);
        if (FAILED(hr)) return false;
        m_visualAttached = SUCCEEDED(m_dcompDevice->Commit());
        return m_visualAttached;
    }

    void WebViewRenderer::Resize(const RECT& boundsPx)
    {
        m_bounds = boundsPx;
        if (!m_compositionController || !m_controller) return;
        if (m_webViewVisual) {
            m_webViewVisual->SetOffsetX(static_cast<float>(m_bounds.left));
            m_webViewVisual->SetOffsetY(static_cast<float>(m_bounds.top));
        }

        m_controller->put_Bounds(m_bounds);
        m_controller->NotifyParentWindowPositionChanged();
        m_controller->put_IsVisible(TRUE);

        if (m_dcompDevice) {
            m_dcompDevice->Commit();
        }
    }

    void WebViewRenderer::Navigate(const std::wstring& url)
    {
        if (!m_webView || url.empty()) return;
        m_webView->Navigate(url.c_str());
    }

    POINT WebViewRenderer::ClientToWebViewPoint(LPARAM lParam) const
    {
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        p.x -= m_bounds.left;
        p.y -= m_bounds.top;
        return p;
    }

    bool WebViewRenderer::HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!m_compositionController) return false;
        const POINT point = ClientToWebViewPoint(lParam);
        if (point.x < 0 || point.y < 0 || point.x >= (m_bounds.right - m_bounds.left) || point.y >= (m_bounds.bottom - m_bounds.top)) return false;

        COREWEBVIEW2_MOUSE_EVENT_KIND kind{};
        switch (msg) {
        case WM_MOUSEMOVE: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE; break;
        case WM_LBUTTONDOWN: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOWN; MoveFocusToWebView(); break;
        case WM_LBUTTONUP: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP; break;
        case WM_RBUTTONDOWN: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOWN; break;
        case WM_RBUTTONUP: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP; break;
        case WM_MOUSEWHEEL: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_WHEEL; break;
        case WM_MOUSEHWHEEL: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_HORIZONTAL_WHEEL; break;
        default: return false;
        }

        UINT32 data = (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) ? static_cast<UINT32>(GET_WHEEL_DELTA_WPARAM(wParam)) : 0;
        ComPtr<ICoreWebView2CompositionController> compositionController;
        HRESULT hr = m_compositionController.As(&compositionController);

        if (FAILED(hr) || !compositionController)
            return false;

        return SUCCEEDED(
            compositionController->SendMouseInput(
                kind,
                static_cast<COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS>(0),
                data,
                point));
    }

    void WebViewRenderer::MoveFocusToWebView()
    {
        if (m_controller) {
            m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
    }

    bool WebViewRenderer::HandleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM)
    {
        if (!m_controller || !IsKeyMessage(msg)) return false;
        MoveFocusToWebView();
        return true;
    }

    void WebViewRenderer::Update()
    {
        // Placeholder for future composition-frame synchronization hooks.
    }

    bool WebViewRenderer::IsReady() const noexcept { return m_initialized && m_webView; }

    void WebViewRenderer::RemoveVisualFromTree()
    {
        if (!m_visualAttached || !m_rootVisual || !m_webViewVisual) return;
        m_rootVisual->RemoveVisual(m_webViewVisual.Get());
        m_dcompDevice->Commit();
        m_visualAttached = false;
    }

    void WebViewRenderer::Shutdown()
    {
        if (m_shuttingDown) return;
        m_shuttingDown = true;

        if (m_controller) {
            m_controller->put_IsVisible(FALSE);
        }

        if (m_webView) {
            m_webView->Navigate(L"about:blank");
        }

        RemoveVisualFromTree();

        if (m_compositionController) {
            m_compositionController->put_RootVisualTarget(nullptr);
        }
        if (m_controller) {
            m_controller->Close();
        }

        m_webView.Reset();
        m_controller.Reset();
        m_compositionController.Reset();
        m_environment.Reset();
        m_webViewVisual.Reset();
        m_rootVisual.Reset();
        m_dcompDevice.Reset();

        m_initialized = false;
    }
}

#endif
