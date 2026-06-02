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

#include "modules/soundboard/infrastructure/WasapiPlayer.hpp"
#include "modules/soundboard/application/SoundBoardService.hpp"

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
		const std::filesystem::path mediaStoragePath = "./media-storage";
		const std::filesystem::path mediaThumbnailsPath = "./media-storage/thumbnails";
	};

	class OverlayMediaController
	{
	public:
		void close();

		void toggleAutoStart(bool enable);
		bool isAutoStartEnabled() const;

	private:
		bool running{ false };

		omc::event::EventBus eventBus;

		omc::application::Updater updater;

		omc::ui::UiRepository uiRepository;
		omc::ui::UiService uiService;
		omc::ui::UiManager uiManager;

		omc::media::MediaManager mediaManager;
		omc::media::SqliteMediaRepository mediaRepository;
		omc::media::MediaService mediaService;

		omc::soundboard::WasapiPlayer soundboardPlayer;
		omc::soundboard::SoundBoardService soundboardService;

		omc::server::ApiServer apiServer;

		std::thread apiThread;

	public:
		OverlayMediaController(const OverlayMediaControllerConfig& config);
		~OverlayMediaController() = default;
	};
}