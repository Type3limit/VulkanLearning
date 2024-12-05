#pragma once
#include "vkbase.h"
#include "deviceMemory.h"
#include "image.h"
#include "vkbaseplus.h"
namespace vulkan {
	class Buffer {
		VkBuffer handle = VK_NULL_HANDLE;
	public:
		Buffer() = default;
		Buffer(VkBufferCreateInfo& createInfo) {
			Create(createInfo);
		}
		Buffer(Buffer&& other) noexcept { MoveHandle; }
		~Buffer() { DestroyHandleBy(vkDestroyBuffer); }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(GraphicsBase::Base().Device(), handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
			auto& physicalDeviceMemoryProperties = GraphicsBase::Base().PhysicalDeviceMemoryProperties();
			for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			//不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
			//if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
			//    outStream << std::format("[ buffer ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
			return memoryAllocateInfo;
		}
		Result BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
			VkResult result = vkBindBufferMemory(GraphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
			if (result)
				outStream << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		Result Create(VkBufferCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			VkResult result = vkCreateBuffer(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
				outStream << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
			return result;
		}
	};


	class BufferMemory :vulkan::Buffer, DeviceMemory {
	public:
		BufferMemory() = default;
		BufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
			Create(createInfo, desiredMemoryProperties);
		}
		BufferMemory(BufferMemory&& other) noexcept :
			vulkan::Buffer(std::move(other)), DeviceMemory(std::move(other)) {
			areBound = other.areBound;
			other.areBound = false;
		}
		~BufferMemory() { areBound = false; }
		//Getter
		//不定义到VkBuffer和VkDeviceMemory的转换函数，因为32位下这俩类型都是uint64_t的别名，会造成冲突（虽然，谁他妈还用32位PC！）
		VkBuffer Buffer() const { return static_cast<const vulkan::Buffer&>(*this); }
		const VkBuffer* AddressOfBuffer() const { return Buffer::Address(); }
		VkDeviceMemory Memory() const { return static_cast<const DeviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return DeviceMemory::Address(); }
		//若areBond为true，则成功分配了设备内存、创建了缓冲区，且成功绑定在一起
		bool AreBound() const { return areBound; }
		using DeviceMemory::AllocationSize;
		using DeviceMemory::MemoryProperties;
		//Const Function
		using DeviceMemory::MapMemory;
		using DeviceMemory::UnmapMemory;
		using DeviceMemory::BufferData;
		using DeviceMemory::RetrieveData;
		//Non-const Function
		//以下三个函数仅用于Create(...)可能执行失败的情况
		Result CreateBuffer(VkBufferCreateInfo& createInfo) {
			return Buffer::Create(createInfo);
		}
		Result AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
			VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
			if (allocateInfo.memoryTypeIndex >= GraphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			return Allocate(allocateInfo);
		}
		Result BindMemory() {
			if (VkResult result = Buffer::BindMemory(Memory()))
				return result;
			areBound = true;
			return VK_SUCCESS;
		}
		//分配设备内存、创建缓冲、绑定
		Result Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
			VkResult result;
			false || //这行用来应对Visual Studio中代码的对齐
				(result = CreateBuffer(createInfo)) || //用||短路执行
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
	};


