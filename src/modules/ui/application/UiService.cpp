#include "UiService.hpp"

#include "modules/ui/application/windows/WebViewWindow.hpp"
#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

namespace omc::ui
{
	struct UiService::Impl {
		OverlayDto OverlayWindowToDto(int id, const omc::ui::window::UiWindow& window) {
			int mediaId = -1;

			if constexpr (std::is_same_v<std::decay_t<decltype(window)>, omc::ui::window::WebViewWindow>) {
				mediaId = static_cast<const omc::ui::window::WebViewWindow&>(window).getMediaId();
			}

			return OverlayDto{
				static_cast<int>(id),
				mediaId,
				window.closed() ? "closed" : "active",
				window.isMaximized,
				Vec2PositionToDto(window.position),
				Vec2SizeToDto(window.size)
			};
		}

		SizeDto Vec2SizeToDto(const Vec2& vec) {
			return SizeDto{
				vec.x,
				vec.y
			};
		}
		
		PositionDto Vec2PositionToDto(const Vec2& vec) {
			return PositionDto{
				vec.x,
				vec.y
			};
		}
	};

	UiService::UiService(omc::event::EventBus& eventBus, UiRepository& uiRepository)
		: impl_(std::make_unique<Impl>()), eventBus(eventBus), uiRepository(uiRepository)
	{
	}

	std::vector<OverlayDto> UiService::getAllOverlays()
	{
		auto& windows = uiRepository.getWindows();
		std::vector<OverlayDto> dtos;
		for (const auto& [id, window] : windows) {
			dtos.push_back(impl_->OverlayWindowToDto(id, *window));
		}
		return dtos;
	}

	OverlayDto UiService::updateOverlay(
		int id,
		std::optional<int> mediaId,
		std::optional<bool> fullscreen,
		std::optional<float> posX,
		std::optional<float> posY,
		std::optional<float> sizeX,
		std::optional<float> sizeY
	)
	{
		auto& windows = uiRepository.getWindows();
		auto it = windows.find(id);
		if (it == windows.end()) {
			return {};
		}

		it->second->setMaximized(fullscreen.value_or(it->second->isMaximized));
		it->second->position.x = posX.value_or(it->second->position.x);
		it->second->position.y = posY.value_or(it->second->position.y);
		it->second->size.x = sizeX.value_or(it->second->size.x);
		it->second->size.y = sizeY.value_or(it->second->size.y);

		return impl_->OverlayWindowToDto(it->first, *it->second);
	}

	std::optional<OverlayDto> UiService::getOverlayById(int id)
	{
		auto& windows = uiRepository.getWindows();
		auto it = windows.find(id);
		if (it == windows.end()) {
			return {};
		}
		return impl_->OverlayWindowToDto(it->first, *it->second);
	}

	bool UiService::deleteOverlayById(int id)
	{
		auto& windows = uiRepository.getWindows();
		windows.erase(id);
		return true;
	}
}