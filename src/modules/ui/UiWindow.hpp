#pragma once

namespace omc::ui
{
	class UiWindow
	{
	public:
		virtual ~UiWindow() = default;

		virtual void render() = 0;
		virtual void update() = 0;
	};
}