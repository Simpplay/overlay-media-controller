#pragma once

#include <string>

namespace omc::media {
	class MediaPlayer {
	public:
		virtual ~MediaPlayer() noexcept = default;

		virtual bool load(const std::string& path) = 0;
		virtual void play() = 0;
		virtual void pause() = 0;
		virtual void stop() = 0;
	};
}