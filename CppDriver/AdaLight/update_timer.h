#pragma once

#include <windows.h>

#include "settings.h"

class update_timer
{
public:
	update_timer(const settings& parameters);

	bool start(HWND hwnd);
	bool stop(HWND hwnd);

	bool throttle(HWND hwnd);
	bool resume(HWND hwnd);

private:
	const settings& _parameters;
	UINT_PTR _timerId = 0;
	bool _timerThrottled = false;
};

