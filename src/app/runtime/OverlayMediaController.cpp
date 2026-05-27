#include "OverlayMediaController.hpp"

#include <iostream>
#include <objbase.h>

#include "version.h"
#include "core/types/Constants.hpp"
#include "core/threading/api/ThreadPool.hpp"

#include "core/event/api/events/ExitApplicationRequestedEvent.hpp"
#include "core/event/api/events/ApplicationHideRequestedEvent.hpp"
#include "core/event/api/events/ApplicationShowRequestedEvent.hpp"

namespace omc::application
{
	void OverlayMediaController::initialize(const OverlayMediaControllerConfig& config)
	{
		std::cout 
			<< APP_NAME << " v" << APP_VERSION << " (" << APP_CHANNEL << ")\n"
			<< "Database: " << config.dbPath << "\n"
			<< "Port: " << config.port << "\n";

		omc::shared::ThreadPool threadPool(std::thread::hardware_concurrency());

		eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const omc::event::ExitApplicationRequestedEvent& event) {
			close();
		});

		std::string dbError;
		if (!mediaRepository->setupDatabase(config.dbPath, dbError)) {
			std::cerr << "Failed to setup database: " << config.dbPath << "\n";
			std::cerr << "Error: " << dbError << "\n";
			return;
		}

		mediaManager->init(eventBus);
		uiManager.init(&threadPool);

		apiServer = std::make_unique<omc::server::ApiServer>();
		apiThread = std::thread([this, config]() {
			apiServer->start(config.port, eventBus, *mediaService, *uiService, *soundboardService);
		});

		std::cout << "Listening on: http://127.0.0.1:" << config.port << "\n";

		if (!config.skipUpdates) {
			std::cout << "Checking for updates...\n";
			if (updater.checkForUpdates()) {
				std::cout << "Update available! Downloading and installing...\n";
				updater.downloadAndInstallUpdates();
			}
			else {
				std::cout << "You are up to date!.\n";
			}
		}

		eventBus.emit(omc::event::ApplicationHideRequestedEvent{});

		running = true;
		while (running) {
			// Process events in thread-safe manner
			eventBus.processQueue();

			if (!running) break;

			// Update and render UI
			uiManager.update();
			uiManager.render();
		}
	}

	void OverlayMediaController::close()
	{
		std::cout << "Closing " << APP_NAME << "...\n";

		if (apiServer)
			apiServer->stop();

		if (apiThread.joinable())
			apiThread.join();

		if (mediaRepository)
			mediaRepository->closeDatabase();

		running = false;
	}
};