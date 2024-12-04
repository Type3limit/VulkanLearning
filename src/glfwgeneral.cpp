#include "glfwgeneral.h"
#include "renderBuffer.h"
#include "shader.h"
#define USE_FRAME_IN_FLIGHT 1

GlfwWrapper* g_pWrapperInstance = nullptr;

void OnWindowKeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (!g_pWrapperInstance || !window)
        return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        g_pWrapperInstance->MakeWindowWindowed({100, 100}, {1280, 720});
    }
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
    {
        g_pWrapperInstance->MakeWindowFullScreen();
    }
	if (key == GLFW_KEY_F9 && action == GLFW_PRESS)
	{
		vulkan::GraphicsBase::Base().RecreateSwapchain(false);
	}
	if (key == GLFW_KEY_F10 && action == GLFW_PRESS)
	{
		vulkan::GraphicsBase::Base().RecreateSwapchain(true);
	}
}

GlfwWrapper::GlfwWrapper()
{
    g_pWrapperInstance = this;
}

GlfwWrapper::~GlfwWrapper()
{
    g_pWrapperInstance = nullptr;
}

int GlfwWrapper::RunWindowLoop()
{
    {
        if (!InitializeWindow({1280, 720}, false, true, true))
            return -1;

		const auto& [renderPass, framebuffers] = RenderPassAndFramebuffers();
		CreateLayout();
		CreatePipeline(); 

        VkClearValue clearColor = {.color = {1.f, 0.f, 0.f, 1.f}};
#if USE_FRAME_IN_FLIGHT
		//使用即时帧
        struct perFrameObjects_t
        {
            vulkan::Fence fence{VK_FENCE_CREATE_SIGNALED_BIT};
            vulkan::Semaphore semaphore_imageIsAvailable;
            vulkan::Semaphore semaphore_renderingIsOver;
            vulkan::CommandBuffer commandBuffer;
        };
        std::vector<perFrameObjects_t> perFrameObjects(vulkan::GraphicsBase::Base().SwapchainImageCount());
        vulkan::CommandPool commandPool(vulkan::GraphicsBase::Base().QueueFamilyIndex_Graphics(),
                                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        for (auto& i : perFrameObjects)
            commandPool.AllocateBuffers(i.commandBuffer);
        uint32_t i = 0;

        while (!glfwWindowShouldClose(m_pWindow))
        {
		    //最小化时停止渲染
            while (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED))
                glfwWaitEvents();

            const auto& [fence, semaphore_imageIsAvailable, semaphore_renderingIsOver, commandBuffer] = perFrameObjects[
                i];
            i = (i + 1) % vulkan::GraphicsBase::Base().SwapchainImageCount();

            fence.WaitAndReset();
            vulkan::GraphicsBase::Base().SwapImage(semaphore_imageIsAvailable);

            commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            renderPass.CmdBegin(commandBuffer, framebuffers[i], {{}, windowSize}, clearColor);
		    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            renderPass.CmdEnd(commandBuffer);
            commandBuffer.End();

            vulkan::GraphicsBase::Base().SubmitCommandBufferGraphics(commandBuffer, semaphore_imageIsAvailable,
                                                                     semaphore_renderingIsOver, fence);
            vulkan::GraphicsBase::Base().PresentImage(semaphore_renderingIsOver);

            glfwPollEvents();
            TitleFps();
        }
#else
		vulkan::Fence fence(VK_FENCE_CREATE_SIGNALED_BIT); 
		vulkan::Semaphore semaphore_imageIsAvailable;
		vulkan::Semaphore semaphore_renderingIsOver;

		vulkan::CommandBuffer commandBuffer;
		vulkan::CommandPool commandPool(vulkan::GraphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		commandPool.AllocateBuffers(commandBuffer);

		while (!glfwWindowShouldClose(m_pWindow)) {
			while (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED))
				glfwWaitEvents();

			fence.WaitAndReset();

			vulkan::GraphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
			auto i = vulkan::GraphicsBase::Base().CurrentImageIndex();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			renderPass.CmdEnd(commandBuffer);
			commandBuffer.End();

			vulkan::GraphicsBase::Base().SubmitCommandBufferGraphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
			vulkan::GraphicsBase::Base().PresentImage(semaphore_renderingIsOver);
			glfwPollEvents();
			TitleFps();
		}
#endif // USE_FRAME_IN_FLIGHT
        TerminateWindow();
        return 0;
    }
}

