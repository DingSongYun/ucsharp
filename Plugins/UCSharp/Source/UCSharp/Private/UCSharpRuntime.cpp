#include "UCSharpLibrary.h"
#include "UCSharpRuntime.h"
#include "UCSharpLogs.h"

#ifdef _WIN32
#include <Windows.h>
#define CORECLR_LIB_NAME TEXT("hostfxr.dll")
#define LoadLib(path) LoadLibraryA(path)
#define GetProcAddr(handle, name) GetProcAddress(handle, name)
typedef HMODULE LibHandle;
#else
#include <dlfcn.h>
#define CORECLR_LIB_NAME TEXT("hostfxr.so")
#define LoadLib(path) dlopen(path, RTLD_LAZY)
#define GetProcAddr(handle, name) dlsym(handle, name)
typedef void* LibHandle;
#endif

/** 定义不同平台的调用约定 */
#ifdef _WIN32
#define CALLTYPE __stdcall
#else
#define CALLTYPE
#endif


// hostfxr 相关的类型定义
struct HostfxrInitializeParameters
{
	size_t Size;
	const char* HostPath;
	const char* DotnetRoot;
};

typedef void* FHostfxrContextPtr;

// 初始化运行时配置的函数指针类型定义
typedef int(CALLTYPE* hostfxr_initialize_for_runtime_config)(const char* runtime_config_path,
														const HostfxrInitializeParameters* parameters,
														FHostfxrContextPtr* host_context_handle);

// 获取运行时委托的函数指针类型定	义
typedef int(CALLTYPE* hostfxr_get_runtime_delegate)(const FHostfxrContextPtr host_context_handle,
												int delegate_type,
												void** delegate);

// 关闭 hostfxr 上下文的函数指针类型定义
typedef int(CALLTYPE* hostfxr_close)(const FHostfxrContextPtr host_context_handle);

// 加载程序集和调用方法的委托类型
typedef int(CALLTYPE* hostfxr_load_assembly_and_get_function_pointer)(const char* assembly_path,
																const char* type_name,
																const char* method_name,
																const char* delegate_type_name,
																void* reserved,
																void** delegate);

// 获取 hostfxr 库中的函数指针
#define HOSTFXR_GET_API(LibHandle, FuncType) \
	static_cast<FuncType>(FPlatformProcess::GetDllExport(LibHandle, TEXT(#FuncType)))
// TODO: 实现一版带check的
#define HOSTFXR_GET_API_CHECKED(LibHandle, FuncType) \
	static_cast<FuncType>(FPlatformProcess::GetDllExport(LibHandle, TEXT(#FuncType)))

class FHostfxrProxy
{
private:
	/** hostfxr 库句柄 */
	LibHandle HostfxrHandle = nullptr;

	/** hostfxr上下文 */
	FHostfxrContextPtr HostContext = nullptr;

	/** 函数指针 */
	hostfxr_load_assembly_and_get_function_pointer LoadAssemblyAndGetFunctionPointerFunc;

public:
	static FHostfxrProxy& Get()
	{
		static FHostfxrProxy sFHostfxrProxy;
		return sFHostfxrProxy;
	}

	bool LoadCoreCLRLibrary()
	{
		HostfxrHandle = LoadLib(TCHAR_TO_UTF8(CORECLR_LIB_NAME));

		if (!HostfxrHandle)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to load hostfxr library: %s"), *FString::FromInt(GetLastError()));
			return false;
		}

		return true;
	}

	/** 初始化CSharp运行时 */
	bool InitializeForRuntimeConfig(const char* ConfigPath)
	{
		auto InitializeForRuntimeConfigFunc = HOSTFXR_GET_API_CHECKED(HostfxrHandle, hostfxr_initialize_for_runtime_config);
		ensureMsgf(InitializeForRuntimeConfigFunc, TEXT("Failed to get hostfxr function: hostfxr_initialize_for_runtime_config"));

		int RetCode = InitializeForRuntimeConfigFunc(ConfigPath, nullptr, &HostContext);
		if (RetCode != 0)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to initialize hostfxr: %d"), RetCode);
			return false;
		}
		return true;
	}

	void CloseHostfxr()
	{
		auto CloseFunc = HOSTFXR_GET_API_CHECKED(HostfxrHandle, hostfxr_close);
		CloseFunc(HostContext);
	}

	/** 加载程序集 */
	bool LoadAssemblyAndGetFunction(int delegate_type, void** delegate)
	{
		checkf(HostContext != nullptr, TEXT("Host context is not initialized. Call InitializeForRuntimeConfig first."));

		if (!LoadAssemblyAndGetFunctionPointerFunc)
		{
			hostfxr_get_runtime_delegate GetRuntimeDelegateFunc =
			    HOSTFXR_GET_API_CHECKED(HostfxrHandle, hostfxr_get_runtime_delegate);
			if (!GetRuntimeDelegateFunc)
			{
				UE_LOG(LogUCSharp, Error, TEXT("Failed to get hostfxr_get_runtime_delegate function pointer"));
				CloseHostfxr();
				return false;
			}
			int RetCode = GetRuntimeDelegateFunc(HostContext,
												4 /* component_entry_point_fn */,
												(void**)&LoadAssemblyAndGetFunctionPointerFunc);
			if (RetCode != 0 || LoadAssemblyAndGetFunctionPointerFunc == nullptr)
			{
				UE_LOG(LogUCSharp, Error, TEXT("Failed to get load_assembly_and_get_function_pointer delegate"));
				CloseHostfxr();
				return false;
			}
		}

		// 获取Add方法指针
		//int rc = LoadAssemblyAndGetFunctionPointerFunc(
		//    "SimpleCSharpLibrary.dll", "SimpleCSharpLibrary.Calculator, SimpleCSharpLibrary", "Add",
		//    "SimpleCSharpLibrary.Calculator+AddDelegate, SimpleCSharpLibrary", nullptr, (void**)&add);

		//if (rc != 0 || add == nullptr)
		//{
		//	std::cerr << "Failed to get function pointer: " << rc << std::endl;
		//	close_fptr(host_context_handle);
		//	return 1;
		//}

		return true;
	}

private:
	/** 私有构造器，不允许外部构造 */
	FHostfxrProxy() {}
};

bool FUCSharpRuntime::Initialize()
{
	FHostfxrProxy& HostfxrProxy = FHostfxrProxy::Get();

	// 1. 加载hostfxr库
	if (!HostfxrProxy.LoadCoreCLRLibrary())
	{
		return false;
	}

	// 2. 初始化.NET运行时
	const char* ConfigPath = TCHAR_TO_UTF8(*UCSharpLibrary::GetRuntimeConfigPath());
	if ( !HostfxrProxy.InitializeForRuntimeConfig(ConfigPath) )
	{
		return false;
	}

	// 3. 加载程序集
	//HostfxrProxy.LoadAssemblyAndGetFunctionPointer();
	return true;
}

void FUCSharpRuntime::Shutdown()
{
} 
