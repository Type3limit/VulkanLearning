#include "vkbase.h"

namespace vulkan {

	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

	// ��̬��Ա����
	GraphicsBase GraphicsBase::singleton;
	// ��̬����ʵ��
	GraphicsBase& GraphicsBase::Base() {
		return singleton;
	}
	// ��������
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
			//������VkBool32����ָʾ�Ƿ�ɻ�ȡ��ָӦ�ñ���ȡ���ܻ�ȡ����Ӧ����������
			VkBool32
				//ֻ��enableGraphicsQueueΪtrueʱ��ȡ֧��ͼ�β����Ķ����������
				supportGraphics = (enableGraphicsQueue && (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)),
				supportPresentation = false,
				//ֻ��enableComputeQueueΪtrueʱ��ȡ֧�ּ���Ķ����������
				supportCompute = (enableComputeQueue && (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT));
			//ֻ�ڴ�����window surfaceʱ��ȡ֧�ֳ��ֵĶ����������
			if (surface)
				if (Result result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) {
					std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
					return result;
				}
			//��ĳ������ͬʱ֧��ͼ�β����ͼ���
			if (supportGraphics && supportCompute) {
				//����Ҫ���֣��������������������ȫ����ͬ
				if (supportPresentation) {
					ig = ip = ic = i;
					break;
				}
				//����ig��ic����ȡ������ͬ���������ǵ�ֵ��дΪi����ȷ������������������ͬ
				if (ig != ic ||
					ig == VK_QUEUE_FAMILY_IGNORED)
					ig = ic = i;
				//�������Ҫ���֣���ô�Ѿ�����break��
				if (!surface)
					break;
			}
			//���κ�һ���������������Ա�ȡ�õ���δ��ȡ�ã�����ֵ��дΪi
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

	// Getter����ʵ��
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

	// Const��Ա����ʵ��
	Result GraphicsBase::CheckInstanceLayers(std::span<const char*> layersToCheck) const
	{
		//����Vulkanʵ��ʧ��ʱ����Ƿ����ȥ��һЩ�Ǳ�Ҫ�Ĳ����չ��
		uint32_t layerCount;
		std::vector<VkLayerProperties> availableLayers;


		///Result VKAPI_CALL vkEnumerateInstanceLayerProperties(...) �Ĳ���˵��
		///uint32_t* pPropertyCount 
		///    ��pPropertiesΪnullptr���򽫿��ò���������ص�* pPropertyCount��
		///    ������* pPropertyCountָ�������ȡ��VkLayerProperties������
		///VkLayerProperties* pProperties
		///    ��pProperties��nullptr����* pPropertyCount�����ò��VkLayerProperties���ص�* pProperties
		if (Result result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) //����ȡ�ÿ��ò������
		{
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
			return result;
		}

		///��layerCountΪ0����˵��û���κο��ò㣨��Ȼû������ܣ�������Ƕ��ѭ������Ƚ��ַ�����
		///��û���ҵ�ĳ��������ƣ���layersToCheck�ж�Ӧ���ַ���ָ������Ϊnullptr��
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

		//һ��˳���򷵻�VK_SUCCESS
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
		// ��Ch1-3���
		return VK_SUCCESS;
	}

