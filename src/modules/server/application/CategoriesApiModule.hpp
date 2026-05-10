#pragma once

#include "IApiModule.hpp"

#include "core/event/api/EventBus.hpp"
#include "modules/media/api/IMediaService.hpp"

namespace omc::server
{
	class CategoriesApiModule : public IApiModule
	{
	public:
		CategoriesApiModule(omc::event::EventBus& eventBus, omc::media::IMediaService& mediaService);
		void registerRoutes(httplib::Server& server) override;

	private:
		omc::event::EventBus& eventBus;
		omc::media::IMediaService& mediaService;

	private:
		void handleGetAllCategories(const httplib::Request& req, httplib::Response& res);
		void handleAddCategory(const httplib::Request& req, httplib::Response& res);
		void handleGetCategoryById(const httplib::Request& req, httplib::Response& res);
		void handleDeleteCategoryById(const httplib::Request& req, httplib::Response& res);
		void handleAddMediaToCategory(const httplib::Request& req, httplib::Response& res);
		void handleRemoveMediaFromCategory(const httplib::Request& req, httplib::Response& res);
	};
}