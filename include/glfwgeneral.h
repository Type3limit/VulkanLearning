#include "vkbase.h"
#include "synchronization.h"
#include "command.h"
#include "pipeline.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "rpwf.h"
#pragma comment(lib, "glfw3.lib")
#pragma once

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


	static auto& RenderPassAndFramebuffers() {
		static const auto& rpwf = vulkanLearning::CreateRpwf_Screen();
		return rpwf;
	}
	//该函数用于创建管线布局
	static void CreateLayout();
	//该函数用于创建管线
	static void CreatePipeline();

public:

	static vulkan::PipelineLayout pipelineLayout_triangle;//管线布局

	static vulkan::Pipeline pipeline_triangle;//管线

private:

	void TitleFps();

	bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true);

	void TerminateWindow();
private:
	GLFWwindow* m_pWindow{nullptr};
	GLFWmonitor* m_pMonitor{ nullptr };
	std::string m_windowTitle{ "Vulkan Learning" };

};

inline vulkan::PipelineLayout  GlfwWrapper::pipelineLayout_triangle;
inline vulkan::Pipeline GlfwWrapper::pipeline_triangle;


struct Vertex {
	glm::vec2 position;
	glm::vec4 color;
};
