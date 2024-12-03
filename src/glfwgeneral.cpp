#include "glfwgeneral.h"


GlfwWrapper* g_pWrapperInstance = nullptr;

void OnWindowKeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (!g_pWrapperInstance ||!window)
		return;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		g_pWrapperInstance->MakeWindowWindowed({}, { 1280,720 });
	}
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
	{
		g_pWrapperInstance->MakeWindowFullScreen();
	}
}

GlfwWrapper::GlfwWrapper() {
	g_pWrapperInstance = this;
}

GlfwWrapper::~GlfwWrapper() {
	g_pWrapperInstance = nullptr;
}

int GlfwWrapper::RunWindowLoop()
{
	{
		if (!InitializeWindow({ 1280,720 }))
			return -1;

		vulkan::Fence fence(VK_FENCE_CREATE_SIGNALED_BIT); //以置位状态创建栅栏
		vulkan::Semaphore semaphore_imageIsAvailable;
		vulkan::Semaphore semaphore_renderingIsOver;

		vulkan::CommandBuffer commandBuffer;
		vulkan::CommandPool commandPool(vulkan::GraphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		commandPool.AllocateBuffers(commandBuffer);

		while (!glfwWindowShouldClose(m_pWindow)) {
			//窗口最小化时阻塞渲染循环
			while (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED))
				glfwWaitEvents();

			fence.WaitAndReset();

			vulkan::GraphicsBase::Base().SwapImage(semaphore_imageIsAvailable);

			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			/*渲染命令，待填充*/
			commandBuffer.End();

			vulkan::GraphicsBase::Base().SubmitCommandBufferGraphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
			vulkan::GraphicsBase::Base().PresentImage(semaphore_renderingIsOver);
			glfwPollEvents();
			TitleFps();
		}
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

void GlfwWrapper::TitleFps()
{

	static double time0 = glfwGetTime();
	static double time1;
	static double dt;
	static int dframe = -1;
	static std::stringstream info;
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1) {
		info.precision(1);
		info << m_windowTitle << "    " << std::fixed << dframe / dt << " FPS";
		glfwSetWindowTitle(m_pWindow, info.str().c_str());
		info.str("");//别忘了在设置完窗口标题后清空所用的stringstream
		time0 = time1;
		dframe = 0;
	}

}

bool GlfwWrapper::InitializeWindow(VkExtent2D size, bool fullScreen , bool isResizable , bool limitFrameRate)
{
	///初始化GLFW
	if (!glfwInit()) {
		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
		return false;
	}

	///创建窗口前，必须调用glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)。
	///GLFW同GLM一样最初是为OpenGL设计的，GLFW_CLIENT_API的默认设置是GLFW_OPENGL_API，
	///这种情况下，GLFW会在创建窗口时创建OpenGL的上下文，这对于Vulkan而言是多余的，所以向GLFW说明不需要OpenGL的API。
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	///指定窗口可否拉伸
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	uint32_t extensionCount = 0;
	const char** extensionNames;

	///用glfwGetRequiredInstanceExtensions(...)获取平台所需的扩展，若执行成功，返回一个指针，
	///指向一个由所需扩展的名称为元素的数组，失败则返回nullptr，并意味着此设备不支持Vulkan。
	/// 不需要手动 delete extensionNames，GLFW的注释及官方文档中写到：
	///The returned array is allocated and freed by GLFW.
	///You should not free it yourself.It is guaranteed to be valid only until the library is terminated.
	extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);


	if (!extensionNames) {
		std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
		glfwTerminate();
		return false;
	}
	for (size_t i = 0; i < extensionCount; i++)
		vulkan::GraphicsBase::Base().AddInstanceExtension(extensionNames[i]);

	///设备级别的扩展，所需的只有"VK_KHR_swapchain
	vulkan::GraphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	///取得当前显示器信息的指针
	m_pMonitor = glfwGetPrimaryMonitor();

	///通常全屏时的图像区域应当跟屏幕分辨率一致，因此还需要取得显示器当前的视频模式
	///不需要手动 delete pMode，GLFW的注释及官方文档中写到：
	///The returned array is allocated and freed by GLFW.You should not free it yourself.
	///It is valid until the specified monitor is disconnected or the library is terminated.
	const GLFWvidmode* pMode = glfwGetVideoMode(m_pMonitor);

	///第四个参数用于指定全屏模式的显示器，若为nullptr则使用窗口模式，第五个参数可传入一个其他窗口的指针，用于与其他窗口分享内容。
	m_pWindow = fullScreen ?
		glfwCreateWindow(pMode->width, pMode->height, m_windowTitle.data(), m_pMonitor, nullptr) :
		glfwCreateWindow(size.width, size.height, m_windowTitle.data(), nullptr, nullptr);

	if (!m_pWindow) {
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
	//在创建window surface前创建Vulkan实例
	vulkan::GraphicsBase::Base().UseLatestApiVersion();
	if (vulkan::GraphicsBase::Base().CreateInstance())
		return false;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (VkResult result = glfwCreateWindowSurface(vulkan::GraphicsBase::Base().Instance(), m_pWindow, nullptr, &surface)) {
		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
		glfwTerminate();
		return false;
	}
	vulkan::GraphicsBase::Base().Surface(surface);
	//通过用||操作符短路执行来省去几行
	if (//获取物理设备，并使用列表中的第一个物理设备，这里不考虑以下任意函数失败后更换物理设备的情况
		vulkan::GraphicsBase::Base().GetPhysicalDevices() ||
		//一个true一个false，暂时不需要计算用的队列
		vulkan::GraphicsBase::Base().DeterminePhysicalDevice(0, true, false) ||
		//创建逻辑设备
		vulkan::GraphicsBase::Base().CreateDevice())
		return false;
	//----------------------------------------
	if (vulkan::GraphicsBase::Base().CreateSwapchain(limitFrameRate))
		return false;
	return true;
}

void GlfwWrapper::TerminateWindow()
{
	/*待后续填充*/
	glfwTerminate();
	vulkan::GraphicsBase::Base().WaitIdle();
}
