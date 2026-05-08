#include "ApiServer.hpp"

#include <httplib.h>

#include "core/threading/api/ThreadPool.hpp"

namespace omc::server
{
	struct ApiServer::Impl {
		httplib::Server server;
		omc::event::EventBus& eventBus;
		int port{ 0 };

		Impl(int port, omc::event::EventBus& eventBus) : port(port), eventBus(eventBus)
		{
			registerEndpoints();
		}

		void registerEndpoints()
		{
			// Health check endpoint
			server.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
				res.set_content("{\"status\":\"ok\"}", "application/json");
			});

			// -- API endpoints --
			// Media sources

			// Get all media sources
			server.Get("/api/media", [this](const httplib::Request& req, httplib::Response& res) {
				// Emit event to get all media sources
				res.set_content("[]", "application/json"); // Placeholder: return empty list
			});

			// Add a new media source
			server.Post("/api/media", [this](const httplib::Request& req, httplib::Response& res) {
				// Placeholder: accept media source data and return created resource
				res.set_content("{\"id\":1}", "application/json");
			});

			// Get a single media source by ID
			server.Get("/api/media/{id}", [this](const httplib::Request& req, httplib::Response& res) {
				// Placeholder: return a single media source by ID
				res.set_content("{\"id\":1}", "application/json");
			});

			// Delete a media source by ID
			server.Delete("/api/media/{id}", [this](const httplib::Request& req, httplib::Response& res) {
				// Placeholder: delete a media source by ID
				res.set_content("{\"status\":\"deleted\"}", "application/json");
			});

			// Show media source in overlay
			server.Get("/api/media/{id}/show", [this](const httplib::Request& req, httplib::Response& res) {
				// Placeholder: trigger showing media source in overlay
				res.set_content("{\"status\":\"shown\"}", "application/json");
			});

		}
	};

	ApiServer::ApiServer() = default;
	ApiServer::~ApiServer() = default;

	ApiServer::ApiServer(ApiServer&&) noexcept = default;
	ApiServer& ApiServer::operator=(ApiServer&&) noexcept = default;

	void ApiServer::start(int port, omc::event::EventBus& eventBus)
	{
		this->impl_ = std::make_unique<Impl>(port, eventBus);
		this->impl_->server.listen("0.0.0.0", port);
	}

	void ApiServer::stop()
	{
		this->impl_->server.stop();
	}
}