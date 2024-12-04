#pragma once
#include <vulkan/vulkan.h>
#ifdef VK_RESULT_THROW
//���1�����ݺ�������ֵȷ���Ƿ����쳣
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

//���2����������������ֵ���ñ�������������
#elif defined VK_RESULT_NODISCARD
struct [[nodiscard]] Result {
	VkResult result;
	Result(VkResult result) :result(result) {}
	operator VkResult() const { return result; }
};
//�ڱ��ļ��йر���ֵ���ѣ���Ϊ�������������
#pragma warning(disable:4834)
#pragma warning(disable:6031)

//���3��ɶ������
#else
using Result = VkResult;
#endif


template<typename T>
class ArrayRef {
	T* const pArray = nullptr;
	size_t count = 0;
public:
	//�ӿղ������죬countΪ0
	ArrayRef() = default;
	//�ӵ��������죬countΪ1
	ArrayRef(T& data) :pArray(&data), count(1) {}
	//�Ӷ������鹹��
	template<size_t elementCount>
	ArrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
	//��ָ���Ԫ�ظ�������
	ArrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
	//���ƹ��죬��T��const���Σ����ݴӶ�Ӧ����const���ΰ汾��arrayRef����
	//24.01.07 ��������ճ��������typo����pArray(&other)��ΪpArray(other.Pointer())
	ArrayRef(const ArrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
	//Getter
	T* Pointer() const { return pArray; }
	size_t Count() const { return count; }
	//Const Function
	T& operator[](size_t index) const { return pArray[index]; }
	T* begin() const { return pArray; }
	T* end() const { return pArray + count; }
	//Non-const Function
	//��ֹ����/�ƶ���ֵ
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

#define ExecuteOnce(...)  { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

inline auto& outStream = std::cout; //����constexpr����Ϊstd::cout�����ⲿ����



// make lambda with capturelist can be converted to function pointer
template<class...Args>
struct FunctionCallBack {
	void(*function)(void*, Args...) = nullptr;
	std::unique_ptr<void, void(*)(void*)> state;
};


template<typename... Args, typename Lambda>
FunctionCallBack<Args...> Voidfy(Lambda&& l) {
	
	using Func = typename std::decay<Lambda>::type;
	
	std::unique_ptr<void, void(*)(void*)> data(
	  new Func(std::forward<Lambda>(l)),
	  +[](void* ptr) { delete (Func*)ptr; }
	);
	
	return {
	  +[](void* v, Args... args)->void {
		Func* f = static_cast<Func*>(v);
		(*f)(std::forward<Args>(args)...);
	  },
	  std::move(data)
	};
}
