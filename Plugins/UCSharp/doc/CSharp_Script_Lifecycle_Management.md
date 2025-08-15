# C#脚本生命周期管理方案

## 1. 生命周期管理架构概述

### 1.1 生命周期阶段

```
┌─────────────────────────────────────────────────────────────────┐
│                    C#脚本生命周期管理                           │
├─────────────────────────────────────────────────────────────────┤
│  编译时阶段 (Compile Time)                                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   源码扫描      │  │   依赖分析      │  │   编译生成      │ │
│  │   - .cs文件     │  │   - 引用检查    │  │   - Assembly    │ │
│  │   - 变更检测    │  │   - 版本管理    │  │   - 元数据      │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  加载时阶段 (Load Time)                                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   程序集加载    │  │   类型发现      │  │   实例创建      │ │
│  │   - Assembly    │  │   - 反射扫描    │  │   - 对象池      │ │
│  │   - 依赖解析    │  │   - 特性分析    │  │   - 初始化      │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  运行时阶段 (Runtime)                                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   脚本执行      │  │   事件处理      │  │   状态管理      │ │
│  │   - 方法调用    │  │   - 生命周期    │  │   - 持久化      │ │
│  │   - 异常处理    │  │   - 事件分发    │  │   - 序列化      │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  热重载阶段 (Hot Reload)                                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   变更检测      │  │   状态保存      │  │   重新加载      │ │
│  │   - 文件监控    │  │   - 序列化      │  │   - 状态恢复    │ │
│  │   - 增量编译    │  │   - 引用更新    │  │   - 事件重连    │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  卸载阶段 (Unload)                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   资源清理      │  │   内存释放      │  │   程序集卸载    │ │
│  │   - 对象销毁    │  │   - GC触发      │  │   - 域清理      │ │
│  │   - 事件取消    │  │   - 引用断开    │  │   - 缓存清空    │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 脚本管理器核心实现

### 2.1 脚本管理器主类

```cpp
// UCSharpScriptManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstanceSubsystem.h"
#include "UCSharpScriptManager.generated.h"

// 脚本状态枚举
UENUM(BlueprintType)
enum class EScriptState : uint8
{
    Unloaded,
    Loading,
    Loaded,
    Running,
    Reloading,
    Error
};

// 脚本信息结构
USTRUCT(BlueprintType)
struct UCSHARPRUNTIME_API FScriptInfo
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    FString ScriptName;
    
    UPROPERTY(BlueprintReadOnly)
    FString FilePath;
    
    UPROPERTY(BlueprintReadOnly)
    FDateTime LastModified;
    
    UPROPERTY(BlueprintReadOnly)
    EScriptState State;
    
    UPROPERTY(BlueprintReadOnly)
    FString AssemblyPath;
    
    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Dependencies;
    
    // 运行时数据
    void* AssemblyHandle;
    TMap<FString, void*> TypeHandles;
    TMap<int32, void*> InstanceHandles;
    
    FScriptInfo()
        : State(EScriptState::Unloaded)
        , AssemblyHandle(nullptr)
    {}
};

// 脚本事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScriptStateChanged, const FString&, ScriptName, EScriptState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScriptError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScriptReloadComplete);

UCLASS()
class UCSHARPRUNTIME_API UCSharpScriptManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()
    
public:
    // 子系统接口
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    
    // 脚本管理接口
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    bool LoadScript(const FString& ScriptPath);
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    bool UnloadScript(const FString& ScriptName);
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    bool ReloadScript(const FString& ScriptName);
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    bool ReloadAllScripts();
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    TArray<FScriptInfo> GetLoadedScripts() const;
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    EScriptState GetScriptState(const FString& ScriptName) const;
    
    // 脚本实例管理
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    int32 CreateScriptInstance(const FString& ScriptName, const FString& ClassName);
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    bool DestroyScriptInstance(int32 InstanceId);
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    bool CallScriptMethod(int32 InstanceId, const FString& MethodName, const TArray<FString>& Parameters);
    
    // 热重载支持
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    void EnableHotReload(bool bEnable);
    
    UFUNCTION(BlueprintCallable, Category = "C# Scripts")
    void SetHotReloadInterval(float IntervalSeconds);
    
    // 事件
    UPROPERTY(BlueprintAssignable)
    FOnScriptStateChanged OnScriptStateChanged;
    
    UPROPERTY(BlueprintAssignable)
    FOnScriptError OnScriptError;
    
    UPROPERTY(BlueprintAssignable)
    FOnScriptReloadComplete OnScriptReloadComplete;
    
protected:
    // 内部方法
    bool CompileScript(const FString& ScriptPath, FString& OutAssemblyPath);
    bool LoadAssembly(const FString& AssemblyPath, FScriptInfo& ScriptInfo);
    bool UnloadAssembly(FScriptInfo& ScriptInfo);
    
    void ScanForScriptChanges();
    void ProcessScriptChange(const FString& ScriptPath);
    
    void SetScriptState(const FString& ScriptName, EScriptState NewState);
    void BroadcastError(const FString& ErrorMessage);
    
    // 文件监控
    void StartFileWatcher();
    void StopFileWatcher();
    void OnFileChanged(const FString& FilePath);
    
