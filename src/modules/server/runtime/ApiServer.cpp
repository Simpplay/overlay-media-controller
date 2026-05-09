#include "ApiServer.hpp"

#include <algorithm>
#include <httplib.h>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "core/types/JsonSerializer.hpp"

namespace omc::server
{
	// ---------------------------------------------------------------------------
	// Helper – extrae el primer capture group de la ruta y lo convierte a int.
	// Lanza std::invalid_argument si el match no existe o no es numérico.
	// ---------------------------------------------------------------------------
	static int extractId(const httplib::Request& req, std::size_t matchIndex = 1)
	{
		if (matchIndex >= req.matches.size())
			throw std::invalid_argument("Missing path parameter");

		return std::stoi(req.matches[matchIndex].str());
	}

	// ---------------------------------------------------------------------------
	// Helper – convierte el std::string del body multipart a span<const byte>
	// sin copias de memoria innecesarias.
	// ---------------------------------------------------------------------------
	static std::span<const std::byte> toByteSpan(const std::string& s) noexcept
	{
		return { reinterpret_cast<const std::byte*>(s.data()), s.size() };
	}

	// ---------------------------------------------------------------------------
	// Helper – respuesta de error homogénea
	// ---------------------------------------------------------------------------
	static void setError(httplib::Response& res, int status, std::string_view message)
	{
		res.status = status;
		res.set_content(
			std::string("{\"error\":\"") + std::string(message) + "\"}",
			"application/json"
		);
	}

	// ---------------------------------------------------------------------------
	// Helper – devuelve la extensión (con punto, en minúsculas) de un filename,
	// o cadena vacía si no tiene extensión.
	// ---------------------------------------------------------------------------
	static std::string fileExtension(const std::string& filename)
	{
		const auto dot = filename.rfind('.');
		if (dot == std::string::npos || dot + 1 == filename.size())
			return {};

		std::string ext = filename.substr(dot); // ".MP4", ".Mov", …
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return ext;
	}

	// ---------------------------------------------------------------------------
	// Helper – infiere el MIME type a partir de la extensión del fichero cuando
	// el cliente reporta el tipo genérico application/octet-stream (o vacío).
	// Solo sobreescribe el tipo declarado si éste es demasiado genérico para ser
	// útil; cualquier tipo específico que venga del cliente se respeta tal cual.
	// ---------------------------------------------------------------------------
	static std::string inferMimeType(const std::string& filename,
		const std::string& declared)
	{
		constexpr std::string_view kGeneric = "application/octet-stream";

		if (!declared.empty() && declared != kGeneric)
			return declared; // el cliente ya mandó algo concreto, no lo tocamos

		static const std::unordered_map<std::string, std::string> kMimeMap{
			// Video
			{ ".mp4",  "video/mp4"            },
			{ ".webm", "video/webm"           },
			{ ".mov",  "video/quicktime"      },
			{ ".avi",  "video/x-msvideo"      },
			{ ".mkv",  "video/x-matroska"     },
			{ ".flv",  "video/x-flv"          },
			{ ".wmv",  "video/x-ms-wmv"       },
			// Audio
			{ ".mp3",  "audio/mpeg"           },
			{ ".wav",  "audio/wav"            },
			{ ".ogg",  "audio/ogg"            },
			{ ".aac",  "audio/aac"            },
			{ ".flac", "audio/flac"           },
			{ ".m4a",  "audio/mp4"            },
			// Imagen
			{ ".png",  "image/png"            },
			{ ".jpg",  "image/jpeg"           },
			{ ".jpeg", "image/jpeg"           },
			{ ".gif",  "image/gif"            },
			{ ".webp", "image/webp"           },
			{ ".svg",  "image/svg+xml"        },
			{ ".bmp",  "image/bmp"            },
			// Documento
			{ ".pdf",  "application/pdf"      },
		};

		const std::string ext = fileExtension(filename);
		if (const auto it = kMimeMap.find(ext); it != kMimeMap.end())
			return it->second;

		// Sin mejor información, dejamos el tipo genérico tal como estaba.
		return declared.empty() ? std::string(kGeneric) : declared;
	}

	// =========================================================================
	// Impl
	// =========================================================================
	struct ApiServer::Impl
	{
		httplib::Server            server;
		omc::event::EventBus& eventBus;
		omc::media::IMediaService& mediaService;
		int                        port{ 0 };
		std::thread                serverThread;

		Impl(int port,
			omc::event::EventBus& eventBus,
			omc::media::IMediaService& mediaService)
			: port(port), eventBus(eventBus), mediaService(mediaService)
		{
			registerEndpoints();
		}

		void listen()
		{
			if (!server.listen("0.0.0.0", port))
				throw std::runtime_error("ApiServer: failed to bind port " + std::to_string(port));
		}

		void stop()
		{
			server.stop();
			if (serverThread.joinable())
				serverThread.join();
		}

		// ------------------------------------------------------------------
		void registerEndpoints()
		{
			// ── Health check ──────────────────────────────────────────────
			server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
				res.set_content("{\"status\":\"ok\"}", "application/json");
				});

			// ── GET /api/media ─────────────────────────────────────────────
			// Lista todas las fuentes de media.
			server.Get("/api/media", [this](const httplib::Request&, httplib::Response& res) {
				try {
					auto sources = mediaService.getAllMediaSources();
					res.set_content(omc::json::JsonSerializer::serialize(sources), "application/json");
				}
				catch (const std::exception& e) {
					setError(res, httplib::StatusCode::InternalServerError_500, e.what());
				}
				});

