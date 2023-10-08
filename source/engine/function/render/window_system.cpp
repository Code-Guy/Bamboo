#include "window_system.h"
#include "engine/core/base/macro.h"
#include "engine/core/config/config_manager.h"
#include "engine/core/event/event_system.h"

#include <tinygltf/stb_image.h>

namespace Bamboo
{
	void WindowSystem::init()
	{
		// initialize glfw
		if (!glfwInit())
		{
			LOG_FATAL("failed to initialize glfw");
			return;
		}

		m_width = g_engine.configManager()->getWindowWidth();
		m_height = g_engine.configManager()->getWindowHeight();

		// create glfw window
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(m_width, m_height, APP_NAME, nullptr, nullptr);
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

		// set input mode
		glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

		// set window icon
		GLFWimage images[1];
		images[0].pixels = stbi_load(TO_ABSOLUTE("asset/engine/texture/icon/bamboo_small.png").c_str(), &images[0].width, &images[0].height, 0, 4);
		glfwSetWindowIcon(m_window, 1, images);
		stbi_image_free(images[0].pixels);
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

	void WindowSystem::getScreenSize(int& width, int& height)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		width = mode->width;
		height = mode->height;
	}

	void WindowSystem::getMousePos(int& x, int& y)
	{
		x = m_mouse_pos_x;
		y = m_mouse_pos_y;
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
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowKeyEvent>(key, scancode, action, mods));
	}

	void WindowSystem::charCallback(GLFWwindow* window, unsigned int codepoint)
	{
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowCharEvent>(codepoint));
	}

	void WindowSystem::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
	{
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowCharModsEvent>(codepoint, mods));
	}

	void WindowSystem::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowMouseButtonEvent>(button, action, mods));
	}

	void WindowSystem::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		window_system->m_mouse_pos_x = xpos;
		window_system->m_mouse_pos_y = ypos;

		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowCursorPosEvent>(xpos, ypos));
	}

	void WindowSystem::cursorEnterCallback(GLFWwindow* window, int entered)
	{
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowCursorEnterEvent>(entered));
	}

	void WindowSystem::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowScrollEvent>(xoffset, yoffset));
	}

	void WindowSystem::dropCallback(GLFWwindow* window, int count, const char** paths)
	{
		WindowSystem* window_system = (WindowSystem*)glfwGetWindowUserPointer(window);
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowDropEvent>(count, paths, 
			window_system->m_mouse_pos_x, window_system->m_mouse_pos_y));
	}

	void WindowSystem::windowSizeCallback(GLFWwindow* window, int width, int height)
	{
		g_engine.eventSystem()->asyncDispatch(std::make_shared<WindowSizeEvent>(width, height));
	}

	void WindowSystem::windowCloseCallback(GLFWwindow* window)
	{
		glfwSetWindowShouldClose(window, true);
	}
}