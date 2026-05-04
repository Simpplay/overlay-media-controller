#include "UiManager.hpp"

#include "infra/window/SDLRenderer.hpp"

namespace omc::ui
{
	void UiManager::render()
	{
		for (const auto& window : windows) {
			window->render();
		}

		renderer.render();
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