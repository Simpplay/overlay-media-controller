#include "FfmpegMediaPlayer.hpp"

#include <stdio.h>

namespace omc::infra {

	FfmpegMediaPlayer::FfmpegMediaPlayer() {
		av_log_set_level(AV_LOG_INFO);
	}

	FfmpegMediaPlayer::~FfmpegMediaPlayer() {
		
	}

	bool FfmpegMediaPlayer::load(const std::string& path) {
		auto pathStr = path.c_str();

		if (avformat_open_input(&fmt_ctx, pathStr, nullptr, nullptr) < 0) {
			printf("Error: Could not open file: %s\n", pathStr);
			return false;
		}

		if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
			printf("Error: Could not find stream information for: %s\n", pathStr);
			avformat_close_input(&fmt_ctx);
			fmt_ctx = nullptr;
			return false;
		}

		return true;
	}

	void FfmpegMediaPlayer::play() {
		if (fmt_ctx == nullptr) {
			printf("Error: Load a file first!");
			return;
		}


	}

	void FfmpegMediaPlayer::pause() {
		
	}

	void FfmpegMediaPlayer::stop() {
		
	}
}