#include "event_system.h"

namespace Bamboo
{
	void EventSystem::tick()
	{
		m_event_queue.process();
	}

	void* EventSystem::addListener(EEventType event_type, const std::function<void(const EventPointer& event_pointer)>& callback)
	{
		EventHandle* event_handle = new EventHandle;
		event_handle->type = event_type;
		event_handle->handle = m_event_queue.appendListener(event_type, callback);
		return event_handle;
	}

	void EventSystem::removeListener(void* handle)
	{
		EventHandle* event_handle = (EventHandle*)handle;
		m_event_queue.removeListener(event_handle->type, event_handle->handle);
		delete event_handle;
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