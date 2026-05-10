#include "OverlaysApiModule.hpp"

#include <nlohmann/json.hpp>

#include "HttpUtils.hpp"

#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

namespace omc::server
{
	OverlaysApiModule::OverlaysApiModule(omc::event::EventBus& eventBus, omc::ui::IUiService& uiService) :
		eventBus(eventBus), uiService(uiService)
	{ }

	void OverlaysApiModule::registerRoutes(httplib::Server& server)
	{
		server.Get(R"(/api/overlays)", [this](const httplib::Request& req, httplib::Response& res) {
			handleGetAllOverlays(req, res);
		});
		server.Post(R"(/api/overlays)", [this](const httplib::Request& req, httplib::Response& res) {
			handleAddOverlay(req, res);
		});
		server.Get(R"(/api/overlays/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleGetOverlayById(req, res);
		});
		server.Patch(R"(/api/overlays/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleUpdateOverlayById(req, res);
		});
		server.Delete(R"(/api/overlays/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleDeleteOverlayById(req, res);
		});
	}

	void OverlaysApiModule::handleGetAllOverlays(const httplib::Request&, httplib::Response& res)
	{
		try {
			auto sources = uiService.getAllOverlays();
			res.set_content(omc::json::JsonSerializer::serialize(sources), "application/json");
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void OverlaysApiModule::handleAddOverlay(const httplib::Request& req, httplib::Response& res)
	{
		try {
			auto body = nlohmann::json::parse(req.body);
			if (!body.contains("media_id") || !body["media_id"].is_number_integer()) {
				setError(res, httplib::StatusCode::BadRequest_400, "Missing media_id parameter in JSON");
				return;
			}

			const int  id = body["media_id"].get<int>();
			const bool fullscreen = body.value("fullscreen", false);
			const float posX = body.contains("position") ? body["position"].value("x", 0.0f) : body.value("x", 0.0f);
			const float posY = body.contains("position") ? body["position"].value("y", 0.0f) : body.value("y", 0.0f);
			const float sizeX = body.contains("size") ? body["size"].value("width", 960.0f) : body.value("width", 960.0f);
			const float sizeY = body.contains("size") ? body["size"].value("height", 640.0f) : body.value("height", 640.0f);

			eventBus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(id, fullscreen, posX, posY, sizeX, sizeY));
			res.set_content("{\"status\":\"shown\"}", "application/json");
		}
		catch (const nlohmann::json::exception& e) {
			setError(res, httplib::StatusCode::BadRequest_400, e.what());
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void OverlaysApiModule::handleGetOverlayById(const httplib::Request& req, httplib::Response& res)
	{
		auto overlay = uiService.getOverlayById(extractId(req));
		if (!overlay) {
			setError(res, httplib::StatusCode::NotFound_404, "Overlay not found");
			return;
		}
		res.set_content(omc::json::JsonSerializer::serialize(*overlay), "application/json");
	}

	void OverlaysApiModule::handleDeleteOverlayById(const httplib::Request& req, httplib::Response& res)
	{
		bool result = uiService.deleteOverlayById(extractId(req));
		if (!result) {
			setError(res, httplib::StatusCode::NotFound_404, "Overlay not found");
			return;
		}
		res.status = httplib::StatusCode::OK_200;
		res.set_content("{\"status\":\"deleted\"}", "application/json");
	}

	void OverlaysApiModule::handleUpdateOverlayById(const httplib::Request& req, httplib::Response& res)
	{
		try {
			const int id = extractId(req);
			auto body = nlohmann::json::parse(req.body);

			std::optional<bool> fullscreen;
			if (body.contains("fullscreen")) fullscreen = body["fullscreen"].get<bool>();
			std::optional<int> mediaId;
			if (body.contains("media_id")) mediaId = body["media_id"].get<int>();
			std::optional<float> posX;
			if (body.contains("position") && body["position"].contains("x")) posX = body["position"]["x"].get<float>();
			else if (body.contains("x")) posX = body["x"].get<float>();
			std::optional<float> posY;
			if (body.contains("position") && body["position"].contains("y")) posY = body["position"]["y"].get<float>();
			else if (body.contains("y")) posY = body["y"].get<float>();
			std::optional<float> sizeX;
			if (body.contains("size") && body["size"].contains("width")) sizeX = body["size"]["width"].get<float>();
			else if (body.contains("width")) sizeX = body["width"].get<float>();
			std::optional<float> sizeY;
			if (body.contains("size") && body["size"].contains("height")) sizeY = body["size"]["height"].get<float>();
			else if (body.contains("height")) sizeY = body["height"].get<float>();

			auto updatedOverlay = uiService.updateOverlay(id, mediaId, fullscreen, posX, posY, sizeX, sizeY);
			res.set_content(omc::json::JsonSerializer::serialize(updatedOverlay), "application/json");
		}
		catch (const nlohmann::json::exception& e) {
			setError(res, httplib::StatusCode::BadRequest_400, e.what());
		}
	}
}
