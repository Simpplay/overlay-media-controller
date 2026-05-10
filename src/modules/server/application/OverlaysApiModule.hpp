#pragma once

#include "IApiModule.hpp"

#include "core/event/api/EventBus.hpp"
#include "modules/ui/api/IUiService.hpp"

namespace omc::server
{
	class OverlaysApiModule : public IApiModule
	{
	public:
		OverlaysApiModule(omc::event::EventBus& eventBus, omc::ui::IUiService& uiService);
		void registerRoutes(httplib::Server& server) override;

	private:
		omc::event::EventBus& eventBus;
		omc::ui::IUiService& uiService;

	private:
		void handleGetAllOverlays(const httplib::Request& req, httplib::Response& res);
		void handleAddOverlay(const httplib::Request& req, httplib::Response& res);
		void handleGetOverlayById(const httplib::Request& req, httplib::Response& res);
		void handleDeleteOverlayById(const httplib::Request& req, httplib::Response& res);
		void handleUpdateOverlayById(const httplib::Request& req, httplib::Response& res);
	};
}