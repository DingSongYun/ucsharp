# UE5 C#插件 - UObject体系集成设计

## 1. UObject体系集成概述

### 1.1 设计目标

本设计旨在实现C#与UE5 UObject体系的深度集成，使C#能够：

1. **继承UObject体系**：C#类可以继承UObject、AActor、UActorComponent等UE5核心类
2. **支持UFunction机制**：C#方法可以标记为UFunction，支持蓝图调用和网络复制
3. **支持UProperty机制**：C#属性可以标记为UProperty，支持序列化、蓝图访问和编辑器显示
4. **蓝图双向互操作**：蓝图可以继承C#类，调用C#函数，访问C#属性
5. **完整的反射支持**：支持UE5的反射系统，包括类型信息、元数据等

### 1.2 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                    UObject体系集成架构图                        │
├─────────────────────────────────────────────────────────────────┤
│  C# 层 (Managed Layer)                                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  C# UObject     │  │  C# Attributes  │  │  C# Reflection  │ │
│  │  - UObject      │  │  - [UFunction]  │  │  - TypeInfo     │ │
│  │  - AActor       │  │  - [UProperty]  │  │  - MetaData     │ │
│  │  - UComponent   │  │  - [UClass]     │  │  - Serializer   │ │
│  │  - UGameMode    │  │  - [BlueprintCallable] │  - Inspector │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  绑定层 (Binding Layer)                                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  类型注册器     │  │  函数绑定器     │  │  属性绑定器     │ │
│  │  - ClassRegistry│  │  - FunctionBinder│  │  - PropertyBinder│ │
│  │  - TypeMapping  │  │  - CallWrapper  │  │  - GetterSetter │ │
│  │  - Inheritance  │  │  - ParamConvert │  │  - Serialization│ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  C++ 层 (Native Layer)                                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  UE5 UObject    │  │  反射系统       │  │  蓝图系统       │ │
│  │  - UObject      │  │  - UClass       │  │  - Blueprint    │ │
│  │  - AActor       │  │  - UFunction    │  │  - K2Node       │ │
│  │  - UComponent   │  │  - UProperty    │  │  - Compiler     │ │
│  │  - UGameMode    │  │  - UStruct      │  │  - VM           │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 2. C# UObject基类设计

### 2.1 核心基类实现

```csharp
// UnrealEngine.Core.UObject.cs
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace UnrealEngine.Core
{
    /// <summary>
    /// UObject基类 - 所有UE对象的根基类
    /// </summary>
    [UClass]
    public abstract class UObject : IDisposable
    {
        // 原生对象句柄
        protected IntPtr NativePtr { get; private set; }
        
        // 对象标识
        public uint ObjectId { get; private set; }
        
        // 类型信息
        public UClass Class => GetClass();
        
        // 对象名称
        [UProperty(BlueprintReadOnly = true)]
        public string Name
        {
            get => GetName();
            set => SetName(value);
        }
        
        // 外部对象引用
        [UProperty(BlueprintReadOnly = true)]
        public UObject Outer => GetOuter();
        
        // 对象标志
        public EObjectFlags ObjectFlags => GetObjectFlags();
        
        // 构造函数
        protected UObject()
        {
            // 通过反射系统创建原生对象
            NativePtr = CreateNativeObject(GetType());
            ObjectId = RegisterManagedObject(this, NativePtr);
        }
        
        // 从原生对象构造
        internal UObject(IntPtr nativePtr)
        {
            NativePtr = nativePtr;
            ObjectId = RegisterManagedObject(this, NativePtr);
        }
        
        // 析构函数
        ~UObject()
        {
            Dispose(false);
        }
        
        // IDisposable实现
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        
        protected virtual void Dispose(bool disposing)
        {
            if (NativePtr != IntPtr.Zero)
            {
                UnregisterManagedObject(ObjectId);
                
                if (disposing)
                {
                    // 托管资源清理
                    OnDisposing();
                }
                
                // 原生资源清理
                DestroyNativeObject(NativePtr);
                NativePtr = IntPtr.Zero;
            }
        }
        
        // 虚函数 - 子类可重写
        protected virtual void OnDisposing() { }
        
        // UObject核心方法
        [UFunction(BlueprintCallable = true)]
        public virtual void BeginDestroy()
        {
            OnBeginDestroy();
            CallNativeBeginDestroy(NativePtr);
        }
        
        protected virtual void OnBeginDestroy() { }
        
        [UFunction(BlueprintCallable = true)]
        public bool IsValidLowLevel()
        {
            return NativePtr != IntPtr.Zero && CallNativeIsValidLowLevel(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true)]
        public UClass GetClass()
        {
            var classPtr = CallNativeGetClass(NativePtr);
            return UClass.FromNative(classPtr);
        }
        
        [UFunction(BlueprintCallable = true)]
        public string GetName()
        {
            return CallNativeGetName(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true)]
        public void SetName(string newName)
        {
            CallNativeSetName(NativePtr, newName);
        }
        
        [UFunction(BlueprintCallable = true)]
        public UObject GetOuter()
        {
            var outerPtr = CallNativeGetOuter(NativePtr);
            return outerPtr != IntPtr.Zero ? FromNative(outerPtr) : null;
        }
        
        [UFunction(BlueprintCallable = true)]
        public UWorld GetWorld()
        {
            var worldPtr = CallNativeGetWorld(NativePtr);
            return worldPtr != IntPtr.Zero ? UWorld.FromNative(worldPtr) : null;
        }
        
        // 类型转换
        public T Cast<T>() where T : UObject
        {
            if (this is T result)
                return result;
                
            // 尝试原生类型转换
            var targetClassPtr = UClass.GetNativeClass<T>();
            if (CallNativeIsA(NativePtr, targetClassPtr))
            {
                return (T)FromNative(NativePtr);
            }
            
            return null;
        }
        
        // 静态工厂方法
        public static UObject FromNative(IntPtr nativePtr)
        {
            if (nativePtr == IntPtr.Zero)
                return null;
                
            // 检查是否已有托管对象
            if (TryGetManagedObject(nativePtr, out var existingObject))
                return existingObject;
                
            // 根据原生类型创建对应的托管对象
            var classPtr = CallNativeGetClass(nativePtr);
            var managedType = UClass.GetManagedType(classPtr);
            
            if (managedType != null)
            {
                return (UObject)Activator.CreateInstance(managedType, nativePtr);
            }
            
            // 默认返回UObject包装
            return new UObjectWrapper(nativePtr);
        }
        
        // 重写Equals和GetHashCode
        public override bool Equals(object obj)
        {
            if (obj is UObject other)
            {
                return NativePtr == other.NativePtr;
            }
            return false;
        }
        
        public override int GetHashCode()
        {
            return NativePtr.GetHashCode();
        }
        
        public override string ToString()
        {
            return $"{GetType().Name}({Name})";
        }
        
        // 原生方法声明
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateNativeObject(Type managedType);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void DestroyNativeObject(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern uint RegisterManagedObject(UObject managedObject, IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void UnregisterManagedObject(uint objectId);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool TryGetManagedObject(IntPtr nativePtr, out UObject managedObject);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeBeginDestroy(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool CallNativeIsValidLowLevel(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CallNativeGetClass(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern string CallNativeGetName(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetName(IntPtr nativePtr, string name);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CallNativeGetOuter(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CallNativeGetWorld(IntPtr nativePtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool CallNativeIsA(IntPtr nativePtr, IntPtr classPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern EObjectFlags GetObjectFlags();
    }
    
    /// <summary>
    /// UObject包装器 - 用于包装未知类型的原生对象
    /// </summary>
    internal class UObjectWrapper : UObject
    {
        internal UObjectWrapper(IntPtr nativePtr) : base(nativePtr)
        {
        }
    }
    
    /// <summary>
    /// 对象标志枚举
    /// </summary>
    [Flags]
    public enum EObjectFlags : uint
    {
        RF_NoFlags = 0x00000000,
        RF_Public = 0x00000001,
        RF_Standalone = 0x00000002,
        RF_MarkAsNative = 0x00000004,
        RF_Transactional = 0x00000008,
        RF_ClassDefaultObject = 0x00000010,
        RF_ArchetypeObject = 0x00000020,
        RF_Transient = 0x00000040,
        RF_MarkAsRootSet = 0x00000080,
        RF_TagGarbageTemp = 0x00000100,
        RF_NeedInitialization = 0x00000200,
        RF_NeedLoad = 0x00000400,
        RF_KeepForCooker = 0x00000800,
        RF_NeedPostLoad = 0x00001000,
        RF_NeedPostLoadSubobjects = 0x00002000,
        RF_NewerVersionExists = 0x00004000,
        RF_BeginDestroyed = 0x00008000,
        RF_FinishDestroyed = 0x00010000,
        RF_BeingRegenerated = 0x00020000,
        RF_DefaultSubObject = 0x00040000,
        RF_WasLoaded = 0x00080000,
        RF_TextExportTransient = 0x00100000,
        RF_LoadCompleted = 0x00200000,
        RF_InheritableComponentTemplate = 0x00400000,
        RF_DuplicateTransient = 0x00800000,
        RF_StrongRefOnFrame = 0x01000000,
        RF_NonPIEDuplicateTransient = 0x02000000,
        RF_Dynamic = 0x04000000,
        RF_WillBeLoaded = 0x08000000
    }
}
```

