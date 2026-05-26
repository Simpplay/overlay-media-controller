#include "SoundBoardManager.hpp"

namespace omc::soundboard
{
	SoundBoardManager::SoundBoardManager(omc::event::EventBus& eventBus) : eventBus(eventBus)
	{
		eventBus.subscribe<omc::event::PlaySoundBoardRequestedEvent>([this](const omc::event::PlaySoundBoardRequestedEvent& event) {
			onPlaySoundRequested(event);
		});
	}

	void SoundBoardManager::onPlaySoundRequested(const omc::event::PlaySoundBoardRequestedEvent& event)
	{

	}
}