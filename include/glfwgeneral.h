#include "vkbase.h"
#include "synchronization.h"
#include "command.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib") //链接编译所需的静态库


class GlfwWrapper
{
public:
	GlfwWrapper();
	~GlfwWrapper();

public:
	int RunWindowLoop();

	/// <summary>
	/// Press F11 to make window full screen
	/// </summary>
	void MakeWindowFullScreen();

	/// <summary>
	/// Press ESC to make window windowed
	/// </summary>
	/// <param name="position"></param>
	/// <param name="size"></param>
	void MakeWindowWindowed(VkOffset2D position,VkExtent2D size);

    GLFWmonitor* MonitorInstance();

private:

	void TitleFps();

	bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true);

	void TerminateWindow();
private:
	GLFWwindow* m_pWindow{nullptr};
	GLFWmonitor* m_pMonitor{ nullptr };
	std::string m_windowTitle{ "Vulkan Learning" };
};
