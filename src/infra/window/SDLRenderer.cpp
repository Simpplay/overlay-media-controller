#include "SDLRenderer.hpp"

#include "core/event/events/ExitApplicationRequestedEvent.hpp"

namespace omc::infra
{
	bool SDLRenderer::init()
	{
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			SDL_Log("SDL_Init failed: %s", SDL_GetError());
			return false;
		}

		window = SDL_CreateWindow(
			"Overlay Media Controller",
			800, 600,
			SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_TRANSPARENT
		);

		renderer = SDL_CreateRenderer(window, NULL);

		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // alpha 0
		SDL_RenderClear(renderer);

		return true;
	}

	void SDLRenderer::update()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			handleSDLEvent(event);
		}
	}

	void SDLRenderer::handleSDLEvent(const SDL_Event& event)
	{
		switch (event.type) {
		case SDL_EVENT_QUIT:
			SDL_Log("Quit event received");
			eventBus.emit(omc::event::ExitApplicationRequestedEvent{});
			break;
		default:
			break;
		}
	}

	void SDLRenderer::render()
	{

	}
}