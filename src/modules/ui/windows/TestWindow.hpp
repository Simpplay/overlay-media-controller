#pragma once

#include <vector>
#include <random>

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

		TestWindow()
		{
			static std::mt19937 rng(std::random_device{}());

			std::uniform_int_distribution<int> posX(50, 449);
			std::uniform_int_distribution<int> posY(50, 349);
			std::uniform_int_distribution<int> color(0, 255);
			std::uniform_int_distribution<int> sizeX(100, 299);
			std::uniform_int_distribution<int> sizeY(75, 224);

			position.x = posX(rng);
			position.y = posY(rng);

			backgroundColor.r = color(rng);
			backgroundColor.g = color(rng);
			backgroundColor.b = color(rng);

			size.x = sizeX(rng);
			size.y = sizeY(rng);
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