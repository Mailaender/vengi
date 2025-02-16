/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/RGBA.h"
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <float.h>

namespace core {

class Color {
public:
	static const uint32_t magnitude = 255;
	static constexpr float magnitudef = 255.0f;
	static constexpr float scaleFactor = 0.7f;
	static const glm::vec4
		Clear,
		White,
		Black,
		LightGray,
		Gray,
		DarkGray,
		LightRed,
		Red,
		DarkRed,
		LightGreen,
		Green,
		DarkGreen,
		Lime,
		LightBlue,
		Blue,
		DarkBlue,
		SteelBlue,
		Olive,
		Pink,
		Purple,
		Yellow,
		Sandy,
		Cyan,
		Orange,
		Brown,
		LightBrown,
		DarkBrown;

	static float getDistance(RGBA rgba, RGBA rgba2);
	static float getDistance(const glm::vec4& color, float hue, float saturation, float brightness);
	static float getDistance(RGBA color, float hue, float saturation, float brightness);

	/**
	 * @brief Get the nearest matching color index from the list
	 * @param color The color to find the closest match to in the given @c colors array
	 * @return index in the colors vector or the first entry if non was found, or @c -1 on error
	 */
	template<class T>
	static int getClosestMatch(const glm::vec4& color, const T& colors, float *distance = nullptr) {
		if (colors.empty()) {
			return -1;
		}

		float minDistance = FLT_MAX;
		int minIndex = -1;

		float hue;
		float saturation;
		float brightness;
		getHSB(color, hue, saturation, brightness);

		for (size_t i = 0; i < colors.size(); ++i) {
			const float val = getDistance(colors[i], hue, saturation, brightness);
			if (val < minDistance) {
				minDistance = val;
				minIndex = (int)i;
			}
		}
		if (distance) {
			*distance = minDistance;
		}
		return minIndex;
	}

	static core::String print(RGBA rgba);

	static int quantize(RGBA* targetBuf, size_t maxTargetBufColors, const RGBA* inputBuf, size_t inputBufColors);

	static glm::vec4 fromRGBA(const RGBA rgba);
	static glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
	static glm::vec4 fromHSB(const float hue, const float saturation, const float brightness, const float alpha = 1.0f);
	static glm::vec4 fromHex(const char* hex);
	static core::String toHex(const RGBA rgba, bool hashPrefix = true);

	static RGBA getRGBA(const glm::vec4&);
	static constexpr RGBA getRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
		return RGBA{r, g, b, a};
	}
	static void getHSB(const glm::vec4&, float& hue, float& saturation, float& brightness);

	static glm::vec3 gray(const glm::vec3&);
	static glm::vec4 gray(const glm::vec4&);
	static glm::vec4 alpha(const glm::vec4&, float alpha);
	static RGBA alpha(const RGBA rgba, uint8_t alpha);
	static float brightness(const glm::vec4&);
	static float intensity(const glm::vec4&);

	static glm::vec4 darker(const glm::vec4& color, float f = 1.0f);
	static glm::vec4 brighter(const glm::vec4& color, float f = 1.0f);
};

}
