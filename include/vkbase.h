// VulkanLearning.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include "globalinclude.h"
#include "singleton.hpp"
#define windowSize vulkan::GraphicsBase::Base().SwapchainCreateInfo().imageExtent
namespace vulkan
{
	/*

		一个呈现图像的Vulkan应用程序需经历以下的步骤以初始化：
		----------------------------------------------
		1.创建Vulkan实例
		----------------------------------------------
		   |应用程序必须显式地告诉操作系统，说明其需要使用Vulkan的功能，
		   |这一步是由创建Vulkan实例（VkInstance）来完成的。
		   |Vulkan实例的底层是一系列Vulkan运行所必需的用于记录状态和信息的变量。
		----------------------------------------------
		2.创建debug messenger（若编译选项为DEBUG）
		----------------------------------------------
		   |Debug messenger用于获取验证层所捕捉到的debug信息。若没有这东西，Vulkan编程可谓寸步难行。
		----------------------------------------------
		3.创建window surface
		----------------------------------------------
		   |Vulkan是平台无关的API，你必须向其提供一个window surface（VkSurfaceKHR），以和平台特定的窗口对接。
		----------------------------------------------
		4.选择物理设备并创建逻辑设备，取得队列
		----------------------------------------------
		   |物理设备即图形处理器，通常为GPU。
		   |Vulkan中所谓的物理设备虽称物理（physical），但并非必须是实体设备。
		   |VkPhysicalDevice类型指代物理设备，从这类handle只能获取物理设备信息。
		   |VkDevice类型是逻辑设备的handle，逻辑设备是编程层面上用来与物理设备交互的对象，
		   |关于分配设备内存、创建Vulkan相关对象的命令大都会被提交给逻辑设备。
		   -------------------------------------------
		   |队列（VkQueue）类似于线程，命令被提交到队列执行，Vulkan官方标准中将队列描述为命令执行引擎的接口：
		   |Vulkan queues provide an interface to the execution engines of a device.
		   |Vulkan核心功能中规定队列支持的操作类型包括图形、计算、数据传送、稀疏绑定四种，图形和计算队列必定也支持数据传送。
		   |一族功能相同的队列称为队列族。
		   |任何支持Vulkan的显卡驱动确保你能找到至少一个同时支持图形和计算操作的队列族。
		——————————————————————————————————————————————
		5.创建交换链
		----------------------------------------------
		   |在将一张图像用于渲染或其他类型的写入时，已渲染好的图像可以被呈现引擎读取，如此交替呈现在窗口中的数张图像的集合即为交换链
		----------------------------------------------
		*/
	class GraphicsBasePlus;

	class GraphicsBase
	{
	
	public:
		// Getter函数
		uint32_t ApiVersion() const;
		VkInstance Instance() const;
		VkPhysicalDevice PhysicalDevice() const;
		const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const;
		const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const;
		VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const;
		uint32_t AvailablePhysicalDeviceCount() const;
		VkDevice Device() const;
		uint32_t QueueFamilyIndex_Graphics() const;
		uint32_t QueueFamilyIndex_Presentation() const;
		uint32_t QueueFamilyIndex_Compute() const;
		VkQueue Queue_Graphics() const;
		VkQueue Queue_Presentation() const;
		VkQueue Queue_Compute() const;
		VkSurfaceKHR Surface() const;
		const VkFormat& AvailableSurfaceFormat(uint32_t index) const;
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const;
		uint32_t AvailableSurfaceFormatCount() const;
		VkSwapchainKHR Swapchain() const;
		VkImage SwapchainImage(uint32_t index) const;
		VkImageView SwapchainImageView(uint32_t index) const;
		uint32_t SwapchainImageCount() const;
		const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const;
		const std::vector<const char*>& InstanceLayers() const;
		const std::vector<const char*>& InstanceExtensions() const;
		const std::vector<const char*>& DeviceExtensions() const;

		// Const成员函数
		Result CheckInstanceLayers(std::span<const char*> layersToCheck) const;
		Result CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;
		Result CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;

