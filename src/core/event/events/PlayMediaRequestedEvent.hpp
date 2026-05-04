#pragma once

#include "core/event/Event.hpp"

namespace omc::event {
	class PlayMediaRequestedEvent : public Event {
	public:
		explicit PlayMediaRequestedEvent(int media_id)
			: media_id(media_id) {}
		int media_id;
	};
}