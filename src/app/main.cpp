#include <string>
#include <filesystem>

#include "runtime/OverlayMediaController.hpp"

#include <CLI/CLI.hpp>

int main(int argc, char* argv[])
{
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

    // Parse CLI arguments and automatically handle errors / --help
    CLI11_PARSE(app, argc, argv);

    // Start the application with the validated configuration
    omc::application::OverlayMediaController controller;
    omc::application::OverlayMediaControllerConfig config;
    config.dbPath = db_path;
    config.port = port;
    config.skipUpdates = skip_updates;
    controller.initialize(config);

    return 0;
}