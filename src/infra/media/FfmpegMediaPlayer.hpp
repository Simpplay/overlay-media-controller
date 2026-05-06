#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "core/types/VideoFrame.hpp"
#include "core/event/EventBus.hpp"
#include "shared/threading/ConcurrentQueue.hpp"
#include "modules/media/MediaPlayer.hpp"

namespace omc::infra {
    class FfmpegMediaPlayer : public std::enable_shared_from_this<FfmpegMediaPlayer>, public omc::media::MediaPlayer {
    public:
        ~FfmpegMediaPlayer() noexcept override;

        FfmpegMediaPlayer(omc::event::EventBus& eventBus, omc::shared::ThreadPool* threadPool);

        FfmpegMediaPlayer(const FfmpegMediaPlayer&) = delete;
        FfmpegMediaPlayer& operator=(const FfmpegMediaPlayer&) = delete;

        bool load(const std::string& path) override;
        void play(int mediaId) override;
        void pause() override;
        void stop() override;

    private:
        struct AVFormatContextDeleter {
            void operator()(AVFormatContext* ctx) const {
                if (ctx) {
                    avformat_close_input(&ctx);
                }
            }
        };

        std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fmt_ctx;

		omc::event::EventBus& eventBus;

        AVCodecContext* codec_ctx = nullptr;
        int video_stream_index = -1;

        SwsContext* sws_ctx = nullptr;

        std::thread decode_thread;
        std::atomic<bool> running{ false };

        void decodeLoop();
    };

}