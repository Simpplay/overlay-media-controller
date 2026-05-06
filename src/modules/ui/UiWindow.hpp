#pragma once

#include <vector>
#include <algorithm>

#if defined(_WIN32)
#include <Windows.h>
#endif

#include "UiTypes.hpp"

namespace omc::ui::window
{
	class UiWindow
	{
	public:
		virtual ~UiWindow() = default;

		virtual void update() = 0;
		virtual void buildDrawCommand(std::vector<DrawCommand>& out)
		{
			if (isClosed) return;

			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(position, size), backgroundColor, zBase });

			if (hasTitlebar()) {
				drawTitlebar(out);
			}

			buildClientDrawCommand(out);
		}
		virtual void buildClientDrawCommand(std::vector<DrawCommand>& out) = 0;
		virtual bool hasTitlebar() const { return true; }
		bool closed() const { return isClosed; }
		void setZBase(int z) { zBase = z; }

		Vec2 position;
		Vec2 size;
		Color backgroundColor;

	protected:
		bool isClosed{ false };
		bool isMaximized{ false };
		bool isDragging{ false };
		bool isResizing{ false };
		bool mouseWasDown{ false };
		Vec2 dragOffset{};
		Vec2 restorePos{};
		Vec2 restoreSize{};
		Vec2 resizeAnchorMouse{};
		Vec2 resizeAnchorSize{};
		int zBase{ 10 };

		static constexpr float kTitleBarHeight = 30.0f;
		static constexpr float kButtonSize = 20.0f;
		static constexpr float kButtonPadding = 6.0f;

		void updateWindowInteraction()
		{
			if (isClosed || !hasTitlebar()) return;

#if defined(_WIN32)
			POINT cursor{};
			GetCursorPos(&cursor);
			const omc::ui::Vec2 mouse{ static_cast<float>(cursor.x), static_cast<float>(cursor.y) };
			const bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			const bool justPressed = mouseDown && !mouseWasDown;
			const bool justReleased = !mouseDown && mouseWasDown;

			const float rightStart = position.x + size.x - kButtonPadding - kButtonSize;
			const omc::ui::Rect titleRect{ position, { size.x, kTitleBarHeight } };
			const omc::ui::Rect closeRect{ { rightStart, position.y + 5.0f }, { kButtonSize, kButtonSize } };
			const omc::ui::Rect maxRect{ { rightStart - (kButtonSize + 4.0f), position.y + 5.0f }, { kButtonSize, kButtonSize } };
			const omc::ui::Rect resizeRect{ { rightStart - 2.0f * (kButtonSize + 4.0f), position.y + 5.0f }, { kButtonSize, kButtonSize } };

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
#endif
		}

	private:
		static bool pointInRect(const omc::ui::Vec2& p, const omc::ui::Rect& r)
		{
			return p.x >= r.position.x && p.x <= r.position.x + r.size.x
				&& p.y >= r.position.y && p.y <= r.position.y + r.size.y;
		}

		void drawTitlebar(std::vector<DrawCommand>& out) const
		{
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(position, { size.x, kTitleBarHeight }), { 35, 35, 35, 255 }, zBase + 1 });

			const float rightStart = position.x + size.x - kButtonPadding - kButtonSize;
			const omc::ui::Vec2 closePos{ rightStart, position.y + 5.0f };
			const omc::ui::Vec2 maxPos{ rightStart - (kButtonSize + 4.0f), position.y + 5.0f };
			const omc::ui::Vec2 resizePos{ rightStart - 2.0f * (kButtonSize + 4.0f), position.y + 5.0f };

			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(closePos, { kButtonSize, kButtonSize }), { 220, 70, 70, 255 }, zBase + 2 });
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(maxPos, { kButtonSize, kButtonSize }), { 70, 180, 240, 255 }, zBase + 2 });
			out.push_back(omc::ui::RectCmd{ omc::ui::Rect(resizePos, { kButtonSize, kButtonSize }), { 230, 180, 70, 255 }, zBase + 2 });
		}

		void toggleMaximize()
		{
#if defined(_WIN32)
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
#endif
		}
	};
}
