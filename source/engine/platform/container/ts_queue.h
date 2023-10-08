#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

// thread-safe queue
template <typename T>
class TSQueue
{
public:
	void push(const T& item)
	{
		// acquire lock
		std::unique_lock<std::mutex> lock(m_mutex);

		// add item
		m_queue.push(item);

		// notify one thread that is waiting
		m_cond.notify_one();
	}

	T pop()
	{
		// acquire lock
		std::unique_lock<std::mutex> lock(m_mutex);

		// wait until queue is not empty
		m_cond.wait(lock, [this]() { return !m_queue.empty(); });

		// retrieve item
		T item = m_queue.front();
		m_queue.pop();

		// return item
		return item;
	}

	bool empty()
	{
		// acquire lock
		std::unique_lock<std::mutex> lock(m_mutex);

		return m_queue.empty();
	}

private:
	// underlying queue
	std::queue<T> m_queue;

	// mutex for thread synchronization
	std::mutex m_mutex;

	// condition variable for signaling
	std::condition_variable m_cond;
};