#include "vkbase.h"
namespace vulkan {
	class Fence {
		VkFence handle = VK_NULL_HANDLE;
	public:
		//fence() = default;
		Fence(VkFenceCreateInfo& createInfo) {
			Create(createInfo);
		}
		//Ĭ�Ϲ���������δ��λ��դ��
		Fence(VkFenceCreateFlags flags = 0) {
			Create(flags);
		}
		Fence(Fence&& other) noexcept { MoveHandle; }
		~Fence() { DestroyHandleBy(vkDestroyFence); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		Result Wait() const {
			VkResult result = vkWaitForFences(GraphicsBase::Base().Device(), 1, &handle, false, UINT64_MAX);
			if (result)
				outStream << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		Result Reset() const {
			VkResult result = vkResetFences(GraphicsBase::Base().Device(), 1, &handle);
			if (result)
				outStream << std::format("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		//��Ϊ���ȴ����������á������ξ������֣�����˺���
		Result WaitAndReset() const {
			VkResult result = Wait();
			result || (result = Reset());
			return result;
		}
		Result Status() const {
			VkResult result = vkGetFenceStatus(GraphicsBase::Base().Device(), handle);
			if (result < 0) //vkGetFenceStatus(...)�ɹ�ʱ�����ֽ�������Բ��ܽ����ж�result�Ƿ��0
				outStream << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		Result Create(VkFenceCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			VkResult result = vkCreateFence(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		Result Create(VkFenceCreateFlags flags = 0) {
			VkFenceCreateInfo createInfo = {
				.flags = flags
			};
			return Create(createInfo);
		}
	};

	class Semaphore {
		VkSemaphore handle = VK_NULL_HANDLE;
	public:
		//semaphore() = default;
		Semaphore(VkSemaphoreCreateInfo& createInfo) {
			Create(createInfo);
		}
		//Ĭ�Ϲ���������δ��λ���ź���
		Semaphore(/*VkSemaphoreCreateFlags flags*/) {
			Create();
		}
		Semaphore(Semaphore&& other) noexcept { MoveHandle; }
		~Semaphore() { DestroyHandleBy(vkDestroySemaphore); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		Result Create(VkSemaphoreCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkResult result = vkCreateSemaphore(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
			return result;
		}
		Result Create(/*VkSemaphoreCreateFlags flags*/) {
			VkSemaphoreCreateInfo createInfo = {};
			return Create(createInfo);
		}
	};

}