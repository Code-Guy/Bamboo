#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <functional>
#include <vector>

namespace Bamboo
{
	struct WindowCreateInfo
	{
		int width;
		int height;
		std::string title;
	};

	class WindowSystem
	{
	public:
		void init(const WindowCreateInfo& window_ci);
		void destroy();

		void pollEvents();
		bool shouldClose();
		GLFWwindow* getWindow() { return m_window; }
		void getWindowSize(int& width, int& height);

		typedef std::function<void()> OnResetFunc;
		typedef std::function<void(int, int, int, int)> OnKeyFunc;
		typedef std::function<void(unsigned int)> OnCharFunc;
		typedef std::function<void(int, unsigned int)> OnCharModsFunc;
		typedef std::function<void(int, int, int)> OnMouseButtonFunc;
		typedef std::function<void(double, double)> OnCursorPosFunc;
		typedef std::function<void(int)> OnCursorEnterFunc;
		typedef std::function<void(double, double)> OnScrollFunc;
		typedef std::function<void(int, const char**)> OnDropFunc;
		typedef std::function<void(int, int)> OnWindowSizeFunc;
		typedef std::function<void()> OnWindowCloseFunc;

		void registerOnKeyFunc(OnKeyFunc func) { m_onKeyFuncs.push_back(func); }
		void registerOnCharFunc(OnCharFunc func) { m_onCharFuncs.push_back(func); }
		void registerOnCharModsFunc(OnCharModsFunc func) { m_onCharModsFuncs.push_back(func); }
		void registerOnMouseButtOnFunc(OnMouseButtonFunc func) { m_onMouseButtonFuncs.push_back(func); }
		void registerOnCursorPosFunc(OnCursorPosFunc func) { m_onCursorPosFuncs.push_back(func); }
		void registerOnCursorEnterFunc(OnCursorEnterFunc func) { m_onCursorEnterFuncs.push_back(func); }
		void registerOnScrollFunc(OnScrollFunc func) { m_onScrollFuncs.push_back(func); }
		void registerOnDropFunc(OnDropFunc func) { m_onDropFuncs.push_back(func); }
		void registerOnWindowSizeFunc(OnWindowSizeFunc func) { m_onWindowSizeFuncs.push_back(func); }
		void registerOnWindowCloseFunc(OnWindowCloseFunc func) { m_onWindowCloseFuncs.push_back(func); }

		bool isMouseButtonDown(int button);
		bool getIsFocus() const { return m_is_focus; }
		void setIsFocus(bool is_focus);

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
		int m_width;
		int m_height;
		bool m_is_focus;

		std::vector<OnKeyFunc>         m_onKeyFuncs;
		std::vector<OnCharFunc>        m_onCharFuncs;
		std::vector<OnCharModsFunc>    m_onCharModsFuncs;
		std::vector<OnMouseButtonFunc> m_onMouseButtonFuncs;
		std::vector<OnCursorPosFunc>   m_onCursorPosFuncs;
		std::vector<OnCursorEnterFunc> m_onCursorEnterFuncs;
		std::vector<OnScrollFunc>      m_onScrollFuncs;
		std::vector<OnDropFunc>        m_onDropFuncs;
		std::vector<OnWindowSizeFunc>  m_onWindowSizeFuncs;
		std::vector<OnWindowCloseFunc> m_onWindowCloseFuncs;
	};
}