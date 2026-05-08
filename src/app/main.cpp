#include <string>

#include "runtime/OverlayMediaController.hpp"

int main(int argc, char* argv[])
{
    // Configuration via environment variables (with sensible defaults).
    const char* db_path_env = std::getenv("OMC_DB_PATH");
    const char* port_env = std::getenv("OMC_PORT");

    const std::string db_path = db_path_env ? db_path_env : "overlay_media_controller.db";
    const int         port = port_env ? std::atoi(port_env) : 8080;

    // Allow an explicit db path as the first CLI argument (overrides env var).
    const std::string effective_db = (argc > 1) ? argv[1] : db_path;
    const int         effective_port = (argc > 2) ? std::atoi(argv[2]) : port;

    omc::application::OverlayMediaController controller;
    controller.initialize(effective_db, effective_port);
    return 0;
}