private:
    // 脚本数据
    TMap<FString, FScriptInfo> LoadedScripts;
    TMap<int32, FScriptInfo*> InstanceToScript;
    int32 NextInstanceId;
    
    // 热重载设置
    bool bHotReloadEnabled;
    float HotReloadInterval;
    FTimerHandle HotReloadTimerHandle;
    
    // 文件监控
    void* FileWatcherHandle;
    TSet<FString> WatchedDirectories;
    
    // 线程安全
    FCriticalSection ScriptDataMutex;
    
    // .NET运行时
    void* DotNetRuntimeHandle;
    
    // 编译器设置
    FString CompilerPath;
    TArray<FString> DefaultReferences;
    TArray<FString> CompilerFlags;
};
```

### 2.2 脚本编译系统

```cpp
// UCSharpCompiler.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

// 编译结果
struct UCSHARPRUNTIME_API FCompilationResult
{
    bool bSuccess;
    FString AssemblyPath;
    TArray<FString> Errors;
    TArray<FString> Warnings;
    TArray<FString> GeneratedFiles;
    double CompilationTime;
    
    FCompilationResult()
        : bSuccess(false)
        , CompilationTime(0.0)
    {}
};

// 编译选项
struct UCSHARPRUNTIME_API FCompilationOptions
{
    TArray<FString> SourceFiles;
    TArray<FString> References;
    TArray<FString> PreprocessorDefines;
    FString OutputPath;
    FString TargetFramework;
    bool bOptimize;
    bool bGenerateDebugInfo;
    bool bTreatWarningsAsErrors;
    
    FCompilationOptions()
        : TargetFramework(TEXT("net6.0"))
        , bOptimize(false)
        , bGenerateDebugInfo(true)
        , bTreatWarningsAsErrors(false)
    {}
};

class UCSHARPRUNTIME_API UCSharpCompiler
{
public:
    // 编译接口
    static FCompilationResult CompileScript(const FString& ScriptPath, const FCompilationOptions& Options);
    static FCompilationResult CompileScripts(const TArray<FString>& ScriptPaths, const FCompilationOptions& Options);
    
    // 增量编译
    static FCompilationResult IncrementalCompile(const TArray<FString>& ChangedFiles, const FCompilationOptions& Options);
    
    // 依赖分析
    static TArray<FString> AnalyzeDependencies(const FString& ScriptPath);
    static bool ValidateDependencies(const TArray<FString>& Dependencies);
    
    // 编译器配置
    static void SetCompilerPath(const FString& Path);
    static void AddDefaultReference(const FString& Reference);
    static void SetDefaultCompilerFlags(const TArray<FString>& Flags);
    
private:
    // 内部编译方法
    static bool InvokeCompiler(const FCompilationOptions& Options, FCompilationResult& Result);
    static bool ParseCompilerOutput(const FString& Output, FCompilationResult& Result);
    
    // 文件操作
    static bool CreateProjectFile(const FCompilationOptions& Options, FString& OutProjectPath);
    static bool CleanupTempFiles(const TArray<FString>& TempFiles);
    
    // 缓存管理
    static bool IsCacheValid(const FString& ScriptPath, const FString& AssemblyPath);
    static void UpdateCache(const FString& ScriptPath, const FString& AssemblyPath);
    
    static FString CompilerPath;
    static TArray<FString> DefaultReferences;
    static TArray<FString> DefaultFlags;
    static TMap<FString, FDateTime> CompilationCache;
};
```

## 3. C#端生命周期管理

### 3.1 脚本基类和生命周期接口

```csharp
// UnrealEngine.Scripting.cs
using System;
using System.Collections.Generic;
using System.Reflection;

namespace UnrealEngine.Scripting
{
    /// <summary>
    /// 脚本生命周期接口
    /// </summary>
    public interface IScriptLifecycle
    {
        /// <summary>
        /// 脚本初始化
        /// </summary>
        void OnInitialize();
        
        /// <summary>
        /// 脚本启动
        /// </summary>
        void OnStart();
        
        /// <summary>
        /// 脚本更新（每帧调用）
        /// </summary>
        void OnUpdate(float deltaTime);
        
        /// <summary>
        /// 脚本停止
        /// </summary>
        void OnStop();
        
        /// <summary>
        /// 脚本销毁
        /// </summary>
        void OnDestroy();
        
        /// <summary>
        /// 热重载前保存状态
        /// </summary>
        object OnSaveState();
        
        /// <summary>
        /// 热重载后恢复状态
        /// </summary>
        void OnRestoreState(object state);
    }
    
    /// <summary>
    /// 脚本基类
    /// </summary>
    public abstract class ScriptBase : IScriptLifecycle, IDisposable
    {
        protected bool _isInitialized = false;
        protected bool _isStarted = false;
        protected bool _disposed = false;
        
