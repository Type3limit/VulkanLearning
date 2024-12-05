#pragma once
#include "vkbase.h"
namespace vulkan
{
	class Image {
		VkImage handle = VK_NULL_HANDLE;
	public:
		Image() = default;
		Image(VkImageCreateInfo& createInfo) {
			Create(createInfo);
		}
		Image(Image&& other) noexcept { MoveHandle; }
		~Image() { DestroyHandleBy(vkDestroyImage); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(GraphicsBase::Base().Device(), handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) {
				auto& physicalDeviceMemoryProperties = GraphicsBase::Base().PhysicalDeviceMemoryProperties();
				for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
					if (memoryTypeBits & 1 << i &&
						(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
						return i;
				return (size_t)UINT32_MAX;
				};
			memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);
			if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
				desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			//不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
			//if (memoryAllocateInfo.memoryTypeIndex == -1)
			//    outStream << std::format("[ image ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
			return memoryAllocateInfo;
		}
		Result BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
			VkResult result = vkBindImageMemory(GraphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
			if (result)
				outStream << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		Result Create(VkImageCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			VkResult result = vkCreateImage(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

	class ImageView {
		VkImageView handle = VK_NULL_HANDLE;
	public:
		ImageView() = default;
		ImageView(VkImageViewCreateInfo& createInfo) {
			Create(createInfo);
		}
		ImageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
			Create(image, viewType, format, subresourceRange, flags);
		}
		ImageView(ImageView&& other) noexcept { MoveHandle; }
		~ImageView() { DestroyHandleBy(vkDestroyImageView); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		Result Create(VkImageViewCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			VkResult result = vkCreateImageView(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
			return result;
		}
		Result Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
			VkImageViewCreateInfo createInfo = {
				.flags = flags,
				.image = image,
				.viewType = viewType,
				.format = format,
				.subresourceRange = subresourceRange
			};
			return Create(createInfo);
		}
	};

}
