#pragma once

#include "core/event/Event.hpp"

namespace omc::event {
	class MediaStartedEvent : public Event {
	public:

		struct MediaInfo {
			std::string path;
			int64_t duration; // en microsegundos
			int width;
			int height;
		};

		MediaStartedEvent(const MediaInfo& media)
			: media(media) {
		}
		const MediaInfo& media;
	};
}