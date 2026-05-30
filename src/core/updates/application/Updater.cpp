#include "Updater.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <Windows.h>

#include "version.h"

#include "core/updates/api/events/RequestUpdateEvent.hpp"
#include "core/event/api/events/ExitApplicationRequestedEvent.hpp"

namespace omc::application
{
	Updater::Updater(omc::event::EventBus& eventBus) : eventBus(eventBus)
	{
		eventBus.subscribe<omc::event::RequestUpdateEvent>([this](const omc::event::RequestUpdateEvent& event) {
			downloadAndInstallUpdates();
		});
	}

	bool Updater::checkForUpdates()
	{
		auto res = cli.Get(LAST_RELEASE_URL, headers);
		if (!res)
		{
			std::cout << "Failed to check for updates\n";
			std::cout << "Error: " << static_cast<int>(res.error()) << '\n';
			return false;
		}

		auto status = res->status;

		if (status == 200)
		{
			nlohmann::json jsonResponse = nlohmann::json::parse(res->body);
			std::string latestReleaseTag = jsonResponse["tag_name"].get<std::string>();
			std::string currentVersion = "v" + std::string(APP_VERSION);

			if (latestReleaseTag != currentVersion)
			{
				if (jsonResponse.contains("assets") && !jsonResponse["assets"].empty())
				{
					latestVersionInfo = VersionInfo{
						.version = latestReleaseTag,
						.downloadLink = jsonResponse["assets"][0]["browser_download_url"].get<std::string>(),
						.sha256 = jsonResponse["assets"][0]["digest"].get<std::string>()
					};
					latestVersionInfo->sha256.erase(0, 7); // Remove "sha256:" prefix

					auto tempDir =
						std::filesystem::temp_directory_path();

					updateDir =
						tempDir /
						("omc_update_" +
							latestVersionInfo->version);

					return latestVersionInfo.has_value();
				}
			}

			return false;
		}

		std::cout << "Failed to check for updates\nError: " << status << '\n';
		return false;
	}

	void Updater::downloadAndInstallUpdates()
	{
		if (!latestVersionInfo)
		{
			std::cout << "No update available\n";
			return;
		}

		std::cout << "Downloading update...\n";

		if (!createTempDirectory())
		{
			std::cout << "Failed to create temporary directory\n";
			return;
		}

		if (!downloadZip())
		{
			std::cout << "Failed to download update\n";
			return;
		}

		if (!verifySHA256())
		{
			std::cout << "Failed to verify update integrity\n";
			return;
		}

		std::cout << "Update downloaded and verified successfully! Installing...\n";

		if (!extractZip())
		{
			std::cout << "Failed to extract update\n";
			return;
		}

		std::cout << "Update downloaded successfully! Installing...\n";

		if (!launchUpdater())
		{
			std::cout << "Failed to launch updater\n";
			return;
		}
	}

	bool Updater::createTempDirectory() const
	{
		if (!updateDir)
			return false;

		std::error_code ec;
		std::filesystem::create_directories(updateDir.value(), ec);

		return !ec;
	}

	bool omc::application::Updater::downloadZip()
	{
		if (!latestVersionInfo)
			return false;

		if (!updateDir)
			return false;

		const std::string url =
			latestVersionInfo.value().downloadLink;

		std::regex re(R"(https://([^/]+)(/.*))");

		std::smatch match;

		if (!std::regex_match(url, match, re))
		{
			return false;
		}

		std::string host = match[1];
		std::string path = match[2];

		httplib::SSLClient cli(host);

		cli.set_follow_location(true);

		auto zipPath = updateDir.value() / zipName;

		std::ofstream file(
			zipPath.string(),
			std::ios::binary
		);

		auto res = cli.Get(
			path,
			[&](const char* data, size_t len)
			{
				file.write(
					data,
					static_cast<std::streamsize>(len)
				);

				return true;
			});

		if (!res || res->status != 200)
		{
			return false;
		}

		return true;
	}

