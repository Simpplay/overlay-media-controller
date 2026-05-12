#include "OverlayMediaController.hpp"

#include <iostream>
#include <objbase.h>

#include "core/types/Constants.hpp"
#include "core/threading/api/ThreadPool.hpp"

#include "core/event/api/events/ExitApplicationRequestedEvent.hpp"

namespace omc::application
{
	void OverlayMediaController::initialize(std::string dbPath, int port)
	{
		std::cout 
			<< APP_NAME << "\n"
			<< "Database: " << dbPath << "\n"
			<< "Port: " << port << "\n";

		omc::shared::ThreadPool threadPool(std::thread::hardware_concurrency());

		eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const omc::event::ExitApplicationRequestedEvent& event) {
			close();
		});

		std::string dbError;
		if (!mediaRepository->setupDatabase(dbPath, dbError)) {
			std::cerr << "Failed to setup database: " << dbPath << "\n";
			std::cerr << "Error: " << dbError << "\n";
			return;
		}

		mediaManager->init(eventBus);
		uiManager.init(&threadPool);

		apiServer = std::make_unique<omc::server::ApiServer>();
		apiThread = std::thread([this, port]() {
			apiServer->start(port, eventBus, *mediaService, *uiService);
		});

		std::cout << "Listening on: http://127.0.0.1:" << port << "\n";

		running = true;
		while (running) {
			// Process events in thread-safe manner
			eventBus.processQueue();

			// Update and render UI
			uiManager.update();
			uiManager.render();
		}

		CoUninitialize();
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