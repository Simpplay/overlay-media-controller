#include "FfmpegMediaPlayer.hpp"

#include <stdio.h>

#include "core/event/events/FrameReadyEvent.hpp"

namespace omc::infra {

    FfmpegMediaPlayer::FfmpegMediaPlayer(omc::event::EventBus& eventBus, omc::shared::ThreadPool* threadPool) : omc::media::MediaPlayer(threadPool), eventBus(eventBus) {
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

		printf("Loaded file: %s\n", pathStr);
		printf("Duration: %lld us\n", fmt_ctx->duration);
		printf("Number of streams: %d\n", fmt_ctx->nb_streams);
		printf("Format: %s\n", fmt_ctx->iformat->name);

        return true;
    }

    void FfmpegMediaPlayer::play(int mediaId) {
        if (!fmt_ctx) {
            printf("Error: Load a file first!\n");
            return;
        }

        this->mediaId = mediaId;

        AVFormatContext* ctx = fmt_ctx.get();

        // 1. Encontrar stream de video
        video_stream_index = -1;
        for (unsigned int i = 0; i < ctx->nb_streams; i++) {
            if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;
                break;
            }
        }

        if (video_stream_index == -1) {
            printf("Error: No video stream found\n");
            return;
        }

        AVStream* videoStream = ctx->streams[video_stream_index];

        // 2. Crear decoder
        const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (!codec) {
            printf("Error: Decoder not found\n");
            return;
        }

        codec_ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx, videoStream->codecpar);

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            printf("Error: Could not open codec\n");
            return;
        }

        // 3. sws
        sws_ctx = sws_getContext(
            codec_ctx->width,
            codec_ctx->height,
            codec_ctx->pix_fmt,
            codec_ctx->width,
            codec_ctx->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            nullptr, nullptr, nullptr
        );

        running = true;

        auto self = shared_from_this();

        threadPool->enqueue([self]() {
            self->decodeLoop();
            });
    }

    void FfmpegMediaPlayer::decodeLoop() {
        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVFrame* rgbFrame = av_frame_alloc();

        int width = codec_ctx->width;
        int height = codec_ctx->height;

        int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
        std::vector<uint8_t> buffer(bufferSize);

        av_image_fill_arrays(
            rgbFrame->data,
            rgbFrame->linesize,
            buffer.data(),
            AV_PIX_FMT_RGBA,
            width,
            height,
            1
        );

        AVFormatContext* ctx = fmt_ctx.get();

        while (running) {

            if (av_read_frame(ctx, packet) < 0)
                break;

            if (packet->stream_index == video_stream_index) {

                if (avcodec_send_packet(codec_ctx, packet) == 0) {

                    while (running && avcodec_receive_frame(codec_ctx, frame) == 0) {

                        sws_scale(
                            sws_ctx,
                            frame->data,
                            frame->linesize,
                            0,
                            height,
                            rgbFrame->data,
                            rgbFrame->linesize
                        );

                        omc::media::VideoFrame vf;
                        vf.width = width;
                        vf.height = height;
                        vf.data->assign(buffer.begin(), buffer.end());

                        auto event = std::make_unique<omc::event::FrameReadyEvent>(omc::event::FrameReadyEvent(
                            mediaId,
                            std::move(vf)
                        ));

                        eventBus.post(std::move(event));
                    }
                }
            }

            av_packet_unref(packet);
        }

        // Flush
        avcodec_send_packet(codec_ctx, nullptr);
        while (running && avcodec_receive_frame(codec_ctx, frame) == 0) {
            // opcional
        }

        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        av_packet_free(&packet);
    }

    void FfmpegMediaPlayer::pause() {
        running = false;
    }

    void FfmpegMediaPlayer::stop() {
        running = false;

        if (codec_ctx) {
            avcodec_free_context(&codec_ctx);
            codec_ctx = nullptr;
        }

        if (sws_ctx) {
            sws_freeContext(sws_ctx);
            sws_ctx = nullptr;
        }

        fmt_ctx.reset();
    }

}