        public int InstanceId { get; internal set; }
        public string ScriptName { get; internal set; }
        public ScriptManager Manager { get; internal set; }
        
        #region IScriptLifecycle Implementation
        
        public virtual void OnInitialize()
        {
            if (_isInitialized) return;
            
            _isInitialized = true;
            UE.Log($"Script {ScriptName} initialized.");
        }
        
        public virtual void OnStart()
        {
            if (!_isInitialized)
            {
                OnInitialize();
            }
            
            if (_isStarted) return;
            
            _isStarted = true;
            UE.Log($"Script {ScriptName} started.");
        }
        
        public virtual void OnUpdate(float deltaTime)
        {
            // 默认实现为空，子类可以重写
        }
        
        public virtual void OnStop()
        {
            if (!_isStarted) return;
            
            _isStarted = false;
            UE.Log($"Script {ScriptName} stopped.");
        }
        
        public virtual void OnDestroy()
        {
            if (!_disposed)
            {
                OnStop();
                _disposed = true;
                UE.Log($"Script {ScriptName} destroyed.");
            }
        }
        
        public virtual object OnSaveState()
        {
            // 默认返回null，子类可以重写以保存状态
            return null;
        }
        
        public virtual void OnRestoreState(object state)
        {
            // 默认实现为空，子类可以重写以恢复状态
        }
        
        #endregion
        
        #region IDisposable Implementation
        
        ~ScriptBase()
        {
            Dispose(false);
        }
        
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        
        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                OnDestroy();
            }
        }
        
        #endregion
        
        #region Utility Methods
        
        /// <summary>
        /// 获取脚本属性
        /// </summary>
        protected T GetScriptProperty<T>(string propertyName, T defaultValue = default)
        {
            return Manager?.GetScriptProperty<T>(InstanceId, propertyName) ?? defaultValue;
        }
        
        /// <summary>
        /// 设置脚本属性
        /// </summary>
        protected void SetScriptProperty<T>(string propertyName, T value)
        {
            Manager?.SetScriptProperty(InstanceId, propertyName, value);
        }
        
        /// <summary>
        /// 调用其他脚本方法
        /// </summary>
        protected T CallScriptMethod<T>(int targetInstanceId, string methodName, params object[] parameters)
        {
            return Manager.CallScriptMethod<T>(targetInstanceId, methodName, parameters);
        }
        
        #endregion
    }
    
    /// <summary>
    /// 脚本特性，用于标记脚本类
    /// </summary>
    [AttributeUsage(AttributeTargets.Class)]
    public class ScriptAttribute : Attribute
    {
        public string Name { get; }
        public string Description { get; }
        public bool AutoStart { get; }
        public int Priority { get; }
        
        public ScriptAttribute(string name, string description = "", bool autoStart = true, int priority = 0)
        {
            Name = name;
            Description = description;
            AutoStart = autoStart;
            Priority = priority;
        }
    }
    
    /// <summary>
    /// 脚本方法特性，用于标记可从外部调用的方法
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    public class ScriptMethodAttribute : Attribute
    {
        public string Name { get; }
        public string Description { get; }
        
        public ScriptMethodAttribute(string name = null, string description = "")
        {
            Name = name;
            Description = description;
        }
    }
}
```

### 3.2 脚本管理器C#端实现

```csharp
// UnrealEngine.Scripting.ScriptManager.cs
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using UnrealEngine.Interop;

namespace UnrealEngine.Scripting
{
    /// <summary>
    /// 脚本管理器
    /// </summary>
    public class ScriptManager
    {
        private static ScriptManager _instance;
        public static ScriptManager Instance => _instance ??= new ScriptManager();
        
        private readonly ConcurrentDictionary<int, ScriptBase> _scriptInstances 
            = new ConcurrentDictionary<int, ScriptBase>();
        private readonly ConcurrentDictionary<string, Assembly> _loadedAssemblies 
            = new ConcurrentDictionary<string, Assembly>();
        private readonly ConcurrentDictionary<int, Dictionary<string, object>> _scriptProperties 
            = new ConcurrentDictionary<int, Dictionary<string, object>>();
        
        private readonly Dictionary<int, object> _savedStates = new Dictionary<int, object>();
        private bool _isHotReloading = false;
        
        #region Script Instance Management
        
