#pragma once

#include "core/event/api/Event.hpp"

namespace omc::event
{
	class ExitApplicationRequestedEvent : public Event
	{
	public:
		ExitApplicationRequestedEvent() = default;
	};
}