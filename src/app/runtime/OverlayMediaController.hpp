#pragma once

#include <memory>

#include "core/event/EventBus.hpp"

#include "modules/ui/UiManager.hpp"

#include "modules/media/MediaManager.hpp"

#include "modules/server/ApiServer.hpp"

namespace omc::application
{
	class OverlayMediaController
	{
	public:
		void initialize(std::string dbPath, int port);
		void close();

	private:
		bool running{ false };

		omc::event::EventBus eventBus;

		omc::ui::UiManager uiManager{ eventBus };

		std::unique_ptr<omc::media::MediaManager> mediaManager;

		std::unique_ptr<omc::server::ApiServer> apiServer;
	};
}