### 2.2 AActor基类实现

```csharp
// UnrealEngine.Core.AActor.cs
using System;
using System.Collections.Generic;
using UnrealEngine.Math;

namespace UnrealEngine.Core
{
    /// <summary>
    /// AActor基类 - 可在世界中放置的对象
    /// </summary>
    [UClass(BlueprintType = true, Blueprintable = true)]
    public abstract class AActor : UObject
    {
        // 根组件
        [UProperty(BlueprintReadOnly = true, Category = "Transform")]
        public USceneComponent RootComponent
        {
            get => GetRootComponent();
            set => SetRootComponent(value);
        }
        
        // 变换信息
        [UProperty(BlueprintReadWrite = true, Category = "Transform")]
        public FTransform ActorTransform
        {
            get => GetActorTransform();
            set => SetActorTransform(value);
        }
        
        [UProperty(BlueprintReadWrite = true, Category = "Transform")]
        public FVector ActorLocation
        {
            get => GetActorLocation();
            set => SetActorLocation(value);
        }
        
        [UProperty(BlueprintReadWrite = true, Category = "Transform")]
        public FRotator ActorRotation
        {
            get => GetActorRotation();
            set => SetActorRotation(value);
        }
        
        [UProperty(BlueprintReadWrite = true, Category = "Transform")]
        public FVector ActorScale3D
        {
            get => GetActorScale3D();
            set => SetActorScale3D(value);
        }
        
        // 可见性
        [UProperty(BlueprintReadWrite = true, Category = "Rendering")]
        public bool Hidden
        {
            get => GetHidden();
            set => SetHidden(value);
        }
        
        // 组件列表
        private readonly List<UActorComponent> _components = new List<UActorComponent>();
        
        // 构造函数
        protected AActor() : base()
        {
        }
        
        internal AActor(IntPtr nativePtr) : base(nativePtr)
        {
        }
        
        // 生命周期事件
        [UFunction(BlueprintImplementableEvent = true, Category = "Game")]
        public virtual void BeginPlay()
        {
            OnBeginPlay();
        }
        
        protected virtual void OnBeginPlay() { }
        
        [UFunction(BlueprintImplementableEvent = true, Category = "Game")]
        public virtual void EndPlay(EEndPlayReason EndPlayReason)
        {
            OnEndPlay(EndPlayReason);
        }
        
        protected virtual void OnEndPlay(EEndPlayReason reason) { }
        
        [UFunction(BlueprintImplementableEvent = true, Category = "Game")]
        public virtual void Tick(float DeltaTime)
        {
            OnTick(DeltaTime);
        }
        
        protected virtual void OnTick(float deltaTime) { }
        
        // 组件管理
        [UFunction(BlueprintCallable = true, Category = "Components")]
        public T CreateDefaultSubobject<T>(string name) where T : UActorComponent, new()
        {
            var component = new T();
            component.SetName(name);
            component.SetOwner(this);
            _components.Add(component);
            
            // 调用原生创建
            var nativeComponent = CallNativeCreateDefaultSubobject(NativePtr, typeof(T), name);
            component.SetNativePtr(nativeComponent);
            
            return component;
        }
        
        [UFunction(BlueprintCallable = true, Category = "Components")]
        public T GetComponentByClass<T>() where T : UActorComponent
        {
            foreach (var component in _components)
            {
                if (component is T result)
                    return result;
            }
            
            // 查找原生组件
            var nativeComponent = CallNativeGetComponentByClass(NativePtr, typeof(T));
            if (nativeComponent != IntPtr.Zero)
            {
                return (T)UActorComponent.FromNative(nativeComponent);
            }
            
            return null;
        }
        
        [UFunction(BlueprintCallable = true, Category = "Components")]
        public UActorComponent[] GetComponents()
        {
            return _components.ToArray();
        }
        
        // 变换操作
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public FTransform GetActorTransform()
        {
            return CallNativeGetActorTransform(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public void SetActorTransform(FTransform newTransform)
        {
            CallNativeSetActorTransform(NativePtr, newTransform);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public FVector GetActorLocation()
        {
            return CallNativeGetActorLocation(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public void SetActorLocation(FVector newLocation)
        {
            CallNativeSetActorLocation(NativePtr, newLocation);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public FRotator GetActorRotation()
        {
            return CallNativeGetActorRotation(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public void SetActorRotation(FRotator newRotation)
        {
            CallNativeSetActorRotation(NativePtr, newRotation);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public FVector GetActorScale3D()
        {
            return CallNativeGetActorScale3D(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Transform")]
        public void SetActorScale3D(FVector newScale)
        {
            CallNativeSetActorScale3D(NativePtr, newScale);
        }
        
        // 可见性控制
        [UFunction(BlueprintCallable = true, Category = "Rendering")]
        public bool GetHidden()
        {
            return CallNativeGetHidden(NativePtr);
        }
        
        [UFunction(BlueprintCallable = true, Category = "Rendering")]
        public void SetHidden(bool hidden)
        {
            CallNativeSetHidden(NativePtr, hidden);
        }
        
        // 根组件操作
        [UFunction(BlueprintCallable = true, Category = "Components")]
        public USceneComponent GetRootComponent()
        {
            var rootPtr = CallNativeGetRootComponent(NativePtr);
            return rootPtr != IntPtr.Zero ? (USceneComponent)UActorComponent.FromNative(rootPtr) : null;
        }
        
        [UFunction(BlueprintCallable = true, Category = "Components")]
        public void SetRootComponent(USceneComponent newRoot)
        {
            CallNativeSetRootComponent(NativePtr, newRoot?.NativePtr ?? IntPtr.Zero);
        }
        
        // 销毁
        [UFunction(BlueprintCallable = true, Category = "Game")]
        public void Destroy()
        {
            CallNativeDestroy(NativePtr);
        }
        
        // 原生方法声明
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CallNativeCreateDefaultSubobject(IntPtr actorPtr, Type componentType, string name);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CallNativeGetComponentByClass(IntPtr actorPtr, Type componentType);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern FTransform CallNativeGetActorTransform(IntPtr actorPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetActorTransform(IntPtr actorPtr, FTransform transform);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern FVector CallNativeGetActorLocation(IntPtr actorPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetActorLocation(IntPtr actorPtr, FVector location);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern FRotator CallNativeGetActorRotation(IntPtr actorPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetActorRotation(IntPtr actorPtr, FRotator rotation);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern FVector CallNativeGetActorScale3D(IntPtr actorPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetActorScale3D(IntPtr actorPtr, FVector scale);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool CallNativeGetHidden(IntPtr actorPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetHidden(IntPtr actorPtr, bool hidden);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CallNativeGetRootComponent(IntPtr actorPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeSetRootComponent(IntPtr actorPtr, IntPtr componentPtr);
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CallNativeDestroy(IntPtr actorPtr);
    }
    
    /// <summary>
    /// 结束游戏原因枚举
    /// </summary>
    public enum EEndPlayReason
    {
        Destroyed,
        LevelTransition,
        EndPlayInEditor,
        RemovedFromWorld,
        Quit
    }
}
```

