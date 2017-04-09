#pragma once

#include <string>
#include <vector>

struct settings
{
	// We serialize to/from AdaLight.config.json in the current directory. See the
	// included AdaLight.config.json for an example and a starting point. If the config file
	// does not exist we'll use the default values in this header and save them out to
	// Adalight.config.json for customization.
	settings(std::wstring&& configFilePath);

	// Minimum LED brightness; some users prefer a small amount of backlighting
	// at all times, regardless of screen content. Higher values are brighter,
	// or set to 0 to disable this feature.
	uint8_t minBrightness = 64;

	// LED transition speed; it's sometimes distracting if LEDs instantaneously
	// track screen contents (such as during bright flashing sequences), so this
	// feature enables a gradual fade to each new LED state. Higher numbers yield
	// slower transitions (max of 0.5), or set to 0 to disable this feature
	// (immediate transition of all LEDs).
	double fade = 0.0;

	// Serial device timeout (in milliseconds), for locating Arduino device
	// running the corresponding LEDstream code.
	DWORD timeout = 5000; // 5 seconds

	// Cap the refresh rate at 30 FPS. If the update takes longer the FPS
	// will actually be lower.
	UINT fpsMax = 30;

	// Timer frequency (in milliseconds) when we're throttled, e.g. when a UAC prompt
	// is displayed. If this value is higher, we'll use less CPU when we can't sample
	// the display, but it will take longer to resume sampling again.
	UINT throttleTimer = 3000; // 3 seconds

	// This struct contains the 2D coordinates corresponding to each pixel in the
	// LED strand, in the order that they're connected (i.e. the first element
	// here belongs to the first LED in the strand, second element is the second
	// LED, and so forth). Each pair in this array consists of an X and Y
	// coordinate specified in the grid units given for that display where
	// { 0, 0 } is the top-left corner of the display.
	struct led_pos
	{
		size_t x;
		size_t y;
	};

	// This struct contains details for each display that the software will
	// process. The horizontalCount is the number LEDs accross the top of the
	// AdaLight board, and the verticalCount is the number of LEDs up and down
	// the sides. These counts are used to figure out how big a block of pixels
	// should be to sample the edge around each LED.  If you have screen(s)
	// attached that are not among those being "Adalighted," you only need to
	// include them in this list if they show up before the "Adalighted"
	// display(s) in the system's display enumeration. If you have multiple
	// displays this might require some trial and error to figure out the precise
	// order relative to your setup. To leave a gap in the list and include another
	// display after that, just include an entry for the skipped display with
	// { 0, 0 } for the horizontalCount and verticalCount.
	struct display_config
	{
		size_t horizontalCount;
		size_t verticalCount;

		std::vector<led_pos> positions;
	};

	std::vector<display_config> displays = {
		// For our example purposes, the coordinate list below forms a ring around
		// the perimeter of a single screen, with a two pixel gap at the bottom to
		// accommodate a monitor stand. Modify this to match your own setup:

		{
			// Screen 0: 10 LEDs across, 5 LEDs down
			10, 5,
			{
				// Bottom edge, left half
				{ 3, 4 }, { 2, 4 }, { 1, 4 },
				// Left edge
				{ 0, 4 }, { 0, 3 }, { 0, 2 }, { 0, 1 },
				// Top edge
				{ 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 },
				{ 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 }, { 9, 0 },
				// Right edge
				{ 9, 1 }, { 9, 2 }, { 9, 3 }, { 9, 4 },
				// Bottom edge, right half
				{ 8, 4 }, { 7, 4 }, { 6, 4 },
			}
		},

		// Hypothetical second display a different arrangement than the first.
		// But you might not want both displays completely ringed with LEDs;
		// the screens might be positioned where they share an edge in common.

		//{
		//	// Screen 1: 9 LEDs across, 6 LEDs down
		//	9, 6,
		//	{
		//		// Bottom edge, left half
		//		{ 3, 5 }, { 2, 5 }, { 1, 5 },
		//		// Left edge
		//		{ 0, 5 }, { 0, 4 }, { 0, 3 }, { 0, 2 }, { 0, 1 },
		//		// Top edge
		//		{ 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 },
		//		{ 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 },
		//		// Right edge
		//		{ 8, 1 }, { 8, 2 }, { 8, 3 }, { 8, 4 }, { 8, 5 },
		//		// Bottom edge, right half
		//		{ 7, 5 }, { 6, 5 }, { 5, 5 },
		//	}
		//},
	};

	DWORD minBrightnessColor;
	size_t totalLedCount;
	double weight;
	UINT delay;

private:
	const std::wstring _configFilePath;
};
