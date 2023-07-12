#pragma once

#include <eventpp/eventqueue.h>
#include <vulkan/vulkan.h>

namespace Bamboo
{
	enum class EventType
	{
		WindowReset, WindowKey, WindowChar, WindowCharMods, WindowMouseButton,
		WindowCursorPos, WindowCursorEnter, WindowScroll, WindowDrop, WindowSize, WindowClose,
		RenderCreateSwapchainObjects, RenderDestroySwapchainObjects, RenderRecordFrame, RenderConstructUI,
		UISelectEntity
	};

	class Event
	{
	public:
		explicit Event(EventType type) : type(type) {}

		EventType type;
	};
	using EventPointer = std::shared_ptr<Event>;

	struct EventPolicy
	{
		static EventType getEvent(const EventPointer& event) 
		{
			return event->type;
		}
	};
	using EventQueue = eventpp::EventQueue<EventType, void(const EventPointer&), EventPolicy>;

	class WindowResetEvent : public Event
	{
	public:
		WindowResetEvent() : Event(EventType::WindowReset) {}
	};

	class WindowKeyEvent : public Event
	{
	public:
		WindowKeyEvent(int key, int scancode, int action, int mods) : Event(EventType::WindowKey),
			key(key), scancode(scancode), action(action), mods(mods)
		{
		}

		int key;
		int scancode;
		int action;
		int mods;
	};

	class WindowCharEvent : public Event
	{
	public:
		WindowCharEvent(unsigned int codepoint) : Event(EventType::WindowChar),
			codepoint(codepoint)
		{
		}

		unsigned int codepoint;
	};

	class WindowCharModsEvent : public Event
	{
	public:
		WindowCharModsEvent(unsigned int codepoint, int mods) : Event(EventType::WindowCharMods),
			codepoint(codepoint), mods(mods)
		{
		}

		unsigned int codepoint;
		int mods;
	};

	class WindowMouseButtonEvent : public Event
	{
	public:
		WindowMouseButtonEvent(int button, int action, int mods) : Event(EventType::WindowMouseButton),
			button(button), action(action), mods(mods)
		{
		}

		int button;
		int action;
		int mods;
	};

	class WindowCursorPosEvent : public Event
	{
	public:
		WindowCursorPosEvent(double xpos, double ypos) : Event(EventType::WindowCursorPos),
			xpos(xpos), ypos(ypos)
		{
		}

		double xpos;
		double ypos;
	};

	class WindowCursorEnterEvent : public Event
	{
	public:
		WindowCursorEnterEvent(int entered) : Event(EventType::WindowCursorEnter),
			entered(entered)
		{
		}

		int entered;
	};

	class WindowScrollEvent : public Event
	{
	public:
		WindowScrollEvent(double xoffset, double yoffset) : Event(EventType::WindowScroll),
			xoffset(xoffset), yoffset(yoffset)
		{
		}

		double xoffset;
		double yoffset;
	};

	class WindowDropEvent : public Event
	{
	public:
		WindowDropEvent(int count, const char** paths, double xpos, double ypos) : Event(EventType::WindowDrop),
			count(count), paths(paths), xpos(xpos), ypos(ypos)
		{
		}

		int count;
		const char** paths;
		double xpos;
		double ypos;
	};

	class WindowSizeEvent : public Event
	{
	public:
		WindowSizeEvent(int width, int height) : Event(EventType::WindowSize),
			width(width), height(height)
		{
		}

		int width;
		int height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() : Event(EventType::WindowClose)
		{
		}
	};

	class RenderCreateSwapchainObjectsEvent : public Event
	{
	public:
		RenderCreateSwapchainObjectsEvent(uint32_t width, uint32_t height) : Event(EventType::RenderCreateSwapchainObjects),
			width(width), height(height)
		{
		}

		uint32_t width;
		uint32_t height;
	};

	class RenderDestroySwapchainObjectsEvent : public Event
	{
	public:
		RenderDestroySwapchainObjectsEvent() : Event(EventType::RenderDestroySwapchainObjects)
		{
		}
	};

	class RenderRecordFrameEvent : public Event
	{
	public:
		RenderRecordFrameEvent(VkCommandBuffer command_buffer, uint32_t flight_index) : Event(EventType::RenderRecordFrame),
			command_buffer(command_buffer), flight_index(flight_index)
		{
		}

		VkCommandBuffer command_buffer;
		uint32_t flight_index;
	};

	class RenderConstructUIEvent : public Event
	{
	public:
		RenderConstructUIEvent() : Event(EventType::RenderConstructUI)
		{
		}
	};
	
	class UISelectEntityEvent : public Event
	{
	public:
		UISelectEntityEvent(uint32_t entity_id) : Event(EventType::UISelectEntity),
			entity_id(entity_id)
		{
		}

		uint32_t entity_id;
	};

	class EventSystem
	{
	public:
		void init() {}
		void tick();
		void destroy() {}

		void addListener(EventType event_type, const std::function<void(const EventPointer& event_pointer)>& callback);
		void asyncDispatch(const EventPointer& event_pointer);
		void syncDispatch(const EventPointer& event_pointer);

	private:
		EventQueue m_event_queue;
	};
}