void GlfwWrapper::MakeWindowFullScreen()
{
    if (!m_pMonitor || !m_pWindow)
        return;
    const GLFWvidmode* pMode = glfwGetVideoMode(m_pMonitor);
    glfwSetWindowMonitor(m_pWindow, m_pMonitor, 0, 0, pMode->width, pMode->height, pMode->refreshRate);
}

void GlfwWrapper::MakeWindowWindowed(VkOffset2D position, VkExtent2D size)
{
    if (!m_pMonitor || !m_pWindow)
        return;
    const GLFWvidmode* pMode = glfwGetVideoMode(m_pMonitor);
    glfwSetWindowMonitor(m_pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
}

GLFWmonitor* GlfwWrapper::MonitorInstance()
{
    return m_pMonitor;
}

void GlfwWrapper::CreateLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);
}

void GlfwWrapper::CreatePipeline()
{
	static vulkan::ShaderModule vert("./shader/Triangle.vert.spv");
	static vulkan::ShaderModule frag("./shader/Triangle.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
		vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		vulkan::GraphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;
		pipeline_triangle.Create(pipelineCiPack);
		};

	auto Destroy = [] {
		pipeline_triangle.~Pipeline();
		};

	vulkan::GraphicsBase::Base().AddCallback_CreateSwapchain(Create);
	vulkan::GraphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	//调用Create()以创建管线
	Create();
}

void GlfwWrapper::TitleFps()
{
    static double time0 = glfwGetTime();
    static double time1;
    static double dt;
    static int dframe = -1;
    static std::stringstream info;
    time1 = glfwGetTime();
    dframe++;
    if ((dt = time1 - time0) >= 1)
    {
        info.precision(1);
        info << m_windowTitle << "    " << std::fixed << dframe / dt << " FPS";
        glfwSetWindowTitle(m_pWindow, info.str().c_str());
        info.str(""); //clear stringstream
        time0 = time1;
        dframe = 0;
    }
}

bool GlfwWrapper::InitializeWindow(VkExtent2D size, bool fullScreen, bool isResizable, bool limitFrameRate)
{
    ///��ʼ��GLFW
    if (!glfwInit())
    {
        std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_RESIZABLE, isResizable);

    uint32_t extensionCount = 0;
    const char** extensionNames;

    extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);


    if (!extensionNames)
    {
        std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
        glfwTerminate();
        return false;
    }
    for (size_t i = 0; i < extensionCount; i++)
        vulkan::GraphicsBase::Base().AddInstanceExtension(extensionNames[i]);

    vulkan::GraphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);


    m_pMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* pMode = glfwGetVideoMode(m_pMonitor);

    m_pWindow = fullScreen
                    ? glfwCreateWindow(pMode->width, pMode->height, m_windowTitle.data(), m_pMonitor, nullptr)
                    : glfwCreateWindow(size.width, size.height, m_windowTitle.data(), nullptr, nullptr);
    if (!m_pWindow)
    {
        std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
        TerminateWindow();
        return false;
    }

    glfwSetKeyCallback(m_pWindow, OnWindowKeyCallBack);

#ifdef _WIN32
    vulkan::GraphicsBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    vulkan::GraphicsBase::Base().AddInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
	uint32_t extensionCount = 0;
	const char** extensionNames;
	extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
	if (!extensionNames) {
		std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
		glfwTerminate();
		return false;
	}
	for (size_t i = 0; i < extensionCount; i++)
		vulkan::GraphicsBase::Base().AddInstanceExtension(extensionNames[i]);
#endif
    vulkan::GraphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    vulkan::GraphicsBase::Base().UseLatestApiVersion();
    if (vulkan::GraphicsBase::Base().CreateInstance())
        return false;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (VkResult result =
        glfwCreateWindowSurface(vulkan::GraphicsBase::Base().Instance(), m_pWindow, nullptr, &surface))
    {
        std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n",
                                 int32_t(result));
        glfwTerminate();
        return false;
    }
    vulkan::GraphicsBase::Base().Surface(surface);
    
	//正常运行返回0，所以||为真时返回false
    if (vulkan::GraphicsBase::Base().GetPhysicalDevices() ||
        vulkan::GraphicsBase::Base().DeterminePhysicalDevice(0, true, false) ||
        vulkan::GraphicsBase::Base().CreateDevice())
        return false;
    if (vulkan::GraphicsBase::Base().CreateSwapchain(limitFrameRate))
        return false;
    return true;
}

void GlfwWrapper::TerminateWindow()
{
    glfwTerminate();
    vulkan::GraphicsBase::Base().WaitIdle();
}
