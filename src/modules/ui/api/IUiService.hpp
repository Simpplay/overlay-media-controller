#pragma once

#include <vector>
#include <optional>

#include "UiDto.hpp"

namespace omc::ui
{
	class IUiService
	{
	public:
		virtual ~IUiService() = default;

		virtual std::vector<OverlayDto> getAllOverlays() = 0;

		virtual OverlayDto updateOverlay(
			int id,
			std::optional<int> mediaId,
			std::optional<bool> fullscreen,
			std::optional<float> posX,
			std::optional<float> posY,
			std::optional<float> sizeX,
			std::optional<float> sizeY
		) = 0;

		virtual std::optional<OverlayDto> getOverlayById(int id) = 0;
		virtual bool deleteOverlayById(int id) = 0;
	};
}
