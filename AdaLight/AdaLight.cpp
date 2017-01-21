// AdaLight.cpp
//
// This is a port of the Processing (Java) driver for the AdaLight project to a C++ Windows Service.
// The service must run in interactive mode to capture the edges of the screen, but otherwise it should
// have much lower overhead than the Processing version and it should run without any UI. If you want
// the preview UI to help figure out your configuration variables, I recommend using the original
// Processing application first and then copying those variables to this file and recompiling.
//

// AdaLight guide: https://learn.adafruit.com/adalight-diy-ambient-tv-lighting/pieces?view=all#download-and-install
// Serial port programming sample: https://code.msdn.microsoft.com/windowsdesktop/Serial-Port-Sample-e8accf30/sourcecode?fileId=67164&pathId=1394200469
// DirectX C# screen capture sample: https://www.codeproject.com/Articles/274461/Very-fast-screen-capture-using-DirectX-in-Csharp
// DirectX 11 C++ references: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476082(v=vs.85).aspx

#include "stdafx.h"

#include <cmath>
#include <chrono>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#undef min
#undef max

static void DisplayLastError();

#pragma region ORIGINAL ADALIGHT.PDE COMMENTS

// "Adalight" is a do-it-yourself facsimile of the Philips Ambilight concept
// for desktop computers and home theater PCs.  This is the host PC-side code
// written in Processing, intended for use with a USB-connected Arduino
// microcontroller running the accompanying LED streaming code.  Requires one
// or more strands of Digital RGB LED Pixels (Adafruit product ID #322,
// specifically the newer WS2801-based type, strand of 25) and a 5 Volt power
// supply (such as Adafruit #276).  You may need to adapt the code and the
// hardware arrangement for your specific display configuration.
// Screen capture adapted from code by Cedrik Kiefer (processing.org forum)

// --------------------------------------------------------------------
//   This file is part of Adalight.

//   Adalight is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as
//   published by the Free Software Foundation, either version 3 of
//   the License, or (at your option) any later version.

//   Adalight is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.

//   You should have received a copy of the GNU Lesser General Public
//   License along with Adalight.  If not, see
//   <http://www.gnu.org/licenses/>.
// --------------------------------------------------------------------

#pragma endregion

#pragma region CONFIGURABLE PROGRAM CONSTANTS

// Minimum LED brightness; some users prefer a small amount of backlighting
// at all times, regardless of screen content.  Higher values are brighter,
// or set to 0 to disable this feature.
constexpr uint8_t c_minBrightness = 64;

// LED transition speed; it's sometimes distracting if LEDs instantaneously
// track screen contents (such as during bright flashing sequences), so this
// feature enables a gradual fade to each new LED state.  Higher numbers yield
// slower transitions (max of 0.5), or set to 0 to disable this feature
// (immediate transition of all LEDs).
constexpr double c_fade = 0.0;
static_assert(c_fade >= 0.0 && c_fade <= 0.5, "c_fade is out of bounds!");
constexpr double c_weight = 1.0 - c_fade;

// Serial device timeout (in milliseconds), for locating Arduino device
// running the corresponding LEDstream code.  See notes later in the code...
// in some situations you may want to entirely comment out that block.
constexpr int c_timeout = 2500;	// 2.5 seconds

// Cap the refresh rate at 30 FPS. If the update takes longer the FPS
// will actually be lower.
constexpr UINT c_fpsMax = 30;
constexpr UINT c_msecDelay = (1000 / c_fpsMax);

// PER-DISPLAY INFORMATION ---------------------------------------------------
// This array contains details for each display that the software will
// process.  If you have screen(s) attached that are not among those being
// "Adalighted," they should not be in this list.  Each triplet in this
// array represents one display.  The first number is the system screen
// number...typically the "primary" display on most systems is identified
// as screen #1, but since arrays are indexed from zero, use 0 to indicate
// the first screen, 1 to indicate the second screen, and so forth.  This
// is the ONLY place system screen numbers are used...ANY subsequent
// references to displays are an index into this list, NOT necessarily the
// same as the system screen number.  For example, if you have a three-
// screen setup and are illuminating only the third display, use '2' for
// the screen number here...and then, in subsequent section, '0' will be
// used to refer to the first/only display in this list.
// The second and third numbers of each triplet represent the width and
// height of a grid of LED pixels attached to the perimeter of this display.
// For example, '9,6' = 9 LEDs across, 6 LEDs down.
struct display_config
{
	uint8_t ledsAccross;
	uint8_t ledsDown;
};

