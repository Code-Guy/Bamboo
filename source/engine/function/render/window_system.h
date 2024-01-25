#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <functional>
#include <vector>

namespace Bamboo
{
	class WindowSystem
	{
	public:
		void init();
		void destroy();

		void pollEvents();
		bool shouldClose();
		void setTitle(const std::string& title);
		GLFWwindow* getWindow() { return m_window; }
		void getWindowSize(int& width, int& height);
		float getMonitorDPIScale();
		void getScreenSize(int& width, int& height);
		void getMousePos(int& x, int& y);

		bool isMouseButtonDown(int button);
		bool getFocus() const { return m_focus; }
		void setFocus(bool focus);

		void toggleFullscreen();

	private:
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void charCallback(GLFWwindow* window, unsigned int codepoint);
		static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		static void cursorEnterCallback(GLFWwindow* window, int entered);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		static void dropCallback(GLFWwindow* window, int count, const char** paths);
		static void windowSizeCallback(GLFWwindow* window, int width, int height);
		static void windowCloseCallback(GLFWwindow* window);

		GLFWwindow* m_window;
		int m_mouse_pos_x;
		int m_mouse_pos_y;
		bool m_focus;
		bool m_fullscreen;

		int m_windowed_width, m_windowed_height;
		int m_windowed_pos_x, m_windowed_pos_y;
		float m_monitor_content_scale;
	};
}