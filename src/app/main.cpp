#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <windows.h>

#include "runtime/OverlayMediaController.hpp"
#include <CLI/CLI.hpp>

int main(int argc, char* argv[])
{
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        return 1;
    }
    
    std::filesystem::path p(exePath);
    std::filesystem::path appDir = p.parent_path();
    std::filesystem::current_path(appDir);

    std::filesystem::path logPath = appDir / "omc_boot_log.txt";
    std::ofstream logFile(logPath, std::ios::out | std::ios::trunc);
    
    std::streambuf* oldCout = nullptr;
    std::streambuf* oldCerr = nullptr;

    if (logFile.is_open()) {
        oldCout = std::cout.rdbuf(logFile.rdbuf());
        oldCerr = std::cerr.rdbuf(logFile.rdbuf());
    }

    std::cout << "--- Starting Overlay Media Controller ---" << std::endl;
    std::cout << "Executable: " << exePath << std::endl;
    std::cout << "Working Directory: " << std::filesystem::current_path() << std::endl;

    try {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            std::cerr << "CoInitializeEx failed with hr=" << hr << std::endl;
            return 1;
        }

        CLI::App app{ "Overlay Media Controller" };

        std::string db_path;
        int port;
        bool skip_updates = false;

        app.add_option("--db-path", db_path, "Path to the SQLite database")
            ->envname("OMC_DB_PATH")
            ->default_val("overlay_media_controller.db");

        app.add_option("--port", port, "Port for the API server")
            ->envname("OMC_PORT")
            ->default_val(8080);

        app.add_flag("--skip-updates", skip_updates, "Skip update checks on startup");

        CLI11_PARSE(app, argc, argv);

        // Start the application with the validated configuration
        omc::application::OverlayMediaControllerConfig config;
        config.dbPath = db_path;
        config.port = port;
        config.skipUpdates = skip_updates;
        
        omc::application::OverlayMediaController controller{ config };

        CoUninitialize();
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        if (logFile.is_open()) {
            logFile.flush();
        }
        return 1;
    } catch (...) {
        std::cerr << "CRITICAL ERROR: Unknown exception" << std::endl;
        return 1;
    }

    return 0;
}