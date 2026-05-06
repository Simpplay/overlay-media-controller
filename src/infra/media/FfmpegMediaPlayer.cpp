#include "FfmpegMediaPlayer.hpp"

#include <stdio.h>

namespace omc::infra {

    FfmpegMediaPlayer::FfmpegMediaPlayer() {
        av_log_set_level(AV_LOG_INFO);
    }

    FfmpegMediaPlayer::~FfmpegMediaPlayer() = default;

    bool FfmpegMediaPlayer::load(const std::string& path) {
        fmt_ctx.reset();

        auto pathStr = path.c_str();

        AVFormatContext* rawCtx = nullptr;

        if (avformat_open_input(&rawCtx, pathStr, nullptr, nullptr) < 0) {
            printf("Error: Could not open file: %s\n", pathStr);
            return false;
        }

        if (avformat_find_stream_info(rawCtx, nullptr) < 0) {
            printf("Error: Could not find stream information for: %s\n", pathStr);
            avformat_close_input(&rawCtx);
            return false;
        }

        // Transfer ownership al unique_ptr
        fmt_ctx.reset(rawCtx);

        return true;
    }

    void FfmpegMediaPlayer::play() {
        if (!fmt_ctx) {
            printf("Error: Load a file first!");
            return;
        }

        // Acceso con get()
        AVFormatContext* ctx = fmt_ctx.get();

        // TODO
    }

    void FfmpegMediaPlayer::pause() {
        // TODO
    }

    void FfmpegMediaPlayer::stop() {
        // Al resetear, se libera automáticamente vía deleter
        fmt_ctx.reset();
    }

}