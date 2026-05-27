#include "ApiServer.hpp"

#include "HttpUtils.hpp"

#include "IApiModule.hpp"
#include "CategoriesApiModule.hpp"
#include "MediaApiModule.hpp"
#include "OverlaysApiModule.hpp"
#include "SoundboardApiModule.hpp"
#include "modules/server/api/events/ServerStartedEvent.hpp"


namespace omc::server
{
	struct ApiServer::Impl {
		httplib::Server server;
		std::vector<std::unique_ptr<IApiModule>> modules;

		void init(omc::event::EventBus& eb, omc::media::IMediaService& ms, omc::ui::IUiService& ui, omc::soundboard::ISoundBoardService& sb) {
			server.set_base_dir("./web");

			// Registramos los módulos
			modules.push_back(std::make_unique<CategoriesApiModule>(eb, ms));
			modules.push_back(std::make_unique<MediaApiModule>(eb, ms));
			modules.push_back(std::make_unique<OverlaysApiModule>(eb, ui));
			modules.push_back(std::make_unique<SoundboardApiModule>(eb, sb));

			// Configuración global (CORS)
			server.Options(R"(.*)", [](const auto&, auto& res) {
				setCorsHeaders(res);
				res.status = 204;
			});

			server.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
				res.set_content("{\"status\":\"ok\"}", "application/json");
			});

			// Cada módulo registra sus rutas
			for (auto& module : modules) {
				module->registerRoutes(server);
			}
		}

		void listen(int port) 
		{
			if (!server.listen("0.0.0.0", port))
				throw std::runtime_error("ApiServer: failed to bind port " + std::to_string(port));
		}

		void stop() 
		{
			server.stop();
		}
	};

	ApiServer::ApiServer() = default;
	ApiServer::~ApiServer() = default;

	ApiServer::ApiServer(ApiServer&&) noexcept = default;
	ApiServer& ApiServer::operator=(ApiServer&&) noexcept = default;

	void ApiServer::start(int port,
		omc::event::EventBus& eventBus,
		omc::media::IMediaService& mediaService,
		omc::ui::IUiService& uiService,
		omc::soundboard::ISoundBoardService& soundboardService)
	{
		if (impl_)
			throw std::logic_error("ApiServer is already running");

		impl_ = std::make_unique<Impl>();
		impl_->init(eventBus, mediaService, uiService, soundboardService);

		eventBus.post(std::make_unique<omc::event::ServerStartedEvent>(port));

		impl_->listen(port);
	}

	void ApiServer::stop()
	{
		if (impl_) {
			impl_->stop();
			impl_.reset();
		}
	}
}