## 3. UFunction机制实现

### 3.1 UFunction特性定义

```csharp
// UnrealEngine.Attributes.UFunctionAttribute.cs
using System;

namespace UnrealEngine.Attributes
{
    /// <summary>
    /// UFunction特性 - 标记方法为UE5函数
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public class UFunctionAttribute : Attribute
    {
        /// <summary>
        /// 是否可在蓝图中调用
        /// </summary>
        public bool BlueprintCallable { get; set; } = false;
        
        /// <summary>
        /// 是否为蓝图实现事件
        /// </summary>
        public bool BlueprintImplementableEvent { get; set; } = false;
        
        /// <summary>
        /// 是否为蓝图原生事件
        /// </summary>
        public bool BlueprintNativeEvent { get; set; } = false;
        
        /// <summary>
        /// 是否为纯函数（无副作用）
        /// </summary>
        public bool BlueprintPure { get; set; } = false;
        
        /// <summary>
        /// 是否支持网络复制
        /// </summary>
        public bool Reliable { get; set; } = false;
        
        /// <summary>
        /// 是否为服务器RPC
        /// </summary>
        public bool Server { get; set; } = false;
        
        /// <summary>
        /// 是否为客户端RPC
        /// </summary>
        public bool Client { get; set; } = false;
        
        /// <summary>
        /// 是否为多播RPC
        /// </summary>
        public bool NetMulticast { get; set; } = false;
        
        /// <summary>
        /// 函数分类
        /// </summary>
        public string Category { get; set; } = "";
        
        /// <summary>
        /// 函数显示名称
        /// </summary>
        public string DisplayName { get; set; } = "";
        
        /// <summary>
        /// 函数工具提示
        /// </summary>
        public string ToolTip { get; set; } = "";
        
        /// <summary>
        /// 关键字
        /// </summary>
        public string Keywords { get; set; } = "";
        
        /// <summary>
        /// 是否在编辑器中调用
        /// </summary>
        public bool CallInEditor { get; set; } = false;
        
        /// <summary>
        /// 执行权限
        /// </summary>
        public EFunctionFlags FunctionFlags { get; set; } = EFunctionFlags.FUNC_None;
    }
    
    /// <summary>
    /// 函数标志枚举
    /// </summary>
    [Flags]
    public enum EFunctionFlags : uint
    {
        FUNC_None = 0x00000000,
        FUNC_Final = 0x00000001,
        FUNC_RequiredAPI = 0x00000002,
        FUNC_BlueprintAuthorityOnly = 0x00000004,
        FUNC_BlueprintCosmetic = 0x00000008,
        FUNC_Net = 0x00000040,
        FUNC_NetReliable = 0x00000080,
        FUNC_NetRequest = 0x00000100,
        FUNC_Exec = 0x00000200,
        FUNC_Native = 0x00000400,
        FUNC_Event = 0x00000800,
        FUNC_NetResponse = 0x00001000,
        FUNC_Static = 0x00002000,
        FUNC_NetMulticast = 0x00004000,
        FUNC_UbergraphFunction = 0x00008000,
        FUNC_MulticastDelegate = 0x00010000,
        FUNC_Public = 0x00020000,
        FUNC_Private = 0x00040000,
        FUNC_Protected = 0x00080000,
        FUNC_Delegate = 0x00100000,
        FUNC_NetServer = 0x00200000,
        FUNC_HasOutParms = 0x00400000,
        FUNC_HasDefaults = 0x00800000,
        FUNC_NetClient = 0x01000000,
        FUNC_DLLImport = 0x02000000,
        FUNC_BlueprintCallable = 0x04000000,
        FUNC_BlueprintEvent = 0x08000000,
        FUNC_BlueprintPure = 0x10000000,
        FUNC_EditorOnly = 0x20000000,
        FUNC_Const = 0x40000000,
        FUNC_NetValidate = 0x80000000
    }
}
```

### 3.2 函数绑定器实现

