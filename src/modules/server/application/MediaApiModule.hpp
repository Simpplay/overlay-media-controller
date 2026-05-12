#pragma once

#include "IApiModule.hpp"

#include "core/event/api/EventBus.hpp"
#include "modules/media/api/IMediaService.hpp"

namespace omc::server
{
	class MediaApiModule : public IApiModule
	{
	public:
		MediaApiModule(omc::event::EventBus& eventBus, omc::media::IMediaService& mediaService);
		void registerRoutes(httplib::Server& server) override;

	private:
		omc::event::EventBus&      eventBus;
		omc::media::IMediaService& mediaService;

	private:
		// Helpers
		std::string extractTitleFromRequest(const httplib::Request& req);

		// Api handlers
		void handleGetAllMedia(const httplib::Request& req, httplib::Response& res);
		void handleAddMedia(const httplib::Request& req, httplib::Response& res);
		void handleShowMedia(const httplib::Request& req, httplib::Response& res);
		void handleUpdateMedia(const httplib::Request& req, httplib::Response& res);
		void handleGetMediaById(const httplib::Request& req, httplib::Response& res);
		void handleDeleteMediaById(const httplib::Request& req, httplib::Response& res);
		void handleShowThumbnail(const httplib::Request& req, httplib::Response& res);
	};
}