#pragma once
#include "command.h"
#include <synchronization.h>

namespace vulkan
{
	class GraphicsBasePlus {
		CommandPool CommandPool_graphics;
		CommandPool CommandPool_presentation;
		CommandPool CommandPool_compute;
		CommandBuffer CommandBuffer_transfer;//从CommandPool_graphics分配
		CommandBuffer CommandBuffer_presentation;
		//静态变量
		static GraphicsBasePlus singleton;
		//--------------------
		GraphicsBasePlus() {
			//在创建逻辑设备时执行Initialize()
			auto Initialize = [] {
				if (GraphicsBase::Base().QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
					singleton.CommandPool_graphics.Create(GraphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
					singleton.CommandPool_graphics.AllocateBuffers(singleton.CommandBuffer_transfer);
				if (GraphicsBase::Base().QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
					singleton.CommandPool_compute.Create(GraphicsBase::Base().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
				if (GraphicsBase::Base().QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
					GraphicsBase::Base().QueueFamilyIndex_Presentation() != GraphicsBase::Base().QueueFamilyIndex_Graphics() &&
					GraphicsBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
					singleton.CommandPool_presentation.Create(GraphicsBase::Base().QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
					singleton.CommandPool_presentation.AllocateBuffers(singleton.CommandBuffer_presentation);
				/*待后续填充*/
				};
			//在销毁逻辑设备时执行CleanUp()
			//如果你不需要更换物理设备或在运行中重启Vulkan（皆涉及重建逻辑设备），那么此CleanUp回调非必要
			//程序运行结束时，无论是否有这个回调，GraphicsBasePlus中的对象必会在析构GraphicsBase前被析构掉
			auto CleanUp = [] {
				singleton.CommandPool_graphics.~CommandPool();
				singleton.CommandPool_presentation.~CommandPool();
				singleton.CommandPool_compute.~CommandPool();
				};
			GraphicsBase::Plus(singleton);
			GraphicsBase::Base().AddCallback_CreateDevice(Initialize);
			GraphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
		}
		GraphicsBasePlus(GraphicsBasePlus&&) = delete;
		~GraphicsBasePlus() = default;
	public:
		//Getter
		const CommandPool& CommandPool_Graphics() const { return CommandPool_graphics; }
		const CommandPool& CommandPool_Compute() const { return CommandPool_compute; }
		const CommandBuffer& CommandBuffer_Transfer() const { return CommandBuffer_transfer; }
		//Const Function
		//简化命令提交
		Result ExecuteCommandBuffer_Graphics(VkCommandBuffer CommandBuffer) const {
			Fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = &CommandBuffer
			};
			VkResult result = GraphicsBase::Base().SubmitCommandBufferGraphics(submitInfo, fence);
			if (!result)
				fence.Wait();
			return result;
		}
		//该函数专用于向呈现队列提交用于接收交换链图像的队列族所有权的命令缓冲区
		Result AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver, VkSemaphore semaphore_ownershipIsTransfered, VkFence fence = VK_NULL_HANDLE) const {
			/*
			if (VkResult result = CommandBuffer_presentation.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
				return result;
			GraphicsBase::Base().CmdTransferImageOwnership(CommandBuffer_presentation);
			if (VkResult result = CommandBuffer_presentation.End())
				return result;
			return GraphicsBase::Base().SubmitCommandBuffer_Presentation(CommandBuffer_presentation, semaphore_renderingIsOver, semaphore_ownershipIsTransfered, fence);*/
			return VK_SUCCESS;
		}
	};
	inline GraphicsBasePlus GraphicsBasePlus::singleton;
}
