#pragma once

#include <memory>

#include "core/event/api/EventBus.hpp"
#include "core/updates/application/Updater.hpp"

#include "modules/ui/runtime/UiManager.hpp"
#include "modules/ui/domain/UiRepository.hpp"
#include "modules/ui/application/UiService.hpp"

#include "modules/media/runtime/MediaManager.hpp"
#include "modules/media/infrastructure/SqliteMediaRepository.hpp"
#include "modules/media/application/MediaService.hpp"

#include "modules/server/runtime/ApiServer.hpp"

namespace omc::application
{
	struct OverlayMediaControllerConfig
	{
		// Obligatory parameters
		std::string dbPath;
		int port;

		// Optional parameters with defaults
		bool skipUpdates = false;
	};

	class OverlayMediaController
	{
	public:
		void initialize(const OverlayMediaControllerConfig& config);
		void close();

	private:
		bool running{ false };

		omc::event::EventBus eventBus;

		omc::application::Updater updater{ eventBus };

		std::shared_ptr<omc::ui::UiRepository> uiRepository = std::make_shared<omc::ui::UiRepository>();
		std::unique_ptr<omc::ui::UiService> uiService = std::make_unique<omc::ui::UiService>(eventBus, uiRepository);
		omc::ui::UiManager uiManager{ eventBus, uiRepository };

		std::unique_ptr<omc::media::MediaManager> mediaManager = std::make_unique<omc::media::MediaManager>();
		std::shared_ptr<omc::media::SqliteMediaRepository> mediaRepository = std::make_shared<omc::media::SqliteMediaRepository>("./media-storage", "./media-storage/thumbnails");
		std::shared_ptr<omc::media::MediaService> mediaService = std::make_shared<omc::media::MediaService>(mediaRepository);

		std::unique_ptr<omc::server::ApiServer> apiServer;

		std::thread apiThread;
	};
}