#pragma once

#include <SDL3/SDL.h>

#include "modules/ui/UiTypes.hpp"

#include "core/event/EventBus.hpp"
#include "core/event/events/ExitApplicationRequestedEvent.hpp"

#include <functional>
#include <vector>

#if defined(SDL_PLATFORM_WIN32)
#include <wtypes.h>
#include <windowsx.h>

namespace omc::infra::win32
{
	HWND getNativeWindowHandle(SDL_Window* window);

	void hookWindowProc(
		HWND hwnd,
		std::function<bool(POINT)> isInteractiveArea
	);

	void unhookWindowProc(HWND hwnd);
}
#endif

namespace omc::infra
{
	class SDLRenderer
	{
	public:
		SDLRenderer(omc::event::EventBus& eventBus) : eventBus(eventBus) {}

		bool init();
		void update();
		void render(const std::vector<omc::ui::DrawCommand>& drawCommands);

		void onExit(const omc::event::ExitApplicationRequestedEvent& event);
	private:
		void handleSDLEvent(const SDL_Event& event);
		void enableClickThrough();
		void disableClickThrough();

		omc::event::EventBus& eventBus;

		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;

		std::vector<omc::ui::DrawCommand> drawCommands;
	};
}