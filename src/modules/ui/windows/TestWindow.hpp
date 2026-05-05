#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include <Windows.h>

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
			updateWindowInteraction();
		}

		void buildDrawCommand(std::vector<omc::ui::DrawCommand>& out) override
		{
			if (isClosed) return;

			const float titleBarHeight = 30.0f;
			const float buttonSize = 20.0f;
			const float buttonPadding = 6.0f;

			// body
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(position, size), backgroundColor, zBase });

			// title bar
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(position, { size.x, titleBarHeight }), { 35, 35, 35, 255 }, zBase + 1 });

			const float rightStart = position.x + size.x - buttonPadding - buttonSize;
			const omc::ui::Vec2 closePos{ rightStart, position.y + 5.0f };
			const omc::ui::Vec2 maxPos{ rightStart - (buttonSize + 4.0f), position.y + 5.0f };
			const omc::ui::Vec2 resizePos{ rightStart - 2.0f * (buttonSize + 4.0f), position.y + 5.0f };

			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(closePos, { buttonSize, buttonSize }), { 220, 70, 70, 255 }, zBase + 2 });
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(maxPos, { buttonSize, buttonSize }), { 70, 180, 240, 255 }, zBase + 2 });
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(resizePos, { buttonSize, buttonSize }), { 230, 180, 70, 255 }, zBase + 2 });
		}

		bool closed() const { return isClosed; }

	private:
		bool isClosed{ false };
		bool isMaximized{ false };
		bool isDragging{ false };
		bool isResizing{ false };
		bool mouseWasDown{ false };
		omc::ui::Vec2 dragOffset{};
		omc::ui::Vec2 restorePos{};
		omc::ui::Vec2 restoreSize{};
		omc::ui::Vec2 resizeAnchorMouse{};
		omc::ui::Vec2 resizeAnchorSize{};
		int zBase{ 10 };

		static bool pointInRect(const omc::ui::Vec2& p, const omc::ui::Rect& r)
		{
			return p.x >= r.position.x && p.x <= r.position.x + r.size.x
				&& p.y >= r.position.y && p.y <= r.position.y + r.size.y;
		}

		void updateWindowInteraction()
		{
			if (isClosed) return;

			POINT cursor{};
			GetCursorPos(&cursor);
			const omc::ui::Vec2 mouse{ static_cast<float>(cursor.x), static_cast<float>(cursor.y) };
			const bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			const bool justPressed = mouseDown && !mouseWasDown;
			const bool justReleased = !mouseDown && mouseWasDown;

			const float titleBarHeight = 30.0f;
			const float buttonSize = 20.0f;
			const float buttonPadding = 6.0f;
			const float rightStart = position.x + size.x - buttonPadding - buttonSize;
			const omc::ui::Rect titleRect{ position, { size.x, titleBarHeight } };
			const omc::ui::Rect closeRect{ { rightStart, position.y + 5.0f }, { buttonSize, buttonSize } };
			const omc::ui::Rect maxRect{ { rightStart - (buttonSize + 4.0f), position.y + 5.0f }, { buttonSize, buttonSize } };
			const omc::ui::Rect resizeRect{ { rightStart - 2.0f * (buttonSize + 4.0f), position.y + 5.0f }, { buttonSize, buttonSize } };

			if (justPressed) {
				if (pointInRect(mouse, closeRect)) {
					isClosed = true;
				}
				else if (pointInRect(mouse, maxRect)) {
					toggleMaximize();
				}
				else if (pointInRect(mouse, resizeRect)) {
					isResizing = true;
					resizeAnchorMouse = mouse;
					resizeAnchorSize = size;
				}
				else if (pointInRect(mouse, titleRect)) {
					isDragging = true;
					dragOffset = { mouse.x - position.x, mouse.y - position.y };
				}
			}

			if (mouseDown && isDragging && !isMaximized) {
				position = { mouse.x - dragOffset.x, mouse.y - dragOffset.y };
			}

			if (mouseDown && isResizing && !isMaximized) {
				size.x = (std::max)(180.0f, resizeAnchorSize.x + (mouse.x - resizeAnchorMouse.x));
				size.y = (std::max)(120.0f, resizeAnchorSize.y + (mouse.y - resizeAnchorMouse.y));
			}

			if (justReleased) {
				isDragging = false;
				isResizing = false;
			}

			mouseWasDown = mouseDown;
		}

		void toggleMaximize()
		{
			const float screenWidth = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
			const float screenHeight = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));

			if (!isMaximized) {
				restorePos = position;
				restoreSize = size;
				position = { 0.0f, 0.0f };
				size = { screenWidth, screenHeight };
				isMaximized = true;
			}
			else {
				position = restorePos;
				size = restoreSize;
				isMaximized = false;
			}
		}
	};
}
