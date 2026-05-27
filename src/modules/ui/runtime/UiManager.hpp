#pragma once

#include <stdio.h>

#include "modules/ui/domain/UiRepository.hpp"

#include "core/event/api/EventBus.hpp"

#include "modules/ui/api/events/WindowOpenRequestedEvent.hpp"
#include "modules/ui/domain/UiWindow.hpp"
#include "modules/ui/domain/RenderTypes.hpp"
#include "modules/ui/infrastructure/Win32Renderer.hpp"

#include "core/threading/api/ThreadPool.hpp"

#include "modules/ui/application/windows/TestWindow.hpp"
#include "modules/ui/application/windows/WebViewWindow.hpp"

#include "core/event/api/events/ApplicationHideRequestedEvent.hpp"
#include "core/event/api/events/ApplicationShowRequestedEvent.hpp"

namespace omc::ui
{
	class UiManager
	{
	public:
		UiManager(omc::event::EventBus& eventBus, std::shared_ptr<UiRepository> uiRepository) : eventBus(eventBus), uiRepository(uiRepository), renderer(eventBus)
		{
			eventBus.subscribe<omc::event::WindowOpenRequestedEvent>([this](const omc::event::WindowOpenRequestedEvent& event) {
				handleWindowOpenRequestedEvent(event);
			});

			eventBus.subscribe<omc::event::ApplicationHideRequestedEvent>([this](const omc::event::ApplicationHideRequestedEvent& event) {
				handleHideApplicationRequestedEvent(event);
			});

			eventBus.subscribe<omc::event::ApplicationShowRequestedEvent>([this](const omc::event::ApplicationShowRequestedEvent& event) {
				handleShowApplicationRequestedEvent(event);
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
		void handleHideApplicationRequestedEvent(const omc::event::ApplicationHideRequestedEvent& event);
		void handleShowApplicationRequestedEvent(const omc::event::ApplicationShowRequestedEvent& event);

		std::shared_ptr<UiRepository> uiRepository;

		omc::shared::ThreadPool* threadPool{ nullptr };

		omc::event::EventBus& eventBus;
		omc::infra::Win32Renderer renderer;
	};
}