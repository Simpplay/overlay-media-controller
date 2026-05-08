#include "UiManager.hpp"

#include "infra/window/Win32Renderer.hpp"

#include <stdio.h>
#include <algorithm>
#include <map>

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
		omc::ui::window::UiWindow* highestZWindow = nullptr;
		int highestZ = -1;

		for (const auto& window : windows) {
			window->update();

			if (window->canInteractWithWindow()) {
				if (window->getZBase() > highestZ) {
					highestZ = window->getZBase();
					highestZWindow = window.get();
				}
			}

			if (window->isInteractingWithTitleBar()) {
				highestZWindow = window.get();
				highestZ = LONG_MAX; // Asegura que esta ventana quede al frente
			}
		}

		if (highestZWindow) {
			highestZWindow->updateWindowInteraction();
		}

		windows.erase(std::remove_if(windows.begin(), windows.end(), [](const auto& window) {
			return window->closed();
		}), windows.end());

		renderer.update();
	}

	void UiManager::handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event)
	{
		auto window = event.window.clone();
		window->setZBase(MAX_Z_INDEX_PER_WINDOW + static_cast<int>(windows.size()) * MAX_Z_INDEX_PER_WINDOW);
		windows.push_back(std::move(window));
	}
}
