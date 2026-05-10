#include "OverlaysApiModule.hpp"

#include <string>
#include <regex>
#include <stdexcept>

#include "HttpUtils.hpp"

#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

namespace omc::server
{
	OverlaysApiModule::OverlaysApiModule(omc::event::EventBus& eventBus, omc::ui::IUiService& uiService) :
		eventBus(eventBus), uiService(uiService)
	{ }

	void OverlaysApiModule::registerRoutes(httplib::Server& server)
	{
		// ── GET /api/overlays ─────────────────────────────────────
        server.Get(R"(/api/overlays)", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetAllOverlays(req, res);
		});

		// ── POST /api/overlays ────────────────────────────────────
		server.Post(R"(/api/overlays)", [this](const httplib::Request& req, httplib::Response& res) {
			handleAddOverlay(req, res);
		});

		// ── GET /api/overlays/:id ─────────────────────────────────
        server.Get(R"(/api/overlays/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetOverlayById(req, res);
		});

		// ── PATCH /api/overlays/:id ─────────────────────────────────
        server.Patch(R"(/api/overlays/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleUpdateOverlayById(req, res);
		});

		// ── DELETE /api/overlays/:id ─────────────────────────────────
        server.Delete(R"(/api/overlays/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleDeleteOverlayById(req, res);
		});
	}

	void OverlaysApiModule::handleGetAllOverlays(const httplib::Request& req, httplib::Response& res)
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
            // La carga útil JSON llega en el cuerpo de la petición
            const std::string& body = req.body;
            std::smatch match;

            // Extraer media_id (Obligatorio)
            if (!std::regex_search(body, match, std::regex(R"("media_id"\s*:\s*(\d+))"))) {
                throw std::invalid_argument("Missing media_id parameter in JSON");
            }
            const int id = std::stoi(match[1].str());

            // Extraer fullscreen (Opcional, por defecto false)
            bool fullscreen = false;
            if (std::regex_search(body, match, std::regex(R"("fullscreen"\s*:\s*(true|false))"))) {
                fullscreen = (match[1].str() == "true");
            }

            // Extraer posiciones x, y (Opcionales, por defecto 0.0)
            float posX = 0.0f, posY = 0.0f;
            if (std::regex_search(body, match, std::regex(R"("x"\s*:\s*([+-]?\d*\.?\d+))"))) {
                posX = std::stof(match[1].str());
            }
            if (std::regex_search(body, match, std::regex(R"("y"\s*:\s*([+-]?\d*\.?\d+))"))) {
                posY = std::stof(match[1].str());
            }

            // Extraer tamaño width, height (Opcionales, por defecto 960.0 x 640.0)
            float sizeX = 960.0f, sizeY = 640.0f;
            if (std::regex_search(body, match, std::regex(R"("width"\s*:\s*([+-]?\d*\.?\d+))"))) {
                sizeX = std::stof(match[1].str());
            }
            if (std::regex_search(body, match, std::regex(R"("height"\s*:\s*([+-]?\d*\.?\d+))"))) {
                sizeY = std::stof(match[1].str());
            }

            // Crear y enviar el evento
            auto event = omc::event::PlayMediaRequestedEvent(
                id, fullscreen, posX, posY, sizeX, sizeY
            );

            eventBus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(event));

            res.set_content("{\"status\":\"shown\"}", "application/json");
        }
        catch (const std::invalid_argument& e) {
            // Enviar el mensaje específico de error de validación
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
        if (req.matches.size() < 2) {
            throw std::invalid_argument("Missing overlay ID in the URL");
        }
        const int id = std::stoi(req.matches[1].str());

        // 2. Parsear el cuerpo de la petición buscando qué campos decidió actualizar el cliente
        const std::string& body = req.body;
        std::smatch match;

        std::optional<bool> fullscreen;
        if (std::regex_search(body, match, std::regex(R"("fullscreen"\s*:\s*(true|false))"))) {
            fullscreen = (match[1].str() == "true");
        }

        std::optional<int> mediaId;
        if (std::regex_search(body, match, std::regex(R"("media_id"\s*:\s*(\d+))"))) {
            mediaId = std::stoi(match[1].str());
        }

        std::optional<float> posX;
        if (std::regex_search(body, match, std::regex(R"("x"\s*:\s*([+-]?\d*\.?\d+))"))) {
            posX = std::stof(match[1].str());
        }

        std::optional<float> posY;
        if (std::regex_search(body, match, std::regex(R"("y"\s*:\s*([+-]?\d*\.?\d+))"))) {
            posY = std::stof(match[1].str());
        }

        std::optional<float> sizeX;
        if (std::regex_search(body, match, std::regex(R"("width"\s*:\s*([+-]?\d*\.?\d+))"))) {
            sizeX = std::stof(match[1].str());
        }

        std::optional<float> sizeY;
        if (std::regex_search(body, match, std::regex(R"("height"\s*:\s*([+-]?\d*\.?\d+))"))) {
            sizeY = std::stof(match[1].str());
        }

        omc::ui::OverlayDto updatedOverlay = uiService.updateOverlay(
            id,
            mediaId,
            fullscreen,
            posX,
            posY,
            sizeX,
            sizeY
        );

		res.set_content(omc::json::JsonSerializer::serialize(updatedOverlay), "application/json");
	}
}