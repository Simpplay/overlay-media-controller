#pragma once

#include "core/event/EventBus.hpp"

namespace omc::application
{
	class OverlayMediaController
	{
	public:
		void initialize();

	private:
		bool running{ false };

		omc::event::EventBus eventBus;
	};
}