```csharp
// UnrealEngine.Binding.FunctionBinder.cs
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using UnrealEngine.Attributes;

namespace UnrealEngine.Binding
{
    /// <summary>
    /// 函数绑定信息
    /// </summary>
    public class FunctionBindingInfo
    {
        public MethodInfo Method { get; set; }
        public UFunctionAttribute Attribute { get; set; }
        public Type DeclaringType { get; set; }
        public string FunctionName { get; set; }
        public IntPtr NativeFunctionPtr { get; set; }
        public Delegate ManagedDelegate { get; set; }
        public ParameterInfo[] Parameters { get; set; }
        public Type ReturnType { get; set; }
    }
    
    /// <summary>
    /// 函数绑定器
    /// </summary>
    public static class FunctionBinder
    {
        private static readonly Dictionary<string, FunctionBindingInfo> _functionBindings = new Dictionary<string, FunctionBindingInfo>();
        private static readonly Dictionary<IntPtr, FunctionBindingInfo> _nativeFunctionMap = new Dictionary<IntPtr, FunctionBindingInfo>();
        
        /// <summary>
        /// 注册类型的所有UFunction
        /// </summary>
        public static void RegisterTypeFunctions(Type type)
        {
            var methods = type.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            
            foreach (var method in methods)
            {
                var ufunctionAttr = method.GetCustomAttribute<UFunctionAttribute>();
                if (ufunctionAttr != null)
                {
                    RegisterFunction(method, ufunctionAttr);
                }
            }
        }
        
        /// <summary>
        /// 注册单个函数
        /// </summary>
        public static void RegisterFunction(MethodInfo method, UFunctionAttribute attribute)
        {
            var functionName = $"{method.DeclaringType.Name}::{method.Name}";
            
            var bindingInfo = new FunctionBindingInfo
            {
                Method = method,
                Attribute = attribute,
                DeclaringType = method.DeclaringType,
                FunctionName = functionName,
                Parameters = method.GetParameters(),
                ReturnType = method.ReturnType
            }
            
            // 创建托管委托
            bindingInfo.ManagedDelegate = CreateManagedDelegate(method);
            
            // 注册到原生系统
            bindingInfo.NativeFunctionPtr = RegisterNativeFunction(
                method.DeclaringType,
                method.Name,
                bindingInfo.ManagedDelegate,
                attribute
            );
            
            _functionBindings[functionName] = bindingInfo;
            _nativeFunctionMap[bindingInfo.NativeFunctionPtr] = bindingInfo;
            
            UE.LogInfo($"Registered UFunction: {functionName}");
        }
        
        /// <summary>
        /// 调用C#函数（从原生代码调用）
        /// </summary>
        public static object InvokeFunction(IntPtr nativeFunctionPtr, UObject target, object[] parameters)
        {
            if (!_nativeFunctionMap.TryGetValue(nativeFunctionPtr, out var bindingInfo))
            {
                throw new InvalidOperationException($"Function not found for native pointer: {nativeFunctionPtr}");
            }
            
            try
            {
                // 转换参数类型
                var convertedParams = ConvertParameters(parameters, bindingInfo.Parameters);
                
                // 调用方法
                var result = bindingInfo.Method.Invoke(target, convertedParams);
                
                // 处理输出参数
                HandleOutParameters(parameters, convertedParams, bindingInfo.Parameters);
                
                return result;
            }
            catch (Exception ex)
            {
                UE.LogError($"Error invoking function {bindingInfo.FunctionName}: {ex.Message}");
                throw;
            }
        }
        
        /// <summary>
        /// 调用原生函数（从C#代码调用）
        /// </summary>
        public static object CallNativeFunction(UObject target, string functionName, params object[] parameters)
        {
            if (!_functionBindings.TryGetValue(functionName, out var bindingInfo))
            {
                throw new InvalidOperationException($"Function not found: {functionName}");
            }
            
            // 调用原生函数
            return CallNativeFunctionInternal(
                target.NativePtr,
                bindingInfo.NativeFunctionPtr,
                parameters
            );
        }
        
        /// <summary>
        /// 获取函数绑定信息
        /// </summary>
        public static FunctionBindingInfo GetFunctionBinding(string functionName)
        {
            _functionBindings.TryGetValue(functionName, out var binding);
            return binding;
        }
        
        /// <summary>
        /// 检查函数是否已注册
        /// </summary>
        public static bool IsFunctionRegistered(string functionName)
        {
            return _functionBindings.ContainsKey(functionName);
        }
        
        private static Delegate CreateManagedDelegate(MethodInfo method)
        {
            // 根据方法签名创建对应的委托类型
            var paramTypes = new List<Type>();
            
            // 添加目标对象类型（如果不是静态方法）
            if (!method.IsStatic)
            {
                paramTypes.Add(typeof(IntPtr)); // UObject*
            }
            
            // 添加参数类型
            foreach (var param in method.GetParameters())
            {
                paramTypes.Add(param.ParameterType);
            }
            
            // 添加返回类型
            paramTypes.Add(method.ReturnType);
            
            // 创建委托类型
            var delegateType = Expression.GetDelegateType(paramTypes.ToArray());
            
            // 创建委托实例
            if (method.IsStatic)
            {
                return Delegate.CreateDelegate(delegateType, method);
            }
            else
            {
                // 为实例方法创建包装委托
                return CreateInstanceMethodDelegate(method, delegateType);
            }
        }
        
        private static Delegate CreateInstanceMethodDelegate(MethodInfo method, Type delegateType)
        {
            // 创建一个包装方法，将IntPtr转换为实际对象
            var wrapperMethod = new DynamicMethod(
                $"Wrapper_{method.Name}",
                method.ReturnType,
                GetParameterTypes(method, true),
                method.DeclaringType,
                true
            );
            
            var il = wrapperMethod.GetILGenerator();
            
            // 加载目标对象（从IntPtr转换）
            il.Emit(OpCodes.Ldarg_0); // IntPtr
            il.Emit(OpCodes.Call, typeof(UObject).GetMethod("FromNative"));
            il.Emit(OpCodes.Castclass, method.DeclaringType);
            
            // 加载其他参数
            for (int i = 1; i <= method.GetParameters().Length; i++)
            {
                il.Emit(OpCodes.Ldarg, i);
            }
            
            // 调用原始方法
            il.Emit(OpCodes.Callvirt, method);
            
            // 返回
            il.Emit(OpCodes.Ret);
            
            return wrapperMethod.CreateDelegate(delegateType);
        }
        
        private static Type[] GetParameterTypes(MethodInfo method, bool includeTarget)
        {
            var paramTypes = new List<Type>();
            
            if (includeTarget && !method.IsStatic)
            {
                paramTypes.Add(typeof(IntPtr));
            }
            
            foreach (var param in method.GetParameters())
            {
                paramTypes.Add(param.ParameterType);
            }
            
            return paramTypes.ToArray();
        }
        
        private static object[] ConvertParameters(object[] sourceParams, ParameterInfo[] targetParams)
        {
            if (sourceParams == null || sourceParams.Length == 0)
                return new object[0];
                
            var convertedParams = new object[targetParams.Length];
            
            for (int i = 0; i < Math.Min(sourceParams.Length, targetParams.Length); i++)
            {
                convertedParams[i] = ConvertParameter(sourceParams[i], targetParams[i].ParameterType);
            }
            
            return convertedParams;
        }
        
        private static object ConvertParameter(object value, Type targetType)
        {
            if (value == null)
                return null;
                
            var sourceType = value.GetType();
            
            // 如果类型匹配，直接返回
            if (targetType.IsAssignableFrom(sourceType))
                return value;
                
            // UObject类型转换
            if (typeof(UObject).IsAssignableFrom(targetType) && value is IntPtr ptr)
            {
                return UObject.FromNative(ptr);
            }
            
            // 基础类型转换
            if (targetType.IsPrimitive || targetType == typeof(string))
            {
                return Convert.ChangeType(value, targetType);
            }
            
            // 结构体类型转换
            if (targetType.IsValueType)
            {
                return ConvertStruct(value, targetType);
            }
            
            return value;
        }
        
        private static object ConvertStruct(object value, Type targetType)
        {
            // 实现结构体转换逻辑
            // 这里需要根据具体的结构体类型进行转换
            return value;
        }
        
        private static void HandleOutParameters(object[] originalParams, object[] convertedParams, ParameterInfo[] paramInfo)
        {
            for (int i = 0; i < paramInfo.Length; i++)
            {
                if (paramInfo[i].IsOut || paramInfo[i].ParameterType.IsByRef)
                {
                    originalParams[i] = convertedParams[i];
                }
            }
        }
        
        // 原生方法声明
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr RegisterNativeFunction(
            Type declaringType,
            string functionName,
            Delegate managedDelegate,
            UFunctionAttribute attribute
        );
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern object CallNativeFunctionInternal(
            IntPtr targetPtr,
            IntPtr functionPtr,
            object[] parameters
        );
    }
}
```

## 4. UProperty机制实现

### 4.1 UProperty特性定义

