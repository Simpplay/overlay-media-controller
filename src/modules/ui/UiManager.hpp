#pragma once

#include "core/event/EventBus.hpp"

#include "core/event/events/WindowOpenRequestedEvent.hpp"
#include "UiWindow.hpp"
#include "infra/window/SDLRenderer.hpp"

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
		}

		void render();
		void update();

	private:
		void handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event);
		std::vector<std::unique_ptr<UiWindow>> windows;

		omc::event::EventBus& eventBus;
		omc::infra::SDLRenderer renderer;
	};
}