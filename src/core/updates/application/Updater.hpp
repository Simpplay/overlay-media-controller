#pragma once

#include <httplib.h>
#include <optional>
#include <filesystem>

#include "core/event/api/EventBus.hpp"

constexpr const char* UPDATE_SERVER_URL = "api.github.com";
constexpr const char* LAST_RELEASE_URL = "/repos/Simpplay/overlay-media-controller/releases/latest";

#define CPPHTTPLIB_OPENSSL_SUPPORT

namespace omc::application
{
	struct VersionInfo
	{
		std::string version;
		std::string downloadLink;
		std::string sha256;
	};

	class Updater
	{
	public:
		Updater(omc::event::EventBus& eventBus);
		bool checkForUpdates();
		void downloadAndInstallUpdates();

	private:
		bool createTempDirectory() const;
		bool downloadZip();
		bool verifySHA256();
		bool extractZip();
		bool launchUpdater();

	private:
		omc::event::EventBus& eventBus;

		httplib::Headers headers = {
			{"User-Agent", "overlay-media-controller"}
		};
		httplib::SSLClient cli{ UPDATE_SERVER_URL };

		std::optional<VersionInfo> latestVersionInfo;

		std::optional<std::filesystem::path> updateDir;
		std::string zipName = "update.zip";
		std::string extractedDirName = "extracted";
	};
}