```csharp
// UnrealEngine.Attributes.UPropertyAttribute.cs
using System;

namespace UnrealEngine.Attributes
{
    /// <summary>
    /// UProperty特性 - 标记属性为UE5属性
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false)]
    public class UPropertyAttribute : Attribute
    {
        /// <summary>
        /// 是否在蓝图中可读
        /// </summary>
        public bool BlueprintReadOnly { get; set; } = false;
        
        /// <summary>
        /// 是否在蓝图中可写
        /// </summary>
        public bool BlueprintReadWrite { get; set; } = false;
        
        /// <summary>
        /// 是否在编辑器中可见
        /// </summary>
        public bool VisibleAnywhere { get; set; } = false;
        
        /// <summary>
        /// 是否在编辑器中可编辑
        /// </summary>
        public bool EditAnywhere { get; set; } = false;
        
        /// <summary>
        /// 是否仅在默认值中可编辑
        /// </summary>
        public bool EditDefaultsOnly { get; set; } = false;
        
        /// <summary>
        /// 是否仅在实例中可编辑
        /// </summary>
        public bool EditInstanceOnly { get; set; } = false;
        
        /// <summary>
        /// 属性分类
        /// </summary>
        public string Category { get; set; } = "";
        
        /// <summary>
        /// 属性显示名称
        /// </summary>
        public string DisplayName { get; set; } = "";
        
        /// <summary>
        /// 属性工具提示
        /// </summary>
        public string ToolTip { get; set; } = "";
        
        /// <summary>
        /// 是否支持网络复制
        /// </summary>
        public bool Replicated { get; set; } = false;
        
        /// <summary>
        /// 复制条件
        /// </summary>
        public ELifetimeCondition ReplicatedUsing { get; set; } = ELifetimeCondition.COND_None;
        
        /// <summary>
        /// 是否保存到配置文件
        /// </summary>
        public bool Config { get; set; } = false;
        
        /// <summary>
        /// 是否为瞬态属性（不保存）
        /// </summary>
        public bool Transient { get; set; } = false;
        
        /// <summary>
        /// 是否为本地化属性
        /// </summary>
        public bool Localized { get; set; } = false;
        
        /// <summary>
        /// 属性元数据
        /// </summary>
        public string Meta { get; set; } = "";
        
        /// <summary>
        /// 属性标志
        /// </summary>
        public EPropertyFlags PropertyFlags { get; set; } = EPropertyFlags.CPF_None;
    }
    
    /// <summary>
    /// 生命周期条件枚举
    /// </summary>
    public enum ELifetimeCondition
    {
        COND_None,
        COND_InitialOnly,
        COND_OwnerOnly,
        COND_SkipOwner,
        COND_SimulatedOnly,
        COND_AutonomousOnly,
        COND_SimulatedOrPhysics,
        COND_InitialOrOwner,
        COND_Custom,
        COND_ReplayOrOwner,
        COND_ReplayOnly,
        COND_SimulatedOnlyNoReplay,
        COND_SimulatedOrPhysicsNoReplay,
        COND_SkipReplay
    }
    
    /// <summary>
    /// 属性标志枚举
    /// </summary>
    [Flags]
    public enum EPropertyFlags : ulong
    {
        CPF_None = 0,
        CPF_Edit = 0x0000000000000001,
        CPF_ConstParm = 0x0000000000000002,
        CPF_BlueprintVisible = 0x0000000000000004,
        CPF_ExportObject = 0x0000000000000008,
        CPF_BlueprintReadOnly = 0x0000000000000010,
        CPF_Mass = 0x0000000000000020,
        CPF_DuplicateTransient = 0x0000000000000040,
        CPF_SubobjectReference = 0x0000000000000080,
        CPF_SaveGame = 0x0000000000000100,
        CPF_NoClear = 0x0000000000000200,
        CPF_ReferenceParm = 0x0000000000000400,
        CPF_BlueprintAssignable = 0x0000000000000800,
        CPF_Deprecated = 0x0000000000001000,
        CPF_IsPlainOldData = 0x0000000000002000,
        CPF_RepSkip = 0x0000000000004000,
        CPF_RepNotify = 0x0000000000008000,
        CPF_Interp = 0x0000000000010000,
        CPF_NonTransactional = 0x0000000000020000,
        CPF_EditorOnly = 0x0000000000040000,
        CPF_NoDestructor = 0x0000000000080000,
        CPF_AutoWeak = 0x0000000000100000,
        CPF_ContainsInstancedReference = 0x0000000000200000,
        CPF_AssetRegistrySearchable = 0x0000000000400000,
        CPF_SimpleDisplay = 0x0000000000800000,
        CPF_AdvancedDisplay = 0x0000000001000000,
        CPF_Protected = 0x0000000002000000,
        CPF_BlueprintCallable = 0x0000000004000000,
        CPF_BlueprintAuthorityOnly = 0x0000000008000000,
        CPF_TextExportTransient = 0x0000000010000000,
        CPF_NonPIEDuplicateTransient = 0x0000000020000000,
        CPF_ExposeOnSpawn = 0x0000000040000000,
        CPF_PersistentInstance = 0x0000000080000000,
        CPF_UObjectWrapper = 0x0000000100000000,
        CPF_HasGetValueTypeHash = 0x0000000200000000,
        CPF_NativeAccessSpecifierPublic = 0x0000000400000000,
        CPF_NativeAccessSpecifierProtected = 0x0000000800000000,
        CPF_NativeAccessSpecifierPrivate = 0x0000001000000000,
        CPF_SkipSerialization = 0x0000002000000000
    }
}
```

### 4.2 属性绑定器实现

```csharp
// UnrealEngine.Binding.PropertyBinder.cs
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using UnrealEngine.Attributes;

namespace UnrealEngine.Binding
{
    /// <summary>
    /// 属性绑定信息
    /// </summary>
    public class PropertyBindingInfo
    {
        public PropertyInfo Property { get; set; }
        public FieldInfo Field { get; set; }
        public UPropertyAttribute Attribute { get; set; }
        public Type DeclaringType { get; set; }
        public string PropertyName { get; set; }
        public Type PropertyType { get; set; }
        public IntPtr NativePropertyPtr { get; set; }
        public Func<object, object> Getter { get; set; }
        public Action<object, object> Setter { get; set; }
        public bool IsReadOnly { get; set; }
    }
    
    /// <summary>
    /// 属性绑定器
    /// </summary>
    public static class PropertyBinder
    {
        private static readonly Dictionary<string, PropertyBindingInfo> _propertyBindings = new Dictionary<string, PropertyBindingInfo>();
        private static readonly Dictionary<IntPtr, PropertyBindingInfo> _nativePropertyMap = new Dictionary<IntPtr, PropertyBindingInfo>();
        
        /// <summary>
        /// 注册类型的所有UProperty
        /// </summary>
        public static void RegisterTypeProperties(Type type)
        {
            // 注册属性
            var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            foreach (var property in properties)
            {
                var upropertyAttr = property.GetCustomAttribute<UPropertyAttribute>();
                if (upropertyAttr != null)
                {
                    RegisterProperty(property, upropertyAttr);
                }
            }
            
            // 注册字段
            var fields = type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            foreach (var field in fields)
            {
                var upropertyAttr = field.GetCustomAttribute<UPropertyAttribute>();
                if (upropertyAttr != null)
                {
                    RegisterField(field, upropertyAttr);
                }
            }
        }
        
        /// <summary>
        /// 注册属性
        /// </summary>
        public static void RegisterProperty(PropertyInfo property, UPropertyAttribute attribute)
        {
            var propertyName = $"{property.DeclaringType.Name}::{property.Name}";
            
            var bindingInfo = new PropertyBindingInfo
            {
                Property = property,
                Attribute = attribute,
                DeclaringType = property.DeclaringType,
                PropertyName = propertyName,
                PropertyType = property.PropertyType,
                IsReadOnly = !property.CanWrite || attribute.BlueprintReadOnly
            };
            
            // 创建访问器
            bindingInfo.Getter = CreatePropertyGetter(property);
            if (property.CanWrite && !attribute.BlueprintReadOnly)
            {
                bindingInfo.Setter = CreatePropertySetter(property);
            }
            
            // 注册到原生系统
            bindingInfo.NativePropertyPtr = RegisterNativeProperty(
                property.DeclaringType,
                property.Name,
                property.PropertyType,
                attribute
            );
            
            _propertyBindings[propertyName] = bindingInfo;
            _nativePropertyMap[bindingInfo.NativePropertyPtr] = bindingInfo;
            
            UE.LogInfo($"Registered UProperty: {propertyName}");
        }
        
        /// <summary>
        /// 注册字段
        /// </summary>
        public static void RegisterField(FieldInfo field, UPropertyAttribute attribute)
        {
            var propertyName = $"{field.DeclaringType.Name}::{field.Name}";
            
            var bindingInfo = new PropertyBindingInfo
            {
                Field = field,
                Attribute = attribute,
                DeclaringType = field.DeclaringType,
                PropertyName = propertyName,
                PropertyType = field.FieldType,
                IsReadOnly = field.IsInitOnly || attribute.BlueprintReadOnly
            };
            
            // 创建访问器
            bindingInfo.Getter = CreateFieldGetter(field);
            if (!field.IsInitOnly && !attribute.BlueprintReadOnly)
            {
                bindingInfo.Setter = CreateFieldSetter(field);
            }
            
            // 注册到原生系统
            bindingInfo.NativePropertyPtr = RegisterNativeProperty(
                field.DeclaringType,
                field.Name,
                field.FieldType,
                attribute
            );
            
            _propertyBindings[propertyName] = bindingInfo;
            _nativePropertyMap[bindingInfo.NativePropertyPtr] = bindingInfo;
            
            UE.LogInfo($"Registered UProperty (Field): {propertyName}");
        }
        
        /// <summary>
        /// 获取属性值（从原生代码调用）
        /// </summary>
        public static object GetPropertyValue(IntPtr nativePropertyPtr, UObject target)
        {
            if (!_nativePropertyMap.TryGetValue(nativePropertyPtr, out var bindingInfo))
            {
                throw new InvalidOperationException($"Property not found for native pointer: {nativePropertyPtr}");
            }
            
            try
            {
                return bindingInfo.Getter(target);
            }
            catch (Exception ex)
            {
                UE.LogError($"Error getting property {bindingInfo.PropertyName}: {ex.Message}");
                throw;
            }
        }
        
        /// <summary>
        /// 设置属性值（从原生代码调用）
        /// </summary>
        public static void SetPropertyValue(IntPtr nativePropertyPtr, UObject target, object value)
        {
            if (!_nativePropertyMap.TryGetValue(nativePropertyPtr, out var bindingInfo))
            {
                throw new InvalidOperationException($"Property not found for native pointer: {nativePropertyPtr}");
            }
            
            if (bindingInfo.IsReadOnly)
            {
                throw new InvalidOperationException($"Property {bindingInfo.PropertyName} is read-only");
            }
            
            if (bindingInfo.Setter == null)
            {
                throw new InvalidOperationException($"Property {bindingInfo.PropertyName} has no setter");
            }
            
            try
            {
                var convertedValue = ConvertPropertyValue(value, bindingInfo.PropertyType);
                bindingInfo.Setter(target, convertedValue);
            }
            catch (Exception ex)
            {
                UE.LogError($"Error setting property {bindingInfo.PropertyName}: {ex.Message}");
                throw;
            }
        }
        
        /// <summary>
        /// 获取属性值（从C#代码调用）
        /// </summary>
        public static object GetPropertyValue(UObject target, string propertyName)
        {
            var fullPropertyName = $"{target.GetType().Name}::{propertyName}";
            
            if (!_propertyBindings.TryGetValue(fullPropertyName, out var bindingInfo))
            {
                throw new InvalidOperationException($"Property not found: {fullPropertyName}");
            }
            
            return bindingInfo.Getter(target);
        }
        
        /// <summary>
        /// 设置属性值（从C#代码调用）
        /// </summary>
        public static void SetPropertyValue(UObject target, string propertyName, object value)
        {
            var fullPropertyName = $"{target.GetType().Name}::{propertyName}";
            
            if (!_propertyBindings.TryGetValue(fullPropertyName, out var bindingInfo))
            {
                throw new InvalidOperationException($"Property not found: {fullPropertyName}");
            }
            
            if (bindingInfo.IsReadOnly)
            {
                throw new InvalidOperationException($"Property {fullPropertyName} is read-only");
            }
            
            var convertedValue = ConvertPropertyValue(value, bindingInfo.PropertyType);
            bindingInfo.Setter(target, convertedValue);
        }
        
        /// <summary>
        /// 获取属性绑定信息
        /// </summary>
        public static PropertyBindingInfo GetPropertyBinding(string propertyName)
        {
            _propertyBindings.TryGetValue(propertyName, out var binding);
            return binding;
        }
        
        /// <summary>
        /// 检查属性是否已注册
        /// </summary>
        public static bool IsPropertyRegistered(string propertyName)
        {
            return _propertyBindings.ContainsKey(propertyName);
        }
        
        private static Func<object, object> CreatePropertyGetter(PropertyInfo property)
        {
            if (!property.CanRead)
                return null;
                
            return (target) => property.GetValue(target);
        }
        
        private static Action<object, object> CreatePropertySetter(PropertyInfo property)
        {
            if (!property.CanWrite)
                return null;
                
            return (target, value) => property.SetValue(target, value);
        }
        
        private static Func<object, object> CreateFieldGetter(FieldInfo field)
        {
            return (target) => field.GetValue(target);
        }
        
        private static Action<object, object> CreateFieldSetter(FieldInfo field)
        {
            if (field.IsInitOnly)
                return null;
                
            return (target, value) => field.SetValue(target, value);
        }
        
        private static object ConvertPropertyValue(object value, Type targetType)
        {
            if (value == null)
                return null;
                
            var sourceType = value.GetType();
            
            // 如果类型匹配，直接返回
            if (targetType.IsAssignableFrom(sourceType))
                return value;
                
            // UObject类型转换
            if (typeof(UObject).IsAssignableFrom(targetType) && value is IntPtr ptr)
            {
                return UObject.FromNative(ptr);
            }
            
            // 基础类型转换
            if (targetType.IsPrimitive || targetType == typeof(string))
            {
                return Convert.ChangeType(value, targetType);
            }
            
            // 枚举类型转换
            if (targetType.IsEnum)
            {
                return Enum.ToObject(targetType, value);
            }
            
            // 结构体类型转换
            if (targetType.IsValueType)
            {
                return ConvertStruct(value, targetType);
            }
            
            return value;
        }
        
        private static object ConvertStruct(object value, Type targetType)
        {
            // 实现结构体转换逻辑
            // 这里需要根据具体的结构体类型进行转换
            return value;
        }
        
        // 原生方法声明
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr RegisterNativeProperty(
            Type declaringType,
            string propertyName,
            Type propertyType,
            UPropertyAttribute attribute
        );
    }
}
```

