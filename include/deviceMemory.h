#pragma once
#include "vkbase.h"
namespace vulkan
{
	class DeviceMemory {
		VkDeviceMemory handle = VK_NULL_HANDLE;
		VkDeviceSize allocationSize = 0; //实际分配的内存大小
		VkMemoryPropertyFlags memoryProperties = 0; //内存属性
		//--------------------
		//该函数用于在映射内存区时，调整非host coherent的内存区域的范围
		VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
			const VkDeviceSize& nonCoherentAtomSize = GraphicsBase::Base().PhysicalDeviceProperties().limits.nonCoherentAtomSize;
			VkDeviceSize _offset = offset;
			offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
			size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
			return _offset - offset;
		}
	protected:
		//用于bufferMemory或imageMemory，定义于此以节省8个字节
		class {
			friend class BufferMemory;
			friend class ImageMemory;
			bool value = false;
			operator bool() const { return value; }
			auto& operator=(bool value) { this->value = value; return *this; }
		} areBound;
	public:
		DeviceMemory() = default;
		DeviceMemory(VkMemoryAllocateInfo& allocateInfo) {
			Allocate(allocateInfo);
		}
		DeviceMemory(DeviceMemory&& other) noexcept {
			MoveHandle;
			allocationSize = other.allocationSize;
			memoryProperties = other.memoryProperties;
			other.allocationSize = 0;
			other.memoryProperties = 0;
		}
		~DeviceMemory() { DestroyHandleBy(vkFreeMemory); allocationSize = 0; memoryProperties = 0; }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		VkDeviceSize AllocationSize() const { return allocationSize; }
		VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }
		//Const Function
		//映射host visible的内存区
		Result MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
			VkDeviceSize inverseDeltaOffset;
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
			if (VkResult result = vkMapMemory(GraphicsBase::Base().Device(), handle, offset, size, 0, &pData)) {
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to map the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				pData = static_cast<uint8_t*>(pData) + inverseDeltaOffset;
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkInvalidateMappedMemoryRanges(GraphicsBase::Base().Device(), 1, &mappedMemoryRange)) {
					outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			return VK_SUCCESS;
		}
		//取消映射host visible的内存区
		Result UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				AdjustNonCoherentMemoryRange(size, offset);
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkFlushMappedMemoryRanges(GraphicsBase::Base().Device(), 1, &mappedMemoryRange)) {
					outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			vkUnmapMemory(GraphicsBase::Base().Device(), handle);
			return VK_SUCCESS;
		}
		//BufferData(...)用于方便地更新设备内存区，适用于用memcpy(...)向内存区写入数据后立刻取消映射的情况
		Result BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
			void* pData_dst;
			if (VkResult result = MapMemory(pData_dst, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}
		Result BufferData(const auto& data_src) const {
			return BufferData(&data_src, sizeof data_src);
		}
		//RetrieveData(...)用于方便地从设备内存区取回数据，适用于用memcpy(...)从内存区取得数据后立刻取消映射的情况
		Result RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
			void* pData_src;
			if (VkResult result = MapMemory(pData_src, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}
		//Non-const Function
		Result Allocate(VkMemoryAllocateInfo& allocateInfo) {
			if (allocateInfo.memoryTypeIndex >= GraphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount) {
				outStream << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			}
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			if (VkResult result = vkAllocateMemory(GraphicsBase::Base().Device(), &allocateInfo, nullptr, &handle)) {
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			//记录实际分配的内存大小
			allocationSize = allocateInfo.allocationSize;
			//取得内存属性
			memoryProperties = GraphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
			return VK_SUCCESS;
		}
	};
}
