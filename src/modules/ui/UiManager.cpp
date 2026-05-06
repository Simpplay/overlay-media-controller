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
		if (event.type == typeid(omc::ui::window::TestWindow)) {
			auto window = std::make_unique<omc::ui::window::TestWindow>();

			window->setZBase(MAX_Z_INDEX_PER_WINDOW + static_cast<int>(windows.size()) * MAX_Z_INDEX_PER_WINDOW);

			windows.push_back(std::move(window));
		} else {
			printf("Received WindowOpenRequestedEvent for unknown window type: %s\n", event.type.name());
		}
	}
}