## 5. 蓝图集成系统

### 5.1 蓝图类注册器

```csharp
// UnrealEngine.Blueprint.BlueprintClassRegistry.cs
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using UnrealEngine.Attributes;
using UnrealEngine.Binding;

namespace UnrealEngine.Blueprint
{
    /// <summary>
    /// 蓝图类信息
    /// </summary>
    public class BlueprintClassInfo
    {
        public Type ManagedType { get; set; }
        public UClassAttribute ClassAttribute { get; set; }
        public IntPtr NativeClassPtr { get; set; }
        public string ClassName { get; set; }
        public Type ParentType { get; set; }
        public List<FunctionBindingInfo> Functions { get; set; } = new List<FunctionBindingInfo>();
        public List<PropertyBindingInfo> Properties { get; set; } = new List<PropertyBindingInfo>();
        public bool IsBlueprintable { get; set; }
        public bool IsBlueprintType { get; set; }
    }
    
    /// <summary>
    /// 蓝图类注册器
    /// </summary>
    public static class BlueprintClassRegistry
    {
        private static readonly Dictionary<Type, BlueprintClassInfo> _managedToBlueprint = new Dictionary<Type, BlueprintClassInfo>();
        private static readonly Dictionary<IntPtr, BlueprintClassInfo> _nativeToBlueprint = new Dictionary<IntPtr, BlueprintClassInfo>();
        private static readonly Dictionary<string, BlueprintClassInfo> _nameToBlueprint = new Dictionary<string, BlueprintClassInfo>();
        
        /// <summary>
        /// 注册C#类为蓝图类
        /// </summary>
        public static void RegisterBlueprintClass(Type type)
        {
            var classAttr = type.GetCustomAttribute<UClassAttribute>();
            if (classAttr == null)
            {
                throw new ArgumentException($"Type {type.Name} must have UClassAttribute to be registered as blueprint class");
            }
            
            var classInfo = new BlueprintClassInfo
            {
                ManagedType = type,
                ClassAttribute = classAttr,
                ClassName = type.Name,
                ParentType = type.BaseType,
                IsBlueprintable = classAttr.Blueprintable,
                IsBlueprintType = classAttr.BlueprintType
            };
            
            // 注册函数
            FunctionBinder.RegisterTypeFunctions(type);
            
            // 注册属性
            PropertyBinder.RegisterTypeProperties(type);
            
            // 收集函数和属性信息
            CollectClassMembers(classInfo);
            
            // 注册到原生系统
            classInfo.NativeClassPtr = RegisterNativeBlueprintClass(
                type,
                classAttr,
                classInfo.Functions.ToArray(),
                classInfo.Properties.ToArray()
            );
            
            // 添加到映射表
            _managedToBlueprint[type] = classInfo;
            _nativeToBlueprint[classInfo.NativeClassPtr] = classInfo;
            _nameToBlueprint[classInfo.ClassName] = classInfo;
            
            UE.LogInfo($"Registered Blueprint Class: {classInfo.ClassName}");
        }
        
        /// <summary>
        /// 获取蓝图类信息
        /// </summary>
        public static BlueprintClassInfo GetBlueprintClass(Type type)
        {
            _managedToBlueprint.TryGetValue(type, out var classInfo);
            return classInfo;
        }
        
        /// <summary>
        /// 获取蓝图类信息（通过原生指针）
        /// </summary>
        public static BlueprintClassInfo GetBlueprintClass(IntPtr nativeClassPtr)
        {
            _nativeToBlueprint.TryGetValue(nativeClassPtr, out var classInfo);
            return classInfo;
        }
        
        /// <summary>
        /// 获取蓝图类信息（通过类名）
        /// </summary>
        public static BlueprintClassInfo GetBlueprintClass(string className)
        {
            _nameToBlueprint.TryGetValue(className, out var classInfo);
            return classInfo;
        }
        
        /// <summary>
        /// 检查类型是否为蓝图类
        /// </summary>
        public static bool IsBlueprintClass(Type type)
        {
            return _managedToBlueprint.ContainsKey(type);
        }
        
        /// <summary>
        /// 创建蓝图实例
        /// </summary>
        public static UObject CreateBlueprintInstance(Type type, UObject outer = null)
        {
            if (!_managedToBlueprint.TryGetValue(type, out var classInfo))
            {
                throw new ArgumentException($"Type {type.Name} is not registered as blueprint class");
            }
            
            // 调用原生创建函数
            var nativeInstance = CallNativeCreateBlueprintInstance(
                classInfo.NativeClassPtr,
                outer?.NativePtr ?? IntPtr.Zero
            );
            
            // 创建托管包装
            return UObject.FromNative(nativeInstance);
        }
        
        private static void CollectClassMembers(BlueprintClassInfo classInfo)
        {
            var type = classInfo.ManagedType;
            
            // 收集函数
            var methods = type.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            foreach (var method in methods)
            {
                var ufunctionAttr = method.GetCustomAttribute<UFunctionAttribute>();
                if (ufunctionAttr != null)
                {
                    var functionBinding = FunctionBinder.GetFunctionBinding($"{type.Name}::{method.Name}");
                    if (functionBinding != null)
                    {
                        classInfo.Functions.Add(functionBinding);
                    }
                }
            }
            
            // 收集属性
            var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            foreach (var property in properties)
            {
                var upropertyAttr = property.GetCustomAttribute<UPropertyAttribute>();
                if (upropertyAttr != null)
                {
                    var propertyBinding = PropertyBinder.GetPropertyBinding($"{type.Name}::{property.Name}");
                    if (propertyBinding != null)
                    {
                        classInfo.Properties.Add(propertyBinding);
                    }
                }
            }
            
            // 收集字段
            var fields = type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            foreach (var field in fields)
            {
                var upropertyAttr = field.GetCustomAttribute<UPropertyAttribute>();
                if (upropertyAttr != null)
                {
                    var propertyBinding = PropertyBinder.GetPropertyBinding($"{type.Name}::{field.Name}");
                    if (propertyBinding != null)
                    {
                        classInfo.Properties.Add(propertyBinding);
                    }
                }
            }
        }
        
        // 原生方法声明
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr RegisterNativeBlueprintClass(
            Type managedType,
            UClassAttribute classAttribute,
            FunctionBindingInfo[] functions,
            PropertyBindingInfo[] properties
        );
        
        [DllImport("UCSharpRuntime", CallingConvention = CallingConvention.Cdecl)]
         private static extern IntPtr CallNativeCreateBlueprintInstance(
             IntPtr classPtr,
             IntPtr outerPtr
         );
     }
 }
 ```