		// 非Const成员函数
		void AddInstanceLayer(const char* layerName);
		void AddInstanceExtension(const char* extensionName);
		void AddDeviceExtension(const char* extensionName);
		Result UseLatestApiVersion();
		Result CreateInstance(VkInstanceCreateFlags flags = 0);
		void Surface(VkSurfaceKHR surface);
		Result GetPhysicalDevices();
		Result DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true);
		Result CreateDevice(VkDeviceCreateFlags flags = 0);
		Result GetSurfaceFormats();
		Result SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat);
		

		Result CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);


		void InstanceLayers(const std::vector<const char*>& layerNames);
		void InstanceExtensions(const std::vector<const char*>& extensionNames);
		void DeviceExtensions(const std::vector<const char*>& extensionNames);
		Result RecreateSwapchain(bool limitFrame = true);

		Result WaitIdle() const;
		Result RecreateDevice(VkDeviceCreateFlags flags = 0);


		uint32_t CurrentImageIndex() const;
		Result SwapImage(VkSemaphore semaphore_imageIsAvailable);

		Result SubmitCommandBufferGraphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;
		Result SubmitCommandBufferGraphics(VkCommandBuffer commandBuffer,
			VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
			VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const;
		Result SubmitCommandBufferGraphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;
		Result SubmitCommandBufferCompute(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;
		Result SubmitCommandBufferCompute(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;

		Result PresentImage(VkPresentInfoKHR& presentInfo);
		Result PresentImage(VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE);

		void Terminate();
		// 静态函数
		static GraphicsBase& Base();

	private:
		std::vector<void(*)()> callbacks_createSwapchain;
		std::vector<void(*)()> callbacks_destroySwapchain;
		std::vector<void(*)()> callbacks_createDevice;
		std::vector<void(*)()> callbacks_destroyDevice;
	public:
		void AddCallback_CreateSwapchain(void(*function)()) {
			callbacks_createSwapchain.push_back(function);
		}
		void AddCallback_DestroySwapchain(void(*function)()) {
			callbacks_destroySwapchain.push_back(function);
		}
		void AddCallback_CreateDevice(void(*function)()) {
			callbacks_createDevice.push_back(function);
		}
		void AddCallback_DestroyDevice(void(*function)()) {
			callbacks_destroyDevice.push_back(function);
		}
        //*pPlus的Getter
		static GraphicsBasePlus& Plus() { return *singleton.pPlus; }
		//*pPlus的Setter，只允许设置pPlus一次
		static void Plus(GraphicsBasePlus& plus) { if (!singleton.pPlus) singleton.pPlus = &plus; }
	private:

		GraphicsBasePlus* pPlus = nullptr;
		uint32_t apiVersion = VK_API_VERSION_1_0;
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		std::vector<VkPhysicalDevice> availablePhysicalDevices;

		VkDevice device;
		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
		VkQueue queue_graphics;
		VkQueue queue_presentation;
		VkQueue queue_compute;

		VkSurfaceKHR surface;
		std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;

		VkSwapchainKHR swapchain;
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

		std::vector<const char*> instanceLayers;
		std::vector<const char*> instanceExtensions;
		std::vector<const char*> deviceExtensions;

		VkDebugUtilsMessengerEXT debugMessenger;

		// 静态变量
		static GraphicsBase singleton;

		uint32_t currentImageIndex = 0;

		private:

		// 私有构造函数
		GraphicsBase() = default;
		GraphicsBase(GraphicsBase&&) = delete;

		// 私有析构函数
		~GraphicsBase();

		// 私有成员函数
		Result CreateSwapchainInternal();
		Result GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]);
		Result CreateDebugMessenger();

	};
	class RenderPass {
		VkRenderPass handle = VK_NULL_HANDLE;
	public:
		RenderPass() = default;
		RenderPass(VkRenderPassCreateInfo& createInfo) {
			Create(createInfo);
		}
		RenderPass(RenderPass&& other) noexcept { MoveHandle; }
		~RenderPass() { DestroyHandleBy(vkDestroyRenderPass); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
			beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginInfo.renderPass = handle;
			vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
		}
		void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer Framebuffer, VkRect2D renderArea, 
			ArrayRef<const VkClearValue> clearValues = {},
			VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const 
		{
			VkRenderPassBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = handle,
				.framebuffer = Framebuffer,
				.renderArea = renderArea,
				.clearValueCount = uint32_t(clearValues.Count()),
				.pClearValues = clearValues.Pointer()
			};
			vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
		}
		void CmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
			vkCmdNextSubpass(commandBuffer, subpassContents);
		}
		void CmdEnd(VkCommandBuffer commandBuffer) const {
			vkCmdEndRenderPass(commandBuffer);
		}
		//Non-const Function
		Result Create(VkRenderPassCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			VkResult result = vkCreateRenderPass(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ RenderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

	class Framebuffer {
		VkFramebuffer handle = VK_NULL_HANDLE;
	public:
		Framebuffer() = default;
		Framebuffer(VkFramebufferCreateInfo& createInfo) {
			Create(createInfo);
		}
		Framebuffer(Framebuffer&& other) noexcept { MoveHandle; }
		~Framebuffer() { DestroyHandleBy(vkDestroyFramebuffer); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		Result Create(VkFramebufferCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			VkResult result = vkCreateFramebuffer(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ Framebuffer ] ERROR\nFailed to create a Framebuffer!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

} // namespace vulkan


