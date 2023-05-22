#include "window_system.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{
	void WindowSystem::init(const WindowCreateInfo& window_ci)
	{
		// initialize glfw
		if (!glfwInit())
		{
			LOG_FATAL("failed to initialize glfw");
			return;
		}

		m_width = window_ci.width;
		m_height = window_ci.height;

		// create glfw window
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(m_width, m_height, window_ci.title.c_str(), nullptr, nullptr);
		if (!m_window)
		{
			LOG_FATAL("failed to create glfw window");
			glfwTerminate();
			return;
		}

		//  set up glfw input callbacks
		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, keyCallback);
		glfwSetCharCallback(m_window, charCallback);
		glfwSetCharModsCallback(m_window, charModsCallback);
		glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
		glfwSetCursorPosCallback(m_window, cursorPosCallback);
		glfwSetCursorEnterCallback(m_window, cursorEnterCallback);
		glfwSetScrollCallback(m_window, scrollCallback);
		glfwSetDropCallback(m_window, dropCallback);
		glfwSetWindowSizeCallback(m_window, windowSizeCallback);
		glfwSetWindowCloseCallback(m_window, windowCloseCallback);

		glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	}

	void WindowSystem::destroy()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void WindowSystem::pollEvents()
	{
		glfwPollEvents();
	}

	bool WindowSystem::shouldClose()
	{
		return (bool)glfwWindowShouldClose(m_window);
	}

	void WindowSystem::setTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_window, title.c_str());
	}

	void WindowSystem::getWindowSize(int& width, int& height)
	{
		width = m_width;
		height = m_height;
	}

	bool WindowSystem::isMouseButtonDown(int button)
	{
		if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST)
		{
			return false;
		}
		return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
	}

	void WindowSystem::setIsFocus(bool is_focus)
	{
		m_is_focus = is_focus;
		glfwSetInputMode(m_window, GLFW_CURSOR, m_is_focus ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	void WindowSystem::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onKeyFuncs)
		{
			func(key, scancode, action, mods);
		}
	}

	void WindowSystem::charCallback(GLFWwindow* window, unsigned int codepoint)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onCharFuncs)
		{
			func(codepoint);
		}
	}

	void WindowSystem::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onCharModsFuncs)
		{
			func(codepoint, mods);
		}
	}

	void WindowSystem::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onMouseButtonFuncs)
		{
			func(button, action, mods);
		}
	}

	void WindowSystem::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onCursorPosFuncs)
		{
			func(xpos, ypos);
		}
	}

	void WindowSystem::cursorEnterCallback(GLFWwindow* window, int entered)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onCursorEnterFuncs)
		{
			func(entered);
		}
	}

	void WindowSystem::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onScrollFuncs)
		{
			func(xoffset, yoffset);
		}
	}

	void WindowSystem::dropCallback(GLFWwindow* window, int count, const char** paths)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onDropFuncs)
		{
			func(count, paths);
		}
	}

	void WindowSystem::windowSizeCallback(GLFWwindow* window, int width, int height)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		for (auto& func : window_system->m_onWindowSizeFuncs)
		{
			func(width, height);
		}
	}

	void WindowSystem::windowCloseCallback(GLFWwindow* window)
	{
		glfwSetWindowShouldClose(window, true);
	}
}