### 5.2 UClass特性定义

```csharp
// UnrealEngine.Attributes.UClassAttribute.cs
using System;

namespace UnrealEngine.Attributes
{
    /// <summary>
    /// UClass特性 - 标记类为UE5类
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public class UClassAttribute : Attribute
    {
        /// <summary>
        /// 是否为蓝图类型
        /// </summary>
        public bool BlueprintType { get; set; } = false;
        
        /// <summary>
        /// 是否可被蓝图继承
        /// </summary>
        public bool Blueprintable { get; set; } = false;
        
        /// <summary>
        /// 是否为抽象类
        /// </summary>
        public bool Abstract { get; set; } = false;
        
        /// <summary>
        /// 是否为已弃用类
        /// </summary>
        public bool Deprecated { get; set; } = false;
        
        /// <summary>
        /// 类显示名称
        /// </summary>
        public string DisplayName { get; set; } = "";
        
        /// <summary>
        /// 类工具提示
        /// </summary>
        public string ToolTip { get; set; } = "";
        
        /// <summary>
        /// 类分类
        /// </summary>
        public string Category { get; set; } = "";
        
        /// <summary>
        /// 关键字
        /// </summary>
        public string Keywords { get; set; } = "";
        
        /// <summary>
        /// 类标志
        /// </summary>
        public EClassFlags ClassFlags { get; set; } = EClassFlags.CLASS_None;
    }
    
    /// <summary>
    /// 类标志枚举
    /// </summary>
    [Flags]
    public enum EClassFlags : uint
    {
        CLASS_None = 0x00000000,
        CLASS_Abstract = 0x00000001,
        CLASS_Compiled = 0x00000002,
        CLASS_Config = 0x00000004,
        CLASS_Transient = 0x00000008,
        CLASS_Parsed = 0x00000010,
        CLASS_MatchedSerializers = 0x00000020,
        CLASS_ProjectUserConfig = 0x00000040,
        CLASS_Native = 0x00000080,
        CLASS_NoExport = 0x00000100,
        CLASS_NotPlaceable = 0x00000200,
        CLASS_PerObjectConfig = 0x00000400,
        CLASS_ReplicationDataIsSetUp = 0x00000800,
        CLASS_EditInlineNew = 0x00001000,
        CLASS_CollapseCategories = 0x00002000,
        CLASS_Interface = 0x00004000,
        CLASS_CustomConstructor = 0x00008000,
        CLASS_Const = 0x00010000,
        CLASS_LayoutChanging = 0x00020000,
        CLASS_CompiledFromBlueprint = 0x00040000,
        CLASS_MinimalAPI = 0x00080000,
        CLASS_RequiredAPI = 0x00100000,
        CLASS_DefaultToInstanced = 0x00200000,
        CLASS_TokenStreamAssembled = 0x00400000,
        CLASS_HasInstancedReference = 0x00800000,
        CLASS_Hidden = 0x01000000,
        CLASS_Deprecated = 0x02000000,
        CLASS_HideDropDown = 0x04000000,
        CLASS_GlobalUserConfig = 0x08000000,
        CLASS_Intrinsic = 0x10000000,
        CLASS_ConstructorHelpers = 0x20000000,
        CLASS_MatchedSerializers2 = 0x40000000,
        CLASS_NewerVersionExists = 0x80000000
    }
}
```

## 6. 使用示例

### 6.1 C#自定义Actor示例

```csharp
// MyCustomActor.cs
using UnrealEngine.Core;
using UnrealEngine.Math;
using UnrealEngine.Attributes;

[UClass(BlueprintType = true, Blueprintable = true)]
public class MyCustomActor : AActor
{
    // UProperty示例
    [UProperty(BlueprintReadWrite = true, EditAnywhere = true, Category = "Custom")]
    public float Health { get; set; } = 100.0f;
    
    [UProperty(BlueprintReadWrite = true, EditAnywhere = true, Category = "Custom")]
    public string PlayerName { get; set; } = "Player";
    
    [UProperty(BlueprintReadOnly = true, VisibleAnywhere = true, Category = "Custom")]
    public bool IsAlive => Health > 0;
    
    // 组件
    [UProperty(BlueprintReadOnly = true, VisibleAnywhere = true, Category = "Components")]
    public UStaticMeshComponent MeshComponent { get; private set; }
    
    // 构造函数
    public MyCustomActor()
    {
        // 创建组件
        MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
        RootComponent = MeshComponent;
        
        // 设置默认值
        Health = 100.0f;
        PlayerName = "DefaultPlayer";
    }
    
    // UFunction示例
    [UFunction(BlueprintCallable = true, Category = "Custom")]
    public void TakeDamage(float damage)
    {
        Health = Math.Max(0, Health - damage);
        
        if (Health <= 0)
        {
            OnDeath();
        }
        
        // 触发蓝图事件
        OnHealthChanged(Health);
    }
    
    [UFunction(BlueprintCallable = true, Category = "Custom")]
    public void Heal(float amount)
    {
        Health = Math.Min(100.0f, Health + amount);
        OnHealthChanged(Health);
    }
    
    [UFunction(BlueprintImplementableEvent = true, Category = "Custom")]
    public virtual void OnHealthChanged(float newHealth)
    {
        // 这个方法将在蓝图中实现
    }
    
    [UFunction(BlueprintImplementableEvent = true, Category = "Custom")]
    public virtual void OnDeath()
    {
        // 这个方法将在蓝图中实现
    }
    
    [UFunction(BlueprintCallable = true, BlueprintPure = true, Category = "Custom")]
    public float GetHealthPercentage()
    {
        return Health / 100.0f;
    }
    
    // 生命周期重写
    protected override void OnBeginPlay()
    {
        base.OnBeginPlay();
        
        UE.LogInfo($"MyCustomActor {PlayerName} began play with {Health} health");
    }
    
    protected override void OnTick(float deltaTime)
    {
        base.OnTick(deltaTime);
        
        // 自定义Tick逻辑
        if (IsAlive)
        {
            // 例如：自动恢复生命值
            if (Health < 100.0f)
            {
                Heal(deltaTime * 5.0f); // 每秒恢复5点生命值
            }
        }
    }
}
```

