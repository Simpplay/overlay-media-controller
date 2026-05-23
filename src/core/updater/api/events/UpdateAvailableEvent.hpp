#pragma once

#include "core/event/api/Event.hpp"

namespace omc::event
{
	class UpdateAvailableEvent : public Event
	{
	public:
		UpdateAvailableEvent() = default;
	};
}