	// ��Const��Ա����ʵ��
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
		//����һ������ֵ���ڱ��һ�������������ѱ��ҹ���δ�ҵ�
		static constexpr uint32_t notFound = INT32_MAX;//== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
		//���������������ϵĽṹ��
		struct queueFamilyIndexCombination {
			uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
			uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
			uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
		};
		//queueFamilyIndexCombinations����Ϊ���������豸����һ�ݶ������������
		static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
		auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];

		//������κζ����������ѱ��ҹ���δ�ҵ�������VK_RESULT_MAX_ENUM
		if (ig == notFound && enableGraphicsQueue ||
			ip == notFound && surface ||
			ic == notFound && enableComputeQueue)
			return VK_RESULT_MAX_ENUM;

		//������κζ���������Ӧ����ȡ����δ���ҹ�
		if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
			ip == VK_QUEUE_FAMILY_IGNORED && surface ||
			ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
			uint32_t indices[3];
			Result result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
			//��GetQueueFamilyIndices(...)����VK_SUCCESS��VK_RESULT_MAX_ENUM��vkGetPhysicalDeviceSurfaceSupportKHR(...)ִ�гɹ���û������������壩��
			//˵��������������������н��ۣ���������queueFamilyIndexCombinations[deviceIndex]����Ӧ����
			//Ӧ����ȡ����������ΪVK_QUEUE_FAMILY_IGNORED��˵��δ�ҵ���Ӧ�����壬VK_QUEUE_FAMILY_IGNORED��~0u����INT32_MAX��λ��õ�����ֵ����notFound
			if (result == VK_SUCCESS ||
				result == VK_RESULT_MAX_ENUM) {
				if (enableGraphicsQueue)
					ig = indices[0] & INT32_MAX;
				if (surface)
					ip = indices[1] & INT32_MAX;
				if (enableComputeQueue)
					ic = indices[2] & INT32_MAX;
			}
			//���GetQueueFamilyIndices(...)ִ��ʧ�ܣ�return
			if (result)
				return result;
		}

		//����������if��֧�Բ�ִ�У���˵������Ķ������������ѱ���ȡ����queueFamilyIndexCombinations[deviceIndex]��ȡ������
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
		//������õ������豸����
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
			//�����ʽδָ����ֻƥ��ɫ�ʿռ䣬ͼ���ʽ��ɶ����ɶ
			for (auto& i : availableSurfaceFormats)
				if (i.colorSpace == surfaceFormat.colorSpace) {
					swapchainCreateInfo.imageFormat = i.format;
					swapchainCreateInfo.imageColorSpace = i.colorSpace;
					formatIsAvailable = true;
					break;
				}
		}
		else
			//����ƥ���ʽ��ɫ�ʿռ�
			for (auto& i : availableSurfaceFormats)
				if (i.format == surfaceFormat.format &&
					i.colorSpace == surfaceFormat.colorSpace) {
					swapchainCreateInfo.imageFormat = i.format;
					swapchainCreateInfo.imageColorSpace = i.colorSpace;
					formatIsAvailable = true;
					break;
				}
		//���û�з��ϵĸ�ʽ��ǡ���и���������Ĵ������
		if (!formatIsAvailable)
			return VK_ERROR_FORMAT_NOT_SUPPORTED;
		//����������Ѵ��ڣ�����RecreateSwapchain()�ؽ�������
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

		//��ȡ������ͼ��
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

		//����image view
		swapchainImageViews.resize(swapchainImageCount);
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchainCreateInfo.imageFormat,
			//.components = {},//�ĸ���Ա��ΪVK_COMPONENT_SWIZZLE_IDENTITY
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
		/*���������*/

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
			//��&&����������·ִ��
			if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
				SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) {
				//����Ҳ�������ͼ���ʽ��ɫ�ʿռ����ϣ���ֻ����ʲô��ʲô������availableSurfaceFormats�еĵ�һ��
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
		swapchainCreateInfo.imageArrayLayers = 1;
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

	Result GraphicsBase::RecreateSwapchain(bool limitFrame) {
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
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		uint32_t surfacePresentModeCount;
		if (Result result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!surfacePresentModeCount)
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to find any surface present mode!\n"),
			abort();

		std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
		if (Result result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
			std::cout << std::format("[ GraphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!limitFrame)
			for (size_t i = 0; i < surfacePresentModeCount; i++)
				if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
					swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
		Result result = vkQueueWaitIdle(queue_graphics);
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
		//���پɽ�����������ڣ�
		if (swapchainCreateInfo.oldSwapchain &&
			swapchainCreateInfo.oldSwapchain != swapchain) {
			vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
			swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		}
		//��ȡ������ͼ������
		while (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
			switch (result) {
			case VK_SUBOPTIMAL_KHR:
			case VK_ERROR_OUT_OF_DATE_KHR:
				if (VkResult result = RecreateSwapchain())
					return result;
				break; //ע���ؽ�������������Ҫ��ȡͼ��ͨ��break�ݹ飬�ٴ�ִ��while�������ж����
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
