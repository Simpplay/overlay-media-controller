#pragma once

#include <memory>

#include "shared/threading/ConcurrentQueue.hpp"

#include "modules/ui/UiWindow.hpp"
#include "core/event/EventBus.hpp"
#include "core/event/events/PlayMediaRequestedEvent.hpp"
#include "core/event/events/FrameReadyEvent.hpp"
#include "core/types/VideoFrame.hpp"

namespace omc::ui::window {

    class MediaWindow : public UiWindow {
    public:
        MediaWindow(int mediaId, omc::event::EventBus& eventBus, bool requestPlayback = true)
            : mediaId(mediaId), eventBus(eventBus)
        {
            position = { 0, 0 };
            size = { 800, 600 };
            backgroundColor = { 0, 0, 0, 255 };

            eventBus.subscribe<omc::event::FrameReadyEvent>([this](const omc::event::FrameReadyEvent& e) {
                onFrameReady(e);
            });

            if (requestPlayback) {
                omc::event::PlayMediaRequestedEvent event{ mediaId };
                eventBus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(event));
            }
        }

        std::unique_ptr<UiWindow> clone() const override {
            return std::make_unique<MediaWindow>(mediaId, eventBus, false);
        }

        void buildClientDrawCommand(std::vector<DrawCommand>& out) override {
            if (currentFrame.has_value()) {
                out.push_back(ImageCmd{ position, size, currentFrame->data, mediaId, zBase + 1 });
            }
            else {
                out.push_back(TextCmd{ { position.x + 10.0f, position.y + 8.0f }, { 255, 255, 255, 255 }, "Loading...", zBase + 3 });
            }
        }

        void update() override {
            std::optional<omc::media::VideoFrame> frame;
            if (frameBuffer.try_pop(frame)) {
                currentFrame = std::move(frame);
            }
        }

        void onFrameReady(const omc::event::FrameReadyEvent& e) {
            if (e.mediaId != mediaId) return;
            frameBuffer.push(e.frame);
        }

    private:
        int mediaId;
        omc::event::EventBus& eventBus;
        std::optional<omc::media::VideoFrame> currentFrame;
        omc::shared::ConcurrentQueue<omc::media::VideoFrame> frameBuffer;
    };

}
