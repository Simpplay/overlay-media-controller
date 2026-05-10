#include "CategoriesApiModule.hpp"

namespace omc::server
{
	CategoriesApiModule::CategoriesApiModule(omc::event::EventBus& eventBus, omc::media::IMediaService& mediaService) :
		eventBus(eventBus), mediaService(mediaService)
	{ }

	void CategoriesApiModule::registerRoutes(httplib::Server& server)
	{

	}

	void CategoriesApiModule::handleGetAllCategories(const httplib::Request& req, httplib::Response& res)
	{

	}

	void CategoriesApiModule::handleAddCategory(const httplib::Request& req, httplib::Response& res)
	{

	}

	void CategoriesApiModule::handleGetCategoryById(const httplib::Request& req, httplib::Response& res)
	{

	}

	void CategoriesApiModule::handleDeleteCategoryById(const httplib::Request& req, httplib::Response& res)
	{

	}

	void CategoriesApiModule::handleAddMediaToCategory(const httplib::Request& req, httplib::Response& res)
	{

	}

	void CategoriesApiModule::handleRemoveMediaFromCategory(const httplib::Request& req, httplib::Response& res)
	{

	}
}