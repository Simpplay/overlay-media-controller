#pragma once

#include <stdio.h>

#include "core/event/EventBus.hpp"

#include "core/event/events/WindowOpenRequestedEvent.hpp"
#include "UiWindow.hpp"
#include "UiTypes.hpp"
#include "infra/window/SDLRenderer.hpp"

#include "modules/ui/windows/TestWindow.hpp"

namespace omc::ui
{
	class UiManager
	{
	public:
		UiManager(omc::event::EventBus& eventBus) : eventBus(eventBus), renderer(eventBus)
		{
			eventBus.subscribe<omc::event::WindowOpenRequestedEvent>([this](const omc::event::WindowOpenRequestedEvent& event) {
				handleWindowOpenRequestedEvent(event);
			});
		}

		void init()
		{
			renderer.init();

			// Test window
			int numTestWindows = 5;

			for (int i = 0; i < numTestWindows; ++i) {
				omc::ui::window::TestWindow testWindow;
				windows.push_back(std::make_unique<omc::ui::window::TestWindow>(testWindow));
			}

			printf("UiManager initialized with %d test windows.\n", static_cast<int>(windows.size()));
		}

		void render();
		void update();

	private:
		void handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event);
		std::vector<std::unique_ptr<omc::ui::window::UiWindow>> windows;

		omc::event::EventBus& eventBus;
		omc::infra::SDLRenderer renderer;
	};
}