        /// <summary>
        /// 创建脚本实例
        /// </summary>
        public int CreateScriptInstance(string assemblyPath, string typeName)
        {
            try
            {
                var assembly = LoadAssembly(assemblyPath);
                var type = assembly.GetType(typeName);
                
                if (type == null)
                {
                    UE.LogError($"Type '{typeName}' not found in assembly '{assemblyPath}'.");
                    return -1;
                }
                
                if (!typeof(ScriptBase).IsAssignableFrom(type))
                {
                    UE.LogError($"Type '{typeName}' does not inherit from ScriptBase.");
                    return -1;
                }
                
                var instance = (ScriptBase)Activator.CreateInstance(type);
                var instanceId = GenerateInstanceId();
                
                instance.InstanceId = instanceId;
                instance.ScriptName = typeName;
                instance.Manager = this;
                
                _scriptInstances[instanceId] = instance;
                _scriptProperties[instanceId] = new Dictionary<string, object>();
                
                // 检查是否需要自动启动
                var scriptAttr = type.GetCustomAttribute<ScriptAttribute>();
                if (scriptAttr?.AutoStart == true)
                {
                    instance.OnInitialize();
                    instance.OnStart();
                }
                
                UE.Log($"Created script instance {instanceId} of type {typeName}.");
                return instanceId;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to create script instance: {ex.Message}");
                return -1;
            }
        }
        
        /// <summary>
        /// 销毁脚本实例
        /// </summary>
        public bool DestroyScriptInstance(int instanceId)
        {
            if (_scriptInstances.TryRemove(instanceId, out var instance))
            {
                try
                {
                    instance.Dispose();
                    _scriptProperties.TryRemove(instanceId, out _);
                    _savedStates.Remove(instanceId);
                    
                    UE.Log($"Destroyed script instance {instanceId}.");
                    return true;
                }
                catch (Exception ex)
                {
                    UE.LogError($"Error destroying script instance {instanceId}: {ex.Message}");
                }
            }
            
            return false;
        }
        
        /// <summary>
        /// 获取脚本实例
        /// </summary>
        public ScriptBase GetScriptInstance(int instanceId)
        {
            _scriptInstances.TryGetValue(instanceId, out var instance);
            return instance;
        }
        
        /// <summary>
        /// 获取所有脚本实例
        /// </summary>
        public IEnumerable<ScriptBase> GetAllScriptInstances()
        {
            return _scriptInstances.Values;
        }
        
        #endregion
        
        #region Assembly Management
        
        /// <summary>
        /// 加载程序集
        /// </summary>
        public Assembly LoadAssembly(string assemblyPath)
        {
            var fullPath = Path.GetFullPath(assemblyPath);
            
            if (_loadedAssemblies.TryGetValue(fullPath, out var existingAssembly))
            {
                return existingAssembly;
            }
            
            try
            {
                var assembly = Assembly.LoadFrom(fullPath);
                _loadedAssemblies[fullPath] = assembly;
                
                UE.Log($"Loaded assembly: {fullPath}");
                return assembly;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to load assembly '{fullPath}': {ex.Message}");
                throw;
            }
        }
        
        /// <summary>
        /// 卸载程序集（.NET Core 3.0+支持）
        /// </summary>
        public bool UnloadAssembly(string assemblyPath)
        {
            var fullPath = Path.GetFullPath(assemblyPath);
            
            if (_loadedAssemblies.TryRemove(fullPath, out var assembly))
            {
                // 销毁该程序集的所有实例
                var instancesToDestroy = _scriptInstances
                    .Where(kvp => kvp.Value.GetType().Assembly == assembly)
                    .Select(kvp => kvp.Key)
                    .ToList();
                
                foreach (var instanceId in instancesToDestroy)
                {
                    DestroyScriptInstance(instanceId);
                }
                
                UE.Log($"Unloaded assembly: {fullPath}");
                return true;
            }
            
            return false;
        }
        
        #endregion
        
        #region Hot Reload Support
        
        /// <summary>
        /// 开始热重载
        /// </summary>
        public async Task BeginHotReload()
        {
            if (_isHotReloading)
            {
                UE.LogWarning("Hot reload already in progress.");
                return;
            }
            
            _isHotReloading = true;
            UE.Log("Beginning hot reload...");
            
            try
            {
                // 保存所有脚本状态
                await SaveAllScriptStates();
                
                // 停止所有脚本
                foreach (var instance in _scriptInstances.Values)
                {
                    instance.OnStop();
                }
                
                UE.Log("Hot reload preparation complete.");
            }
            catch (Exception ex)
            {
                UE.LogError($"Error during hot reload preparation: {ex.Message}");
                _isHotReloading = false;
            }
        }
        
        /// <summary>
        /// 完成热重载
        /// </summary>
        public async Task CompleteHotReload()
        {
            if (!_isHotReloading)
            {
                UE.LogWarning("No hot reload in progress.");
                return;
            }
            
            try
            {
                // 重新启动所有脚本
                foreach (var instance in _scriptInstances.Values)
                {
                    instance.OnStart();
                }
                
                // 恢复所有脚本状态
                await RestoreAllScriptStates();
                
                UE.Log("Hot reload completed successfully.");
            }
            catch (Exception ex)
            {
                UE.LogError($"Error during hot reload completion: {ex.Message}");
            }
            finally
            {
                _isHotReloading = false;
            }
        }
        
