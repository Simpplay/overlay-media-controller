#pragma once

#include "IApiModule.hpp"

#include "core/event/api/EventBus.hpp"
#include "modules/soundboard/api/ISoundBoardService.hpp"

namespace omc::server
{
	class SoundboardApiModule : public IApiModule
	{
	public:
		SoundboardApiModule(omc::event::EventBus& eventBus, omc::soundboard::ISoundBoardService& soundboardService);
		void registerRoutes(httplib::Server& server) override;

	private:
		omc::event::EventBus& eventBus;
		omc::soundboard::ISoundBoardService& soundboardService;

	private:
		void handlePlaySound(const httplib::Request& req, httplib::Response& res);
		void handleGetAudioDevices(const httplib::Request& req, httplib::Response& res);
		void handleSetAudioDevice(const httplib::Request& req, httplib::Response& res);
	};
}