#pragma once

#include "core/event/api/EventBus.hpp"

namespace omc::application
{
	class Updater
	{
	public:
		Updater(omc::event::EventBus& eventBus);
		bool checkForUpdates();
		void downloadAndInstallUpdates();

	private:
		omc::event::EventBus& eventBus;
	};
}