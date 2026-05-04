#include "SDLRenderer.hpp"

namespace omc::infra
{
	bool SDLRenderer::init()
	{
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			SDL_Log("SDL_Init failed: %s", SDL_GetError());
			return false;
		}

        eventBus.subscribe<omc::event::ExitApplicationRequestedEvent>([this](const auto& event) {
            onExit(event);
        });

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

		#if defined(SDL_PLATFORM_WIN32)
        {
            HWND hwnd = omc::infra::win32::getNativeWindowHandle(window);

			const auto& drawCommands = this->drawCommands;

            omc::infra::win32::hookWindowProc(hwnd, [drawCommands](POINT pt) {
                bool found = false;
                
                for (const auto& draw : drawCommands) {
                    if (std::holds_alternative<omc::ui::RectCmd>(draw)) {
                        const auto& rectCmd = std::get<omc::ui::RectCmd>(draw);
                        RECT rect{
                            static_cast<LONG>(rectCmd.rect.position.x),
                            static_cast<LONG>(rectCmd.rect.position.y),
                            static_cast<LONG>(rectCmd.rect.position.x + rectCmd.rect.size.x),
                            static_cast<LONG>(rectCmd.rect.position.y + rectCmd.rect.size.y)
                        };
                        if (PtInRect(&rect, pt)) {
                            found = true;
                            break;
                        }
                    }
                }
                
                return found;
            });
        }
		#endif

		enableClickThrough();

		return true;
	}

    void SDLRenderer::onExit(const omc::event::ExitApplicationRequestedEvent& event)
    {
        SDL_Log("ExitApplicationRequestedEvent received. Cleaning up SDL resources.");
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();
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
			eventBus.emit(omc::event::ExitApplicationRequestedEvent{});

            #if defined(SDL_PLATFORM_WIN32)
            {
                HWND hwnd = omc::infra::win32::getNativeWindowHandle(window);
                omc::infra::win32::unhookWindowProc(hwnd);
			}
            #endif
			break;
		default:
			break;
		}
	}

    void SDLRenderer::render(const std::vector<omc::ui::DrawCommand>& drawCommands)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (const auto& draw : drawCommands) {
            std::visit([this](auto&& cmd) {
                using T = std::decay_t<decltype(cmd)>;

                if constexpr (std::is_same_v<T, omc::ui::RectCmd>) {
                    SDL_SetRenderDrawColor(renderer, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);

                    SDL_FRect rect{
                        cmd.rect.position.x,
                        cmd.rect.position.y,
                        cmd.rect.size.x,
                        cmd.rect.size.y
                    };

                    SDL_RenderFillRect(renderer, &rect);
                }

                }, draw);
        }

        SDL_RenderPresent(renderer);
    }

	void SDLRenderer::enableClickThrough()
	{
		#if defined(SDL_PLATFORM_WIN32)
        {
            HWND hwnd = omc::infra::win32::getNativeWindowHandle(window);
            SetWindowLong(hwnd, GWL_EXSTYLE,
                GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        }
		#endif
	}

	void SDLRenderer::disableClickThrough()
	{
		#if defined(SDL_PLATFORM_WIN32)
        {
            HWND hwnd = omc::infra::win32::getNativeWindowHandle(window);
            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
        }
		#endif
	}
}

#if defined(SDL_PLATFORM_WIN32)

#include <cassert>
#include <unordered_map>

namespace omc::infra::win32
{
    struct WindowHookData {
        WNDPROC originalProc = nullptr;
        std::function<bool(POINT)> isInteractiveArea;
    };

    // Estado global controlado (clave: HWND)
    static std::unordered_map<HWND, WindowHookData> g_hooks;

    HWND getNativeWindowHandle(SDL_Window* window)
    {
        return (HWND)SDL_GetPointerProperty(
            SDL_GetWindowProperties(window),
            SDL_PROP_WINDOW_WIN32_HWND_POINTER,
            NULL
        );
    }

    static LRESULT CALLBACK WndProcHook(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {

        auto it = g_hooks.find(hwnd);

        if (it == g_hooks.end()) {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        WindowHookData& data = it->second;

        switch (msg)
        {
        case WM_NCHITTEST:
        {
            POINT pt{
                GET_X_LPARAM(lParam),
                GET_Y_LPARAM(lParam)
            };

            ScreenToClient(hwnd, &pt);

            if (data.isInteractiveArea && data.isInteractiveArea(pt)) {
				SDL_Log("Interactive area hit at (%d, %d)", pt.x, pt.y);
                return HTCLIENT;
            }
            else {
                return HTTRANSPARENT;
            }
        }
        }

        if (data.originalProc) {
            return CallWindowProc(
                data.originalProc,
                hwnd,
                msg,
                wParam,
                lParam
            );
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void hookWindowProc(
        HWND hwnd,
        std::function<bool(POINT)> isInteractiveArea
    )
    {
        assert(hwnd);

        // Evitar doble hook
        if (g_hooks.contains(hwnd)) {
            return;
        }

        SetLastError(0);

        auto prev = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(hwnd, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&WndProcHook))
            );

        if (!prev && GetLastError() != 0) {
            // fallo real
            return;
        }

        WindowHookData data;
        data.originalProc = prev;
        data.isInteractiveArea = std::move(isInteractiveArea);

        g_hooks.emplace(hwnd, std::move(data));
    }

    void unhookWindowProc(HWND hwnd)
    {
        auto it = g_hooks.find(hwnd);
        if (it == g_hooks.end()) {
            return;
        }

        WindowHookData& data = it->second;

        if (data.originalProc) {
            SetWindowLongPtr(
                hwnd,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(data.originalProc)
            );
        }

        g_hooks.erase(it);
    }

} // namespace

#endif