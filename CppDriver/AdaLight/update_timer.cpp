#include "stdafx.h"
#include "update_timer.h"

update_timer::update_timer(const settings& parameters, std::function<void(std::shared_ptr<update_timer>)>&& onUpdate, std::function<void(std::shared_ptr<update_timer>)>&& onStop)
	: _parameters(parameters)
	, _onUpdate(std::move(onUpdate))
	, _onStop(std::move(onStop))
{
}

bool update_timer::start()
{
	if (!_timerStarted)
	{
		auto timer = shared_from_this();

		_timerMutex.lock();
		_timerStarted = true;

		std::lock_guard<std::mutex> workerGuard(_workerMutex);

		_workerStarted = true;
		_workerThread = std::thread([timer]()
		{
			std::unique_lock<std::mutex> workerLock(timer->_workerMutex);

			while (timer->_workerStarted)
			{
				timer->_workerCondition.wait(workerLock, [timer]()
				{
					return timer->_timerFired;
				});

				timer->_timerFired = false;
				timer->_onUpdate(timer);
			}

			timer->_onStop(timer);
		});

		_timerThread = std::thread([timer]()
		{
			bool stopped = false;
			std::unique_lock<std::timed_mutex> timerLock(timer->_timerMutex, std::defer_lock);
			auto started = std::chrono::high_resolution_clock::now();

			while (!stopped)
			{
				const auto delay = std::chrono::milliseconds(timer->_timerThrottled
					? timer->_parameters.throttleTimer
					: timer->_parameters.delay);
				const auto until = started + delay;

				// This should always timeout as long as the _timerMutex is locked by the main thread.
				stopped = timerLock.try_lock_until(until);
				started = std::chrono::high_resolution_clock::now();

				std::unique_lock<std::mutex> workerLock(timer->_workerMutex);

				if (stopped)
				{
					timer->_workerStarted = false;
				}

				timer->_timerFired = true;
				workerLock.unlock();
				timer->_workerCondition.notify_one();
			}

			timer->_workerThread.join();
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