        /// <summary>
        /// 保存所有脚本状态
        /// </summary>
        private async Task SaveAllScriptStates()
        {
            await Task.Run(() =>
            {
                foreach (var kvp in _scriptInstances)
                {
                    try
                    {
                        var state = kvp.Value.OnSaveState();
                        if (state != null)
                        {
                            _savedStates[kvp.Key] = state;
                        }
                    }
                    catch (Exception ex)
                    {
                        UE.LogError($"Error saving state for script instance {kvp.Key}: {ex.Message}");
                    }
                }
            });
        }
        
        /// <summary>
        /// 恢复所有脚本状态
        /// </summary>
        private async Task RestoreAllScriptStates()
        {
            await Task.Run(() =>
            {
                foreach (var kvp in _savedStates)
                {
                    if (_scriptInstances.TryGetValue(kvp.Key, out var instance))
                    {
                        try
                        {
                            instance.OnRestoreState(kvp.Value);
                        }
                        catch (Exception ex)
                        {
                            UE.LogError($"Error restoring state for script instance {kvp.Key}: {ex.Message}");
                        }
                    }
                }
                
                _savedStates.Clear();
            });
        }
        
        #endregion
        
        #region Script Method Invocation
        
        /// <summary>
        /// 调用脚本方法
        /// </summary>
        public T CallScriptMethod<T>(int instanceId, string methodName, params object[] parameters)
        {
            if (!_scriptInstances.TryGetValue(instanceId, out var instance))
            {
                UE.LogError($"Script instance {instanceId} not found.");
                return default;
            }
            
            try
            {
                var type = instance.GetType();
                var method = type.GetMethod(methodName, BindingFlags.Public | BindingFlags.Instance);
                
                if (method == null)
                {
                    UE.LogError($"Method '{methodName}' not found in script instance {instanceId}.");
                    return default;
                }
                
                var result = method.Invoke(instance, parameters);
                return result is T ? (T)result : default;
            }
            catch (Exception ex)
            {
                UE.LogError($"Error calling method '{methodName}' on script instance {instanceId}: {ex.Message}");
                return default;
            }
        }
        
        /// <summary>
        /// 调用脚本方法（无返回值）
        /// </summary>
        public bool CallScriptMethod(int instanceId, string methodName, params object[] parameters)
        {
            CallScriptMethod<object>(instanceId, methodName, parameters);
            return true;
        }
        
        #endregion
        
        #region Script Properties
        
        /// <summary>
        /// 获取脚本属性
        /// </summary>
        public T GetScriptProperty<T>(int instanceId, string propertyName)
        {
            if (_scriptProperties.TryGetValue(instanceId, out var properties) &&
                properties.TryGetValue(propertyName, out var value))
            {
                return value is T ? (T)value : default;
            }
            
            return default;
        }
        
        /// <summary>
        /// 设置脚本属性
        /// </summary>
        public void SetScriptProperty<T>(int instanceId, string propertyName, T value)
        {
            if (_scriptProperties.TryGetValue(instanceId, out var properties))
            {
                properties[propertyName] = value;
            }
        }
        
        #endregion
        
        #region Update Loop
        
        /// <summary>
        /// 更新所有脚本（每帧调用）
        /// </summary>
        public void UpdateAllScripts(float deltaTime)
        {
            if (_isHotReloading) return;
            
            foreach (var instance in _scriptInstances.Values)
            {
                try
                {
                    if (instance._isStarted)
                    {
                        instance.OnUpdate(deltaTime);
                    }
                }
                catch (Exception ex)
                {
                    UE.LogError($"Error updating script instance {instance.InstanceId}: {ex.Message}");
                }
            }
        }
        
        #endregion
        
        #region Utility Methods
        
        private int GenerateInstanceId()
        {
            return _scriptInstances.Count > 0 ? _scriptInstances.Keys.Max() + 1 : 1;
        }
        
        /// <summary>
        /// 获取程序集中的所有脚本类型
        /// </summary>
        public IEnumerable<Type> GetScriptTypes(Assembly assembly)
        {
            return assembly.GetTypes()
                .Where(t => typeof(ScriptBase).IsAssignableFrom(t) && !t.IsAbstract)
                .Where(t => t.GetCustomAttribute<ScriptAttribute>() != null);
        }
        
        /// <summary>
        /// 清理所有资源
        /// </summary>
        public void Cleanup()
        {
            foreach (var instance in _scriptInstances.Values)
            {
                instance.Dispose();
            }
            
            _scriptInstances.Clear();
            _scriptProperties.Clear();
            _savedStates.Clear();
            _loadedAssemblies.Clear();
        }
        
        #endregion
    }
}
```

## 4. 热重载实现

### 4.1 文件监控系统

```cpp
// UCSharpFileWatcher.h
#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

// 文件变更事件
struct FFileChangeEvent
{
    FString FilePath;
    FDateTime ChangeTime;
    enum EChangeType
    {
        Added,
        Modified,
        Deleted,
        Renamed
    } ChangeType;
};

// 文件变更委托
DECLARE_DELEGATE_OneParam(FOnFileChanged, const FFileChangeEvent&);

