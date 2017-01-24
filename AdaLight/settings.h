#pragma once

#include <vector>

struct settings
{
	settings();

	// Minimum LED brightness; some users prefer a small amount of backlighting
	// at all times, regardless of screen content. Higher values are brighter,
	// or set to 0 to disable this feature.
	const uint8_t minBrightness = 64;

	// LED transition speed; it's sometimes distracting if LEDs instantaneously
	// track screen contents (such as during bright flashing sequences), so this
	// feature enables a gradual fade to each new LED state. Higher numbers yield
	// slower transitions (max of 0.5), or set to 0 to disable this feature
	// (immediate transition of all LEDs).
	const double fade = 0.0;

	// Serial device timeout (in milliseconds), for locating Arduino device
	// running the corresponding LEDstream code. See notes later in the code...
	// in some situations you may want to entirely comment out that block.
	const DWORD timeout = 5000;	// 5 seconds

	// Cap the refresh rate at 30 FPS. If the update takes longer the FPS
	// will actually be lower.
	const UINT fpsMax = 30;

	// PER-LED INFORMATION -------------------------------------------------------
	// This array contains the 2D coordinates corresponding to each pixel in the
	// LED strand, in the order that they're connected (i.e. the first element
	// here belongs to the first LED in the strand, second element is the second
	// LED, and so forth). Each triplet in this array consists of a display
	// number (an index into the display array above, NOT necessarily the same as
	// the system screen number) and an X and Y coordinate specified in the grid
	// units given for that display. {0,0,0} is the top-left corner of the first
	// display in the array.
	// For our example purposes, the coordinate list below forms a ring around
	// the perimeter of a single screen, with a one pixel gap at the bottom to
	// accommodate a monitor stand. Modify this to match your own setup:
	struct led_pos
	{
		size_t x;
		size_t y;
	};

	// PER-DISPLAY INFORMATION ---------------------------------------------------
	// This array contains details for each display that the software will
	// process. If you have screen(s) attached that are not among those being
	// "Adalighted," they should not be in this list. Each triplet in this
	// array represents one display. The first number is the system screen
	// number...typically the "primary" display on most systems is identified
	// as screen #1, but since arrays are indexed from zero, use 0 to indicate
	// the first screen, 1 to indicate the second screen, and so forth. This
	// is the ONLY place system screen numbers are used...ANY subsequent
	// references to displays are an index into this list, NOT necessarily the
	// same as the system screen number. For example, if you have a three-
	// screen setup and are illuminating only the third display, use '2' for
	// the screen number here...and then, in subsequent section, '0' will be
	// used to refer to the first/only display in this list.
	// The second and third numbers of each triplet represent the width and
	// height of a grid of LED pixels attached to the perimeter of this display.
	// For example, '9, 6' = 9 LEDs across, 6 LEDs down.
	struct display_config
	{
		size_t horizontalCount;
		size_t verticalCount;

		std::vector<led_pos> positions;
	};

	const std::vector<display_config> displays = {
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

	const DWORD minBrightnessColor;
	const size_t totalLedCount;
	const double weight;
	const UINT delay;
};
