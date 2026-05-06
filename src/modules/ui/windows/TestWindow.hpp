#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include "modules/ui/UiTypes.hpp"
#include "modules/ui/UiWindow.hpp"

namespace omc::ui::window
{
	class TestWindow : public UiWindow
	{
	public:
		TestWindow()
		{
			static std::mt19937 rng(std::random_device{}());

			std::uniform_int_distribution<int> posX(50, 449);
			std::uniform_int_distribution<int> posY(50, 349);
			std::uniform_int_distribution<int> color(0, 255);
			std::uniform_int_distribution<int> sizeX(200, 399);
			std::uniform_int_distribution<int> sizeY(140, 299);

			position = { static_cast<float>(posX(rng)), static_cast<float>(posY(rng)) };
			size = { static_cast<float>(sizeX(rng)), static_cast<float>(sizeY(rng)) };
			backgroundColor = { static_cast<uint8_t>(color(rng)), static_cast<uint8_t>(color(rng)), static_cast<uint8_t>(color(rng)), 235 };
		}

		void update() override 
		{

		}

		std::unique_ptr<UiWindow> clone() const override
		{
			return std::make_unique<TestWindow>(*this);
		}

		void buildClientDrawCommand(std::vector<omc::ui::DrawCommand>& out) override
		{
			out.push_back(omc::ui::TextCmd{ { position.x + 10.0f, position.y + 8.0f }, { 255, 255, 255, 255 }, "TestWindow", zBase + 3 });
		}
	};
}
