#pragma once

#include <vector>

#include "UiTypes.hpp"

namespace omc::ui::window
{
	class UiWindow
	{
	public:
		virtual ~UiWindow() = default;

		virtual void update() = 0;
		virtual void buildDrawCommand(std::vector<DrawCommand>& out) = 0;

		Vec2 position;
		Vec2 size;
		Color backgroundColor;
	};
}