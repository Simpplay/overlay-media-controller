#pragma once

#include <string>

#include "shared/threading/ThreadPool.hpp"

namespace omc::media {
	class MediaPlayer {
	public:
		MediaPlayer(omc::shared::ThreadPool* threadPool) : threadPool(threadPool) {}

		virtual ~MediaPlayer() noexcept = default;

		virtual bool load(const std::string& path) = 0;
		virtual void play(int mediaId) = 0;
		virtual void pause() = 0;
		virtual void stop() = 0;

	protected:
		omc::shared::ThreadPool* threadPool{ nullptr };
		int mediaId = -1;
	};
}