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

		vulkan::Fence fence(VK_FENCE_CREATE_SIGNALED_BIT); //����λ״̬����դ��
		vulkan::Semaphore semaphore_imageIsAvailable;
		vulkan::Semaphore semaphore_renderingIsOver;

		vulkan::CommandBuffer commandBuffer;
		vulkan::CommandPool commandPool(vulkan::GraphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		commandPool.AllocateBuffers(commandBuffer);

		while (!glfwWindowShouldClose(m_pWindow)) {
			//������С��ʱ������Ⱦѭ��
			while (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED))
				glfwWaitEvents();

			fence.WaitAndReset();

			vulkan::GraphicsBase::Base().SwapImage(semaphore_imageIsAvailable);

			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			/*��Ⱦ��������*/
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
		info.str("");//�������������괰�ڱ����������õ�stringstream
		time0 = time1;
		dframe = 0;
	}

}

bool GlfwWrapper::InitializeWindow(VkExtent2D size, bool fullScreen , bool isResizable , bool limitFrameRate)
{
	///��ʼ��GLFW
	if (!glfwInit()) {
		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
		return false;
	}

	///��������ǰ���������glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)��
	///GLFWͬGLMһ�������ΪOpenGL��Ƶģ�GLFW_CLIENT_API��Ĭ��������GLFW_OPENGL_API��
	///��������£�GLFW���ڴ�������ʱ����OpenGL�������ģ������Vulkan�����Ƕ���ģ�������GLFW˵������ҪOpenGL��API��
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	///ָ�����ڿɷ�����
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	uint32_t extensionCount = 0;
	const char** extensionNames;

	///��glfwGetRequiredInstanceExtensions(...)��ȡƽ̨�������չ����ִ�гɹ�������һ��ָ�룬
	///ָ��һ����������չ������ΪԪ�ص����飬ʧ���򷵻�nullptr������ζ�Ŵ��豸��֧��Vulkan��
	/// ����Ҫ�ֶ� delete extensionNames��GLFW��ע�ͼ��ٷ��ĵ���д����
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

	///�豸�������չ�������ֻ��"VK_KHR_swapchain
	vulkan::GraphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	///ȡ�õ�ǰ��ʾ����Ϣ��ָ��
	m_pMonitor = glfwGetPrimaryMonitor();

	///ͨ��ȫ��ʱ��ͼ������Ӧ������Ļ�ֱ���һ�£���˻���Ҫȡ����ʾ����ǰ����Ƶģʽ
	///����Ҫ�ֶ� delete pMode��GLFW��ע�ͼ��ٷ��ĵ���д����
	///The returned array is allocated and freed by GLFW.You should not free it yourself.
	///It is valid until the specified monitor is disconnected or the library is terminated.
	const GLFWvidmode* pMode = glfwGetVideoMode(m_pMonitor);

	///���ĸ���������ָ��ȫ��ģʽ����ʾ������Ϊnullptr��ʹ�ô���ģʽ������������ɴ���һ���������ڵ�ָ�룬�������������ڷ������ݡ�
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
	//�ڴ���window surfaceǰ����Vulkanʵ��
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
	//ͨ����||��������·ִ����ʡȥ����
	if (//��ȡ�����豸����ʹ���б��еĵ�һ�������豸�����ﲻ�����������⺯��ʧ�ܺ���������豸�����
		vulkan::GraphicsBase::Base().GetPhysicalDevices() ||
		//һ��trueһ��false����ʱ����Ҫ�����õĶ���
		vulkan::GraphicsBase::Base().DeterminePhysicalDevice(0, true, false) ||
		//�����߼��豸
		vulkan::GraphicsBase::Base().CreateDevice())
		return false;
	//----------------------------------------
	if (vulkan::GraphicsBase::Base().CreateSwapchain(limitFrameRate))
		return false;
	return true;
}

void GlfwWrapper::TerminateWindow()
{
	/*���������*/
	glfwTerminate();
	vulkan::GraphicsBase::Base().WaitIdle();
}
