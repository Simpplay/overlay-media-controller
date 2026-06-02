#pragma once

#include <memory>

#include "modules/ui/domain/UiRepository.hpp"
#include "core/event/api/EventBus.hpp"

#include "IUiService.hpp"

namespace omc::ui
{
	class UiService : public IUiService
	{
	public:
		UiService(omc::event::EventBus& eventBus, UiRepository& uiRepository);

		std::vector<OverlayDto> getAllOverlays();

		OverlayDto updateOverlay(
			int id,
			std::optional<int> mediaId,
			std::optional<bool> fullscreen,
			std::optional<float> posX,
			std::optional<float> posY,
			std::optional<float> sizeX,
			std::optional<float> sizeY
		);

		std::optional<OverlayDto> getOverlayById(int id);
		bool deleteOverlayById(int id);

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;

		omc::event::EventBus& eventBus;
		UiRepository uiRepository;
	};
}