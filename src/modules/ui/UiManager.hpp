#pragma once

#include <stdio.h>

#include "core/event/EventBus.hpp"

#include "core/event/events/WindowOpenRequestedEvent.hpp"
#include "UiWindow.hpp"
#include "UiTypes.hpp"
#include "infra/window/Win32Renderer.hpp"

#include "modules/ui/windows/TestWindow.hpp"
#include "modules/ui/windows/MediaWindow.hpp"

constexpr auto MAX_Z_INDEX_PER_WINDOW = 10;

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

			printf("UiManager initialized..\n");
		}

		void init()
		{
			renderer.init();

			// Test window
			int numTestWindows = 2;

			for (int i = 0; i < numTestWindows; ++i) {
				eventBus.emit(omc::event::WindowOpenRequestedEvent(typeid(omc::ui::window::TestWindow)));
			}

			eventBus.emit(omc::event::WindowOpenRequestedEvent(typeid(omc::ui::window::MediaWindow)));
		}

		void render();
		void update();

	private:
		void handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event);
		std::vector<std::unique_ptr<omc::ui::window::UiWindow>> windows;

		omc::event::EventBus& eventBus;
		omc::infra::Win32Renderer renderer;
	};
}