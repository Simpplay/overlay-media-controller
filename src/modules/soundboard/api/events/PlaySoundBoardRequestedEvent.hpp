#pragma once

#include <string>
#include <cstdint>

#include "core/event/api/Event.hpp"

namespace omc::event {
	class PlaySoundBoardRequestedEvent : public Event {
	public:
		explicit PlaySoundBoardRequestedEvent(int media_id)
			: id(media_id) {
		}

		int id;
	};
}