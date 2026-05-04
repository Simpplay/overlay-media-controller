#pragma once

#include "core/event/Event.hpp"

namespace omc::event
{
	class ExitApplicationRequestedEvent : public Event
	{
	public:
		ExitApplicationRequestedEvent() = default;
	};
}