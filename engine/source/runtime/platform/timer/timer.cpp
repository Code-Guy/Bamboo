#include "timer.h"
#include <chrono>

namespace Bamboo
{

	void TimerManager::init()
	{
		m_time = 0.0f;
	}

	void TimerManager::tick(float delta_time)
	{
		m_time += delta_time;
		for (auto iter = m_timers.begin(); iter != m_timers.end();)
		{
			Timer& timer = iter->second;
			timer.current_time += delta_time;
			if (timer.current_time > timer.interval)
			{
				timer.func();
				if (timer.loop)
				{
					while (timer.current_time > timer.interval)
					{
						timer.current_time -= timer.interval;
					}
				}
				else
				{
					iter = m_timers.erase(iter);
					continue;
				}
			}
			++iter;
		}
	}

	void TimerManager::destroy()
	{
		m_timers.clear();
	}

	TimerHandle TimerManager::addTimer(float interval, const std::function<void(void)>& func, bool loop, bool loop_im_call)
	{
		m_timers[m_timer_handle++] = { interval, func, loop, (loop && loop_im_call) ? (interval + 0.001f) : 0.0f };
		return m_timer_handle;
	}

	void TimerManager::removeTimer(TimerHandle timer_handle)
	{
		if (m_timers.find(timer_handle) != m_timers.end())
		{
			m_timers.erase(timer_handle);
		}
	}

	void StopWatch::start()
	{
		m_start_time = std::chrono::steady_clock::now();
	}

	long long StopWatch::stop()
	{
		m_end_time = std::chrono::steady_clock::now();
		m_elapsed_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_end_time - m_start_time).count();
		return m_elapsed_time_ms;
	}

	float StopWatch::elapsedTime()
	{
		return m_elapsed_time_ms / 1000.0;
	}

	long long StopWatch::elapsedTimeMs()
	{
		return m_elapsed_time_ms;
	}

	TimeOuter::TimeOuter(float out_time) : out_time(out_time)
	{
		trigger_timestamp = 0;
	}

	void TimeOuter::trigger()
	{
		trigger_timestamp = getTimestampMs();
	}

	bool TimeOuter::isTimeOut() const
	{
		return (getTimestampMs() - trigger_timestamp) * 0.001f > out_time;
	}

	long long getTimestampMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

}