#include "OverlayMediaController.hpp"

#include <iostream>
#include <objbase.h>
#include <filesystem>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

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
		std::cout << "Initializing " << APP_NAME << " v" << APP_VERSION << " (" << APP_CHANNEL << ")\n";
		std::cout << "Database: " << config.dbPath << "\n";
		std::cout << "Port: " << config.port << "\n";

		toggleAutoStart(true);
		std::cout << "Auto-start enabled: " << (isAutoStartEnabled() ? "Yes" : "No") << "\n";

		std::cout << "Starting thread pool...\n";
		omc::shared::ThreadPool threadPool(std::thread::hardware_concurrency());

		std::cout << "Setting up event bus...\n";
		eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const omc::event::ExitApplicationRequestedEvent& event) {
			std::cout << "Exit requested via event bus.\n";
			close();
		});

		std::string dbError;
		std::cout << "Setting up database...\n";
		if (!mediaRepository->setupDatabase(config.dbPath, dbError)) {
			std::cerr << "Failed to setup database: " << config.dbPath << "\n";
			std::cerr << "Error: " << dbError << "\n";
			return;
		}

		std::cout << "Initializing managers...\n";
		mediaManager->init(eventBus);
		uiManager.init(&threadPool);

		std::cout << "Starting API server on port " << config.port << "...\n";
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

		std::cout << "Application started. Entering main loop.\n";
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
		std::cout << "Main loop exited.\n";
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

	void OverlayMediaController::toggleAutoStart(bool enable)
	{
#if defined(_WIN32) || defined(_WIN64)
		HKEY hKey;
		const char* subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
		if (RegOpenKeyExA(HKEY_CURRENT_USER, subKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
			std::cerr << "Failed to open registry key for auto-start configuration.\n";
			return;
		}
		if (enable) {
			char exePath[MAX_PATH];
			GetModuleFileNameA(NULL, exePath, MAX_PATH);

			std::string quotedPath = "\"" + std::string(exePath) + "\"";

			if (RegSetValueExA(hKey, APP_NAME, 0, REG_SZ,
				(const BYTE*)quotedPath.c_str(),
				quotedPath.length() + 1) != ERROR_SUCCESS) {
				std::cerr << "Failed to set registry value for auto-start.\n";
			}
		}
		else {
			if (RegDeleteValueA(hKey, APP_NAME) != ERROR_SUCCESS) {
				std::cerr << "Failed to delete registry value for auto-start.\n";
			}
		}
		RegCloseKey(hKey);
#endif
	}

	bool OverlayMediaController::isAutoStartEnabled() const
	{
#if defined(_WIN32) || defined(_WIN64)
		HKEY hKey;
		const char* subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
		if (RegOpenKeyExA(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
			std::cerr << "Failed to open registry key for auto-start configuration.\n";
			return false;
		}
		char value[MAX_PATH];
		DWORD valueSize = sizeof(value);
		LONG result = RegQueryValueExA(hKey, APP_NAME, NULL, NULL, (LPBYTE)value, &valueSize);
		RegCloseKey(hKey);
		return result == ERROR_SUCCESS;
#endif
		return false;
	}
};