constexpr display_config c_displays[] = {
	{ 10, 5 },	// Screen 0, 10 LEDs across, 5 LEDs down
	// { 1, 9, 6 },	// Screen 1, 9 LEDs across, 6 LEDs down
};

// PER-LED INFORMATION -------------------------------------------------------
// This array contains the 2D coordinates corresponding to each pixel in the
// LED strand, in the order that they're connected (i.e. the first element
// here belongs to the first LED in the strand, second element is the second
// LED, and so forth).  Each triplet in this array consists of a display
// number (an index into the display array above, NOT necessarily the same as
// the system screen number) and an X and Y coordinate specified in the grid
// units given for that display.  {0,0,0} is the top-left corner of the first
// display in the array.
// For our example purposes, the coordinate list below forms a ring around
// the perimeter of a single screen, with a one pixel gap at the bottom to
// accommodate a monitor stand.  Modify this to match your own setup:
struct led_pos
{
	size_t screenId;
	uint8_t x;
	uint8_t y;
};

constexpr led_pos c_leds[] = {
	// Bottom edge, left half
	{ 0,3,4 }, { 0,2,4 }, { 0,1,4 }, { 0,0,4 },
	// Left edge
	{ 0,0,3 }, { 0,0,2 }, { 0,0,1 },
	// Top edge
	{ 0,0,0 }, { 0,1,0 }, { 0,2,0 }, { 0,3,0 }, { 0,4,0 },
	{ 0,5,0 }, { 0,6,0 }, { 0,7,0 }, { 0,8,0 }, { 0,9,0 },
	// Right edge
	{ 0,9,1 }, { 0,9,2 }, { 0,9,3 },
	// Bottom edge, right half
	{ 0,9,4 }, { 0,8,4 }, { 0,7,4 }, { 0,6,4 },

	// Hypothetical second display a different arrangement than the first.
	// But you might not want both displays completely ringed with LEDs;
	// the screens might be positioned where they share an edge in common.

	//{1,3,5}, {1,2,5}, {1,1,5}, {1,0,5}, // Bottom edge, left half
	//{1,0,4}, {1,0,3}, {1,0,2}, {1,0,1}, // Left edge
	//{1,0,0}, {1,1,0}, {1,2,0}, {1,3,0}, {1,4,0}, // Top edge
	//{1,5,0}, {1,6,0}, {1,7,0}, {1,8,0}, // More top edge
	//{1,8,1}, {1,8,2}, {1,8,3}, {1,8,4}, // Right edge
	//{1,8,5}, {1,7,5}, {1,6,5}, {1,5,5},  // Bottom edge, right half
};

#pragma endregion

#pragma region RUNTIME CONSTANTS