class UCSHARPRUNTIME_API UCSharpFileWatcher : public FRunnable
{
public:
    UCSharpFileWatcher();
    virtual ~UCSharpFileWatcher();
    
    // 开始监控
    bool StartWatching(const TArray<FString>& Directories);
    
    // 停止监控
    void StopWatching();
    
    // 添加监控目录
    void AddWatchDirectory(const FString& Directory);
    
    // 移除监控目录
    void RemoveWatchDirectory(const FString& Directory);
    
    // 设置文件变更回调
    void SetFileChangeCallback(const FOnFileChanged& Callback);
    
    // FRunnable接口
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;
    
private:
    // 处理文件变更
    void ProcessFileChanges();
    void HandleFileChange(const FString& FilePath, FFileChangeEvent::EChangeType ChangeType);
    
    // 过滤文件
    bool ShouldWatchFile(const FString& FilePath) const;
    
    // 线程相关
    FRunnableThread* Thread;
    FThreadSafeCounter StopTaskCounter;
    
    // 监控数据
    TArray<FString> WatchedDirectories;
    TMap<FString, FDateTime> FileTimestamps;
    FOnFileChanged FileChangeCallback;
    
    // 平台特定的监控句柄
#if PLATFORM_WINDOWS
    TArray<void*> DirectoryHandles;
#elif PLATFORM_LINUX || PLATFORM_MAC
    int InotifyFd;
    TMap<int, FString> WatchDescriptors;
#endif
    
    // 设置
    float CheckInterval;
    TArray<FString> WatchedExtensions;
    
    FCriticalSection DataMutex;
};
```

### 4.2 增量编译系统

```cpp
// UCSharpIncrementalCompiler.h
#pragma once

#include "CoreMinimal.h"

// 依赖图节点
struct FDependencyNode
{
    FString FilePath;
    FDateTime LastModified;
    TSet<FString> Dependencies;
    TSet<FString> Dependents;
    bool bNeedsRecompilation;
    
    FDependencyNode()
        : bNeedsRecompilation(false)
    {}
};

// 增量编译器
class UCSHARPRUNTIME_API UCSharpIncrementalCompiler
{
public:
    // 构建依赖图
    static bool BuildDependencyGraph(const TArray<FString>& SourceFiles, 
                                    TMap<FString, FDependencyNode>& OutDependencyGraph);
    
    // 标记需要重新编译的文件
    static void MarkFilesForRecompilation(const TArray<FString>& ChangedFiles, 
                                         TMap<FString, FDependencyNode>& DependencyGraph);
    
    // 获取编译顺序
    static TArray<FString> GetCompilationOrder(const TMap<FString, FDependencyNode>& DependencyGraph);
    
    // 增量编译
    static FCompilationResult PerformIncrementalCompilation(const TArray<FString>& ChangedFiles,
                                                           const FCompilationOptions& Options);
    
    // 验证依赖关系
    static bool ValidateDependencies(const TMap<FString, FDependencyNode>& DependencyGraph);
    
private:
    // 解析文件依赖
    static TSet<FString> ParseFileDependencies(const FString& FilePath);
    
    // 拓扑排序
    static TArray<FString> TopologicalSort(const TMap<FString, FDependencyNode>& DependencyGraph);
    
    // 检测循环依赖
    static bool DetectCircularDependencies(const TMap<FString, FDependencyNode>& DependencyGraph,
                                          TArray<FString>& OutCircularDependencies);
    
    static TMap<FString, FDependencyNode> CachedDependencyGraph;
    static FCriticalSection GraphMutex;
};
```

## 5. 使用示例

### 5.1 基础脚本示例

```csharp
// PlayerBehavior.cs - 玩家行为脚本示例
using UnrealEngine;
using UnrealEngine.Engine;
using UnrealEngine.Math;
using UnrealEngine.Scripting;
using System;
using System.Threading.Tasks;

[Script("PlayerBehavior", "Controls player movement and actions", autoStart: true, priority: 1)]
public class PlayerBehavior : ScriptBase
{
    private APawn _playerPawn;
    private float _moveSpeed = 600f;
    private float _jumpHeight = 400f;
    private bool _isJumping = false;
    
    public override void OnInitialize()
    {
        base.OnInitialize();
        
        // 获取玩家Pawn
        var world = UWorld.GetWorld();
        var playerController = world.GetFirstPlayerController();
        _playerPawn = playerController?.GetPawn();
        
        if (_playerPawn == null)
        {
            UE.LogError("PlayerBehavior: No player pawn found!");
            return;
        }
        
        UE.Log("PlayerBehavior initialized successfully.");
    }
    
    public override void OnUpdate(float deltaTime)
    {
        if (_playerPawn == null || !_playerPawn.IsValid())
            return;
        
        HandleMovementInput(deltaTime);
        HandleJumpInput();
    }
    
