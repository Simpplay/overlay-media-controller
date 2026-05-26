#include "SoundboardApiModule.hpp"

#include <nlohmann/json.hpp>
#include "HttpUtils.hpp"

#include "modules/soundboard/api/SoundboardDto.hpp"
#include "modules/soundboard/api/events/PlaySoundBoardRequestedEvent.hpp"

namespace omc::server
{
	SoundboardApiModule::SoundboardApiModule(omc::event::EventBus& eventBus, omc::soundboard::ISoundBoardService& soundboardService) :
		eventBus(eventBus), soundboardService(soundboardService)
	{
	}

	void SoundboardApiModule::registerRoutes(httplib::Server& server)
	{
		server.Post(R"(/api/soundboard)", [this](const httplib::Request& req, httplib::Response& res) { handlePlaySound(req, res); });
		server.Get(R"(/api/soundboard/devices)", [this](const httplib::Request& req, httplib::Response& res) { handleGetAudioDevices(req, res); });
		server.Put(R"(/api/soundboard/devices)", [this](const httplib::Request& req, httplib::Response& res) { handleSetAudioDevice(req, res); });
	}

	void SoundboardApiModule::handlePlaySound(const httplib::Request& req, httplib::Response& res)
	{
		try {
			auto body = nlohmann::json::parse(req.body);
			if (!body.contains("media_id") || !body["media_id"].is_number_integer()) {
				setError(res, httplib::StatusCode::BadRequest_400, "Missing media_id parameter in JSON");
				return;
			}

			const int   id     = body["media_id"].get<int>();
			const float volume = body.value("volume", 100.0f);
			const bool  force  = body.value("force", false);

			eventBus.post(std::make_unique<omc::event::PlaySoundBoardRequestedEvent>(id, volume, force));
			res.set_content("{\"status\":\"shown\"}", "application/json");
		}
		catch (const nlohmann::json::exception& e) {
			setError(res, httplib::StatusCode::BadRequest_400, e.what());
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void SoundboardApiModule::handleGetAudioDevices(const httplib::Request& req, httplib::Response& res)
	{
		try {
			auto sources = soundboardService.getAudioDevices();
			res.set_content(omc::json::JsonSerializer::serialize(sources), "application/json");
		}
		catch (const std::exception& e) {
			setError(res, httplib::StatusCode::InternalServerError_500, e.what());
		}
	}

	void SoundboardApiModule::handleSetAudioDevice(const httplib::Request& req, httplib::Response& res)
	{
		auto body = nlohmann::json::parse(req.body);
		if (!body.contains("device_id") || !body["device_id"].is_number_integer()) {
			setError(res, httplib::StatusCode::BadRequest_400, "Missing device_id parameter in JSON");
			return;
		}

		const int id = body["device_id"].get<int>();
		omc::soundboard::SetInputDeviceDto dto{ .device_id = id };

		bool result = soundboardService.setAudioDevice(dto);
		if (result) {
			res.set_content("{\"status\":\"success\"}", "application/json");
		}
		else {
			setError(res, httplib::StatusCode::InternalServerError_500, "Failed to set audio device");
		}
	}
}