	class StagingBuffer {
		static inline class {
			StagingBuffer* pointer = Create();
			StagingBuffer* Create() {
				static StagingBuffer stagingBuffer;
				pointer = &stagingBuffer;
				GraphicsBase::Base().AddCallback_DestroyDevice([] { stagingBuffer.~stagingBuffer(); });
				return pointer;
			}
		public:
			StagingBuffer& Get() const { return *pointer; }
		} stagingBuffer_mainThread;
	protected:
		BufferMemory bufferMemory;
		VkDeviceSize memoryUsage = 0;
		Image aliasedImage;
	public:
		StagingBuffer() = default;
		StagingBuffer(VkDeviceSize size) {
			Expand(size);
		}
		//Getter
		operator VkBuffer() const { return bufferMemory.Buffer(); }
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		VkImage AliasedImage() const { return aliasedImage; }
		//Const Function
		void RetrieveData(void* pData_src, VkDeviceSize size) const {
			bufferMemory.RetrieveData(pData_src, size);
		}
		//Non-const Function
		void Expand(VkDeviceSize size) {
			if (size <= AllocationSize())
				return;
			Release();
			VkBufferCreateInfo bufferCreateInfo = {
				.size = size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			};
			bufferMemory.Create(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		void Release() {
			bufferMemory.~bufferMemory();
		}
		void* MapMemory(VkDeviceSize size) {
			Expand(size);
			void* pData_dst = nullptr;
			bufferMemory.MapMemory(pData_dst, size);
			memoryUsage = size;
			return pData_dst;
		}
		void UnmapMemory() {
			bufferMemory.UnmapMemory(memoryUsage);
			memoryUsage = 0;
		}
		void BufferData(const void* pData_src, VkDeviceSize size) {
			Expand(size);
			bufferMemory.BufferData(pData_src, size);
		}
		[[nodiscard]]
		VkImage AliasedImage2d(VkFormat format, VkExtent2D extent) {
			return {};
		/*	if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
				return VK_NULL_HANDLE;
			VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height;
			if (imageDataSize > AllocationSize())
				return VK_NULL_HANDLE;
			VkImageFormatProperties imageFormatProperties = {};
			vkGetPhysicalDeviceImageFormatProperties(graphicsBase::Base().PhysicalDevice(),
				format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imageFormatProperties);
			if (extent.width > imageFormatProperties.maxExtent.width ||
				extent.height > imageFormatProperties.maxExtent.height ||
				imageDataSize > imageFormatProperties.maxResourceSize)
				return VK_NULL_HANDLE;
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_LINEAR,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
			};
			aliasedImage.~image();
			aliasedImage.Create(imageCreateInfo);
			VkImageSubresource subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			VkSubresourceLayout subresourceLayout = {};
			vkGetImageSubresourceLayout(GraphicsBase::Base().Device(), aliasedImage, &subResource, &subresourceLayout);
			if (subresourceLayout.size != imageDataSize)
				return VK_NULL_HANDLE;
			aliasedImage.BindMemory(bufferMemory.Memory());
			return aliasedImage;*/
		}
		//Static Function
		static VkBuffer Buffer_MainThread() {
			return stagingBuffer_mainThread.Get();
		}
		static void Expand_MainThread(VkDeviceSize size) {
			stagingBuffer_mainThread.Get().Expand(size);
		}
		static void Release_MainThread() {
			stagingBuffer_mainThread.Get().Release();
		}
		static void* MapMemory_MainThread(VkDeviceSize size) {
			return stagingBuffer_mainThread.Get().MapMemory(size);
		}
		static void UnmapMemory_MainThread() {
			stagingBuffer_mainThread.Get().UnmapMemory();
		}
		static void BufferData_MainThread(const void* pData_src, VkDeviceSize size) {
			stagingBuffer_mainThread.Get().BufferData(pData_src, size);
		}
		static void RetrieveData_MainThread(void* pData_src, VkDeviceSize size) {
			stagingBuffer_mainThread.Get().RetrieveData(pData_src, size);
		}
		[[nodiscard]]
		static VkImage AliasedImage2d_MainThread(VkFormat format, VkExtent2D extent) {
			return stagingBuffer_mainThread.Get().AliasedImage2d(format, extent);
		}
	};

	class DeviceLocalBuffer {
	protected:
		BufferMemory bufferMemory;
	public:
		DeviceLocalBuffer() = default;
		DeviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
			Create(size, desiredUsages_Without_transfer_dst);
		}
		//Getter
		operator VkBuffer() const { return bufferMemory.Buffer(); }
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		//Const Function
		void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				bufferMemory.BufferData(pData_src, size, offset);
				return;
			}
			StagingBuffer::BufferData_MainThread(pData_src, size);
			auto& commandBuffer = GraphicsBase::Plus().CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VkBufferCopy region = { 0, offset, size };
			vkCmdCopyBuffer(commandBuffer, StagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), 1, &region);
			commandBuffer.End();
			GraphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
		}
		void TransferData(const void* pData_src, uint32_t elementCount, VkDeviceSize elementSize, VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				void* pData_dst = nullptr;
				bufferMemory.MapMemory(pData_dst, stride_dst * elementCount, offset);
				for (size_t i = 0; i < elementCount; i++)
					memcpy(stride_dst * i + static_cast<uint8_t*>(pData_dst), stride_src * i + static_cast<const uint8_t*>(pData_src), size_t(elementSize));
				bufferMemory.UnmapMemory(elementCount * stride_dst, offset);
				return;
			}
			StagingBuffer::BufferData_MainThread(pData_src, stride_src * elementCount);
			auto& commandBuffer = GraphicsBase::Plus().CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			std::unique_ptr<VkBufferCopy[]> regions = std::make_unique<VkBufferCopy[]>(elementCount);
			for (size_t i = 0; i < elementCount; i++)
				regions[i] = { stride_src * i, stride_dst * i + offset, elementSize };
			vkCmdCopyBuffer(commandBuffer, StagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), elementCount, regions.get());
			commandBuffer.End();
			GraphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
		}
		void TransferData(const auto& data_src) const {
			TransferData(&data_src, sizeof data_src);
		}
		void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size_Limited_to_65536, VkDeviceSize offset = 0) const {
			vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), offset, size_Limited_to_65536, pData_src);
		}
		void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const auto& data_src) const {
			vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), 0, sizeof data_src, &data_src);
		}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
			VkBufferCreateInfo bufferCreateInfo = {
				.size = size,
				.usage = desiredUsages_Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			};
			false ||
				bufferMemory.CreateBuffer(bufferCreateInfo) ||
				bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
				bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
				bufferMemory.BindMemory();
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
			GraphicsBase::Base().WaitIdle();
			bufferMemory.~bufferMemory();
			Create(size, desiredUsages_Without_transfer_dst);
		}
	};

	class VertexBuffer :public DeviceLocalBuffer {
	public:
		VertexBuffer() = default;
		VertexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :DeviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			DeviceLocalBuffer::Create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			DeviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
		}
	};
}