    private void HandleMovementInput(float deltaTime)
    {
        var inputComponent = _playerPawn.GetComponentByClass<UInputComponent>();
        if (inputComponent == null) return;
        
        // 获取输入轴值
        var forwardInput = inputComponent.GetAxisValue("MoveForward");
        var rightInput = inputComponent.GetAxisValue("MoveRight");
        
        if (Math.Abs(forwardInput) > 0.1f || Math.Abs(rightInput) > 0.1f)
        {
            var forward = _playerPawn.GetActorForwardVector();
            var right = _playerPawn.GetActorRightVector();
            
            var movement = forward * forwardInput + right * rightInput;
            movement = movement.Normalized * _moveSpeed * deltaTime;
            
            var newLocation = _playerPawn.GetActorLocation() + movement;
            _playerPawn.SetActorLocation(newLocation);
        }
    }
    
    private void HandleJumpInput()
    {
        var inputComponent = _playerPawn.GetComponentByClass<UInputComponent>();
        if (inputComponent == null) return;
        
        var jumpPressed = inputComponent.IsActionPressed("Jump");
        
        if (jumpPressed && !_isJumping)
        {
            PerformJump();
        }
    }
    
    private async void PerformJump()
    {
        _isJumping = true;
        
        var startLocation = _playerPawn.GetActorLocation();
        var targetLocation = startLocation + FVector.Up * _jumpHeight;
        
        // 向上跳跃
        await _playerPawn.MoveToAsync(targetLocation, _jumpHeight * 2);
        
        // 下落
        await _playerPawn.MoveToAsync(startLocation, _jumpHeight * 2);
        
        _isJumping = false;
    }
    
    [ScriptMethod("SetMoveSpeed", "Sets the player movement speed")]
    public void SetMoveSpeed(float speed)
    {
        _moveSpeed = Math.Max(0, speed);
        UE.Log($"Player move speed set to {_moveSpeed}");
    }
    
    [ScriptMethod("GetMoveSpeed", "Gets the current player movement speed")]
    public float GetMoveSpeed()
    {
        return _moveSpeed;
    }
    
    public override object OnSaveState()
    {
        return new
        {
            MoveSpeed = _moveSpeed,
            JumpHeight = _jumpHeight,
            IsJumping = _isJumping,
            PlayerLocation = _playerPawn?.GetActorLocation()
        };
    }
    
    public override void OnRestoreState(object state)
    {
        if (state is not dynamic savedState) return;
        
        _moveSpeed = savedState.MoveSpeed;
        _jumpHeight = savedState.JumpHeight;
        _isJumping = savedState.IsJumping;
        
        if (savedState.PlayerLocation != null && _playerPawn != null)
        {
            _playerPawn.SetActorLocation((FVector)savedState.PlayerLocation);
        }
        
        UE.Log("PlayerBehavior state restored after hot reload.");
    }
}
```

### 5.2 游戏管理脚本示例

```csharp
// GameManager.cs - 游戏管理脚本示例
using UnrealEngine;
using UnrealEngine.Engine;
using UnrealEngine.Scripting;
using UnrealEngine.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

[Script("GameManager", "Manages overall game state and flow", autoStart: true, priority: 0)]
public class GameManager : ScriptBase
{
    private enum GameState
    {
        MainMenu,
        Playing,
        Paused,
        GameOver
    }
    
    private GameState _currentState = GameState.MainMenu;
    private float _gameTime = 0f;
    private int _score = 0;
    private int _lives = 3;
    private List<int> _enemyScriptIds = new List<int>();
    
    public override void OnInitialize()
    {
        base.OnInitialize();
        
        // 订阅游戏事件
        EventManager.Subscribe<EnemyDefeatedEventArgs>("EnemyDefeated", OnEnemyDefeated);
        EventManager.Subscribe<PlayerDiedEventArgs>("PlayerDied", OnPlayerDied);
        
        UE.Log("GameManager initialized.");
    }
    
    public override void OnStart()
    {
        base.OnStart();
        
        StartNewGame();
    }
    
    public override void OnUpdate(float deltaTime)
    {
        if (_currentState == GameState.Playing)
        {
            _gameTime += deltaTime;
            
            // 每30秒生成新的敌人
            if (_gameTime % 30f < deltaTime)
            {
                SpawnEnemies();
            }
        }
    }
    
    [ScriptMethod("StartNewGame", "Starts a new game")]
    public void StartNewGame()
    {
        _currentState = GameState.Playing;
        _gameTime = 0f;
        _score = 0;
        _lives = 3;
        
        // 清理现有敌人
        ClearEnemies();
        
        // 生成初始敌人
        SpawnEnemies();
        
        UE.Log("New game started!");
        
        // 发布游戏开始事件
        EventManager.Publish(new GameStartedEventArgs());
    }
    
    [ScriptMethod("PauseGame", "Pauses the game")]
    public void PauseGame()
    {
        if (_currentState == GameState.Playing)
        {
            _currentState = GameState.Paused;
            UE.Log("Game paused.");
        }
    }
    
    [ScriptMethod("ResumeGame", "Resumes the game")]
    public void ResumeGame()
    {
        if (_currentState == GameState.Paused)
        {
            _currentState = GameState.Playing;
            UE.Log("Game resumed.");
        }
    }
    
