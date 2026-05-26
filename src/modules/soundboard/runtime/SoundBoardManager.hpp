#pragma once

#include "core/event/api/EventBus.hpp"

#include "modules/soundboard/api/events/PlaySoundBoardRequestedEvent.hpp"

namespace omc::soundboard
{
	class SoundBoardManager
	{
	public:
		SoundBoardManager(omc::event::EventBus& eventBus);
		~SoundBoardManager() = default;

	private:
		void onPlaySoundRequested(const omc::event::PlaySoundBoardRequestedEvent& event);

	private:
		omc::event::EventBus& eventBus;
	};
}