#pragma once

#include <memory>

#include "core/event/EventBus.hpp"
#include "modules/ui/UiManager.hpp"
#include "modules/media/MediaManager.hpp"
#include "modules/media/MediaPlayer.hpp"

namespace omc::application
{
	class OverlayMediaController
	{
	public:
		void initialize();
		void close();

	private:
		bool running{ false };

		omc::event::EventBus eventBus;

		omc::ui::UiManager uiManager{ eventBus };
		std::shared_ptr<omc::media::MediaPlayer> mediaPlayer;
		std::unique_ptr<omc::media::MediaManager> mediaManager;
	};
}