namespace runtime_constants {
namespace details {

constexpr uint8_t c_ledCountHi = (_countof(c_leds) - 1) >> 8;
constexpr uint8_t c_ledCountLo = (_countof(c_leds) - 1) & 0xFF;
constexpr uint8_t c_ledCountChecksum = c_ledCountHi ^ c_ledCountLo ^ 0x55;

constexpr uint8_t c_serialDataHeader[] = {
	'A', 'd', 'a',		// Magic word
	c_ledCountHi,		// LED count high byte
	c_ledCountLo,		// LED count low byte
	c_ledCountChecksum,	// Checksum
};

constexpr size_t c_serialDataSize = (3 * _countof(c_leds));

} // namespace details

constexpr size_t c_serialDataOffset = _countof(details::c_serialDataHeader);

static uint8_t s_serialDataFrame[c_serialDataOffset + details::c_serialDataSize];

// Pre-fill the header in the static buffer since it never changes.
static void fill_serial_header()
{
	memset(s_serialDataFrame, 0, _countof(s_serialDataFrame));
	memcpy_s(s_serialDataFrame, sizeof(s_serialDataFrame),
		details::c_serialDataHeader, sizeof(details::c_serialDataHeader));
}

struct gamma_factors
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

static gamma_factors s_gamma_table[256];

// Pre-compute the gamma correction table for LED brightness levels.
static void compute_gamma_table()
{
	for (size_t i = 0; i <= _countof(s_gamma_table); i++)
	{
		const double f = pow(static_cast<double>(i) / 255.0, 2.8);

		s_gamma_table[i].r = static_cast<uint8_t>(f * 255.0);
		s_gamma_table[i].g = static_cast<uint8_t>(f * 240.0);
		s_gamma_table[i].b = static_cast<uint8_t>(f * 220.0);
	}
}

// Initialize the runtime constants.
static void initialize()
{
	fill_serial_header();
	compute_gamma_table();
}

} // namespace runtime_constants

#pragma endregion

#pragma region SESSION STATE VARIABLES

