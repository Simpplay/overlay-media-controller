#pragma once

#include <string>
#include <variant>
#include <cstdint>
#include <memory>

namespace omc::ui
{
	struct Vec2 {
		float x{ 0.0f };
		float y{ 0.0f };

		constexpr Vec2() noexcept = default;
		constexpr Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}
	};

	struct Rect {
		Vec2 position{};
		Vec2 size{};

		constexpr Rect() noexcept = default;
		constexpr Rect(Vec2 pos, Vec2 sz) noexcept : position(pos), size(sz) {}
	};

	struct Color {
		uint8_t r{ 0 }, g{ 0 }, b{ 0 }, a{ 255 };

		constexpr Color() noexcept = default;
		constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) noexcept
			: r(r_), g(g_), b(b_), a(a_) {
		}
	};

	// Tipos específicos de draw commands
	struct RectCmd {
		Rect rect;
		Color color;
		int zIndex;
	};

	struct TextCmd {
		Vec2 position;
		Color color;
		std::string text;
		int zIndex;
	};

	struct ImageCmd {
		Vec2 position;
		Vec2 size;           // tamaño de display en pantalla
		std::shared_ptr<std::vector<uint8_t>> data;
		int  frameWidth;     // ancho real de los píxeles del frame
		int  frameHeight;    // alto real de los píxeles del frame
		int  imageId;
		int  zIndex;
	};

	using DrawCommand = std::variant<RectCmd, TextCmd, ImageCmd>;
}