#include "stdafx.h"
#include "update_timer.h"

update_timer::update_timer(const settings& parameters)
	: _parameters(parameters)
{
}

bool update_timer::start(HWND hwnd)
{
	if (0 == _timerId)
	{
		_timerId = SetTimer(hwnd, 1, _timerThrottled ? _parameters.throttleTimer : _parameters.delay, nullptr);
		return true;
	}

	return false;
}

bool update_timer::stop(HWND hwnd)
{
	if (0 != _timerId)
	{
		KillTimer(hwnd, _timerId);
		_timerId = 0;
		return true;
	}

	return false;
}

bool update_timer::throttle(HWND hwnd)
{
	if (!_timerThrottled)
	{
		_timerThrottled = true;

		if (0 != _timerId)
		{
			_timerId = SetTimer(hwnd, _timerId, _parameters.throttleTimer, nullptr);
			return true;
		}
	}

	return false;
}

bool update_timer::resume(HWND hwnd)
{
	if (_timerThrottled)
	{
		_timerThrottled = false;

		if (0 != _timerId)
		{
			_timerId = SetTimer(hwnd, _timerId, _parameters.delay, nullptr);
			return true;
		}
	}

	return false;
}
