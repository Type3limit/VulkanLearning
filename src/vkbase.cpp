#include "vkbase.h"

namespace vulkan {

	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

	// 静态成员定义
	GraphicsBase GraphicsBase::singleton;
	// 静态函数实现
	GraphicsBase& GraphicsBase::Base() {
		return singleton;
	}
	// 析构函数
	GraphicsBase::~GraphicsBase() {
		if (!instance)
			return;
		if (device) {
			WaitIdle();
			if (swapchain) {
				for (auto& i : callbacks_destroySwapchain)
					i();
				for (auto& i : swapchainImageViews)
					if (i)
						vkDestroyImageView(device, i, nullptr);
				vkDestroySwapchainKHR(device, swapchain, nullptr);
			}
			for (auto& i : callbacks_destroyDevice)
				i();
			vkDestroyDevice(device, nullptr);
		}
		if (surface)
			vkDestroySurfaceKHR(instance, surface, nullptr);
		if (debugMessenger) {
			PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger =
				reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (DestroyDebugUtilsMessenger)
				DestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
	}

	Result GraphicsBase::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		if (!queueFamilyCount)
			return VK_RESULT_MAX_ENUM;
		std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());
		auto& [ig, ip, ic] = queueFamilyIndices;
		ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			//这三个VkBool32变量指示是否可获取（指应该被获取且能获取）相应队列族索引
			VkBool32
				//只在enableGraphicsQueue为true时获取支持图形操作的队列族的索引
				supportGraphics = (enableGraphicsQueue && (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)),
				supportPresentation = false,
				//只在enableComputeQueue为true时获取支持计算的队列族的索引
				supportCompute = (enableComputeQueue && (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT));
			//只在创建了window surface时获取支持呈现的队列族的索引
			if (surface)
				if (Result result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) {
					std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
					return result;
				}
			//若某队列族同时支持图形操作和计算
			if (supportGraphics && supportCompute) {
				//若需要呈现，最好是三个队列族索引全部相同
				if (supportPresentation) {
					ig = ip = ic = i;
					break;
				}
				//除非ig和ic都已取得且相同，否则将它们的值覆写为i，以确保两个队列族索引相同
				if (ig != ic ||
					ig == VK_QUEUE_FAMILY_IGNORED)
					ig = ic = i;
				//如果不需要呈现，那么已经可以break了
				if (!surface)
					break;
			}
			//若任何一个队列族索引可以被取得但尚未被取得，将其值覆写为i
			if (supportGraphics &&
				ig == VK_QUEUE_FAMILY_IGNORED)
				ig = i;
			if (supportPresentation &&
				ip == VK_QUEUE_FAMILY_IGNORED)
				ip = i;
			if (supportCompute &&
				ic == VK_QUEUE_FAMILY_IGNORED)
				ic = i;
		}
		if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
			ip == VK_QUEUE_FAMILY_IGNORED && surface ||
			ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
			return VK_RESULT_MAX_ENUM;
		queueFamilyIndex_graphics = ig;
		queueFamilyIndex_presentation = ip;
		queueFamilyIndex_compute = ic;
		return VK_SUCCESS;
	}

	Result GraphicsBase::CreateDebugMessenger() {
		static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32 {
				std::cout << std::format("{}\n\n", pCallbackData->pMessage);
				return VK_FALSE;
			};
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = DebugUtilsMessengerCallback
		};
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
			reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		if (vkCreateDebugUtilsMessenger) {
			Result result = vkCreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger);
			if (result)
				std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
			return result;
		}
		std::cout <<
			std::format("[ GraphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
		return VK_RESULT_MAX_ENUM;
	}

	// Getter函数实现
	uint32_t GraphicsBase::ApiVersion() const {
		return apiVersion;
	}

	VkInstance GraphicsBase::Instance() const {
		return instance;
	}

	VkPhysicalDevice GraphicsBase::PhysicalDevice() const {
		return physicalDevice;
	}

	const VkPhysicalDeviceProperties& GraphicsBase::PhysicalDeviceProperties() const {
		return physicalDeviceProperties;
	}

	const VkPhysicalDeviceMemoryProperties& GraphicsBase::PhysicalDeviceMemoryProperties() const {
		return physicalDeviceMemoryProperties;
	}

	VkPhysicalDevice GraphicsBase::AvailablePhysicalDevice(uint32_t index) const {
		return availablePhysicalDevices[index];
	}

	uint32_t GraphicsBase::AvailablePhysicalDeviceCount() const {
		return static_cast<uint32_t>(availablePhysicalDevices.size());
	}

	VkDevice GraphicsBase::Device() const {
		return device;
	}

	uint32_t GraphicsBase::QueueFamilyIndex_Graphics() const {
		return queueFamilyIndex_graphics;
	}

	uint32_t GraphicsBase::QueueFamilyIndex_Presentation() const {
		return queueFamilyIndex_presentation;
	}

	uint32_t GraphicsBase::QueueFamilyIndex_Compute() const {
		return queueFamilyIndex_compute;
	}

	VkQueue GraphicsBase::Queue_Graphics() const {
		return queue_graphics;
	}

	VkQueue GraphicsBase::Queue_Presentation() const {
		return queue_presentation;
	}

	VkQueue GraphicsBase::Queue_Compute() const {
		return queue_compute;
	}

	VkSurfaceKHR GraphicsBase::Surface() const {
		return surface;
	}

	const VkFormat& GraphicsBase::AvailableSurfaceFormat(uint32_t index) const {
		return availableSurfaceFormats[index].format;
	}

	const VkColorSpaceKHR& GraphicsBase::AvailableSurfaceColorSpace(uint32_t index) const {
		return availableSurfaceFormats[index].colorSpace;
	}

	uint32_t GraphicsBase::AvailableSurfaceFormatCount() const {
		return static_cast<uint32_t>(availableSurfaceFormats.size());
	}

	VkSwapchainKHR GraphicsBase::Swapchain() const {
		return swapchain;
	}

	VkImage GraphicsBase::SwapchainImage(uint32_t index) const {
		return swapchainImages[index];
	}

	VkImageView GraphicsBase::SwapchainImageView(uint32_t index) const {
		return swapchainImageViews[index];
	}

	uint32_t GraphicsBase::SwapchainImageCount() const {
		return static_cast<uint32_t>(swapchainImages.size());
	}

	const VkSwapchainCreateInfoKHR& GraphicsBase::SwapchainCreateInfo() const {
		return swapchainCreateInfo;
	}

	const std::vector<const char*>& GraphicsBase::InstanceLayers() const {
		return instanceLayers;
	}

	const std::vector<const char*>& GraphicsBase::InstanceExtensions() const {
		return instanceExtensions;
	}

	const std::vector<const char*>& GraphicsBase::DeviceExtensions() const {
		return deviceExtensions;
	}

	// Const成员函数实现
	Result GraphicsBase::CheckInstanceLayers(std::span<const char*> layersToCheck) const
	{
		//创建Vulkan实例失败时检查是否可以去掉一些非必要的层或扩展。
		uint32_t layerCount;
		std::vector<VkLayerProperties> availableLayers;


		///Result VKAPI_CALL vkEnumerateInstanceLayerProperties(...) 的参数说明
		///uint32_t* pPropertyCount 
		///    若pProperties为nullptr，则将可用层的数量返回到* pPropertyCount，
		///    否则由* pPropertyCount指定所需获取的VkLayerProperties的数量
		///VkLayerProperties* pProperties
		///    若pProperties非nullptr，则将* pPropertyCount个可用层的VkLayerProperties返回到* pProperties
		if (Result result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) //首先取得可用层的总数
		{
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
			return result;
		}

		///若layerCount为0，则说明没有任何可用层（虽然没这个可能），否则嵌套循环逐个比较字符串，
		///若没有找到某个层的名称，将layersToCheck中对应的字符串指针设置为nullptr：
		if (layerCount)
		{
			availableLayers.resize(layerCount);
			if (Result result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) {
				std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
				return result;
			}
			for (auto& i : layersToCheck) {
				bool found = false;
				for (auto& j : availableLayers)
					if (!strcmp(i, j.layerName)) {
						found = true;
						break;
					}
				if (!found)
					i = nullptr;
			}
		}
		else
		{
			for (auto& i : layersToCheck)
				i = nullptr;
		}

		//一切顺利则返回VK_SUCCESS
		return VK_SUCCESS;
	}

	Result GraphicsBase::CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName) const {
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> availableExtensions;
		if (Result result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr)) {
			layerName ?
				std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName) :
				std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of instance extensions!\n");
			return result;
		}
		if (extensionCount) {
			availableExtensions.resize(extensionCount);
			if (Result result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) {
				std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
				return result;
			}
			for (auto& i : extensionsToCheck) {
				bool found = false;
				for (auto& j : availableExtensions)
					if (!strcmp(i, j.extensionName)) {
						found = true;
						break;
					}
				if (!found)
					i = nullptr;
			}
		}
		else
			for (auto& i : extensionsToCheck)
				i = nullptr;
		return VK_SUCCESS;
	}

	Result GraphicsBase::CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName) const {
		// 待Ch1-3填充
		return VK_SUCCESS;
	}

	// 非Const成员函数实现
	void GraphicsBase::AddInstanceLayer(const char* layerName) {
		instanceLayers.push_back(layerName);
	}

	void GraphicsBase::AddInstanceExtension(const char* extensionName) {
		instanceExtensions.push_back(extensionName);
	}

	void GraphicsBase::AddDeviceExtension(const char* extensionName) {
		deviceExtensions.push_back(extensionName);
	}

	Result GraphicsBase::UseLatestApiVersion() {

		if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
			return vkEnumerateInstanceVersion(&apiVersion);
		return VK_SUCCESS;
	}

	Result GraphicsBase::CreateInstance(VkInstanceCreateFlags flags) {
		if constexpr (ENABLE_DEBUG_MESSENGER)
			AddInstanceLayer("VK_LAYER_KHRONOS_validation"),
			AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		VkApplicationInfo applicatianInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.apiVersion = apiVersion
		};
		VkInstanceCreateInfo instanceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.flags = flags,
			.pApplicationInfo = &applicatianInfo,
			.enabledLayerCount = uint32_t(instanceLayers.size()),
			.ppEnabledLayerNames = instanceLayers.data(),
			.enabledExtensionCount = uint32_t(instanceExtensions.size()),
			.ppEnabledExtensionNames = instanceExtensions.data()
		};
		if (Result result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
			return result;
		}
		std::cout << std::format(
			"Vulkan API Version: {}.{}.{}\n",
			VK_VERSION_MAJOR(apiVersion),
			VK_VERSION_MINOR(apiVersion),
			VK_VERSION_PATCH(apiVersion));
		if constexpr (ENABLE_DEBUG_MESSENGER)
			CreateDebugMessenger();
		return VK_SUCCESS;
	}

	void GraphicsBase::Surface(VkSurfaceKHR surface) {
		if (!this->surface) {
			this->surface = surface;
		}
	}

	Result GraphicsBase::GetPhysicalDevices() {
		uint32_t deviceCount;
		if (Result result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!deviceCount)
		{
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to find any physical device supports vulkan!\n");
			abort();
		}

		availablePhysicalDevices.resize(deviceCount);
		Result result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
		if (result)
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", int32_t(result));
		return result;
	}

	Result GraphicsBase::DeterminePhysicalDevice(uint32_t deviceIndex, bool enableGraphicsQueue, bool enableComputeQueue) {
		//定义一个特殊值用于标记一个队列族索引已被找过但未找到
		static constexpr uint32_t notFound = INT32_MAX;//== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
		//定义队列族索引组合的结构体
		struct queueFamilyIndexCombination {
			uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
			uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
			uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
		};
		//queueFamilyIndexCombinations用于为各个物理设备保存一份队列族索引组合
		static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
		auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];

		//如果有任何队列族索引已被找过但未找到，返回VK_RESULT_MAX_ENUM
		if (ig == notFound && enableGraphicsQueue ||
			ip == notFound && surface ||
			ic == notFound && enableComputeQueue)
			return VK_RESULT_MAX_ENUM;

		//如果有任何队列族索引应被获取但还未被找过
		if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
			ip == VK_QUEUE_FAMILY_IGNORED && surface ||
			ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
			uint32_t indices[3];
			Result result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
			//若GetQueueFamilyIndices(...)返回VK_SUCCESS或VK_RESULT_MAX_ENUM（vkGetPhysicalDeviceSurfaceSupportKHR(...)执行成功但没找齐所需队列族），
			//说明对所需队列族索引已有结论，保存结果到queueFamilyIndexCombinations[deviceIndex]中相应变量
			//应被获取的索引若仍为VK_QUEUE_FAMILY_IGNORED，说明未找到相应队列族，VK_QUEUE_FAMILY_IGNORED（~0u）与INT32_MAX做位与得到的数值等于notFound
			if (result == VK_SUCCESS ||
				result == VK_RESULT_MAX_ENUM) {
				if (enableGraphicsQueue)
					ig = indices[0] & INT32_MAX;
				if (surface)
					ip = indices[1] & INT32_MAX;
				if (enableComputeQueue)
					ic = indices[2] & INT32_MAX;
			}
			//如果GetQueueFamilyIndices(...)执行失败，return
			if (result)
				return result;
		}

		//若以上两个if分支皆不执行，则说明所需的队列族索引皆已被获取，从queueFamilyIndexCombinations[deviceIndex]中取得索引
		else {
			queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
			queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
			queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
		}
		physicalDevice = availablePhysicalDevices[deviceIndex];
		return VK_SUCCESS;
	}

	Result GraphicsBase::CreateDevice(VkDeviceCreateFlags flags) {
		float queuePriority = 1.f;
		VkDeviceQueueCreateInfo queueCreateInfos[3] = {
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority },
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority },
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority } };
		uint32_t queueCreateInfoCount = 0;
		if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
			queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
		if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
			queueFamilyIndex_presentation != queueFamilyIndex_graphics)
			queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
		if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
			queueFamilyIndex_compute != queueFamilyIndex_graphics &&
			queueFamilyIndex_compute != queueFamilyIndex_presentation)
			queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
		VkDeviceCreateInfo deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.flags = flags,
			.queueCreateInfoCount = queueCreateInfoCount,
			.pQueueCreateInfos = queueCreateInfos,
			.enabledExtensionCount = uint32_t(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &physicalDeviceFeatures
		};
		if (Result result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
			vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
		if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
			vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
		if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
			vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
		//输出所用的物理设备名称
		std::cout << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);
		for (auto& i : callbacks_createDevice)
			i();
		return VK_SUCCESS;
	}

	Result GraphicsBase::GetSurfaceFormats() {
		uint32_t surfaceFormatCount;
		if (Result result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!surfaceFormatCount)
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to find any supported surface format!\n"),
			abort();
		availableSurfaceFormats.resize(surfaceFormatCount);
		Result result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
		if (result)
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
		return result;
	}

	Result GraphicsBase::SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
		bool formatIsAvailable = false;
		if (!surfaceFormat.format) {
			//如果格式未指定，只匹配色彩空间，图像格式有啥就用啥
			for (auto& i : availableSurfaceFormats)
				if (i.colorSpace == surfaceFormat.colorSpace) {
					swapchainCreateInfo.imageFormat = i.format;
					swapchainCreateInfo.imageColorSpace = i.colorSpace;
					formatIsAvailable = true;
					break;
				}
		}
		else
			//否则匹配格式和色彩空间
			for (auto& i : availableSurfaceFormats)
				if (i.format == surfaceFormat.format &&
					i.colorSpace == surfaceFormat.colorSpace) {
					swapchainCreateInfo.imageFormat = i.format;
					swapchainCreateInfo.imageColorSpace = i.colorSpace;
					formatIsAvailable = true;
					break;
				}
		//如果没有符合的格式，恰好有个语义相符的错误代码
		if (!formatIsAvailable)
			return VK_ERROR_FORMAT_NOT_SUPPORTED;
		//如果交换链已存在，调用RecreateSwapchain()重建交换链
		if (swapchain)
			return RecreateSwapchain();
		return VK_SUCCESS;
	}

	Result GraphicsBase::CreateSwapchainInternal()
	{
		if (Result result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
			return result;
		}

		//获取交换连图像
		uint32_t swapchainImageCount;
		if (Result result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
			return result;
		}
		swapchainImages.resize(swapchainImageCount);
		if (Result result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data())) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
			return result;
		}

		//创建image view
		swapchainImageViews.resize(swapchainImageCount);
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchainCreateInfo.imageFormat,
			//.components = {},//四个成员皆为VK_COMPONENT_SWIZZLE_IDENTITY
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};
		for (size_t i = 0; i < swapchainImageCount; i++) {
			imageViewCreateInfo.image = swapchainImages[i];
			if (Result result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i])) {
				std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
				return result;
			}
		}
		return VK_SUCCESS;
	}

	Result GraphicsBase::CreateSwapchain(bool limitFrameRate, VkSwapchainCreateFlagsKHR flags) {
		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		if (Result result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
			return result;
		}
		swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
		swapchainCreateInfo.imageExtent =
			surfaceCapabilities.currentExtent.width == -1 ?
			VkExtent2D{
				glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
				glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
				surfaceCapabilities.currentExtent;
		/*待后续填充*/

		if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
			swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		else
			for (size_t i = 0; i < 4; i++)
				if (surfaceCapabilities.supportedCompositeAlpha & 1 << i) {
					swapchainCreateInfo.compositeAlpha = 
						VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
					break;
				}

		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		else
			std::cout << std::format("[ GraphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");

		if (availableSurfaceFormats.empty())
			if (Result result = GetSurfaceFormats())
				return result;

		if (!swapchainCreateInfo.imageFormat)
			//用&&操作符来短路执行
			if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
				SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) {
				//如果找不到上述图像格式和色彩空间的组合，那只能有什么用什么，采用availableSurfaceFormats中的第一组
				swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
				swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
				std::cout << std::format("[ GraphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
			}

		uint32_t surfacePresentModeCount;
		if (Result result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!surfacePresentModeCount)
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to find any surface present mode!\n"),
			abort();
		/*
		VK_PRESENT_MODE_IMMEDIATE_KHR表示立即模式，该模式下不限制帧率且帧率在所有模式中是最高的。该模式不等待垂直同步信号，一旦图片渲染完，用于呈现的图像就会被立刻替换掉，这可能导致画面撕裂。
		VK_PRESENT_MODE_FIFO_KHR表示先入先出模式，该模式限制帧率与屏幕刷新率一致，这种模式是必定支持的。在该模式下，图像被推送进一个用于待呈现图像的队列，然后等待垂直同步信号，按顺序被推出队列并输出到屏幕，因此叫先入先出。
		VK_PRESENT_MODE_FIFO_RELAXED_KHR同VK_PRESENT_MODE_FIFO_KHR的差别在于，若屏幕上图像的停留时间长于一个刷新间隔，呈现引擎可能在下一个垂直同步信号到来前便试图将呈现队列中的图像输出到屏幕，该模式相比VK_PRESENT_MODE_FIFO_KHR更不容易引起阻塞或迟滞，但在帧率较低时可能会导致画面撕裂。
		VK_PRESENT_MODE_MAILBOX_KHR是一种类似于三重缓冲的模式。它的待呈现图像队列中只容纳一个元素，在等待垂直同步信号期间若有新的图像入队，那么旧的图像会直接出队而不被输出到屏幕（即出队不需要等待垂直同步信号，因此不限制帧率），出现在屏幕上的总会是最新的图像。
		*/
		std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
		if (Result result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
			return result;
		}
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (!limitFrameRate)
			for (size_t i = 0; i < surfacePresentModeCount; i++)
				if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
					swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}

		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.flags = flags;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.clipped = VK_TRUE;

		if (Result result = CreateSwapchainInternal())
			return result;

		for (auto& i : callbacks_createSwapchain)
			i();
		return VK_SUCCESS;
	}

	void GraphicsBase::InstanceLayers(const std::vector<const char*>& layerNames) {
		instanceLayers = layerNames;
	}

	void GraphicsBase::InstanceExtensions(const std::vector<const char*>& extensionNames) {
		instanceExtensions = extensionNames;
	}

	void GraphicsBase::DeviceExtensions(const std::vector<const char*>& extensionNames) {
		deviceExtensions = extensionNames;
	}

	Result GraphicsBase::RecreateSwapchain() {
		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		if (Result result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (surfaceCapabilities.currentExtent.width == 0 ||
			surfaceCapabilities.currentExtent.height == 0)
			return VK_SUBOPTIMAL_KHR;
		swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
		swapchainCreateInfo.oldSwapchain = swapchain;
		Result result = vkQueueWaitIdle(queue_graphics);
		//仅在等待图形队列成功，且图形与呈现所用队列不同时等待呈现队列
		if (!result &&
			queue_graphics != queue_presentation)
			result = vkQueueWaitIdle(queue_presentation);
		if (result) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
			return result;
		}
		for (auto& i : swapchainImageViews)
		{
			if (i)
			{
				vkDestroyImageView(device, i, nullptr);
			}
		}
		swapchainImageViews.resize(0);
		if (result = CreateSwapchainInternal())
			return result;
		/*待后续填充*/
		for (auto& i : callbacks_createSwapchain)
			i();
		return VK_SUCCESS;
	}

	Result GraphicsBase::WaitIdle() const
	{
		Result result = vkDeviceWaitIdle(device);
		if (result)
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
		return result;
	}

	Result GraphicsBase::RecreateDevice(VkDeviceCreateFlags flags)
	{
		if (Result result = WaitIdle())
			return result;
		if (swapchain) {
			for (auto& i : callbacks_destroySwapchain)
				i();
			for (auto& i : swapchainImageViews)
				if (i)
					vkDestroyImageView(device, i, nullptr);
			swapchainImageViews.resize(0);
			vkDestroySwapchainKHR(device, swapchain, nullptr);
			swapchain = VK_NULL_HANDLE;
			swapchainCreateInfo = {};
		}
		for (auto& i : callbacks_destroyDevice)
			i();
		if (device)
			vkDestroyDevice(device, nullptr),
			device = VK_NULL_HANDLE;
		return CreateDevice(flags);
	}

	uint32_t GraphicsBase::CurrentImageIndex() const
	{
		return currentImageIndex;
	}

	Result GraphicsBase::SwapImage(VkSemaphore semaphore_imageIsAvailable)
	{
		//销毁旧交换链（若存在）
		if (swapchainCreateInfo.oldSwapchain &&
			swapchainCreateInfo.oldSwapchain != swapchain) {
			vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
			swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		}
		//获取交换链图像索引
		while (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
			switch (result) {
			case VK_SUBOPTIMAL_KHR:
			case VK_ERROR_OUT_OF_DATE_KHR:
				if (VkResult result = RecreateSwapchain())
					return result;
				break; //注意重建交换链后仍需要获取图像，通过break递归，再次执行while的条件判定语句
			default:
				outStream << std::format("[ GraphicsBase ] ERROR\nFailed to acquire the next image!\nError code: {}\n", int32_t(result));
				return result;
			}
		return VK_SUCCESS;
	}

	Result GraphicsBase::SubmitCommandBufferGraphics(VkSubmitInfo& submitInfo, VkFence fence) const
	{
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkResult result = vkQueueSubmit(queue_graphics, 1, &submitInfo, fence);
		if (result)
			outStream << std::format("[ GraphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}

	Result GraphicsBase::SubmitCommandBufferGraphics(VkCommandBuffer commandBuffer, VkSemaphore semaphore_imageIsAvailable, VkSemaphore semaphore_renderingIsOver, VkFence fence, VkPipelineStageFlags waitDstStage_imageIsAvailable) const
	{
		
				VkSubmitInfo submitInfo = {
					.commandBufferCount = 1,
					.pCommandBuffers = &commandBuffer
				};
				if (semaphore_imageIsAvailable)
					submitInfo.waitSemaphoreCount = 1,
					submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable,
					submitInfo.pWaitDstStageMask = &waitDstStage_imageIsAvailable;
				if (semaphore_renderingIsOver)
					submitInfo.signalSemaphoreCount = 1,
					submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
				return SubmitCommandBufferGraphics(submitInfo, fence);
	
	}

	Result GraphicsBase::SubmitCommandBufferGraphics(VkCommandBuffer commandBuffer, VkFence fence) const
	{
		VkSubmitInfo submitInfo = {
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
		};
		return SubmitCommandBufferGraphics(submitInfo, fence);
	}

	Result GraphicsBase::SubmitCommandBufferCompute(VkSubmitInfo& submitInfo, VkFence fence) const
	{
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkResult result = vkQueueSubmit(queue_compute, 1, &submitInfo, fence);
		if (result)
			outStream << std::format("[ GraphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}

	Result GraphicsBase::SubmitCommandBufferCompute(VkCommandBuffer commandBuffer, VkFence fence) const
	{
		VkSubmitInfo submitInfo = {
	   .commandBufferCount = 1,
	   .pCommandBuffers = &commandBuffer
		};
		return SubmitCommandBufferCompute(submitInfo, fence);
	}

	Result GraphicsBase::PresentImage(VkPresentInfoKHR& presentInfo)
	{
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		switch (VkResult result = vkQueuePresentKHR(queue_presentation, &presentInfo)) {
		case VK_SUCCESS:
			return VK_SUCCESS;
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			return RecreateSwapchain();
		default:
			outStream << std::format("[ graphicsBase ] ERROR\nFailed to queue the image for presentation!\nError code: {}\n", int32_t(result));
			return result;
		}
	}

	Result GraphicsBase::PresentImage(VkSemaphore semaphore_renderingIsOver)
	{
		VkPresentInfoKHR presentInfo = {
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &currentImageIndex
		};
		if (semaphore_renderingIsOver)
			presentInfo.waitSemaphoreCount = 1,
			presentInfo.pWaitSemaphores = &semaphore_renderingIsOver;
		return PresentImage(presentInfo);
	}

	void GraphicsBase::Terminate()
	{
		this->~GraphicsBase();
		instance = VK_NULL_HANDLE;
		physicalDevice = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
		surface = VK_NULL_HANDLE;
		swapchain = VK_NULL_HANDLE;
		swapchainImages.resize(0);
		swapchainImageViews.resize(0);
		swapchainCreateInfo = {};
		debugMessenger = VK_NULL_HANDLE;
	}



} // namespace vulkan
