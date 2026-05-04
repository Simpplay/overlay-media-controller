#include "UiManager.hpp"

#include "infra/window/SDLRenderer.hpp"

#include <stdio.h>

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

		renderer.update();
	}

	void UiManager::handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event)
	{

	}
}