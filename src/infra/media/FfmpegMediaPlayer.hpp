#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
}

#include "modules/media/MediaPlayer.hpp"

namespace omc::infra {

    class FfmpegMediaPlayer : public omc::media::MediaPlayer {
    public:
        FfmpegMediaPlayer();
        ~FfmpegMediaPlayer() noexcept override;

        FfmpegMediaPlayer(const FfmpegMediaPlayer&) = delete;
        FfmpegMediaPlayer& operator=(const FfmpegMediaPlayer&) = delete;

        bool load(const std::string& path) override;
        void play() override;
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
    };

}