#pragma once

#include <stdio.h>

#include "core/event/infraestructure/EventBus.hpp"

#include "modules/ui/api/events/WindowOpenRequestedEvent.hpp"
#include "modules/ui/domain/UiWindow.hpp"
#include "modules/ui/domain/RenderTypes.hpp"
#include "modules/ui/infraestructure/Win32Renderer.hpp"

#include "core/threading/api/ThreadPool.hpp"

#include "modules/ui/application/windows/TestWindow.hpp"
#include "modules/ui/application/windows/WebViewWindow.hpp"

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

		void init(omc::shared::ThreadPool* threadPool)
		{
			this->threadPool = threadPool;
			renderer.init();

			// Test window
			int numTestWindows = 0;

			for (int i = 0; i < numTestWindows; ++i) {
				omc::ui::window::TestWindow testWindow;
				eventBus.emit(omc::event::WindowOpenRequestedEvent(testWindow));
			}

			// Here create a webview window and navigate to a URL, e.g.: youtube.com, and test if it renders correctly and is interactive.
			eventBus.emit(omc::event::WindowOpenRequestedEvent{
				omc::ui::window::WebViewWindow("https://youtube.com", {200, 150}, {960, 640})
			});
		}

		void render();
		void update();

	private:
		void handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event);
		std::vector<std::unique_ptr<omc::ui::window::UiWindow>> windows;

		omc::shared::ThreadPool* threadPool{ nullptr };

		omc::event::EventBus& eventBus;
		omc::infra::Win32Renderer renderer;
	};
}