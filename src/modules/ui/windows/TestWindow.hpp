#pragma once

#include <vector>
#include <stdlib.h>
#include <time.h>

#include "modules/ui/UiWindow.hpp"
#include "modules/ui/UiTypes.hpp"

namespace omc::ui::window
{
	class TestWindow : public UiWindow
	{
	public:
		omc::ui::Vec2 position;
		omc::ui::Color backgroundColor;
		omc::ui::Vec2 size;
		

		TestWindow() : backgroundColor({ 255, 0, 0, 128 }), size({ 200, 150 }), position({ 100, 100 })
		{
			srand(time(NULL));

			position.x = static_cast<float>(rand() % 400 + 50);
			position.y = static_cast<float>(rand() % 300 + 50);

			backgroundColor.r = static_cast<uint8_t>(rand() % 256);
			backgroundColor.g = static_cast<uint8_t>(rand() % 256);
			backgroundColor.b = static_cast<uint8_t>(rand() % 256);

			size.x = static_cast<float>(rand() % 200 + 100);
			size.y = static_cast<float>(rand() % 150 + 75);
		}

		void update() override
		{
			
		}

		void buildDrawCommand(std::vector<omc::ui::DrawCommand>& out) override
		{
			omc::ui::DrawCommand cmd = omc::ui::RectCmd{ omc::ui::Rect(position, size), backgroundColor };

			out.push_back(cmd);
		}
	};
}