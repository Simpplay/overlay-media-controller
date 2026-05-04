#pragma once

#include "core/event/EventBus.hpp"
#include "modules/ui/UiManager.hpp"

namespace omc::application
{
	class OverlayMediaController
	{
	public:
		void initialize();
		void close();

	private:
		bool running{ false };

		omc::event::EventBus eventBus;

		omc::ui::UiManager uiManager{ eventBus };
	};
}