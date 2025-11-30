# UCSharp C# 侧继承 UObject 与蓝图调用方案

## 目标
- 在 C# 侧以语义方式“继承”UObject/Actor，并由原生承载类映射托管对象。
- 通过运行时注册或桥接，使托管方法可作为蓝图节点调用。

## 架构
- 原生承载：`UCSManagedObject : UObject`，`UCSManagedActor : AActor` 持有 `ManagedTypeName` 与 `ManagedHandle`。
- 托管基类：`UCSharp.Core.UObject` 持有原生句柄与生命周期钩子。
- 互操作：`IUCSharpInterop` 维护 `Native↔Managed` 映射与类型编组。
- 运行时：`FUCSharpRuntime` 负责 hostfxr 初始化与托管委托获取/调用。

## 类型设计
- 原生
  - `UCSManagedObject`
    - `FString ManagedTypeName`
    - `UCSharpInterop::ManagedObjectHandle ManagedHandle`
    - `bool InitializeManaged()` / `void ShutdownManaged()`
    - `bool CallManaged(FName Method, TArray<FVariant> Args, FVariant& OutResult)`
  - `UCSManagedActor : UCSManagedObject`
    - `void BeginPlay()` / `void EndPlay()` / `void Tick(float)` → 托管生命周期
    - `UFUNCTION(BlueprintCallable) int32 ManagedAdd(int32 A, int32 B)`
  - `FManagedTypeRegistry`
    - 运行时注册托管方法/属性为 UFunction/UPROPERTY（后续阶段）
- 托管
  - `UCSharp.Core.UObject`
    - `IntPtr NativeHandle`
    - `void OnInitialize()` / `void OnDestroy()`
  - 示例：`UCSharp.Examples.Math.Add(int,int)` 供蓝图桥接验证

## 绑定流程
- 原生→托管：原生对象创建时调用 `CreateManagedObject(UObject*)` 建立 `ManagedHandle` 映射。
- 托管→原生：托管对象使用弱引用句柄访问原生；需要强保活时由托管请求 `AddToRoot`。
- 清理：原生失效移除映射与绑定；托管异常统一记录并返回默认值。

## 蓝图暴露
- MVP：提供统一桥接 UFUNCTION（如 `ManagedAdd`）调用托管 UnmanagedCallersOnly 委托。
- 后续：根据托管特性 `[UFunction(BlueprintCallable)]` 动态注册 UFunction，并绑定统一执行函数。

## 编组与类型映射
- 覆盖基础类型：`bool/int32/float/double/FString/UObject*`。
- `UObject*` 依赖 `Native↔Managed` 映射查询与有效性校验。

## 线程与生命周期
- 所有跨语言调用在游戏线程执行。
- Actor 生命周期钩子转发为托管方法，保持一致性。

## MVP 范围
- 新增 `UCSManagedObject`/`UCSManagedActor`。
- 在 C# 暴露 `InvokeStaticAdd(int,int)`（UnmanagedCallersOnly）与示例 `Math.Add`。
- 在原生运行时缓存并调用该委托；蓝图通过 `ManagedAdd` 调用 C#。

## 验证
- 构建 Managed 与 C++ 插件后启动 Editor。
- 蓝图基于 `UCSManagedActor` 调用 `ManagedAdd(2,3)` 期望返回 5。