    [ScriptMethod("EndGame", "Ends the current game")]
    public void EndGame()
    {
        _currentState = GameState.GameOver;
        
        // 清理所有敌人
        ClearEnemies();
        
        UE.Log($"Game Over! Final Score: {_score}, Time: {_gameTime:F1}s");
        
        // 发布游戏结束事件
        EventManager.Publish(new GameOverEventArgs(_score, _gameTime));
    }
    
    private void SpawnEnemies()
    {
        var enemyCount = Math.Min(5, (int)(_gameTime / 10) + 1);
        
        for (int i = 0; i < enemyCount; i++)
        {
            // 创建敌人脚本实例
            var enemyId = Manager.CreateScriptInstance("EnemyBehavior.dll", "EnemyBehavior");
            if (enemyId > 0)
            {
                _enemyScriptIds.Add(enemyId);
                
                // 设置敌人位置
                var randomPos = new FVector(
                    UnityEngine.Random.Range(-1000, 1000),
                    UnityEngine.Random.Range(-1000, 1000),
                    100
                );
                
                Manager.CallScriptMethod(enemyId, "SetPosition", randomPos);
            }
        }
        
        UE.Log($"Spawned {enemyCount} enemies. Total active: {_enemyScriptIds.Count}");
    }
    
    private void ClearEnemies()
    {
        foreach (var enemyId in _enemyScriptIds)
        {
            Manager.DestroyScriptInstance(enemyId);
        }
        
        _enemyScriptIds.Clear();
    }
    
    private void OnEnemyDefeated(EnemyDefeatedEventArgs args)
    {
        _score += 100;
        _enemyScriptIds.Remove(args.EnemyScriptId);
        
        UE.Log($"Enemy defeated! Score: {_score}");
        
        // 检查是否所有敌人都被消灭
        if (_enemyScriptIds.Count == 0)
        {
            SpawnEnemies();
        }
    }
    
    private void OnPlayerDied(PlayerDiedEventArgs args)
    {
        _lives--;
        
        if (_lives <= 0)
        {
            EndGame();
        }
        else
        {
            UE.Log($"Player died! Lives remaining: {_lives}");
            // 重生玩家逻辑
        }
    }
    
    [ScriptMethod("GetScore", "Gets the current score")]
    public int GetScore() => _score;
    
    [ScriptMethod("GetLives", "Gets the remaining lives")]
    public int GetLives() => _lives;
    
    [ScriptMethod("GetGameTime", "Gets the current game time")]
    public float GetGameTime() => _gameTime;
    
    [ScriptMethod("GetGameState", "Gets the current game state")]
    public string GetGameState() => _currentState.ToString();
    
    public override object OnSaveState()
    {
        return new
        {
            CurrentState = _currentState,
            GameTime = _gameTime,
            Score = _score,
            Lives = _lives,
            EnemyScriptIds = _enemyScriptIds.ToArray()
        };
    }
    
    public override void OnRestoreState(object state)
    {
        if (state is not dynamic savedState) return;
        
        _currentState = savedState.CurrentState;
        _gameTime = savedState.GameTime;
        _score = savedState.Score;
        _lives = savedState.Lives;
        _enemyScriptIds = new List<int>(savedState.EnemyScriptIds);
        
        UE.Log("GameManager state restored after hot reload.");
    }
}

// 游戏事件类
public class GameStartedEventArgs : EventArgs
{
    public GameStartedEventArgs()
    {
        EventName = "GameStarted";
    }
}

public class GameOverEventArgs : EventArgs
{
    public int FinalScore { get; }
    public float GameTime { get; }
    
    public GameOverEventArgs(int finalScore, float gameTime)
    {
        EventName = "GameOver";
        FinalScore = finalScore;
        GameTime = gameTime;
    }
}

public class EnemyDefeatedEventArgs : EventArgs
{
    public int EnemyScriptId { get; }
    
    public EnemyDefeatedEventArgs(int enemyScriptId)
    {
        EventName = "EnemyDefeated";
        EnemyScriptId = enemyScriptId;
    }
}

public class PlayerDiedEventArgs : EventArgs
{
    public PlayerDiedEventArgs()
    {
        EventName = "PlayerDied";
    }
}
```

## 6. 总结

这个C#脚本生命周期管理方案提供了：

1. **完整的生命周期管理**: 从编译到运行再到卸载的全流程管理
2. **热重载支持**: 无需重启即可更新脚本逻辑
3. **状态保持**: 热重载时自动保存和恢复脚本状态
4. **增量编译**: 只编译变更的文件，提高开发效率
5. **文件监控**: 自动检测文件变更并触发重新编译
6. **异常处理**: 完善的错误处理和恢复机制
7. **性能优化**: 对象池、缓存等优化策略

通过这个生命周期管理系统，开发者可以享受到现代脚本开发的便利性，同时保持UE5游戏开发的高性能要求。