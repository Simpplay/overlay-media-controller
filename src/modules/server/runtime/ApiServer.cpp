#include "ApiServer.hpp"

#include <algorithm>
#include <fstream>
#include <httplib.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "core/types/JsonSerializer.hpp"

#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

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
	// Helper – añade headers CORS permisivos.
	// Necesario cuando el HTML se sirve desde un origen distinto (file://, otro
	// puerto, etc.) y el navegador realiza peticiones cross-origin al servidor.
	// ---------------------------------------------------------------------------
	static void setCorsHeaders(httplib::Response& res)
	{
		res.set_header("Access-Control-Allow-Origin",  "*");
		res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
		res.set_header("Access-Control-Allow-Headers", "Content-Type, Range");
		res.set_header("Access-Control-Expose-Headers",
		               "Content-Length, Content-Range, Accept-Ranges");
	}

	// ---------------------------------------------------------------------------
	// Parsea el valor del header "Range: bytes=start-end".
	// Soporta las tres formas del RFC 7233:
	//   bytes=500-999     → rango cerrado
	//   bytes=500-        → desde 500 hasta el final
	//   bytes=-500        → últimos 500 bytes
	// Devuelve false si el rango es inválido o no satisfacible.
	// ---------------------------------------------------------------------------
	static bool parseRangeHeader(
		const std::string& value,
		size_t             totalSize,
		size_t&            start,
		size_t&            end)
	{
		if (!value.starts_with("bytes=") || totalSize == 0) {
			return false;
		}

		const auto spec  = value.substr(6);
		const auto dash  = spec.find('-');
		if (dash == std::string::npos) {
			return false;
		}

		const auto startToken = spec.substr(0, dash);
		const auto endToken   = spec.substr(dash + 1);

		try {
			if (startToken.empty()) {
				// suffix-length: "bytes=-N"
				const size_t suffixLen = static_cast<size_t>(std::stoull(endToken));
				if (suffixLen == 0) return false;
				start = (suffixLen >= totalSize) ? 0 : totalSize - suffixLen;
				end   = totalSize - 1;
			}
			else {
				start = static_cast<size_t>(std::stoull(startToken));
				end   = endToken.empty()
				        ? (totalSize - 1)
				        : static_cast<size_t>(std::stoull(endToken));
			}
		}
		catch (...) {
			return false;
		}

		return start <= end && end < totalSize;
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
		omc::event::EventBus&      eventBus;
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

			// ── OPTIONS preflight – CORS ───────────────────────────────────
			// Los navegadores envían un preflight OPTIONS antes de peticiones
			// cross-origin con headers personalizados (ej: Range).
			server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
				setCorsHeaders(res);
				res.status = httplib::StatusCode::NoContent_204;
				});

			// ── GET /api/media ─────────────────────────────────────────────
			// Lista todas las fuentes de media.
			server.Get("/api/media", [this](const httplib::Request&, httplib::Response& res) {
				try {
					setCorsHeaders(res);
					auto sources = mediaService.getAllMediaSources();
					res.set_content(omc::json::JsonSerializer::serialize(sources), "application/json");
				}
				catch (const std::exception& e) {
					setError(res, httplib::StatusCode::InternalServerError_500, e.what());
				}
				});

			// ── GET /media/:id ─────────────────────────────────────────────
			// Sirve el fichero binario con soporte completo de Range requests
			// (RFC 7233) para que los navegadores puedan hacer seek en vídeos.
			server.Get(R"(/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
				try {
					setCorsHeaders(res);

					const int id = extractId(req);
					auto media   = mediaService.getMediaFileById(id);

					if (!media) {
						setError(res, httplib::StatusCode::NotFound_404, "Media source not found");
						return;
					}

					// ── Abrir el fichero UNA vez ───────────────────────────
					const std::string filePath = media->filepath;
					auto streamPtr = std::make_shared<std::ifstream>(
					    filePath, std::ios::binary | std::ios::ate);

					if (!*streamPtr) {
						setError(res, httplib::StatusCode::NotFound_404, "Media file not found");
						return;
					}

					const size_t totalSize = static_cast<size_t>(streamPtr->tellg());
					// Volver al inicio; la posición concreta se fijará en el provider.
					streamPtr->seekg(0, std::ios::beg);

					// ── MIME type ──────────────────────────────────────────
					const std::string contentType =
					    inferMimeType(media->filename, media->contentType);

					// ── Headers generales ──────────────────────────────────
					res.set_header("Accept-Ranges", "bytes");

					// ── Calcular rango ─────────────────────────────────────
					size_t start   = 0;
					size_t end     = totalSize > 0 ? totalSize - 1 : 0;
					bool   partial = false;

					const auto rangeHeader = req.get_header_value("Range");
					if (!rangeHeader.empty()) {
						if (!parseRangeHeader(rangeHeader, totalSize, start, end)) {
							res.status = httplib::StatusCode::RangeNotSatisfiable_416;
							res.set_header("Content-Range",
							               "bytes */" + std::to_string(totalSize));
							return;
						}
						partial = true;
					}

					const size_t chunkSize =
					    (totalSize == 0) ? 0 : (end - start + 1);

					res.status = partial
					    ? httplib::StatusCode::PartialContent_206
					    : httplib::StatusCode::OK_200;

					if (partial) {
						res.set_header("Content-Range",
						    "bytes " + std::to_string(start) +
						    "-"      + std::to_string(end)   +
						    "/"      + std::to_string(totalSize));
					}

					// ── Content provider ───────────────────────────────────
					streamPtr->seekg(0, std::ios::end);
					const size_t totalFileSize = static_cast<size_t>(streamPtr->tellg());
					streamPtr->seekg(0, std::ios::beg);

					constexpr size_t BUFFER_SIZE = 64 * 1024;
					auto bufferPtr = std::make_shared<std::vector<char>>(BUFFER_SIZE);

					res.set_content_provider(
						totalFileSize,
						contentType,
						[streamPtr, bufferPtr](size_t offset, size_t length, httplib::DataSink& sink) -> bool
						{
							streamPtr->seekg(static_cast<std::streamoff>(offset));

							const size_t toRead = std::min(length, size_t{ BUFFER_SIZE });

							streamPtr->read(bufferPtr->data(), static_cast<std::streamsize>(toRead));
							const size_t bytesRead = static_cast<size_t>(streamPtr->gcount());

							if (bytesRead == 0) {
								return false; // Archivo terminado o error de disco
							}

							return sink.write(bufferPtr->data(), bytesRead);
						});
				}
				catch (const std::invalid_argument&) {
					setError(res, httplib::StatusCode::BadRequest_400, "Invalid ID format");
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
			server.Post("/api/media", [this](const httplib::Request& req, httplib::Response& res) {
				setCorsHeaders(res);

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
					setCorsHeaders(res);
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
					setCorsHeaders(res);
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
			server.Get(R"(/api/media/(\d+)/show)", [this](const httplib::Request& req, httplib::Response& res) {
				try {
					setCorsHeaders(res);
					const int  id = extractId(req);

					const bool fullscreen = req.has_param("fullscreen") ? req.get_param_value("fullscreen") == "true" : false;
					const std::string position = req.has_param("position") ? req.get_param_value("position") : "0,0";
					const std::string size = req.has_param("size") ? req.get_param_value("size") : "960,640";

					const float posX = std::stoi(position.substr(0, position.find(',')));
					const float posY = std::stoi(position.substr(position.find(',') + 1));
					const float sizeX = std::stoi(size.substr(0, size.find(',')));
					const float sizeY = std::stoi(size.substr(size.find(',') + 1));

					auto event = omc::event::PlayMediaRequestedEvent(
						id, fullscreen, posX, posY, sizeX, sizeY
					);

					eventBus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(event));

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
