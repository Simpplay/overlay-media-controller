#pragma once

#include "modules/ui/domain/UiWindow.hpp"
#include <vector>
#include <string>
#include <atomic>

namespace omc::ui::window
{
	class WebViewWindow : public UiWindow
    {
    public:
        explicit WebViewWindow(std::string url,
            Vec2 pos = { 200.0f, 150.0f },
            Vec2 sz = { 960.0f, 640.0f })
            : m_url(std::move(url))
            , m_id(s_nextId++)
        {
            position = pos;
            size = sz;
            backgroundColor = { 18, 18, 18, 240 };
        }

        // ---- UiWindow interface --------------------------------

        std::unique_ptr<UiWindow> clone() const override
        {
            return std::make_unique<WebViewWindow>(*this);
        }

        void update() override { /* interaction handled by base */ }

        void buildClientDrawCommand(std::vector<DrawCommand>& out) override
        {
            if (isClosed) return;

            const float contentY = position.y + kTitleBarHeight;
            const float contentH = size.y - kTitleBarHeight;
            if (contentH <= 0.0f) return;

            out.push_back(omc::ui::WebViewCmd{
                m_id,
                m_url,
                omc::ui::Rect({ position.x, contentY }, { size.x, contentH }),
                zBase + 1
                });
        }

        // ---- Accessors -----------------------------------------

        int         getMediaId()  const noexcept { return m_id; }
        const std::string& getUrl() const noexcept { return m_url; }

        void navigate(std::string url) { m_url = std::move(url); }

    private:
        std::string m_url;
        int         m_id;

        // Each WebViewWindow gets a unique ID for the lifetime of
        // the process; IDs are never reused.
        inline static std::atomic<int> s_nextId{ 1 };
    };

} // namespace omc::ui::window