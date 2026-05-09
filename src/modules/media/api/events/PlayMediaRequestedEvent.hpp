#pragma once

#include "core/event/api/Event.hpp"

namespace omc::event {
	class PlayMediaRequestedEvent : public Event {
	public:
		explicit PlayMediaRequestedEvent(int media_id, bool fullscreen, float posX, float posY, float width, float height)
			: id(media_id), fullscreen(fullscreen), posX(posX), posY(posY), width(width), height(height) {}

		int id;
		bool fullscreen{false};
		float posX{0.0f};
		float posY{0.0f};
		float width{0.0f};
		float height{0.0f};
	};
}