#include "OverlayMediaController.hpp"

#include <iostream>

#include "core/types/Constants.hpp"
#include "shared/threading/ThreadPool.hpp"

#include "core/event/events/ExitApplicationRequestedEvent.hpp"

namespace omc::application
{
	void OverlayMediaController::initialize()
	{
		std::cout << "Initializing " << APP_NAME << "...\n";

		omc::shared::ThreadPool threadPool(std::thread::hardware_concurrency());

		eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const omc::event::ExitApplicationRequestedEvent& event) {
			close();
		});

		uiManager.init(&threadPool);

		running = true;
		while (running) {
			// Process events in thread-safe manner
			eventBus.processQueue();

			// Update and render UI
			uiManager.update();
			uiManager.render();
		}
	}

	void OverlayMediaController::close()
	{
		std::cout << "Closing " << APP_NAME << "...\n";
		running = false;
	}
};