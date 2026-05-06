#pragma once

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "modules/media/MediaPLayer.hpp"

namespace omc::infra {
	class FfmpegMediaPlayer : omc::media::MediaPlayer {

	public:
		~FfmpegMediaPlayer();
		FfmpegMediaPlayer();

		bool load(const std::string& path) override;
		void play() override;
		void pause() override;
		void stop() override;

	private:
		AVFormatContext* fmt_ctx = nullptr;
	};
}