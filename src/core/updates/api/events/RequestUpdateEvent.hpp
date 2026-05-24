#pragma once

#include "core/event/api/Event.hpp"

namespace omc::event
{
	class RequestUpdateEvent : public Event
	{
	public:
		RequestUpdateEvent() = default;
	};
}