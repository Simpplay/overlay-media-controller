#pragma once

#include <stdio.h>

#include "core/event/api/EventBus.hpp"

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