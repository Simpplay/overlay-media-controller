#pragma once

#include "core/event/Event.hpp"
#include "core/types/VideoFrame.hpp"

namespace omc::event {
	class FrameReadyEvent : public Event {
	public:
		FrameReadyEvent(int mediaId, const omc::media::VideoFrame& frame)
			: mediaId(mediaId), frame(frame) {
		}
		int mediaId;
		omc::media::VideoFrame frame;
	};
}