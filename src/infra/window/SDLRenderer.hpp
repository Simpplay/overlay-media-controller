#pragma once

#include <SDL3/SDL.h>

#include "core/event/EventBus.hpp"

namespace omc::infra
{
	class SDLRenderer
	{
	public:
		SDLRenderer(omc::event::EventBus& eventBus) : eventBus(eventBus) {}

		bool init();
		void update();
		void render();

	private:
		void handleSDLEvent(const SDL_Event& event);

		omc::event::EventBus& eventBus;

		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
	};
}