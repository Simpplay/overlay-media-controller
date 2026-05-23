#include "Updater.hpp"

#include "core/updater/api/events/RequestUpdateEvent.hpp"

namespace omc::application
{
	Updater::Updater(omc::event::EventBus& eventBus) : eventBus(eventBus)
	{
		eventBus.subscribe<omc::event::RequestUpdateEvent>([this](const omc::event::RequestUpdateEvent& event) {
			downloadAndInstallUpdates();
		});
	}

	bool Updater::checkForUpdates()
	{
		return false;
	}

	void Updater::downloadAndInstallUpdates()
	{
		// Placeholder implementation
	}
}