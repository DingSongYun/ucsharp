#include "UCSharpRuntime.h"
#include "UCSharpLibrary.h"
#include "UCSharpLogs.h"

#include "hostfxr.h"
#include "coreclr_delegates.h"

#include <vector>
#ifdef _WIN32
#include <Windows.h>
#define CORECLR_LIB_NAME TEXT("hostfxr.dll")
#define LoadLib(path) LoadLibraryW(path)
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

/** 平台字符类型 */
#ifdef _WIN32
using char_t = wchar_t;
#else
using char_t = char;
#endif

typedef void* FHostfxrContextPtr;


// 获取 hostfxr 库中的函数指针
#define HOSTFXR_GET_API(LibHandle, FuncType) \
	static_cast<FuncType>(FPlatformProcess::GetDllExport(LibHandle, TEXT(#FuncType)))
// TODO: 实现一版带check的
#define HOSTFXR_GET_API_CHECKED(LibHandle, FuncType) \
	static_cast<FuncType##_fn>(FPlatformProcess::GetDllExport(LibHandle, TEXT(#FuncType)))


static FString GetEnv(const wchar_t* Name)
{
	DWORD n = GetEnvironmentVariableW(Name, nullptr, 0);
	if (n == 0)
		return L"";
	std::vector<wchar_t> Buffer;
	Buffer.resize(n);
	GetEnvironmentVariableW(Name, Buffer.data(), n);
	return FString(Buffer.data());
}

static FString FindHostfxrPath()
{
#ifdef _WIN32
	HMODULE LibNethost = LoadLib(L"nethost.dll");
	if (LibNethost)
	{
		typedef int(HOSTFXR_CALLTYPE * get_hostfxr_path_fn)(char_t*, size_t*, void*);
		auto get_hostfxr_path = (get_hostfxr_path_fn)GetProcAddress(LibNethost, "get_hostfxr_path");
		if (get_hostfxr_path)
		{
			char_t buffer[MAX_PATH];
			size_t buffer_size = sizeof(buffer) / sizeof(char_t);
			if (get_hostfxr_path(buffer, &buffer_size, nullptr) == 0)
			{
				FreeLibrary(LibNethost);
				return FString(buffer);
			}
		}
		FreeLibrary(LibNethost);
	}
	FString DotnetRoot = GetEnv(L"DOTNET_ROOT");
	if (DotnetRoot.IsEmpty())
	{
		DotnetRoot = L"C:\\Program Files\\dotnet";
		if (!FPaths::DirectoryExists(DotnetRoot))
		{
			DotnetRoot = L"C:\\Program Files (x86)\\dotnet";
		}
	}
	FString FxrPath = FPaths::Combine(DotnetRoot, TEXT("host"), TEXT("fxr"));
	WIN32_FIND_DATAW FindData{};
	HANDLE hFind = FindFirstFileW(*(FxrPath + L"\\*"), &FindData);
	std::vector<FString> Dirs;
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && FindData.cFileName[0] != L'.')
				Dirs.emplace_back(FindData.cFileName);
		} while (FindNextFileW(hFind, &FindData));
		FindClose(hFind);
	}
	if (Dirs.empty())
		return L"";
	std::sort(Dirs.begin(), Dirs.end(), [](const FString& a, const FString& b) { return a < b; });
	return FPaths::Combine(FxrPath, Dirs.back(), TEXT("hostfxr.dll"));
#else
	return TEXT("hostfxr.so");
#endif
}

class FHostfxrProxy
{
private:
	/** hostfxr 库句柄 */
	LibHandle HostfxrHandle = nullptr;

	/** hostfxr上下文 */
	FHostfxrContextPtr HostContext = nullptr;

	/** 函数指针 */

	// 初始化运行时配置的函数指针类型定义
	hostfxr_initialize_for_runtime_config_fn		init_for_config_fptr;
	// 获取运行时委托的函数指针类型定	义
	hostfxr_get_runtime_delegate_fn					get_delegate_fptr;
	// 关闭 hostfxr 上下文
	hostfxr_close_fn								close_fptr;

	// 加载程序集和调用方法的委托类型
	load_assembly_and_get_function_pointer_fn LoadAssemblyAndGetFunctionPointerFunc;
	// 获取方法指针的委托类型
	get_function_pointer_fn get_function_pointer;

public:
	static FHostfxrProxy& Get()
	{
		static FHostfxrProxy sFHostfxrProxy;
		return sFHostfxrProxy;
	}

	bool LoadCoreCLRLibrary()
	{
		HostfxrHandle = LoadLib(*FindHostfxrPath());
		if (!HostfxrHandle)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to load hostfxr library: %s"), *FString::FromInt(GetLastError()));
			return false;
		}