### 6.2 C#自定义组件示例

```csharp
// MyCustomComponent.cs
using UnrealEngine.Core;
using UnrealEngine.Math;
using UnrealEngine.Attributes;

[UClass(BlueprintType = true, Blueprintable = true)]
public class MyCustomComponent : UActorComponent
{
    [UProperty(BlueprintReadWrite = true, EditAnywhere = true, Category = "Movement")]
    public float MovementSpeed { get; set; } = 100.0f;
    
    [UProperty(BlueprintReadWrite = true, EditAnywhere = true, Category = "Movement")]
    public FVector MovementDirection { get; set; } = FVector.ForwardVector;
    
    [UProperty(BlueprintReadOnly = true, VisibleAnywhere = true, Category = "Movement")]
    public FVector CurrentVelocity { get; private set; }
    
    private AActor _owner;
    
    protected override void OnBeginPlay()
    {
        base.OnBeginPlay();
        _owner = GetOwner();
    }
    
    [UFunction(BlueprintCallable = true, Category = "Movement")]
    public void StartMovement(FVector direction)
    {
        MovementDirection = direction.GetSafeNormal();
        CurrentVelocity = MovementDirection * MovementSpeed;
    }
    
    [UFunction(BlueprintCallable = true, Category = "Movement")]
    public void StopMovement()
    {
        CurrentVelocity = FVector.ZeroVector;
    }
    
    [UFunction(BlueprintCallable = true, Category = "Movement")]
    public void SetMovementSpeed(float newSpeed)
    {
        MovementSpeed = newSpeed;
        if (CurrentVelocity.SizeSquared() > 0)
        {
            CurrentVelocity = MovementDirection * MovementSpeed;
        }
    }
    
    protected override void OnTick(float deltaTime)
    {
        base.OnTick(deltaTime);
        
        if (_owner != null && CurrentVelocity.SizeSquared() > 0)
        {
            var newLocation = _owner.ActorLocation + CurrentVelocity * deltaTime;
            _owner.SetActorLocation(newLocation);
        }
    }
}
```

### 6.3 蓝图继承C#类示例

在蓝图编辑器中，可以：

1. **创建蓝图类**：选择`MyCustomActor`作为父类
2. **访问C#属性**：在蓝图中可以直接读写`Health`、`PlayerName`等属性
3. **调用C#函数**：可以调用`TakeDamage`、`Heal`、`GetHealthPercentage`等函数
4. **实现C#事件**：可以实现`OnHealthChanged`、`OnDeath`等事件
5. **重写C#方法**：可以重写虚方法添加自定义逻辑

## 7. C++端实现要点

### 7.1 原生类注册系统

```cpp
// UCSharpClassRegistry.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "Engine/Blueprint.h"

class UCSHARPRUNTIME_API UCSharpClassRegistry
{
public:
    // 注册C#类到UE5反射系统
    static UClass* RegisterCSharpClass(
        const FString& ClassName,
        UClass* ParentClass,
        const TArray<FCSharpFunctionInfo>& Functions,
        const TArray<FCSharpPropertyInfo>& Properties
    );
    
    // 创建C#类实例
    static UObject* CreateCSharpInstance(UClass* Class, UObject* Outer = nullptr);
    
    // 获取C#类信息
    static FCSharpClassInfo* GetCSharpClassInfo(UClass* Class);
    
    // 检查是否为C#类
    static bool IsCSharpClass(UClass* Class);
    
private:
    static TMap<UClass*, FCSharpClassInfo> CSharpClasses;
};

// C#函数信息结构
struct FCSharpFunctionInfo
{
    FString FunctionName;
    TArray<FCSharpParameterInfo> Parameters;
    FCSharpParameterInfo ReturnValue;
    uint32 FunctionFlags;
    void* ManagedDelegate;
};

// C#属性信息结构
struct FCSharpPropertyInfo
{
    FString PropertyName;
    FString PropertyType;
    uint64 PropertyFlags;
    void* Getter;
    void* Setter;
};
```

### 7.2 函数调用桥接

```cpp
// UCSharpFunctionBridge.cpp
#include "UCSharpFunctionBridge.h"
#include "UCSharpRuntime.h"

void UCSharpFunctionBridge::CallCSharpFunction(
    UObject* Target,
    UFunction* Function,
    void* Parameters
)
{
    // 获取C#函数信息
    auto* ClassInfo = UCSharpClassRegistry::GetCSharpClassInfo(Target->GetClass());
    if (!ClassInfo)
    {
        return;
    }
    
    auto* FunctionInfo = ClassInfo->FindFunction(Function->GetName());
    if (!FunctionInfo || !FunctionInfo->ManagedDelegate)
    {
        return;
    }
    
    // 转换参数
    TArray<void*> ConvertedParams;
    ConvertParametersToManaged(Function, Parameters, ConvertedParams);
    
    // 调用C#函数
    void* Result = UCSharpRuntime::InvokeManagedDelegate(
        FunctionInfo->ManagedDelegate,
        Target,
        ConvertedParams.GetData(),
        ConvertedParams.Num()
    );
    
    // 处理返回值
    if (Function->ReturnValueOffset != MAX_uint16)
    {
        ConvertReturnValueFromManaged(Function, Result, Parameters);
    }
    
    // 处理输出参数
    HandleOutParameters(Function, ConvertedParams, Parameters);
}
```

## 8. 方案总结

### 8.1 核心特性

1. **完整UObject体系支持**：C#可以继承UObject、AActor、UActorComponent等核心类
2. **UFunction机制**：支持蓝图调用、网络复制、事件系统
3. **UProperty机制**：支持蓝图访问、编辑器显示、序列化
4. **双向互操作**：蓝图可以继承C#类，调用C#函数，访问C#属性
5. **完整反射支持**：与UE5反射系统深度集成
6. **类型安全**：强类型绑定，编译时检查
7. **性能优化**：函数指针缓存，批量调用优化
8. **调试支持**：完整的调试信息和错误处理

### 8.2 技术优势

1. **原生集成**：与UE5核心系统深度集成，不是简单的脚本层
2. **高性能**：直接内存访问，最小化跨语言调用开销
3. **完整功能**：支持UE5的所有核心特性和机制
4. **开发友好**：现代C#语法，强大的IDE支持
5. **可扩展性**：模块化设计，易于扩展新功能

### 8.3 实施建议

1. **分阶段实现**：先实现基础UObject支持，再逐步添加高级功能
2. **充分测试**：建立完整的测试用例，确保稳定性
3. **性能监控**：实时监控性能指标，持续优化
4. **文档完善**：提供详细的API文档和使用示例
5. **社区支持**：建立开发者社区，收集反馈和建议

这个UObject体系集成设计为UE5 C#插件提供了完整的面向对象编程支持，使C#开发者能够充分利用UE5的强大功能，同时保持代码的简洁性和可维护性。