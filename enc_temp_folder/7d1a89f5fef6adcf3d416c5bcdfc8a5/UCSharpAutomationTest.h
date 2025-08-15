#pragma once

#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Tests/AutomationCommon.h"
#include "Misc/AutomationTest.h"
#include "UCSharp.h"


/**
 * UCSharp自动化测试模块
 * 负责加载和初始化UCSharp插件
 */
class FUCSharpAutomationTestModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

/**
 * UCSharp自动化测试基类
 * 提供UCSharp框架测试的通用功能
 */
class UCSHARPAUTOMATIONTEST_API FUCSharpTestBase : public FAutomationTestBase
{
public:
	FUCSharpTestBase(const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask)
	{
	}

protected:
	/**
	 * 测试前的初始化
	 */
	virtual void SetUp();

	/**
	 * 测试后的清理
	 */
	virtual void TearDown();

	/**
	 * 验证UCSharp插件是否正确加载
	 */
	bool IsUCSharpPluginLoaded() const;

	/**
	 * 验证C#运行时是否初始化
	 */
	bool IsCSharpRuntimeInitialized() const;

	/**
	 * 创建测试用的C# Actor
	 */
	class AActor* CreateTestCSharpActor(UWorld* World, const FString& ActorClassName);

	/**
	 * 验证C#对象的属性访问
	 */
	bool TestCSharpPropertyAccess(UObject* Object, const FString& PropertyName, const FString& ExpectedValue);

	/**
	 * 验证C#方法调用
	 */
	bool TestCSharpMethodCall(UObject* Object, const FString& MethodName, const TArray<FString>& Parameters);
};

#define IMPLEMENT_UCSHARP_AUTOMATION_TEST( TClass, PrettyName) \
	IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TClass, FUCSharpTestBase, PrettyName, EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)


/**
 * UCSharp核心功能测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpCoreTest, "UCSharp.Core.BasicFunctionality")

/**
 * UCSharp UObject绑定测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpUObjectBindingTest, "UCSharp.Core.UObjectBinding")

/**
 * UCSharp Actor创建和生命周期测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpActorLifecycleTest, "UCSharp.Core.ActorLifecycle")

/**
 * UCSharp属性访问测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpPropertyAccessTest, "UCSharp.Core.PropertyAccess")

/**
 * UCSharp方法调用测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpMethodCallTest, "UCSharp.Core.MethodCall")

/**
 * UCSharp内存管理测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpMemoryManagementTest, "UCSharp.Performance.MemoryManagement")

/**
 * UCSharp性能测试
 */
IMPLEMENT_UCSHARP_AUTOMATION_TEST(FUCSharpPerformanceTest, "UCSharp.Performance.Benchmarks")