		// 获取函数指针
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4191)
#endif
		init_for_config_fptr = reinterpret_cast <hostfxr_initialize_for_runtime_config_fn>(GetProcAddr(HostfxrHandle, "hostfxr_initialize_for_runtime_config"));
		get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddr(HostfxrHandle, "hostfxr_get_runtime_delegate"));
		close_fptr = reinterpret_cast<hostfxr_close_fn>(GetProcAddr(HostfxrHandle, "hostfxr_close"));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		check(init_for_config_fptr && get_delegate_fptr && close_fptr);
		return true;
	}

	/** 初始化CSharp运行时 */
	bool InitializeForRuntimeConfig(const char_t* ConfigPath)
	{
		int RetCode = init_for_config_fptr(ConfigPath, nullptr, &HostContext);
		if (RetCode != 0)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to initialize hostfxr: %d"), RetCode);
			return false;
		}

		// 初始化委托

		RetCode = get_delegate_fptr(HostContext, hdt_load_assembly_and_get_function_pointer, (void**)&LoadAssemblyAndGetFunctionPointerFunc);
		if (RetCode != 0 || LoadAssemblyAndGetFunctionPointerFunc == nullptr)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to get load_assembly_and_get_function_pointer delegate"));
			return false;
		}

		RetCode = get_delegate_fptr(HostContext, hdt_get_function_pointer, (void**)&get_function_pointer);
		if (RetCode != 0 || get_function_pointer == nullptr)
		{
			UE_LOG(LogUCSharp, Error, TEXT("Failed to get hdt_get_function_pointer func"));
			return false;
		}
		return true;
	}

	void CloseHostfxr()
	{
		close_fptr(HostContext);
	}

	/** 加载程序集 */
	bool LoadAssemblyAndGetFunction(const char_t* AssemblyPath, const char_t* TypeName, const char_t* MethodName, const char_t* DelegateType, void** OutDelegate)
	{
		checkf(HostContext != nullptr, TEXT("Host context is not initialized. Call InitializeForRuntimeConfig first."));
		check(LoadAssemblyAndGetFunctionPointerFunc);

		// 获取Add方法指针
		int RetCode = LoadAssemblyAndGetFunctionPointerFunc(AssemblyPath, TypeName, MethodName, DelegateType, nullptr, OutDelegate);

		if (RetCode != 0 || *OutDelegate == nullptr)
		{
			UE_LOG(LogUCSharp, Error,
				TEXT("Failed to load assembly and get function pointer, ret=%d, TypeName(%s), MethodName(%s)"),
				RetCode, TypeName, MethodName);
			return false;
		}

		return true;
	}

private:
	/** 私有构造器，不允许外部构造 */
	FHostfxrProxy() {}
};

bool FUCSharpRuntime::Initialize()
{
	FHostfxrProxy& HostfxrProxy = FHostfxrProxy::Get();

	// For Debug
	FPlatformMisc::SetEnvironmentVar(TEXT("COREHOST_TRACE"), TEXT("1"));
	FString TraceFile = FPaths::Combine(FPaths::ProjectLogDir(), TEXT("hostfxr.txt"));
	FPlatformMisc::SetEnvironmentVar(TEXT("COREHOST_TRACEFILE"), *TraceFile);

	// 1. 加载hostfxr库
	if (!HostfxrProxy.LoadCoreCLRLibrary())
	{
		return false;
	}

	// 2. 初始化.NET运行时
	FString ConfigPathStr = FPaths::ConvertRelativePathToFull(UCSharpLibrary::GetRuntimeConfigPath());
	FPaths::MakePlatformFilename(ConfigPathStr);
	if ( !HostfxrProxy.InitializeForRuntimeConfig(*ConfigPathStr) )
	{
		return false;
	}

	// 3. 加载程序集
	typedef void(CORECLR_DELEGATE_CALLTYPE * hello_fn)(const char*);

	FString AssemblyPath = UCSharpLibrary::GetAssemblyPath();
	AssemblyPath = FPaths::ConvertRelativePathToFull(AssemblyPath);
	FPaths::MakePlatformFilename(AssemblyPath);
	const char_t* TypeName = L"UCSharp.Program, UCSharp.Managed";
	const char_t* MethodName = L"Hello";

	hello_fn hello;
	if (HostfxrProxy.LoadAssemblyAndGetFunction(*AssemblyPath, TypeName, L"Hello",
		UNMANAGEDCALLERSONLY_METHOD, (void**)&hello))
	{
		hello("UCSharp");
	}

	UE_LOG(LogUCSharp, Log, TEXT("Managed InitializeUnmanaged invoked successfully"));
	return true;
}

void FUCSharpRuntime::Shutdown()
{
}
