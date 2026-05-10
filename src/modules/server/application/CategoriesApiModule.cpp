#include "CategoriesApiModule.hpp"

#include <nlohmann/json.hpp>

#include "HttpUtils.hpp"

namespace omc::server
{
	CategoriesApiModule::CategoriesApiModule(omc::event::EventBus& eventBus, omc::media::IMediaService& mediaService) :
		eventBus(eventBus), mediaService(mediaService)
	{ }

	void CategoriesApiModule::registerRoutes(httplib::Server& server)
	{
		server.Get(R"(/api/categories)", [this](const httplib::Request& req, httplib::Response& res) { handleGetAllCategories(req, res); });
		server.Post(R"(/api/categories)", [this](const httplib::Request& req, httplib::Response& res) { handleAddCategory(req, res); });
		server.Get(R"(/api/categories/(\d+))", [this](const httplib::Request& req, httplib::Response& res) { handleGetCategoryById(req, res); });
		server.Delete(R"(/api/categories/(\d+))", [this](const httplib::Request& req, httplib::Response& res) { handleDeleteCategoryById(req, res); });
		server.Put(R"(/api/categories/(\d+)/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) { handleAddMediaToCategory(req, res); });
		server.Delete(R"(/api/categories/(\d+)/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) { handleRemoveMediaFromCategory(req, res); });
	}

	void CategoriesApiModule::handleGetAllCategories(const httplib::Request&, httplib::Response& res)
	{
		try {
			auto categories = mediaService.getAllCategories();
			res.set_content(omc::json::JsonSerializer::serialize(categories), "application/json");
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void CategoriesApiModule::handleAddCategory(const httplib::Request& req, httplib::Response& res)
	{
		try {
			auto body = nlohmann::json::parse(req.body);
			if (!body.contains("name") || !body["name"].is_string() || body["name"].get<std::string>().empty()) {
				setError(res, httplib::StatusCode::BadRequest_400, "Missing or invalid 'name' parameter");
				return;
			}

			std::string name = body["name"].get<std::string>();
			if (mediaService.createCategory(name)) {
				res.status = httplib::StatusCode::Created_201;
				res.set_content("{\"status\":\"created\"}", "application/json");
			}
			else {
				setError(res, httplib::StatusCode::InternalServerError_500, "Failed to create category");
			}
		}
		catch (const nlohmann::json::exception& e) {
			setError(res, httplib::StatusCode::BadRequest_400, e.what());
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void CategoriesApiModule::handleGetCategoryById(const httplib::Request& req, httplib::Response& res)
	{
		try {
			auto category = mediaService.getCategoryById(extractId(req));
			if (!category) {
				setError(res, httplib::StatusCode::NotFound_404, "Category not found");
				return;
			}
			res.set_content(omc::json::JsonSerializer::serialize(*category), "application/json");
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void CategoriesApiModule::handleDeleteCategoryById(const httplib::Request& req, httplib::Response& res)
	{
		try {
			if (mediaService.deleteCategoryById(extractId(req))) {
				res.set_content("{\"status\":\"deleted\"}", "application/json");
			}
			else {
				setError(res, httplib::StatusCode::NotFound_404, "Category not found or could not be deleted");
			}
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void CategoriesApiModule::handleAddMediaToCategory(const httplib::Request& req, httplib::Response& res)
	{
		try {
			int categoryId = extractId(req, 1);
			int mediaId = extractId(req, 2);
			if (mediaService.addMediaToCategory(mediaId, categoryId)) {
				res.set_content("{\"status\":\"linked\"}", "application/json");
			}
			else {
				setError(res, httplib::StatusCode::BadRequest_400, "Could not add media to category");
			}
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void CategoriesApiModule::handleRemoveMediaFromCategory(const httplib::Request& req, httplib::Response& res)
	{
		try {
			int categoryId = extractId(req, 1);
			int mediaId = extractId(req, 2);
			if (mediaService.removeMediaFromCategory(mediaId, categoryId)) {
				res.set_content("{\"status\":\"unlinked\"}", "application/json");
			}
			else {
				setError(res, httplib::StatusCode::BadRequest_400, "Could not remove media from category");
			}
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}
}
