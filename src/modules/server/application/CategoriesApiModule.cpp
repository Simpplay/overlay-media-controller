#include "CategoriesApiModule.hpp"
#include <string>
#include <regex>
#include "HttpUtils.hpp"

namespace omc::server
{
    CategoriesApiModule::CategoriesApiModule(omc::event::EventBus& eventBus, omc::media::IMediaService& mediaService) :
        eventBus(eventBus), mediaService(mediaService)
    {
    }

    void CategoriesApiModule::registerRoutes(httplib::Server& server)
    {
        server.Get(R"(/api/categories)", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetAllCategories(req, res);
            });

        server.Post(R"(/api/categories)", [this](const httplib::Request& req, httplib::Response& res) {
            handleAddCategory(req, res);
            });

        server.Get(R"(/api/categories/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetCategoryById(req, res);
            });

        server.Delete(R"(/api/categories/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleDeleteCategoryById(req, res);
            });

        server.Put(R"(/api/categories/(\d+)/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleAddMediaToCategory(req, res);
            });

        server.Delete(R"(/api/categories/(\d+)/media/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            handleRemoveMediaFromCategory(req, res);
            });
    }

    void CategoriesApiModule::handleGetAllCategories(const httplib::Request& req, httplib::Response& res)
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
            std::smatch match;
            if (!std::regex_search(req.body, match, std::regex(R"("name"\s*:\s*"([^"]+)")"))) {
                setError(res, httplib::StatusCode::BadRequest_400, "Missing or invalid 'name' parameter");
            return;
        }

        std::string name = match[1].str();
        if (mediaService.createCategory(name)) {
            res.status = httplib::StatusCode::Created_201;
            res.set_content("{\"status\":\"created\"}", "application/json");
        }
        else {
            setError(res, httplib::StatusCode::InternalServerError_500, "Failed to create category");
        }
    }
    catch (const std::exception& e) {
        setError(res, httplib::StatusCode::InternalServerError_500, e.what());
    }
}

void CategoriesApiModule::handleGetCategoryById(const httplib::Request& req, httplib::Response& res)
{
    try {
        int id = extractId(req); // Usando la utilidad vista en Overlays
        auto category = mediaService.getCategoryById(id);

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
        if (req.matches.size() < 3) {
            setError(res, httplib::StatusCode::BadRequest_400, "Invalid parameters");
            return;
        }

        int categoryId = std::stoi(req.matches[1].str());
        int mediaId = std::stoi(req.matches[2].str());

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
        if (req.matches.size() < 3) {
            setError(res, httplib::StatusCode::BadRequest_400, "Invalid parameters");
            return;
        }

        int categoryId = std::stoi(req.matches[1].str());
        int mediaId = std::stoi(req.matches[2].str());

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