#include "UiManager.hpp"

#include "infra/window/Win32Renderer.hpp"

#include <stdio.h>
#include <algorithm>

namespace omc::ui
{
	void UiManager::render()
	{
		std::vector<DrawCommand> drawCommands;
		for (const auto& window : windows) {
			window->buildDrawCommand(drawCommands);
		}

		renderer.render(drawCommands);
	}

	void UiManager::update()
	{
		for (const auto& window : windows) {
			window->update();
		}

		windows.erase(std::remove_if(windows.begin(), windows.end(), [](const auto& window) {
			return window->closed();
		}), windows.end());

		renderer.update();
	}

	void UiManager::handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event)
	{
		printf("Received WindowOpenRequestedEvent for window type: %s\n", event.type.name());
		if (event.type == typeid(omc::ui::window::TestWindow)) {
			auto window = std::make_unique<omc::ui::window::TestWindow>();

			printf("Received %s with pos: (%f, %f) - size: (%f, %f)\n", event.type.name(), window->position.x, window->position.y, window->size.x, window->size.y);
			windows.push_back(std::move(window));
		} else {
			printf("Received WindowOpenRequestedEvent for unknown window type: %s\n", event.type.name());
		}
	}
}
