#pragma once

#include <stdio.h>

#include "core/event/EventBus.hpp"

#include "core/event/events/WindowOpenRequestedEvent.hpp"
#include "core/event/events/FrameReadyEvent.hpp"
#include "UiWindow.hpp"
#include "UiTypes.hpp"
#include "infra/window/Win32Renderer.hpp"

#include "shared/threading/ThreadPool.hpp"

#include "modules/ui/windows/TestWindow.hpp"
#include "modules/ui/windows/MediaWindow.hpp"

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

			eventBus.subscribe<omc::event::FrameReadyEvent>([this](const omc::event::FrameReadyEvent& event) {
				handleFrameReadyEvent(event);
			});

			printf("UiManager initialized..\n");
		}

		void init(omc::shared::ThreadPool* threadPool)
		{
			this->threadPool = threadPool;
			renderer.init();

			// Test window
			int numTestWindows = 2;

			for (int i = 0; i < numTestWindows; ++i) {
				omc::ui::window::TestWindow testWindow;
				eventBus.emit(omc::event::WindowOpenRequestedEvent(testWindow));
			}

			omc::ui::window::MediaWindow mediaWindow(1, eventBus);
			eventBus.emit(omc::event::WindowOpenRequestedEvent(mediaWindow));

			//omc::event::PlayMediaRequestedEvent playEvent{ 1 };
			//eventBus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(playEvent));
		}

		void render();
		void update();

	private:
		void handleFrameReadyEvent(const omc::event::FrameReadyEvent& event);
		void handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event);
		std::vector<std::unique_ptr<omc::ui::window::UiWindow>> windows;

		omc::shared::ThreadPool* threadPool{ nullptr };

		omc::event::EventBus& eventBus;
		omc::infra::Win32Renderer renderer;
	};
}