namespace session_state {

static IDXGIFactory1Ptr s_dxgiFactory;

// Load DirectX.
static bool load_dxgi()
{
	if (!s_dxgiFactory)
	{
		CreateDXGIFactory1(_uuidof(IDXGIFactory1), reinterpret_cast<void**>(&s_dxgiFactory));
	}

	return s_dxgiFactory;
}

struct display_duplication
{
	IDXGIAdapter1Ptr adapter;
	ID3D11DevicePtr device;
	ID3D11DeviceContextPtr context;
	IDXGIOutputDuplicationPtr duplication;
	ID3D11Texture2DPtr staging;
	SIZE bounds;
};

// Display interfaces mapped to output indices
static std::vector<display_duplication> s_devices;

// Samples take the center point of each cell in a 16x16 grid
constexpr size_t c_pixelSamples = 16;

struct led_pixel_offsets
{
	size_t offset[c_pixelSamples * c_pixelSamples];
};

static std::unordered_map<size_t, led_pixel_offsets> s_pixelOffsets;

// Previous colors for fades
static DWORD s_previousColors[_countof(c_leds)];

constexpr uint8_t c_minBrightnessComponent = (c_minBrightness / 3) & 0xFF;
constexpr DWORD c_minBrightnessColor = (c_minBrightnessComponent << 24) | (c_minBrightnessComponent << 16) | (c_minBrightnessComponent << 8) | 0xFF;

// Frame counter
static size_t s_frameCount = 0;

// Elapsed time
static ULONGLONG s_tickCountStart = 0;

// Frame rate of the last active period.
static double s_frameRate = 0.0;

// Clear out the cached resources.
static void free_resources()
{
	s_devices.clear();
	s_pixelOffsets.clear();

	s_frameRate = static_cast<double>(s_frameCount * 1000) / static_cast<double>(GetTickCount64() - s_tickCountStart);
	s_frameCount = 0;
	s_tickCountStart = 0;
}

// Fill in the current display bounds for all known displays.
static void create_resources(HWND hwnd)
{
	s_tickCountStart = GetTickCount64();

	if (!load_dxgi())
	{
		return;
	}

	s_devices.reserve(_countof(c_displays));

	// Get the display dimensions and devices.
	IDXGIAdapter1Ptr adapter;

	for (UINT i = 0; s_devices.size() < _countof(c_displays) && SUCCEEDED(s_dxgiFactory->EnumAdapters1(i, &adapter)); ++i)
	{
		IDXGIOutputPtr output;

		for (UINT j = 0; s_devices.size() < _countof(c_displays) && SUCCEEDED(adapter->EnumOutputs(j, &output)); ++j)
		{
			IDXGIOutput1Ptr output1(output);
			DXGI_OUTPUT_DESC outputDescription;
			IDXGIOutputDuplicationPtr duplication;
			ID3D11DevicePtr device;
			ID3D11DeviceContextPtr context;

			constexpr D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;
			constexpr ULONG createFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

			if (output1
				&& SUCCEEDED(output1->GetDesc(&outputDescription))
				&& outputDescription.AttachedToDesktop
				&& SUCCEEDED(D3D11CreateDevice(adapter, driverType, NULL, createFlags, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context))
				&& SUCCEEDED(output1->DuplicateOutput(device, &duplication)))
			{
				DXGI_OUTDUPL_DESC duplicationDescription;

				duplication->GetDesc(&duplicationDescription);

				const bool useMapDesktopSurface = !!duplicationDescription.DesktopImageInSystemMemory;
				ID3D11Texture2DPtr staging;
				const RECT& bounds = outputDescription.DesktopCoordinates;
				const LONG width = bounds.right - bounds.left;
				const LONG height = bounds.bottom - bounds.top;

				if (!useMapDesktopSurface)
				{
					D3D11_TEXTURE2D_DESC textureDescription;

					textureDescription.Width = static_cast<UINT>(width);
					textureDescription.Height = static_cast<UINT>(height);
					textureDescription.MipLevels = 1;
					textureDescription.ArraySize = 1;
					textureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
					textureDescription.SampleDesc.Count = 1;
					textureDescription.SampleDesc.Quality = 0;
					textureDescription.Usage = D3D11_USAGE_STAGING;
					textureDescription.BindFlags = 0;
					textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
					textureDescription.MiscFlags = 0;

					device->CreateTexture2D(&textureDescription, NULL, &staging);
				}

				if (useMapDesktopSurface || staging)
				{
					s_devices.push_back({
						adapter,
						device,
						context,
						duplication,
						staging,
						{ width, height }
					});
				}
			}

			output.Release();
		}

		adapter.Release();
	}

	// Calculate the sub-sampled pixel offsets
	for (size_t i = 0; i < _countof(c_leds); ++i)
	{
		const auto& led = c_leds[i];

		if (led.screenId >= s_devices.size())
		{
			// Skip this LED, we don't have that display.
			continue;
		}

		const auto& displayBounds = s_devices[led.screenId].bounds;
		auto& pixelOffsets = s_pixelOffsets[i];
		const auto& display = c_displays[led.screenId];
		const double rangeX = (static_cast<double>(displayBounds.cx) / static_cast<double>(display.ledsAccross));
		const double stepX = rangeX / static_cast<double>(c_pixelSamples);
		const double startX = (rangeX * static_cast<double>(led.x)) + (stepX / 2.0);
		const double rangeY = (static_cast<double>(displayBounds.cy) / static_cast<double>(display.ledsDown));
		const double stepY = rangeY / static_cast<double>(c_pixelSamples);
		const double startY = (rangeY * static_cast<double>(led.y)) + (stepY / 2.0);

		size_t x[c_pixelSamples];
		size_t y[c_pixelSamples];

		for (size_t j = 0; j < c_pixelSamples; ++j)
		{
			x[j] = static_cast<size_t>(startX + (stepX * static_cast<double>(j)));
			y[j] = static_cast<size_t>(startY + (stepY * static_cast<double>(j)));
		}

		for (size_t row = 0; row < c_pixelSamples; ++row)
		{
			for (size_t col = 0; col < c_pixelSamples; ++col)
			{
				const size_t pixelIndex = (row * c_pixelSamples) + col;
				const size_t pixelOffset = y[row] * static_cast<size_t>(displayBounds.cx) + x[col];

				pixelOffsets.offset[pixelIndex] = pixelOffset;
			}
		}
	}

	// Re-initialize the previous colors for fades.
	for (auto& color : s_previousColors)
	{
		color = c_minBrightnessColor;
	}
}

static void fill_serial_data()
{
	// Take a screenshot for all of the devices that require a staging texture.
	for (auto& device : s_devices)
	{
		if (!device.staging)
		{
			continue;
		}

		const auto& displayBounds = device.bounds;
		const UINT displayWidth = static_cast<UINT>(displayBounds.cx);
		const UINT displayHeight = static_cast<UINT>(displayBounds.cy);
		IDXGIResourcePtr resource;
		DXGI_OUTDUPL_FRAME_INFO info;
		ID3D11Texture2DPtr screenTexture;

		// TODO: Recreate the duplication interface if this fails with DXGI_ERROR_ACCESS_LOST.
		if (SUCCEEDED(device.duplication->AcquireNextFrame(c_msecDelay, &info, &resource)))
		{
			screenTexture = resource;

			if (screenTexture)
			{
				device.context->CopyResource(device.staging, screenTexture);
			}

			screenTexture.Release();
			resource.Release();

			device.duplication->ReleaseFrame();
		}
	}

	size_t serialDataOffset = runtime_constants::c_serialDataOffset;

	for (size_t i = 0; i < _countof(c_leds); ++i)
	{
		const auto& led = c_leds[i];
		auto itrPixelOffsets = s_pixelOffsets.find(i);

		if (itrPixelOffsets == s_pixelOffsets.cend()
			|| led.screenId >= s_devices.size())
		{
			continue;
		}

		const auto& device = s_devices[led.screenId];
		D3D11_MAPPED_SUBRESOURCE stagingMap;
		DXGI_MAPPED_RECT desktopMap;
		const uint8_t* pixels = nullptr;

		if (device.staging)
		{
			if (FAILED(device.context->Map(device.staging, 0, D3D11_MAP_READ, 0, &stagingMap)))
			{
				continue;
			}

			// TODO: We really should take the RowPitch into account
			pixels = reinterpret_cast<const uint8_t*>(stagingMap.pData);
		}
		else
		{
			if (FAILED(device.duplication->MapDesktopSurface(&desktopMap)))
			{
				continue;
			}

			// TODO: We really should take the Pitch into account
			pixels = reinterpret_cast<const uint8_t*>(desktopMap.pBits);
		}

		// Get the average RGB values for the sampled pixels.
		constexpr double divisor = static_cast<double>(_countof(itrPixelOffsets->second.offset));
		double r = 0.0;
		double g = 0.0;
		double b = 0.0;

		for (auto offset : itrPixelOffsets->second.offset)
		{
			const size_t byteOffset = offset * sizeof(DWORD);

			b += static_cast<double>(pixels[byteOffset]);
			g += static_cast<double>(pixels[byteOffset + 1]);
			r += static_cast<double>(pixels[byteOffset + 2]);
		}

		if (device.staging)
		{
			device.context->Unmap(device.staging, 0);
		}
		else
		{
			device.duplication->UnMapDesktopSurface();
		}

		r /= divisor;
		g /= divisor;
		b /= divisor;

		// Average in the previous color if fading is enabled.
		if (c_fade > 0.0)
		{
			const DWORD previousColor = s_previousColors[i];

			r = (r * c_weight) + (static_cast<double>((previousColor & 0xFF000000) >> 24) * c_fade);
			g = (g * c_weight) + (static_cast<double>((previousColor & 0xFF0000) >> 16) * c_fade);
			b = (b * c_weight) + (static_cast<double>((previousColor & 0xFF00) >> 8) * c_fade);
		}

		constexpr double minBrightness = static_cast<double>(c_minBrightness);
		const double sum = r + g + b;

		// Boost pixels that fall below the minimum brightness.
		if (sum < minBrightness)
		{
			if (sum == 0.0)
			{
				// Spread equally to R, G, and B.
				constexpr double value = minBrightness / 3.0;

				r = value;
				g = value;
				b = value;
			}
			else
			{
				// Spread the "brightness deficit" back into R, G, and B in proportion
				// to their individual contribition to that deficit.  Rather than simply
				// boosting all pixels at the low end, this allows deep (but saturated)
				// colors to stay saturated...they don't "pink out."
				const double deficit = minBrightness - sum;
				const double sum2 = 2.0 * sum;

				r += (deficit * (sum - r)) / sum2;
				g += (deficit * (sum - g)) / sum2;
				b += (deficit * (sum - b)) / sum2;
			}
		}

		const uint8_t ledR = static_cast<uint8_t>(r);
		const uint8_t ledG = static_cast<uint8_t>(g);
		const uint8_t ledB = static_cast<uint8_t>(b);

		s_previousColors[i] = (ledB << 24) | (ledG << 16) | (ledR << 8) | 0xFF;

		// Write the gamma corrected values to the serial data.
		const auto& gammaR = runtime_constants::s_gamma_table[ledR];
		const auto& gammaG = runtime_constants::s_gamma_table[ledG];
		const auto& gammaB = runtime_constants::s_gamma_table[ledB];

		runtime_constants::s_serialDataFrame[serialDataOffset++] = gammaR.r;
		runtime_constants::s_serialDataFrame[serialDataOffset++] = gammaG.g;
		runtime_constants::s_serialDataFrame[serialDataOffset++] = gammaB.b;
	}

	++s_frameCount;
}

} // namespace session_state

