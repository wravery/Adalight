#include "stdafx.h"
#include "settings.h"

#include <cpprest/json.h>

#include <numeric>

settings::settings()
	: minBrightnessColor((((minBrightness / 3) & 0xFF) << 24) | (((minBrightness / 3) & 0xFF) << 16) | (((minBrightness / 3) & 0xFF) << 8) | 0xFF)
	, totalLedCount(std::accumulate(displays.cbegin(), displays.cend(), size_t(),
			[](size_t count, const display_config& display)
		{
			return count + display.positions.size();
		}))
	, weight(1.0 - fade)
	, delay(1000 / fpsMax)
{
}
