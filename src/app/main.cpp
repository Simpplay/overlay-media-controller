#include "core/application/OverlayMediaController.hpp"

int main(int argc, char* argv[])
{
    omc::application::OverlayMediaController controller;
    controller.initialize();
    return 0;
}


/*#include "api_server.hpp"
#include "database.hpp"
#include "media_controller.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    // Configuration via environment variables (with sensible defaults).
    const char* db_path_env  = std::getenv("OMC_DB_PATH");
    const char* port_env     = std::getenv("OMC_PORT");

    const std::string db_path = db_path_env ? db_path_env : "overlays.db";
    const int         port    = port_env    ? std::atoi(port_env) : 8080;

    // Allow an explicit db path as the first CLI argument (overrides env var).
    const std::string effective_db = (argc > 1) ? argv[1] : db_path;
    const int         effective_port = (argc > 2) ? std::atoi(argv[2]) : port;

    std::cout << "overlay-media-controller v0.1.0\n"
              << "  database : " << effective_db   << '\n'
              << "  port     : " << effective_port << '\n';

    try {
        Database        db(effective_db);
        MediaController media;
        ApiServer       server(db, media, effective_port);

        std::cout << "Listening on http://0.0.0.0:" << effective_port << '\n';
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
*/