#pragma endregion

#pragma region LED CONTROL LOGIC

namespace serial_data {

static HANDLE s_portHandle = INVALID_HANDLE_VALUE;

// Close the port if it's open.
static void ClosePort()
{
	if (INVALID_HANDLE_VALUE != s_portHandle)
	{
		CloseHandle(s_portHandle);
		s_portHandle = INVALID_HANDLE_VALUE;
	}
}

// Find the LED controller's COM port.
static bool OpenPort()
{
	static bool s_triedOnce = false;

	if (!s_triedOnce
		&& INVALID_HANDLE_VALUE == s_portHandle)
	{
		s_triedOnce = true;
		s_portHandle = CreateFileW(L"COM3", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		return true;

		DCB config = {};
		//COMMTIMEOUTS timeouts;

		config.DCBlength = sizeof(config);
		//COMMTIMEOUTS timeouts = {
		//	50,	// ReadIntervalTimeout
		//	50,	// ReadTotalTimeoutMultiplier
		//	10,	// ReadTotalTimeoutConstant
		//	10,	// WriteTotalTimeoutMultiplier
		//	50	// WriteTotalTimeoutConstant
		//};

		if (INVALID_HANDLE_VALUE != s_portHandle
			|| !GetCommState(s_portHandle, &config))
		{
			DisplayLastError();
			ClosePort();
			return false;
		}

		config.BaudRate = CBR_115200;
		config.fBinary = true;
		config.ByteSize = 8;
		config.Parity = NOPARITY;
		config.StopBits = 1;

		if (!SetCommState(s_portHandle, &config))
		{
			DisplayLastError();
			ClosePort();
			return false;
		}

		//SetCommTimeouts(s_portHandle, &timeouts);
	}

	return INVALID_HANDLE_VALUE != s_portHandle;
}

// Pass the current serial data frame through the serial port.
static void SendSerialData()
{
	if (!OpenPort())
	{
		return;
	}

	DWORD cbWritten = 0;

	if (!WriteFile(s_portHandle, reinterpret_cast<const void*>(runtime_constants::s_serialDataFrame), sizeof(runtime_constants::s_serialDataFrame), &cbWritten, nullptr)
		|| sizeof(runtime_constants::s_serialDataFrame) != static_cast<size_t>(cbWritten))
	{
		DisplayLastError();
		ClosePort();
	}
}

} // namespace serial_data

// Reset the LED strip.
static void ResetLEDs()
{
	runtime_constants::fill_serial_header();
	serial_data::SendSerialData();
}

// Update the LED strip.
static void UpdateLEDs()
{
	session_state::fill_serial_data();
	serial_data::SendSerialData();
}

#pragma endregion

#pragma region WINDOWS APPLICATION LOGIC

// Hidden window class name
static PCWSTR s_windowClassName = L"AdaLightListener";

static bool s_timerSet = false;

// Start the timer if it's not running.
static void StartTimer(HWND hwnd)
{
	if (!s_timerSet)
	{
		SetTimer(hwnd, 0, c_msecDelay, nullptr);
		s_timerSet = true;
	}
}

// Stop the timer if it is running.
static void StopTimer(HWND hwnd)
{
	if (s_timerSet)
	{
		KillTimer(hwnd, 0);
		s_timerSet = false;

		ResetLEDs();
	}
}

static bool s_connectedToConsole = !GetSystemMetrics(SM_REMOTESESSION);

// Handle attaching and reattaching to the console.
static void AttachToConsole(HWND hwnd)
{
	if (s_connectedToConsole)
	{
		session_state::create_resources(hwnd);
		StartTimer(hwnd);
	}
}

// Handle detaching from the console.
static void DetachFromConsole(HWND hwnd)
{
	if (s_connectedToConsole)
	{
		StopTimer(hwnd);
		session_state::free_resources();
		ResetLEDs();
	}
}

// Process window messages to the hidden window.
static LRESULT CALLBACK HiddenWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
			WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
			break;

