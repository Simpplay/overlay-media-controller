#include "OverlayMediaController.hpp"

#include <iostream>

namespace omc::application
{
	void OverlayMediaController::initialize()
	{
		std::cout << "Initializing OverlayMediaController...\n";

		running = true;
		while (running) {
			
			eventBus.processQueue();
			// Placeholder for main loop logic, e.g., handling media playback, responding to events,
			// and managing overlays. In a real application, this would likely involve more complex

		}
	}
};