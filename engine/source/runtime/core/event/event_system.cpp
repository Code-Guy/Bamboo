#include "event_system.h"

namespace Bamboo
{
	void EventSystem::tick()
	{
		m_event_queue.process();
	}

	void EventSystem::addListener(EventType event_type, const std::function<void(const EventPointer& event_pointer)>& callback)
	{
		m_event_queue.appendListener(event_type, callback);
	}

	void EventSystem::asyncDispatch(const EventPointer& event_pointer)
	{
		m_event_queue.enqueue(event_pointer);
	}

	void EventSystem::syncDispatch(const EventPointer& event_pointer)
	{
		m_event_queue.dispatch(event_pointer);
	}

}