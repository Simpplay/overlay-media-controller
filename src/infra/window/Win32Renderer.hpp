#pragma once

#if defined(_WIN32)
#include <Windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <dcomp.h>
#include <vector>
#include <algorithm>
#include <variant>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dcomp.lib")

#include "HitTestManager.hpp"
#include "modules/ui/UiTypes.hpp"
#include "core/event/EventBus.hpp"
#include "core/event/events/ExitApplicationRequestedEvent.hpp"

namespace omc::infra
{
	class Win32Renderer
	{
	public:
		Win32Renderer(omc::event::EventBus& eventBus);
		~Win32Renderer();

		bool init();
		void update();
		void render(const std::vector<omc::ui::DrawCommand>& drawCommands);
		void onExit(const omc::event::ExitApplicationRequestedEvent& event);

		bool InitPipeline();
		bool InitD3D(int width, int height);
		void CleanupD3D();
		bool CreateOverlayWindow(int width, int height);
		void UpdateClickThrough(HWND hwnd, bool interactive);
		bool CreateDynamicVertexBuffer(size_t maxVertices);
	private:
		struct Impl;
		std::unique_ptr<Impl> m_pimpl;

		omc::event::EventBus& eventBus;
		std::vector<omc::ui::DrawCommand> drawCommands;
	};
}
#else
#include <vector>

#include "modules/ui/UiTypes.hpp"
#include "core/event/EventBus.hpp"
#include "core/event/events/ExitApplicationRequestedEvent.hpp"

namespace omc::infra
{
	class Win32Renderer
	{
	public:
		explicit Win32Renderer(omc::event::EventBus&) {}
		~Win32Renderer() = default;

		bool init() { return true; }
		void update() {}
		void render(const std::vector<omc::ui::DrawCommand>&) {}
		void onExit(const omc::event::ExitApplicationRequestedEvent&) {}
	};
}
#endif
