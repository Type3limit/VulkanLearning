#include "globalinclude.h"
#include "vkbase.h"
namespace vulkan
{
    class CommandBuffer {
        friend class CommandPool;//封装命令池的commandPool类负责分配和释放命令缓冲区，需要让其能访问私有成员handle
        VkCommandBuffer handle = VK_NULL_HANDLE;
    public:
        CommandBuffer() = default;
        CommandBuffer(CommandBuffer&& other) noexcept { MoveHandle; }
        //因释放命令缓冲区的函数被我定义在封装命令池的commandPool类中，没析构器
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        //这里没给inheritanceInfo设定默认参数，因为C++标准中规定对空指针解引用是未定义行为
        // （尽管运行期不必发生，且至少MSVC编译器允许这种代码），而我又一定要传引用而非指针，因而形成了两个Begin(...)
        Result Begin(VkCommandBufferUsageFlags usageFlags, VkCommandBufferInheritanceInfo& inheritanceInfo) const {
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            VkCommandBufferBeginInfo beginInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = usageFlags,
                .pInheritanceInfo = &inheritanceInfo
            };
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        Result Begin(VkCommandBufferUsageFlags usageFlags = 0) const {
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = usageFlags,
            };
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        Result End() const {
            VkResult result = vkEndCommandBuffer(handle);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

    class CommandPool {
        VkCommandPool handle = VK_NULL_HANDLE;
    public:
        CommandPool() = default;
        CommandPool(VkCommandPoolCreateInfo& createInfo) {
            Create(createInfo);
        }
        CommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            Create(queueFamilyIndex, flags);
        }
        CommandPool(CommandPool&& other) noexcept { MoveHandle; }
        ~CommandPool() { DestroyHandleBy(vkDestroyCommandPool); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        Result AllocateBuffers(ArrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            VkCommandBufferAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = handle,
                .level = level,
                .commandBufferCount = uint32_t(buffers.Count())
            };
            VkResult result = vkAllocateCommandBuffers(GraphicsBase::Base().Device(), &allocateInfo, buffers.Pointer());
            if (result)
                outStream << std::format("[ commandPool ] ERROR\nFailed to allocate command buffers!\nError code: {}\n", int32_t(result));
            return result;
        }
        Result AllocateBuffers(ArrayRef<CommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            return AllocateBuffers(
                { &buffers[0].handle, buffers.Count() },
                level);
        }
        void FreeBuffers(ArrayRef<VkCommandBuffer> buffers) const {
            vkFreeCommandBuffers(GraphicsBase::Base().Device(), handle, buffers.Count(), buffers.Pointer());
            memset(buffers.Pointer(), 0, buffers.Count() * sizeof(VkCommandBuffer));
        }
        void FreeBuffers(ArrayRef<CommandBuffer> buffers) const {
            FreeBuffers({ &buffers[0].handle, buffers.Count() });
        }
        //Non-const Function
        Result Create(VkCommandPoolCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            VkResult result = vkCreateCommandPool(GraphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
            return result;
        }
        Result Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            VkCommandPoolCreateInfo createInfo = {
                .flags = flags,
                .queueFamilyIndex = queueFamilyIndex
            };
            return Create(createInfo);
        }
    };
}