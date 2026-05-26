#pragma once

#include <string>
#include <cstdint>

#include "core/event/api/Event.hpp"

namespace omc::event {
	class PlaySoundBoardRequestedEvent : public Event {
	public:
		explicit PlaySoundBoardRequestedEvent(int media_id, float volume, bool force)
			: id(media_id), volume(volume), force(force) {
		}

		int id;
		float volume;
		bool force;
	};
}