		case WM_DESTROY:
			StopTimer(hwnd);
			WTSUnRegisterSessionNotification(hwnd);
			PostQuitMessage(0);
			break;

		case WM_WTSSESSION_CHANGE:
			switch (wParam)
			{
				case WTS_CONSOLE_CONNECT:
					s_connectedToConsole = true;
					AttachToConsole(hwnd);
					break;

				case WTS_CONSOLE_DISCONNECT:
					DetachFromConsole(hwnd);
					s_connectedToConsole = false;
					break;

				case WTS_SESSION_LOCK:
					DetachFromConsole(hwnd);
					break;

				case WTS_SESSION_UNLOCK:
					AttachToConsole(hwnd);
					break;

				default:
					break;
			}

			break;

		case WM_DISPLAYCHANGE:
			if (s_connectedToConsole)
			{
				session_state::free_resources();
				session_state::create_resources(hwnd);
			}
			break;

		case WM_TIMER:
			UpdateLEDs();
			break;

		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

// Show a message box for any errors.
static void DisplayLastError()
{
	DWORD errorCode = GetLastError();
	PWSTR errorString = nullptr;

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, 0, reinterpret_cast<PWSTR>(&errorString), 0, nullptr);
	MessageBoxW(HWND_DESKTOP, errorString, nullptr, MB_ICONERROR);
	LocalFree(reinterpret_cast<HLOCAL>(errorString));
}

