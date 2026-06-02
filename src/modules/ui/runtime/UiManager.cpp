#include "UiManager.hpp"

#include "modules/ui/infrastructure/Win32Renderer.hpp"

#include <stdio.h>
#include <algorithm>
#include <map>

namespace omc::ui
{
	void UiManager::render()
	{
		std::vector<DrawCommand> drawCommands;
		uiRepository.renderWindows(drawCommands);

		renderer.render(drawCommands);
	}

	void UiManager::update()
	{
		uiRepository.updateWindows();

		renderer.update();
	}

	void UiManager::handleWindowOpenRequestedEvent(const omc::event::WindowOpenRequestedEvent& event)
	{
		auto window = event.window->clone();
		window->setZBase(MAX_Z_INDEX_PER_WINDOW + uiRepository.getWindowCount() * MAX_Z_INDEX_PER_WINDOW);
		uiRepository.addWindow(std::move(window));
	}

	void UiManager::handleHideApplicationRequestedEvent(const omc::event::ApplicationHideRequestedEvent& event)
	{
		renderer.HideProgramWindow();
	}

	void UiManager::handleShowApplicationRequestedEvent(const omc::event::ApplicationShowRequestedEvent& event)
	{
		renderer.ShowProgramWindow();
	}
}
