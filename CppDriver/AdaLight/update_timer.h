#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "settings.h"

class update_timer
{
public:
	update_timer(const settings& parameters, std::function<void(update_timer&)>&& onUpdate, std::function<void(update_timer&)>&& onStop);

	bool start();
	bool stop();

	bool throttle();
	bool resume();

private:
	const settings& _parameters;
	const std::function<void(update_timer&)> _onUpdate;
	const std::function<void(update_timer&)> _onStop;
	bool _timerStarted = false;
	bool _timerThrottled = false;
	bool _timerFired = false;
	bool _workerStarted = false;
	std::timed_mutex _timerMutex;
	std::mutex _workerMutex;
	std::condition_variable _workerCondition;
	std::thread _timerThread;
	std::thread _workerThread;
};
