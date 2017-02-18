// AdaLight.cpp
//
// This is a port of the Processing (Java) driver for the AdaLight project to a C++ Windows Application.
// This version runs without any UI, so you might want to use the preview UI in the original Processing
// application first to figure out your settings and then copy those variables to settings.* and recompile.
//
// AdaLight guide: https://learn.adafruit.com/adalight-diy-ambient-tv-lighting/pieces?view=all#download-and-install
// Serial port programming sample: https://code.msdn.microsoft.com/windowsdesktop/Serial-Port-Sample-e8accf30/sourcecode?fileId=67164&pathId=1394200469
// DirectX 11 C++ references: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476082(v=vs.85).aspx

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

#include "stdafx.h"

#include <cmath>
#include <string>
#include <sstream>

#include "settings.h"
#include "gamma_correction.h"
#include "serial_buffer.h"
#include "screen_samples.h"
#include "serial_port.h"
#include "update_timer.h"

// TODO: Put this in some sort of persistence? Create configuration UI? Parse the settings from the command line?
static const settings parameters;

static serial_buffer serial(parameters);
static gamma_correction gamma;
static screen_samples samples(parameters, gamma);
static serial_port port(parameters);

static update_timer timer(parameters, [](update_timer& timer)
{
	// Try to get the resources and resume the timer
	if (samples.empty())
	{
		if (samples.create_resources())
		{
			timer.resume();
		}
		else if (timer.throttle())
		{
			// Reset the LED strip.
			serial.clear();
			port.send(serial);
			return;
		}
	}

	// Update the LED strip.
	samples.take_samples(serial);
	port.send(serial);
}, [](update_timer& /*timer*/)
{
	// Reset the LED strip.
	serial.clear();
	port.send(serial);
});

// Hidden window class name
static PCWSTR s_windowClassName = L"AdaLightListener";

static bool s_connectedToConsole = !GetSystemMetrics(SM_REMOTESESSION);

// Handle attaching and reattaching to the console.
static void AttachToConsole()
{
	if (s_connectedToConsole)
	{
		port.open();

		if (samples.create_resources())
		{
			timer.resume();
		}

		timer.start();
	}
}

// Handle detaching from the console.
static void DetachFromConsole()
{
	if (s_connectedToConsole)
	{
		timer.stop();
		samples.free_resources();
		port.close();
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
			WTSUnRegisterSessionNotification(hwnd);
			DetachFromConsole();
			PostQuitMessage(0);
			break;

		case WM_WTSSESSION_CHANGE:
			switch (wParam)
			{
				case WTS_CONSOLE_CONNECT:
					s_connectedToConsole = true;
					AttachToConsole();
					break;

				case WTS_CONSOLE_DISCONNECT:
					DetachFromConsole();
					s_connectedToConsole = false;
					break;

				case WTS_SESSION_LOCK:
					DetachFromConsole();
					break;

				case WTS_SESSION_UNLOCK:
					AttachToConsole();
					break;

				default:
					break;
			}

			break;

		case WM_DISPLAYCHANGE:
			if (s_connectedToConsole)
			{
				samples.free_resources();
				samples.create_resources();
			}
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
	// TODO: Run in an interactive service? Tough to deal with user switching. Need to handle WM_DISPLAYCHANGE with a hidden top-level window.
	// Windows Service C++ sample: https://www.codeproject.com/articles/499465/simple-windows-service-in-cplusplus

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
	AttachToConsole();

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
