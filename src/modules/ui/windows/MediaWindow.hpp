#pragma once

#include <string>

#include "modules/ui/UiTypes.hpp"
#include "modules/ui/UiWindow.hpp"

#include "modules/media/MediaPlayer.hpp"

namespace omc::ui::window
{
	class MediaWindow : public UiWindow
	{
	public:
		MediaWindow(omc::media::MediaPlayer& player, const std::string& mediaPath)
		{
			position = { 0, 0 };
			size = { 800, 600 };
			backgroundColor = { 0, 0, 0, 255 };

			player.load(mediaPath);
			player.play();
		}

		std::unique_ptr<UiWindow> clone() const override
		{
			return std::make_unique<MediaWindow>(*this);
		}

		void buildClientDrawCommand(std::vector<omc::ui::DrawCommand>& out) override
		{
			out.push_back(omc::ui::TextCmd{ { position.x + 10.0f, position.y + 8.0f }, { 255, 255, 255, 255 }, "MediaWindow", zBase + 3 });
		}

		void update() override
		{

		}
	};
}