			// ── POST /api/media ────────────────────────────────────────────
			// Añade una nueva fuente de media.
			// Content-type esperado: multipart/form-data
			//   campo "media"  → archivo binario (obligatorio)
			//   campo "title"  → nombre descriptivo (opcional; fallback al nombre de fichero)
			//
			// Correcciones aplicadas:
			//   · El MIME type se infiere desde la extensión del fichero original
			//     cuando el cliente reporta application/octet-stream o lo omite.
			//   · Si se proporciona un "title" sin extensión, se le añade
			//     automáticamente la extensión del fichero original para que el
			//     servicio pueda determinar el formato correctamente.
			server.Post("/api/media", [this](const httplib::Request& req, httplib::Response& res) {
				if (!req.is_multipart_form_data()) {
					setError(res, httplib::StatusCode::BadRequest_400, "Expected multipart/form-data");
					return;
				}

				if (!req.form.has_file("media")) {
					setError(res, httplib::StatusCode::BadRequest_400, "Missing 'media' file field");
					return;
				}

				const auto& file = req.form.get_file("media");

				// ── Nombre final del recurso ───────────────────────────────
				// Si el usuario especifica un título personalizado pero no incluye
				// extensión, heredamos la extensión del fichero original para no
				// perder información de formato.
				std::string rawName;
				if (req.form.has_field("title")) {
					rawName = req.form.get_field("title");

					// Añadir la extensión original solo si el título carece de ella.
					if (fileExtension(rawName).empty()) {
						const std::string ext = fileExtension(file.filename);
						if (!ext.empty())
							rawName += ext;
					}
				}
				else {
					rawName = file.filename;
				}

				const auto safeName = httplib::sanitize_filename(rawName);
				if (safeName.empty()) {
					setError(res, httplib::StatusCode::BadRequest_400, "Invalid filename");
					return;
				}

				// ── MIME type ─────────────────────────────────────────────
				// Algunos clientes (curl, navegadores con ficheros desconocidos)
				// envían application/octet-stream aunque el fichero sea un .mp4.
				// Inferimos el tipo real desde la extensión del fichero original.
				const std::string mimeType = inferMimeType(file.filename, file.content_type);

				try {
					auto dto = mediaService.addMediaSource(
						toByteSpan(file.content),
						safeName,
						mimeType
					);
					res.status = httplib::StatusCode::Created_201;
					res.set_content(omc::json::JsonSerializer::serialize(dto), "application/json");
				}
				catch (const std::exception& e) {
					setError(res, httplib::StatusCode::InternalServerError_500, e.what());
				}
				});

			// ── GET /api/media/:id ─────────────────────────────────────────
			// Devuelve una fuente de media por su ID.
			server.Get(R"(/api/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
				try {
					const int id = extractId(req);
					auto      dto = mediaService.getMediaSourceById(id);

					if (!dto) {
						setError(res, httplib::StatusCode::NotFound_404, "Media source not found");
						return;
					}
					res.set_content(omc::json::JsonSerializer::serialize(*dto), "application/json");
				}
				catch (const std::invalid_argument&) {
					setError(res, httplib::StatusCode::BadRequest_400, "Invalid ID format");
				}
				catch (const std::exception& e) {
					setError(res, httplib::StatusCode::InternalServerError_500, e.what());
				}
				});

			// ── DELETE /api/media/:id ──────────────────────────────────────
			// Elimina una fuente de media por su ID.
			server.Delete(R"(/api/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
				try {
					const int  id = extractId(req);
					const bool deleted = mediaService.deleteMediaSourceById(id);

					if (!deleted) {
						setError(res, httplib::StatusCode::NotFound_404, "Media source not found");
						return;
					}
					res.status = httplib::StatusCode::OK_200;
				}
				catch (const std::invalid_argument&) {
					setError(res, httplib::StatusCode::BadRequest_400, "Invalid ID format");
				}
				catch (const std::exception& e) {
					setError(res, httplib::StatusCode::InternalServerError_500, e.what());
				}
				});

			// ── GET /api/media/:id/show ────────────────────────────────────
			// Muestra la fuente de media en el overlay.
			// NOTA: el segmento "show" debe matchearse *antes* de la ruta genérica
			//       /api/media/(\d+) para evitar ambigüedades. cpp-httplib respeta
			//       el orden de registro, por lo que este handler debe registrarse
			//       DESPUÉS del handler genérico de :id (más específico primero).
			server.Get(R"(/api/media/(\d+)/show)", [this](const httplib::Request& req, httplib::Response& res) {
				try {
					const int  id = extractId(req);
					// const bool shown = mediaService.showMediaSourceInOverlay(id);
					const bool shown = false; // TODO: implementar esta función en MediaService

					if (!shown) {
						setError(res, httplib::StatusCode::NotFound_404, "Media source not found");
						return;
					}
					res.set_content("{\"status\":\"shown\"}", "application/json");
				}
				catch (const std::invalid_argument&) {
					setError(res, httplib::StatusCode::BadRequest_400, "Invalid ID format");
				}
				catch (const std::exception& e) {
					setError(res, httplib::StatusCode::InternalServerError_500, e.what());
				}
				});
		}
	};

	// =========================================================================
	// ApiServer – public interface
	// =========================================================================
	ApiServer::ApiServer() = default;
	ApiServer::~ApiServer() = default;

	ApiServer::ApiServer(ApiServer&&) noexcept = default;
	ApiServer& ApiServer::operator=(ApiServer&&) noexcept = default;

	void ApiServer::start(int port,
		omc::event::EventBus& eventBus,
		omc::media::IMediaService& mediaService)
	{
		if (impl_)
			throw std::logic_error("ApiServer is already running");

		impl_ = std::make_unique<Impl>(port, eventBus, mediaService);
		impl_->listen();
	}

	void ApiServer::stop()
	{
		if (impl_) {
			impl_->stop();
			impl_.reset();
		}
	}
}