// Run the program.
int WINAPI wWinMain(HINSTANCE hinstExe, HINSTANCE /*hinstPrev*/, PWSTR /*wzCmdLine*/, int /*nCmdShow*/)
{
	runtime_constants::initialize();

	//$TODO: Run in an interactive service? Tough to deal with user switching. Need to handle WM_DISPLAYCHANGE with a hidden top-level window.
	// Windows Service C++ sample: https://www.codeproject.com/articles/499465/simple-windows-service-in-cplusplus

	//$TODO: Send commands to COM3/auto-detect

	// Create the hidden window.
	WNDCLASSEXW windowClass = {
		sizeof(windowClass),	// cbSize
		0,						// style
		HiddenWindowProc,		// lpfnWndProc
		0,						// cbClsExtra
		0,						// cbWndExtra
		hinstExe,				// hInstance
		NULL,					// hIcon
		NULL,					// hCursor
		NULL,					// hbrBackground
		nullptr,				// lpszMenuName
		s_windowClassName,		// lpszClassName
		NULL					// hIconSm
	};

	if (!RegisterClassExW(&windowClass))
	{
		DisplayLastError();
		return 1;
	}

	HWND hwnd = CreateWindowExW(0, s_windowClassName, nullptr, 0, 0, 0, 0, 0, HWND_DESKTOP, NULL, hinstExe, nullptr);

	// Start processing messages/timers.
	AttachToConsole(hwnd);

	BOOL result;
	MSG msg;

	while (result = GetMessageW(&msg, NULL, 0, 0))
	{
		if (-1 == result)
		{
			DisplayLastError();
			return 2;
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return msg.wParam;
}

#pragma endregion