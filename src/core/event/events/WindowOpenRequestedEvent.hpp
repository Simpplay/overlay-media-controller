#pragma once

#include "core/event/Event.hpp"
#include <string>

namespace omc::event {
	class WindowOpenRequestedEvent : public Event {
	public:
		explicit WindowOpenRequestedEvent(const std::string& window_name)
			: window_name(window_name) {}
		std::string window_name;
	};
}