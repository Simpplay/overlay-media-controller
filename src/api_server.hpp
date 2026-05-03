#pragma once

#include <memory>

// Forward declarations to keep httplib.h out of the public header.
class Database;
class MediaController;

/// HTTP API server exposing the overlay-media-controller REST endpoints.
///
/// Endpoints
/// ---------
/// GET    /health                    – liveness probe
/// GET    /api/overlays              – list all overlays
/// POST   /api/overlays              – create overlay (JSON body)
/// GET    /api/overlays/:id          – get one overlay
/// DELETE /api/overlays/:id          – delete overlay
/// PUT    /api/overlays/:id/active   – set active flag (JSON body: {"active": true})
/// GET    /api/overlays/:id/probe    – probe the overlay's source media
class ApiServer {
public:
    ApiServer(Database& db, MediaController& media, int port = 8080);
    ~ApiServer();

    // Non-copyable
    ApiServer(const ApiServer&)            = delete;
    ApiServer& operator=(const ApiServer&) = delete;

    /// Block and serve requests until stop() is called from another thread.
    void run();

    /// Signal the server to stop accepting new connections.
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
