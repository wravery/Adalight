#include "stdafx.h"
#include "update_timer.h"

update_timer::update_timer(const settings& parameters, std::function<void(update_timer&)>&& onUpdate, std::function<void(update_timer&)>&& onStop)
	: _parameters(parameters)
	, _onUpdate(std::move(onUpdate))
	, _onStop(std::move(onStop))
{
}

bool update_timer::start()
{
	if (!_timerStarted)
	{
		_timerMutex.lock();
		_timerStarted = true;

		std::lock_guard<std::mutex> workerGuard(_workerMutex);

		_workerStarted = true;
		_workerThread = std::thread([this]()
		{
			std::unique_lock<std::mutex> workerLock(_workerMutex);

			while (_workerStarted)
			{
				_workerCondition.wait(workerLock, [this]()
				{
					return _timerFired;
				});

				_timerFired = false;
				_onUpdate(*this);
			}

			_onStop(*this);
		});

		_timerThread = std::thread([this]()
		{
			bool stopped = false;
			std::unique_lock<std::timed_mutex> timerLock(_timerMutex, std::defer_lock);

			while (!stopped)
			{
				const auto delay = std::chrono::milliseconds(_timerThrottled
					? _parameters.throttleTimer
					: _parameters.delay);

				// This should always timeout as long as the _timerMutex is locked by the main thread.
				stopped = timerLock.try_lock_for(delay);

				std::unique_lock<std::mutex> workerLock(_workerMutex);

				if (stopped)
				{
					_workerStarted = false;
				}

				_timerFired = true;
				workerLock.unlock();
				_workerCondition.notify_one();
			}

			_workerThread.join();
		});

		return true;
	}

	return false;
}

bool update_timer::stop()
{
	if (_timerStarted)
	{
		_timerStarted = false;
		_timerMutex.unlock();
		_timerThread.join();

		return true;
	}

	return false;
}

bool update_timer::throttle()
{
	if (!_timerThrottled)
	{
		_timerThrottled = true;

		if (_timerStarted)
		{
			return true;
		}
	}

	return false;
}

bool update_timer::resume()
{
	if (_timerThrottled)
	{
		_timerThrottled = false;

		if (_timerStarted)
		{
			return true;
		}
	}

	return false;
}