	static std::optional<std::string> computeSHA256(
		const std::filesystem::path& filePath)
	{
		std::ifstream file(filePath, std::ios::binary);

		if (!file)
			return std::nullopt;

		SHA256_CTX ctx;
		SHA256_Init(&ctx);

		char buffer[8192];

		while (file.good())
		{
			file.read(buffer, sizeof(buffer));

			auto bytesRead =
				static_cast<size_t>(file.gcount());

			if (bytesRead > 0)
			{
				SHA256_Update(
					&ctx,
					buffer,
					bytesRead
				);
			}
		}

		unsigned char hash[SHA256_DIGEST_LENGTH];

		SHA256_Final(hash, &ctx);

		std::ostringstream oss;

		for (unsigned char byte : hash)
		{
			oss << std::hex
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>(byte);
		}

		return oss.str();
	}

	bool omc::application::Updater::verifySHA256()
	{
		if (!latestVersionInfo)
			return false;

		if (!updateDir)
			return false;

		auto zipPath = updateDir.value() / zipName;

		if (zipPath.empty())
			return false;

		auto calculatedHash =
			computeSHA256(zipPath);

		if (!calculatedHash)
		{
			std::cout << "Cannot compute SHA256\n";
			return false;
		}

		if (*calculatedHash != latestVersionInfo->sha256)
		{
			std::cout << "SHA256 mismatch!\n";
			std::cout << "Expected: " << latestVersionInfo->sha256 << '\n';
			std::cout << "Calculated: " << *calculatedHash << '\n';

			std::filesystem::remove(zipPath);

			return false;
		}

		return true;
	}

	bool Updater::extractZip()
	{
		auto zipPath = updateDir.value() / zipName;
		auto extractDir = updateDir.value() / extractedDirName;

		std::string args =
			"powershell -Command \"Expand-Archive -Force '"
			+ zipPath.string() + "' '" + extractDir.string() + "'\"";

		STARTUPINFOA si{}; si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};

		if (!CreateProcessA(nullptr, args.data(), nullptr, nullptr,
			FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
			return false;

		WaitForSingleObject(pi.hProcess, 60'000);

		DWORD exitCode = 1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return exitCode == 0;
	}

	static std::filesystem::path getExecutablePath()
	{
		char buffer[MAX_PATH];

		DWORD length = GetModuleFileNameA(
			nullptr,
			buffer,
			MAX_PATH
		);

		if (length == 0)
		{
			throw std::runtime_error("GetModuleFileName failed");
		}

		return std::filesystem::path(buffer);
	}

	bool Updater::launchUpdater()
	{
		DWORD pid = GetCurrentProcessId();

		auto exePath = getExecutablePath();
		auto program_path = exePath.parent_path();
		auto exeName = exePath.filename().string();
		auto extractDir = updateDir.value() / extractedDirName;

		std::filesystem::path updatePath = extractDir;

		if (std::filesystem::exists(extractDir) && std::filesystem::is_directory(extractDir)) {
			int dirCount = 0;
			int fileCount = 0;
			std::filesystem::path subDir;
			for (const auto& entry : std::filesystem::directory_iterator(extractDir)) {
				if (entry.is_directory()) {
					subDir = entry.path();
					dirCount++;
				}
				else {
					fileCount++;
				}
			}

			// If there's exactly one directory and no files, it's likely a nested root folder
			if (dirCount == 1 && fileCount == 0) {
				updatePath = subDir;
			}
		}

		// Copy updater.exe to a temporary location to avoid locking itself during update
		auto originalUpdaterPath = program_path / "updater.exe";
		auto tempUpdaterPath = updateDir.value() / "updater_internal.exe";

		std::error_code ec;
		if (std::filesystem::exists(originalUpdaterPath)) {
			std::filesystem::copy_file(originalUpdaterPath, tempUpdaterPath, std::filesystem::copy_options::overwrite_existing, ec);
		}

		std::filesystem::path updaterToRun = ec ? originalUpdaterPath : tempUpdaterPath;

		std::string command =
			"\"" + updaterToRun.string() + "\"" +
			" --pid " + std::to_string(pid) +
			" --program \"" + program_path.string() + "\"" +
			" --executable \"" + exeName + "\"" +
			" --update \"" + updatePath.string() + "\"";

		STARTUPINFOA si{};
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi{};

		if (!CreateProcessA(
			nullptr,
			command.data(),
			nullptr,
			nullptr,
			FALSE,
			CREATE_NO_WINDOW | DETACHED_PROCESS,
			nullptr,
			nullptr,
			&si,
			&pi
		)) {
			return false;
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		eventBus.post(std::make_unique<omc::event::ExitApplicationRequestedEvent>());
		return true;
	}
}