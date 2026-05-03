#include "media_controller.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <cstdio>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// RAII helpers
// ---------------------------------------------------------------------------

namespace {

struct FormatCtxGuard {
    AVFormatContext* ctx{nullptr};
    ~FormatCtxGuard() { if (ctx) avformat_close_input(&ctx); }
};

struct CodecCtxGuard {
    AVCodecContext* ctx{nullptr};
    ~CodecCtxGuard() { if (ctx) avcodec_free_context(&ctx); }
};

struct FrameGuard {
    AVFrame* frame{nullptr};
    explicit FrameGuard(AVFrame* f = nullptr) : frame(f) {}
    ~FrameGuard() { if (frame) av_frame_free(&frame); }
};

struct AvBufGuard {
    uint8_t* buf{nullptr};
    explicit AvBufGuard(uint8_t* b = nullptr) : buf(b) {}
    ~AvBufGuard() { if (buf) av_free(buf); }
};

struct PacketGuard {
    AVPacket* pkt{nullptr};
    explicit PacketGuard(AVPacket* p = nullptr) : pkt(p) {}
    ~PacketGuard() { if (pkt) av_packet_free(&pkt); }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// MediaController
// ---------------------------------------------------------------------------

MediaController::MediaController()  = default;
MediaController::~MediaController() = default;

MediaInfo MediaController::probe(const std::string& path) const
{
    FormatCtxGuard fg;
    if (avformat_open_input(&fg.ctx, path.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("MediaController: cannot open '" + path + "'");

    if (avformat_find_stream_info(fg.ctx, nullptr) < 0)
        throw std::runtime_error("MediaController: cannot read stream info from '" + path + "'");

    MediaInfo info;
    info.format_name = fg.ctx->iformat->long_name
                     ? fg.ctx->iformat->long_name
                     : fg.ctx->iformat->name;

    if (fg.ctx->duration != AV_NOPTS_VALUE)
        info.duration = static_cast<double>(fg.ctx->duration) / AV_TIME_BASE;

    // Find the first video stream.
    for (unsigned i = 0; i < fg.ctx->nb_streams; ++i) {
        AVStream* s = fg.ctx->streams[i];
        if (s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            info.width  = s->codecpar->width;
            info.height = s->codecpar->height;
            const AVCodec* codec = avcodec_find_decoder(s->codecpar->codec_id);
            info.codec_name = codec ? codec->name : "unknown";
            break;
        }
    }

    return info;
}

bool MediaController::generate_thumbnail(const std::string& source_path,
                                          const std::string& output_path,
                                          double             time_offset) const
{
    // --- open input ---
    FormatCtxGuard fg;
    if (avformat_open_input(&fg.ctx, source_path.c_str(), nullptr, nullptr) < 0)
        return false;

    if (avformat_find_stream_info(fg.ctx, nullptr) < 0)
        return false;

    // --- find best video stream ---
    int video_idx = av_find_best_stream(fg.ctx, AVMEDIA_TYPE_VIDEO,
                                        -1, -1, nullptr, 0);
    if (video_idx < 0)
        return false;

    AVStream* video_stream = fg.ctx->streams[video_idx];

    // --- open decoder ---
    const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!codec)
        return false;

    CodecCtxGuard cg;
    cg.ctx = avcodec_alloc_context3(codec);
    if (!cg.ctx)
        return false;

    if (avcodec_parameters_to_context(cg.ctx, video_stream->codecpar) < 0)
        return false;

    if (avcodec_open2(cg.ctx, codec, nullptr) < 0)
        return false;

    // --- seek ---
    if (time_offset > 0.0) {
        int64_t ts = static_cast<int64_t>(time_offset * AV_TIME_BASE);
        av_seek_frame(fg.ctx, -1, ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(cg.ctx);
    }

    // --- decode first frame ---
    FrameGuard frame_g(av_frame_alloc());
    PacketGuard pkt_g(av_packet_alloc());
    if (!frame_g.frame || !pkt_g.pkt)
        return false;

    bool got_frame = false;
    while (!got_frame && av_read_frame(fg.ctx, pkt_g.pkt) >= 0) {
        if (pkt_g.pkt->stream_index == video_idx) {
            if (avcodec_send_packet(cg.ctx, pkt_g.pkt) == 0)
                got_frame = (avcodec_receive_frame(cg.ctx, frame_g.frame) == 0);
        }
        av_packet_unref(pkt_g.pkt);
    }
    if (!got_frame)
        return false;

    const int w = cg.ctx->width;
    const int h = cg.ctx->height;

    // --- convert to RGB24 ---
    SwsContext* sws = sws_getContext(w, h,
                                     static_cast<AVPixelFormat>(frame_g.frame->format),
                                     w, h, AV_PIX_FMT_RGB24,
                                     SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws)
        return false;

    FrameGuard rgb_g(av_frame_alloc());
    if (!rgb_g.frame) {
        sws_freeContext(sws);
        return false;
    }

    const int    buf_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    AvBufGuard   buf_g(static_cast<uint8_t*>(av_malloc(static_cast<size_t>(buf_size))));
    if (!buf_g.buf) {
        sws_freeContext(sws);
        return false;
    }

    av_image_fill_arrays(rgb_g.frame->data, rgb_g.frame->linesize,
                         buf_g.buf, AV_PIX_FMT_RGB24, w, h, 1);

    sws_scale(sws,
              frame_g.frame->data, frame_g.frame->linesize, 0, h,
              rgb_g.frame->data,   rgb_g.frame->linesize);
    sws_freeContext(sws);

    // --- encode to PNG ---
    const AVCodec* png_codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!png_codec)
        return false;

    CodecCtxGuard png_cg;
    png_cg.ctx = avcodec_alloc_context3(png_codec);
    if (!png_cg.ctx)
        return false;

    png_cg.ctx->width     = w;
    png_cg.ctx->height    = h;
    png_cg.ctx->pix_fmt   = AV_PIX_FMT_RGB24;
    png_cg.ctx->time_base = {1, 25};

    if (avcodec_open2(png_cg.ctx, png_codec, nullptr) < 0)
        return false;

    rgb_g.frame->format = AV_PIX_FMT_RGB24;
    rgb_g.frame->width  = w;
    rgb_g.frame->height = h;
    rgb_g.frame->pts    = 0;

    PacketGuard out_pkt_g(av_packet_alloc());
    if (!out_pkt_g.pkt)
        return false;

    bool success = false;
    if (avcodec_send_frame(png_cg.ctx, rgb_g.frame) == 0 &&
        avcodec_receive_packet(png_cg.ctx, out_pkt_g.pkt) == 0)
    {
        std::FILE* f = std::fopen(output_path.c_str(), "wb");
        if (f) {
            std::fwrite(out_pkt_g.pkt->data, 1,
                        static_cast<size_t>(out_pkt_g.pkt->size), f);
            std::fclose(f);
            success = true;
        }
        av_packet_unref(out_pkt_g.pkt);
    }

    return success;
}
