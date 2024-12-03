#pragma once
#include <vulkan/vulkan.h>
#ifdef VK_RESULT_THROW
//情况1：根据函数返回值确定是否抛异常
class Result {
	VkResult result;
public:
	static void(*callback_throw)(VkResult);
	Result(VkResult result) :result(result) {}
	Result(Result&& other) noexcept :result(other.result) { other.result = VK_SUCCESS; }
	~Result() noexcept(false) {
		if (uint32_t(result) < VK_RESULT_MAX_ENUM)
			return;
		if (callback_throw)
			callback_throw(result);
		throw result;
	}
	operator VkResult() {
		VkResult result = this->result;
		this->result = VK_SUCCESS;
		return result;
	}
};
inline void(*Result::callback_throw)(VkResult);

//情况2：若抛弃函数返回值，让编译器发出警告
#elif defined VK_RESULT_NODISCARD
struct [[nodiscard]] Result {
	VkResult result;
	Result(VkResult result) :result(result) {}
	operator VkResult() const { return result; }
};
//在本文件中关闭弃值提醒（因为我懒得做处理）
#pragma warning(disable:4834)
#pragma warning(disable:6031)

//情况3：啥都不干
#else
using Result = VkResult;
#endif


template<typename T>
class ArrayRef {
	T* const pArray = nullptr;
	size_t count = 0;
public:
	//从空参数构造，count为0
	ArrayRef() = default;
	//从单个对象构造，count为1
	ArrayRef(T& data) :pArray(&data), count(1) {}
	//从顶级数组构造
	template<size_t elementCount>
	ArrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
	//从指针和元素个数构造
	ArrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
	//复制构造，若T带const修饰，兼容从对应的无const修饰版本的arrayRef构造
	//24.01.07 修正因复制粘贴产生的typo：从pArray(&other)改为pArray(other.Pointer())
	ArrayRef(const ArrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
	//Getter
	T* Pointer() const { return pArray; }
	size_t Count() const { return count; }
	//Const Function
	T& operator[](size_t index) const { return pArray[index]; }
	T* begin() const { return pArray; }
	T* end() const { return pArray + count; }
	//Non-const Function
	//禁止复制/移动赋值
	ArrayRef& operator=(const ArrayRef&) = delete;
};


#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
#else
#define ENABLE_DEBUG_MESSENGER false
#endif

#define DestroyHandleBy(Func) \
if (handle)\
{ \
Func(vulkan::GraphicsBase::Base().Device(), handle, nullptr);\
handle = VK_NULL_HANDLE;\
}

#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;

#define DefineMoveAssignmentOperator(type) type& operator=(type&& other) { this->~type(); MoveHandle; return *this; }

#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }

#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

//inline auto& outStream = std::cout;//不是constexpr，因为std::cout具有外部链接

