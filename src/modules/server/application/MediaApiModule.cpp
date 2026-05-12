#include "MediaApiModule.hpp"

#include <filesystem>

#include "HttpUtils.hpp"
#include <nlohmann/json.hpp>

namespace omc::server
{
	MediaApiModule::MediaApiModule(omc::event::EventBus& eventBus, omc::media::IMediaService& mediaService) :
		eventBus(eventBus), mediaService(mediaService)
	{ }

	void MediaApiModule::registerRoutes(httplib::Server& server)
	{
		// ── GET /api/media ─────────────────────────────────────────────
		server.Get("/api/media", [this](const httplib::Request& req, httplib::Response& res) {
			handleGetAllMedia(req, res);
		});

		// ── POST /api/media ────────────────────────────────────────────
		server.Post("/api/media", [this](const httplib::Request& req, httplib::Response& res) {
			handleAddMedia(req, res);
		});

		// ── GET /api/media/:id ─────────────────────────────────────────
		server.Get(R"(/api/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleGetMediaById(req, res);
		});

		// ── DELETE /api/media/:id ──────────────────────────────────────
		server.Delete(R"(/api/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleDeleteMediaById(req, res);
		});

		// ── PATCH /api/media/:id ────────────────────────────────────────
		server.Patch(R"(/api/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleUpdateMedia(req, res);
		});

		// ── GET /media/:id ─────────────────────────────────────────────
		server.Get(R"(/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
			handleShowMedia(req, res);
		});

		// ── GET /api/media/:id/thumbnail ─────────────────────────────────────────────
		server.Get(R"(/api/media/(\d+)/thumbnail)", [this](const httplib::Request& req, httplib::Response& res) {
			handleShowThumbnail(req, res);
		});
	}

	// Helpers

	std::string MediaApiModule::extractTitleFromRequest(const httplib::Request& req)
	{
		std::string fileName;
		if (req.form.has_field("title")) {
			const auto& title = req.form.get_field("title");

			if (fileExtension(title).empty()) {
				if (req.form.has_file("media")) {
					const std::string ext = fileExtension(req.form.get_file("media").filename);
					fileName = title + ext;
				}
				else {
					fileName = title;
				}
			}
		}
		else if (req.form.has_file("media")) {
			fileName = req.form.get_file("media").filename;
		}
		else {
			fileName = "";
			auto body = nlohmann::json::parse(req.body);
			if (body.contains("title") && body["title"].is_string() && !body["title"].get<std::string>().empty()) {
				fileName = body["title"].get<std::string>();
			}
		}

		return httplib::sanitize_filename(fileName);
	}

	void MediaApiModule::handleGetAllMedia(const httplib::Request& req, httplib::Response& res)
	{
		try {
			omc::media::SearchMediaDto searchDto;

			if (req.has_param("query"))
				searchDto.query = req.get_param_value("query");

			if (req.has_param("category"))
				searchDto.category = req.get_param_value("category");

			auto sources = mediaService.getAllMediaSources(searchDto);
			res.set_content(omc::json::JsonSerializer::serialize(sources), "application/json");
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void MediaApiModule::handleAddMedia(const httplib::Request& req, httplib::Response& res)
	{
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
		const auto safeName = extractTitleFromRequest(req);
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
	}

	void MediaApiModule::handleShowMedia(const httplib::Request& req, httplib::Response& res)
	{
		try {
			const int id = extractId(req);
			auto media = mediaService.getMediaFileById(id);

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
			size_t start = 0;
			size_t end = totalSize > 0 ? totalSize - 1 : 0;
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
					"-" + std::to_string(end) +
					"/" + std::to_string(totalSize));
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
	}

	void MediaApiModule::handleGetMediaById(const httplib::Request& req, httplib::Response& res)
	{
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
	}

	void MediaApiModule::handleDeleteMediaById(const httplib::Request& req, httplib::Response& res)
	{
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
	}

	void MediaApiModule::handleUpdateMedia(const httplib::Request& req, httplib::Response& res)
	{
		try {
			const int id = extractId(req);
			omc::media::UpdateMediaDto dto;

			dto.id = id;

			std::string newTitle = extractTitleFromRequest(req);
			std::cout << "Extracted title: '" << newTitle << "'" << std::endl;
			if (!newTitle.empty()) {
				std::cout << "Updating title to: '" << newTitle << "'" << std::endl;
				dto.title = newTitle;
			}

			if (!mediaService.updateMediaSource(dto)) {
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
	}

	void MediaApiModule::handleShowThumbnail(const httplib::Request& req, httplib::Response& res)
	{
		try {
			const int id = extractId(req);
			auto media = mediaService.getMediaFileById(id);

			if (!media.has_value() || media->thumbnailPath.empty()) {
				setError(
					res,
					httplib::StatusCode::NotFound_404,
					"Thumbnail not found");

				return;
			}

			// To absolute path
			std::filesystem::path absPath = std::filesystem::absolute(media->thumbnailPath);
			
			res.set_file_content(absPath.string(), "image/png");
		}
		catch (const std::invalid_argument&) {
			setError(
				res,
				httplib::StatusCode::BadRequest_400,
				"Invalid ID format");
		}
		catch (const std::exception& e) {
			setError(
				res,
				httplib::StatusCode::InternalServerError_500,
				e.what());
		}
	}
}
