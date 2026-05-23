#include <string>
#include <filesystem>

#include "runtime/OverlayMediaController.hpp"

#include <CLI/CLI.hpp>

int main(int argc, char* argv[])
{
    CLI::App app{ "Overlay Media Controller" };

    std::string db_path;
    int port;
	bool cleanup = false;

    app.add_option("--db-path", db_path, "Path to the SQLite database")
        ->envname("OMC_DB_PATH")
        ->default_val("overlay_media_controller.db");

    app.add_option("--port", port, "Port for the API server")
        ->envname("OMC_PORT")
        ->default_val(8080);

    app.add_flag("--cleanup", cleanup, "Clean up old files on startup");

    // Parse CLI arguments and automatically handle errors / --help
    CLI11_PARSE(app, argc, argv);

    if (cleanup)
    {
        
    }

    // Start the application with the validated configuration
    omc::application::OverlayMediaController controller;
    controller.initialize(db_path.c_str(), port);

    return 0;
}