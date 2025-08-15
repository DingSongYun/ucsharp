# UE5 API C#封装策略详细设计

## 1. 封装架构概述

### 1.1 分层封装架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     C# API 封装层次结构                         │
├─────────────────────────────────────────────────────────────────┤
│  应用层 API (Application Layer)                                │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   游戏框架      │  │   AI系统        │  │   UI系统        │ │
│  │   - GameMode    │  │   - Behavior    │  │   - UMG         │ │
│  │   - PlayerCtrl  │  │   - Blackboard  │  │   - Slate       │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  高级 API 层 (High-Level API)                                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   扩展方法      │  │   LINQ支持      │  │   异步操作      │ │
│  │   - Fluent API  │  │   - 查询语法    │  │   - async/await │ │
│  │   - 链式调用    │  │   - 集合操作    │  │   - Task<T>     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  中间适配层 (Adapter Layer)                                    │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   类型适配      │  │   异常处理      │  │   内存管理      │ │
│  │   - 自动转换    │  │   - 异常映射    │  │   - 生命周期    │ │
│  │   - 空值处理    │  │   - 错误恢复    │  │   - 资源清理    │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  核心绑定层 (Core Binding Layer)                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   UObject系统   │  │   数学库        │  │   容器类型      │ │
│  │   - 反射支持    │  │   - Vector/Quat │  │   - TArray      │ │
│  │   - 属性访问    │  │   - Transform   │  │   - TMap        │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 核心类型系统封装

### 2.1 UObject 基础封装

```csharp
// UnrealEngine.Core.UObject.cs
using System;
using System.Collections.Generic;
using System.Reflection;
using UnrealEngine.Interop;

namespace UnrealEngine
{
    /// <summary>
    /// UE5 UObject的C#封装基类
    /// </summary>
    public abstract class UObject : IDisposable, IEquatable<UObject>
    {
        protected IntPtr NativePtr;
        protected int ObjectId;
        private bool _disposed = false;
        
        // 静态类型缓存
        private static readonly Dictionary<Type, UClass> _typeToUClass 
            = new Dictionary<Type, UClass>();
        
        protected UObject(IntPtr nativePtr)
        {
            NativePtr = nativePtr;
            ObjectId = ObjectManager.RegisterObject(this, nativePtr);
        }
        
        /// <summary>
        /// 获取对象的UClass
        /// </summary>
        public UClass GetClass()
        {
            var type = GetType();
            if (!_typeToUClass.TryGetValue(type, out var uclass))
            {
                var classPtr = NativeMethods.GetUClass(type.Name);
                uclass = new UClass(classPtr);
                _typeToUClass[type] = uclass;
            }
            return uclass;
        }
        
        /// <summary>
        /// 获取对象名称
        /// </summary>
        public string GetName()
        {
            return NativeMethods.GetObjectName(NativePtr);
        }
        
        /// <summary>
        /// 检查对象是否有效
        /// </summary>
        public bool IsValid()
        {
            return NativePtr != IntPtr.Zero && NativeMethods.IsValidObject(NativePtr);
        }
        
        /// <summary>
        /// 获取对象的外部对象
        /// </summary>
        public UObject GetOuter()
        {
            var outerPtr = NativeMethods.GetObjectOuter(NativePtr);
            return outerPtr != IntPtr.Zero ? ObjectManager.GetManagedObject(outerPtr) : null;
        }
        
        /// <summary>
        /// 调用UFunction
        /// </summary>
        protected T CallFunction<T>(string functionName, params object[] parameters)
        {
            var function = GetClass().FindFunction(functionName);
            if (function == null)
            {
                throw new InvalidOperationException($"Function '{functionName}' not found in class '{GetClass().GetName()}'.");
            }
            
            return function.Invoke<T>(this, parameters);
        }
        
        /// <summary>
        /// 获取属性值
        /// </summary>
        protected T GetProperty<T>(string propertyName)
        {
            var property = GetClass().FindProperty(propertyName);
            if (property == null)
            {
                throw new InvalidOperationException($"Property '{propertyName}' not found in class '{GetClass().GetName()}'.");
            }
            
            return property.GetValue<T>(this);
        }
        
        /// <summary>
        /// 设置属性值
        /// </summary>
        protected void SetProperty<T>(string propertyName, T value)
        {
            var property = GetClass().FindProperty(propertyName);
            if (property == null)
            {
                throw new InvalidOperationException($"Property '{propertyName}' not found in class '{GetClass().GetName()}'.");
            }
            
            property.SetValue(this, value);
        }
        
        #region IDisposable Implementation
        
        ~UObject()
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
                ObjectManager.UnregisterObject(ObjectId);
                _disposed = true;
            }
        }
        
        #endregion
        
        #region IEquatable Implementation
        
        public bool Equals(UObject other)
        {
            return other != null && NativePtr == other.NativePtr;
        }
        
        public override bool Equals(object obj)
        {
            return Equals(obj as UObject);
        }
        
        public override int GetHashCode()
        {
            return NativePtr.GetHashCode();
        }
        
        public static bool operator ==(UObject left, UObject right)
        {
            return ReferenceEquals(left, right) || (left?.Equals(right) ?? false);
        }
        
        public static bool operator !=(UObject left, UObject right)
        {
            return !(left == right);
        }
        
        #endregion
    }
}
```

### 2.2 Actor 系统封装

```csharp
// UnrealEngine.Engine.Actor.cs
using System;
using System.Collections.Generic;
using System.Linq;
using UnrealEngine.Math;

namespace UnrealEngine.Engine
{
    /// <summary>
    /// AActor的C#封装
    /// </summary>
    public class AActor : UObject
    {
        public AActor(IntPtr nativePtr) : base(nativePtr) { }
        
        #region Transform Operations
        
        /// <summary>
        /// 获取Actor的世界坐标
        /// </summary>
        public FVector GetActorLocation()
        {
            return CallFunction<FVector>(nameof(GetActorLocation));
        }
        
        /// <summary>
        /// 设置Actor的世界坐标
        /// </summary>
        public void SetActorLocation(FVector newLocation, bool sweep = false, bool teleport = false)
        {
            CallFunction<bool>(nameof(SetActorLocation), newLocation, sweep, teleport);
        }
        
        /// <summary>
        /// 获取Actor的旋转
        /// </summary>
        public FRotator GetActorRotation()
        {
            return CallFunction<FRotator>(nameof(GetActorRotation));
        }
        
        /// <summary>
        /// 设置Actor的旋转
        /// </summary>
        public void SetActorRotation(FRotator newRotation, bool teleport = false)
        {
            CallFunction<bool>(nameof(SetActorRotation), newRotation, teleport);
        }
        
        /// <summary>
        /// 获取Actor的缩放
        /// </summary>
        public FVector GetActorScale3D()
        {
            return CallFunction<FVector>(nameof(GetActorScale3D));
        }
        
        /// <summary>
        /// 设置Actor的缩放
        /// </summary>
        public void SetActorScale3D(FVector newScale)
        {
            CallFunction<void>(nameof(SetActorScale3D), newScale);
        }
        
        /// <summary>
        /// 获取Actor的变换矩阵
        /// </summary>
        public FTransform GetActorTransform()
        {
            return CallFunction<FTransform>(nameof(GetActorTransform));
        }
        
        /// <summary>
        /// 设置Actor的变换矩阵
        /// </summary>
        public void SetActorTransform(FTransform newTransform, bool sweep = false, bool teleport = false)
        {
            CallFunction<bool>(nameof(SetActorTransform), newTransform, sweep, teleport);
        }
        
        #endregion
        
        #region Component Management
        
        /// <summary>
        /// 获取根组件
        /// </summary>
        public USceneComponent GetRootComponent()
        {
            var componentPtr = CallFunction<IntPtr>(nameof(GetRootComponent));
            return componentPtr != IntPtr.Zero ? new USceneComponent(componentPtr) : null;
        }
        
        /// <summary>
        /// 根据类型获取组件
        /// </summary>
        public T GetComponentByClass<T>() where T : UActorComponent
        {
            var componentPtr = CallFunction<IntPtr>("GetComponentByClass", typeof(T).Name);
            return componentPtr != IntPtr.Zero ? (T)Activator.CreateInstance(typeof(T), componentPtr) : null;
        }
        
        /// <summary>
        /// 获取所有组件
        /// </summary>
        public IEnumerable<UActorComponent> GetComponents()
        {
            var componentPtrs = CallFunction<IntPtr[]>("GetComponents");
            return componentPtrs.Select(ptr => new UActorComponent(ptr));
        }
        
        /// <summary>
        /// 根据类型获取所有组件
        /// </summary>
        public IEnumerable<T> GetComponents<T>() where T : UActorComponent
        {
            return GetComponents().OfType<T>();
        }
        
        /// <summary>
        /// 添加组件
        /// </summary>
        public T AddComponent<T>(string componentName = null) where T : UActorComponent
        {
            componentName ??= typeof(T).Name;
            var componentPtr = CallFunction<IntPtr>("AddComponent", typeof(T).Name, componentName);
            return componentPtr != IntPtr.Zero ? (T)Activator.CreateInstance(typeof(T), componentPtr) : null;
        }
        
        #endregion
        
        #region Lifecycle Events
        
        /// <summary>
        /// 游戏开始时调用
        /// </summary>
        public virtual void BeginPlay()
        {
            CallFunction<void>(nameof(BeginPlay));
        }
        
        /// <summary>
        /// 每帧更新
        /// </summary>
        public virtual void Tick(float deltaTime)
        {
            CallFunction<void>(nameof(Tick), deltaTime);
        }
        
        /// <summary>
        /// 游戏结束时调用
        /// </summary>
        public virtual void EndPlay(EEndPlayReason reason)
        {
            CallFunction<void>(nameof(EndPlay), reason);
        }
        
        #endregion
        
        #region Collision and Physics
        
        /// <summary>
        /// 启用碰撞
        /// </summary>
        public void SetActorEnableCollision(bool enable)
        {
            CallFunction<void>(nameof(SetActorEnableCollision), enable);
        }
        
        /// <summary>
        /// 设置碰撞响应
        /// </summary>
        public void SetCollisionResponseToChannel(ECollisionChannel channel, ECollisionResponse response)
        {
            CallFunction<void>(nameof(SetCollisionResponseToChannel), channel, response);
        }
        
        #endregion
        
        #region Utility Methods
        
        /// <summary>
        /// 销毁Actor
        /// </summary>
        public void Destroy()
        {
            CallFunction<void>("Destroy");
        }
        
        /// <summary>
        /// 获取世界对象
        /// </summary>
        public UWorld GetWorld()
        {
            var worldPtr = CallFunction<IntPtr>(nameof(GetWorld));
            return worldPtr != IntPtr.Zero ? new UWorld(worldPtr) : null;
        }
        
        /// <summary>
        /// 检查Actor是否在游戏中
        /// </summary>
        public bool IsInGameWorld()
        {
            return CallFunction<bool>(nameof(IsInGameWorld));
        }
        
        #endregion
    }
    
    /// <summary>
    /// 游戏结束原因枚举
    /// </summary>
    public enum EEndPlayReason
    {
        Destroyed,
        LevelTransition,
        EndPlayInEditor,
        RemovedFromWorld,
        Quit
    }
    
    /// <summary>
    /// 碰撞通道枚举
    /// </summary>
    public enum ECollisionChannel
    {
        WorldStatic,
        WorldDynamic,
        Pawn,
        Visibility,
        Camera,
        PhysicsBody,
        Vehicle,
        Destructible
    }
    
    /// <summary>
    /// 碰撞响应枚举
    /// </summary>
    public enum ECollisionResponse
    {
        Ignore,
        Overlap,
        Block
    }
}
```

### 2.3 数学库封装

```csharp
// UnrealEngine.Math.cs
using System;
using System.Runtime.InteropServices;

namespace UnrealEngine.Math
{
    /// <summary>
    /// 三维向量
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct FVector : IEquatable<FVector>
    {
        public float X, Y, Z;
        
        public FVector(float x, float y, float z)
        {
            X = x; Y = y; Z = z;
        }
        
        public FVector(float value) : this(value, value, value) { }
        
        #region Static Properties
        
        public static FVector Zero => new FVector(0, 0, 0);
        public static FVector One => new FVector(1, 1, 1);
        public static FVector Forward => new FVector(1, 0, 0);
        public static FVector Right => new FVector(0, 1, 0);
        public static FVector Up => new FVector(0, 0, 1);
        
        #endregion
        
        #region Properties
        
        /// <summary>
        /// 向量长度
        /// </summary>
        public float Length => (float)System.Math.Sqrt(X * X + Y * Y + Z * Z);
        
        /// <summary>
        /// 向量长度的平方
        /// </summary>
        public float LengthSquared => X * X + Y * Y + Z * Z;
        
        /// <summary>
        /// 单位向量
        /// </summary>
        public FVector Normalized
        {
            get
            {
                var length = Length;
                return length > 0 ? this / length : Zero;
            }
        }
        
        #endregion
        
        #region Methods
        
        /// <summary>
        /// 归一化向量
        /// </summary>
        public void Normalize()
        {
            var length = Length;
            if (length > 0)
            {
                X /= length;
                Y /= length;
                Z /= length;
            }
        }
        
        /// <summary>
        /// 点积
        /// </summary>
        public float Dot(FVector other)
        {
            return X * other.X + Y * other.Y + Z * other.Z;
        }
        
        /// <summary>
        /// 叉积
        /// </summary>
        public FVector Cross(FVector other)
        {
            return new FVector(
                Y * other.Z - Z * other.Y,
                Z * other.X - X * other.Z,
                X * other.Y - Y * other.X
            );
        }
        
        /// <summary>
        /// 距离
        /// </summary>
        public float Distance(FVector other)
        {
            return (this - other).Length;
        }
        
        /// <summary>
        /// 距离的平方
        /// </summary>
        public float DistanceSquared(FVector other)
        {
            return (this - other).LengthSquared;
        }
        
        /// <summary>
        /// 线性插值
        /// </summary>
        public static FVector Lerp(FVector a, FVector b, float t)
        {
            t = System.Math.Max(0, System.Math.Min(1, t));
            return a + (b - a) * t;
        }
        
        /// <summary>
        /// 球面线性插值
        /// </summary>
        public static FVector Slerp(FVector a, FVector b, float t)
        {
            var dot = a.Normalized.Dot(b.Normalized);
            dot = System.Math.Max(-1, System.Math.Min(1, dot));
            
            var theta = System.Math.Acos(dot) * t;
            var relativeVec = (b - a * dot).Normalized;
            
            return a * (float)System.Math.Cos(theta) + relativeVec * (float)System.Math.Sin(theta);
        }
        
        #endregion
        
        #region Operators
        
        public static FVector operator +(FVector a, FVector b)
        {
            return new FVector(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        }
        
        public static FVector operator -(FVector a, FVector b)
        {
            return new FVector(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
        }
        
        public static FVector operator -(FVector a)
        {
            return new FVector(-a.X, -a.Y, -a.Z);
        }
        
        public static FVector operator *(FVector a, float scalar)
        {
            return new FVector(a.X * scalar, a.Y * scalar, a.Z * scalar);
        }
        
        public static FVector operator *(float scalar, FVector a)
        {
            return a * scalar;
        }
        
        public static FVector operator /(FVector a, float scalar)
        {
            return new FVector(a.X / scalar, a.Y / scalar, a.Z / scalar);
        }
        
        public static bool operator ==(FVector a, FVector b)
        {
            return a.Equals(b);
        }
        
        public static bool operator !=(FVector a, FVector b)
        {
            return !a.Equals(b);
        }
        
        #endregion
        
        #region IEquatable Implementation
        
        public bool Equals(FVector other)
        {
            const float epsilon = 1e-6f;
            return System.Math.Abs(X - other.X) < epsilon &&
                   System.Math.Abs(Y - other.Y) < epsilon &&
                   System.Math.Abs(Z - other.Z) < epsilon;
        }
        
        public override bool Equals(object obj)
        {
            return obj is FVector other && Equals(other);
        }
        
        public override int GetHashCode()
        {
            return HashCode.Combine(X, Y, Z);
        }
        
        #endregion
        
        public override string ToString()
        {
            return $"({X:F2}, {Y:F2}, {Z:F2})";
        }
    }
    
    /// <summary>
    /// 旋转器
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct FRotator : IEquatable<FRotator>
    {
        public float Pitch, Yaw, Roll;
        
        public FRotator(float pitch, float yaw, float roll)
        {
            Pitch = pitch; Yaw = yaw; Roll = roll;
        }
        
        public static FRotator Zero => new FRotator(0, 0, 0);
        
        /// <summary>
        /// 转换为四元数
        /// </summary>
        public FQuat ToQuaternion()
        {
            // 实现旋转器到四元数的转换
            var pitchRad = Pitch * System.Math.PI / 180.0;
            var yawRad = Yaw * System.Math.PI / 180.0;
            var rollRad = Roll * System.Math.PI / 180.0;
            
            var cy = System.Math.Cos(yawRad * 0.5);
            var sy = System.Math.Sin(yawRad * 0.5);
            var cp = System.Math.Cos(pitchRad * 0.5);
            var sp = System.Math.Sin(pitchRad * 0.5);
            var cr = System.Math.Cos(rollRad * 0.5);
            var sr = System.Math.Sin(rollRad * 0.5);
            
            return new FQuat(
                (float)(cy * cp * sr - sy * sp * cr),
                (float)(sy * cp * sr + cy * sp * cr),
                (float)(sy * cp * cr - cy * sp * sr),
                (float)(cy * cp * cr + sy * sp * sr)
            );
        }
        
        public bool Equals(FRotator other)
        {
            const float epsilon = 1e-6f;
            return System.Math.Abs(Pitch - other.Pitch) < epsilon &&
                   System.Math.Abs(Yaw - other.Yaw) < epsilon &&
                   System.Math.Abs(Roll - other.Roll) < epsilon;
        }
        
        public override bool Equals(object obj)
        {
            return obj is FRotator other && Equals(other);
        }
        
        public override int GetHashCode()
        {
            return HashCode.Combine(Pitch, Yaw, Roll);
        }
        
        public override string ToString()
        {
            return $"(P={Pitch:F2}, Y={Yaw:F2}, R={Roll:F2})";
        }
    }
    
    /// <summary>
    /// 四元数
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct FQuat : IEquatable<FQuat>
    {
        public float X, Y, Z, W;
        
        public FQuat(float x, float y, float z, float w)
        {
            X = x; Y = y; Z = z; W = w;
        }
        
        public static FQuat Identity => new FQuat(0, 0, 0, 1);
        
        /// <summary>
        /// 四元数长度
        /// </summary>
        public float Length => (float)System.Math.Sqrt(X * X + Y * Y + Z * Z + W * W);
        
        /// <summary>
        /// 归一化四元数
        /// </summary>
        public FQuat Normalized
        {
            get
            {
                var length = Length;
                return length > 0 ? new FQuat(X / length, Y / length, Z / length, W / length) : Identity;
            }
        }
        
        /// <summary>
        /// 四元数乘法
        /// </summary>
        public static FQuat operator *(FQuat a, FQuat b)
        {
            return new FQuat(
                a.W * b.X + a.X * b.W + a.Y * b.Z - a.Z * b.Y,
                a.W * b.Y + a.Y * b.W + a.Z * b.X - a.X * b.Z,
                a.W * b.Z + a.Z * b.W + a.X * b.Y - a.Y * b.X,
                a.W * b.W - a.X * b.X - a.Y * b.Y - a.Z * b.Z
            );
        }
        
        /// <summary>
        /// 旋转向量
        /// </summary>
        public FVector RotateVector(FVector vector)
        {
            var qvec = new FVector(X, Y, Z);
            var uv = qvec.Cross(vector);
            var uuv = qvec.Cross(uv);
            
            return vector + (uv * W + uuv) * 2.0f;
        }
        
        public bool Equals(FQuat other)
        {
            const float epsilon = 1e-6f;
            return System.Math.Abs(X - other.X) < epsilon &&
                   System.Math.Abs(Y - other.Y) < epsilon &&
                   System.Math.Abs(Z - other.Z) < epsilon &&
                   System.Math.Abs(W - other.W) < epsilon;
        }
        
        public override bool Equals(object obj)
        {
            return obj is FQuat other && Equals(other);
        }
        
        public override int GetHashCode()
        {
            return HashCode.Combine(X, Y, Z, W);
        }
    }
    
    /// <summary>
    /// 变换矩阵
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct FTransform : IEquatable<FTransform>
    {
        public FQuat Rotation;
        public FVector Translation;
        public FVector Scale3D;
        
        public FTransform(FVector translation, FQuat rotation, FVector scale)
        {
            Translation = translation;
            Rotation = rotation;
            Scale3D = scale;
        }
        
        public FTransform(FVector translation, FQuat rotation) 
            : this(translation, rotation, FVector.One) { }
        
        public FTransform(FVector translation) 
            : this(translation, FQuat.Identity, FVector.One) { }
        
        public static FTransform Identity => new FTransform(FVector.Zero, FQuat.Identity, FVector.One);
        
        /// <summary>
        /// 变换向量
        /// </summary>
        public FVector TransformPosition(FVector position)
        {
            return Rotation.RotateVector(position * Scale3D) + Translation;
        }
        
        /// <summary>
        /// 逆变换向量
        /// </summary>
        public FVector InverseTransformPosition(FVector position)
        {
            // 实现逆变换
            var relativePos = position - Translation;
            var inverseRotation = new FQuat(-Rotation.X, -Rotation.Y, -Rotation.Z, Rotation.W);
            return inverseRotation.RotateVector(relativePos) / Scale3D;
        }
        
        public bool Equals(FTransform other)
        {
            return Translation.Equals(other.Translation) &&
                   Rotation.Equals(other.Rotation) &&
                   Scale3D.Equals(other.Scale3D);
        }
        
        public override bool Equals(object obj)
        {
            return obj is FTransform other && Equals(other);
        }
        
        public override int GetHashCode()
        {
            return HashCode.Combine(Translation, Rotation, Scale3D);
        }
    }
}
```

## 3. 高级API层设计

### 3.1 扩展方法和Fluent API

```csharp
// UnrealEngine.Extensions.cs
using System;
using System.Collections.Generic;
using System.Linq;
using UnrealEngine.Engine;
using UnrealEngine.Math;

namespace UnrealEngine.Extensions
{
    /// <summary>
    /// Actor扩展方法
    /// </summary>
    public static class ActorExtensions
    {
        /// <summary>
        /// 流畅的变换设置API
        /// </summary>
        public static AActor WithLocation(this AActor actor, FVector location)
        {
            actor.SetActorLocation(location);
            return actor;
        }
        
        public static AActor WithRotation(this AActor actor, FRotator rotation)
        {
            actor.SetActorRotation(rotation);
            return actor;
        }
        
        public static AActor WithScale(this AActor actor, FVector scale)
        {
            actor.SetActorScale3D(scale);
            return actor;
        }
        
        public static AActor WithTransform(this AActor actor, FTransform transform)
        {
            actor.SetActorTransform(transform);
            return actor;
        }
        
        /// <summary>
        /// 链式组件添加
        /// </summary>
        public static AActor AddComponent<T>(this AActor actor, out T component, string name = null) 
            where T : UActorComponent
        {
            component = actor.AddComponent<T>(name);
            return actor;
        }
        
        /// <summary>
        /// 条件执行
        /// </summary>
        public static AActor When(this AActor actor, bool condition, Action<AActor> action)
        {
            if (condition)
            {
                action(actor);
            }
            return actor;
        }
        
        /// <summary>
        /// 批量设置属性
        /// </summary>
        public static AActor Configure(this AActor actor, Action<AActor> configure)
        {
            configure(actor);
            return actor;
        }
    }
    
    /// <summary>
    /// 向量扩展方法
    /// </summary>
    public static class VectorExtensions
    {
        /// <summary>
        /// 向量链式操作
        /// </summary>
        public static FVector Add(this FVector vector, FVector other)
        {
            return vector + other;
        }
        
        public static FVector Multiply(this FVector vector, float scalar)
        {
            return vector * scalar;
        }
        
        public static FVector Clamp(this FVector vector, FVector min, FVector max)
        {
            return new FVector(
                System.Math.Max(min.X, System.Math.Min(max.X, vector.X)),
                System.Math.Max(min.Y, System.Math.Min(max.Y, vector.Y)),
                System.Math.Max(min.Z, System.Math.Min(max.Z, vector.Z))
            );
        }
        
        /// <summary>
        /// 向量分量访问
        /// </summary>
        public static float GetComponent(this FVector vector, int index)
        {
            return index switch
            {
                0 => vector.X,
                1 => vector.Y,
                2 => vector.Z,
                _ => throw new ArgumentOutOfRangeException(nameof(index))
            };
        }
        
        public static FVector SetComponent(this FVector vector, int index, float value)
        {
            return index switch
            {
                0 => new FVector(value, vector.Y, vector.Z),
                1 => new FVector(vector.X, value, vector.Z),
                2 => new FVector(vector.X, vector.Y, value),
                _ => throw new ArgumentOutOfRangeException(nameof(index))
            };
        }
    }
    
    /// <summary>
    /// 集合扩展方法
    /// </summary>
    public static class CollectionExtensions
    {
        /// <summary>
        /// 查找最近的Actor
        /// </summary>
        public static AActor FindClosest(this IEnumerable<AActor> actors, FVector position)
        {
            return actors
                .Where(a => a.IsValid())
                .OrderBy(a => a.GetActorLocation().Distance(position))
                .FirstOrDefault();
        }
        
        /// <summary>
        /// 按距离排序
        /// </summary>
        public static IEnumerable<AActor> OrderByDistance(this IEnumerable<AActor> actors, FVector position)
        {
            return actors
                .Where(a => a.IsValid())
                .OrderBy(a => a.GetActorLocation().Distance(position));
        }
        
        /// <summary>
        /// 在范围内的Actor
        /// </summary>
        public static IEnumerable<AActor> WithinRange(this IEnumerable<AActor> actors, FVector center, float radius)
        {
            var radiusSquared = radius * radius;
            return actors
                .Where(a => a.IsValid() && a.GetActorLocation().DistanceSquared(center) <= radiusSquared);
        }
        
        /// <summary>
        /// 按类型过滤
        /// </summary>
        public static IEnumerable<T> OfActorType<T>(this IEnumerable<AActor> actors) where T : AActor
        {
            return actors.OfType<T>();
        }
    }
}
```

### 3.2 异步操作支持

```csharp
// UnrealEngine.Async.cs
using System;
using System.Threading;
using System.Threading.Tasks;
using UnrealEngine.Engine;

namespace UnrealEngine.Async
{
    /// <summary>
    /// UE5异步操作支持
    /// </summary>
    public static class AsyncOperations
    {
        /// <summary>
        /// 异步等待指定时间
        /// </summary>
        public static async Task DelayAsync(float seconds, CancellationToken cancellationToken = default)
        {
            var milliseconds = (int)(seconds * 1000);
            await Task.Delay(milliseconds, cancellationToken);
        }
        
        /// <summary>
        /// 异步等待条件满足
        /// </summary>
        public static async Task WaitUntilAsync(Func<bool> condition, float checkInterval = 0.1f, 
                                               CancellationToken cancellationToken = default)
        {
            while (!condition() && !cancellationToken.IsCancellationRequested)
            {
                await DelayAsync(checkInterval, cancellationToken);
            }
        }
        
        /// <summary>
        /// 异步移动Actor
        /// </summary>
        public static async Task MoveToAsync(this AActor actor, FVector targetLocation, float speed = 100f, 
                                           CancellationToken cancellationToken = default)
        {
            var startLocation = actor.GetActorLocation();
            var distance = startLocation.Distance(targetLocation);
            var duration = distance / speed;
            
            var startTime = DateTime.Now;
            
            while (!cancellationToken.IsCancellationRequested)
            {
                var elapsed = (float)(DateTime.Now - startTime).TotalSeconds;
                var progress = Math.Min(elapsed / duration, 1.0f);
                
                var currentLocation = FVector.Lerp(startLocation, targetLocation, progress);
                actor.SetActorLocation(currentLocation);
                
                if (progress >= 1.0f)
                    break;
                
                await DelayAsync(0.016f, cancellationToken); // ~60 FPS
            }
        }
        
        /// <summary>
        /// 异步旋转Actor
        /// </summary>
        public static async Task RotateToAsync(this AActor actor, FRotator targetRotation, float speed = 90f, 
                                             CancellationToken cancellationToken = default)
        {
            var startRotation = actor.GetActorRotation();
            var startQuat = startRotation.ToQuaternion();
            var targetQuat = targetRotation.ToQuaternion();
            
            // 计算旋转角度
            var dot = Math.Abs(startQuat.X * targetQuat.X + startQuat.Y * targetQuat.Y + 
                              startQuat.Z * targetQuat.Z + startQuat.W * targetQuat.W);
            var angle = (float)(Math.Acos(Math.Min(dot, 1.0)) * 2.0 * 180.0 / Math.PI);
            
            var duration = angle / speed;
            var startTime = DateTime.Now;
            
            while (!cancellationToken.IsCancellationRequested)
            {
                var elapsed = (float)(DateTime.Now - startTime).TotalSeconds;
                var progress = Math.Min(elapsed / duration, 1.0f);
                
                // 四元数球面插值
                var currentQuat = SlerpQuaternion(startQuat, targetQuat, progress);
                var currentRotation = QuaternionToRotator(currentQuat);
                actor.SetActorRotation(currentRotation);
                
                if (progress >= 1.0f)
                    break;
                
                await DelayAsync(0.016f, cancellationToken);
            }
        }
        
        private static FQuat SlerpQuaternion(FQuat a, FQuat b, float t)
        {
            // 实现四元数球面线性插值
            var dot = a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
            
            if (dot < 0.0f)
            {
                b = new FQuat(-b.X, -b.Y, -b.Z, -b.W);
                dot = -dot;
            }
            
            if (dot > 0.9995f)
            {
                // 线性插值
                var result = new FQuat(
                    a.X + t * (b.X - a.X),
                    a.Y + t * (b.Y - a.Y),
                    a.Z + t * (b.Z - a.Z),
                    a.W + t * (b.W - a.W)
                );
                return result.Normalized;
            }
            
            var theta0 = Math.Acos(dot);
            var theta = theta0 * t;
            var sinTheta = Math.Sin(theta);
            var sinTheta0 = Math.Sin(theta0);
            
            var s0 = Math.Cos(theta) - dot * sinTheta / sinTheta0;
            var s1 = sinTheta / sinTheta0;
            
            return new FQuat(
                (float)(s0 * a.X + s1 * b.X),
                (float)(s0 * a.Y + s1 * b.Y),
                (float)(s0 * a.Z + s1 * b.Z),
                (float)(s0 * a.W + s1 * b.W)
            );
        }
        
        private static FRotator QuaternionToRotator(FQuat quat)
        {
            // 四元数转旋转器
            var sinr_cosp = 2 * (quat.W * quat.X + quat.Y * quat.Z);
            var cosr_cosp = 1 - 2 * (quat.X * quat.X + quat.Y * quat.Y);
            var roll = (float)(Math.Atan2(sinr_cosp, cosr_cosp) * 180.0 / Math.PI);
            
            var sinp = 2 * (quat.W * quat.Y - quat.Z * quat.X);
            var pitch = (float)(Math.Abs(sinp) >= 1 ? 
                Math.CopySign(Math.PI / 2, sinp) * 180.0 / Math.PI : 
                Math.Asin(sinp) * 180.0 / Math.PI);
            
            var siny_cosp = 2 * (quat.W * quat.Z + quat.X * quat.Y);
            var cosy_cosp = 1 - 2 * (quat.Y * quat.Y + quat.Z * quat.Z);
            var yaw = (float)(Math.Atan2(siny_cosp, cosy_cosp) * 180.0 / Math.PI);
            
            return new FRotator(pitch, yaw, roll);
        }
    }
    
    /// <summary>
    /// 协程支持
    /// </summary>
    public static class Coroutines
    {
        /// <summary>
        /// 启动协程
        /// </summary>
        public static async Task StartCoroutine(Func<Task> coroutine)
        {
            await coroutine();
        }
        
        /// <summary>
        /// 等待帧结束
        /// </summary>
        public static async Task WaitForEndOfFrame()
        {
            await DelayAsync(0.016f); // 假设60FPS
        }
        
        /// <summary>
        /// 等待固定更新
        /// </summary>
        public static async Task WaitForFixedUpdate()
        {
            await DelayAsync(0.02f); // 假设50Hz物理更新
        }
    }
}
```

## 4. 使用示例

### 4.1 基础使用示例

```csharp
// GameLogic.cs - 游戏逻辑示例
using UnrealEngine;
using UnrealEngine.Engine;
using UnrealEngine.Math;
using UnrealEngine.Extensions;
using UnrealEngine.Async;
using System.Threading.Tasks;

public class PlayerController : APlayerController
{
    private APawn _controlledPawn;
    
    public override void BeginPlay()
    {
        base.BeginPlay();
        
        _controlledPawn = GetPawn();
        
        // 使用流畅API设置初始状态
        _controlledPawn
            .WithLocation(new FVector(0, 0, 100))
            .WithRotation(new FRotator(0, 0, 0))
            .Configure(pawn => 
            {
                pawn.SetActorEnableCollision(true);
                // 其他配置...
            });
    }
    
    public async Task MovePlayerToLocation(FVector targetLocation)
    {
        if (_controlledPawn != null)
        {
            // 异步移动玩家
            await _controlledPawn.MoveToAsync(targetLocation, speed: 200f);
            
            // 移动完成后的逻辑
            UE.Log($"Player reached target location: {targetLocation}");
        }
    }
    
    public void FindNearbyEnemies(float searchRadius = 1000f)
    {
        var playerLocation = _controlledPawn.GetActorLocation();
        
        // 使用LINQ风格的查询
        var nearbyEnemies = GetWorld()
            .GetAllActorsOfClass<AEnemy>()
            .WithinRange(playerLocation, searchRadius)
            .OrderByDistance(playerLocation)
            .Take(5);
        
        foreach (var enemy in nearbyEnemies)
        {
            UE.Log($"Found enemy: {enemy.GetName()} at distance: {enemy.GetActorLocation().Distance(playerLocation)}");
        }
    }
}

public class Enemy : APawn
{
    private float _health = 100f;
    private bool _isAlive = true;
    
    public override void BeginPlay()
    {
        base.BeginPlay();
        
        // 添加组件
        this.AddComponent<UStaticMeshComponent>(out var meshComponent, "EnemyMesh")
            .AddComponent<UCapsuleComponent>(out var collisionComponent, "Collision")
            .Configure(enemy => 
            {
                // 配置碰撞
                collisionComponent.SetCollisionResponseToChannel(ECollisionChannel.Pawn, ECollisionResponse.Block);
            });
    }
    
    public async Task TakeDamage(float damage)
    {
        _health -= damage;
        
        if (_health <= 0 && _isAlive)
        {
            _isAlive = false;
            await PlayDeathAnimation();
            Destroy();
        }
    }
    
    private async Task PlayDeathAnimation()
    {
        // 播放死亡动画
        var animComponent = GetComponentByClass<UAnimationComponent>();
        if (animComponent != null)
        {
            animComponent.PlayAnimation("DeathAnim");
            await DelayAsync(2.0f); // 等待动画播放完成
        }
    }
}
```

### 4.2 高级使用示例

```csharp
// AdvancedGameplay.cs - 高级游戏玩法示例
using UnrealEngine;
using UnrealEngine.Engine;
using UnrealEngine.Math;
using UnrealEngine.Extensions;
using UnrealEngine.Async;
using UnrealEngine.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

public class GameManager : AGameModeBase
{
    private List<APlayer> _players = new List<APlayer>();
    private List<AEnemy> _enemies = new List<AEnemy>();
    private bool _gameStarted = false;
    
    public override void BeginPlay()
    {
        base.BeginPlay();
        
        // 订阅游戏事件
        EventManager.Subscribe<PlayerJoinedEventArgs>("PlayerJoined", OnPlayerJoined);
        EventManager.Subscribe<EnemyDefeatedEventArgs>("EnemyDefeated", OnEnemyDefeated);
        
        StartGameAsync();
    }
    
    private async Task StartGameAsync()
    {
        UE.Log("Game starting in 3 seconds...");
        await DelayAsync(3.0f);
        
        _gameStarted = true;
        SpawnEnemies();
        
        UE.Log("Game started!");
        
        // 开始游戏循环
        _ = GameLoopAsync();
    }
    
    private async Task GameLoopAsync()
    {
        while (_gameStarted)
        {
            // 每5秒检查游戏状态
            await DelayAsync(5.0f);
            
            if (_enemies.Count == 0)
            {
                await OnAllEnemiesDefeated();
                break;
            }
            
            // 检查玩家状态
            var alivePlayers = _players.Where(p => p.IsAlive()).ToList();
            if (alivePlayers.Count == 0)
            {
                await OnAllPlayersDefeated();
                break;
            }
        }
    }
    
    private void SpawnEnemies()
    {
        var spawnPoints = GetWorld()
            .GetAllActorsOfClass<AEnemySpawnPoint>()
            .OrderBy(_ => Guid.NewGuid()) // 随机排序
            .Take(10);
        
        foreach (var spawnPoint in spawnPoints)
        {
            var enemy = GetWorld().SpawnActor<AEnemy>(spawnPoint.GetActorLocation());
            _enemies.Add(enemy);
        }
    }
    
    private void OnPlayerJoined(PlayerJoinedEventArgs args)
    {
        _players.Add(args.Player);
        UE.Log($"Player {args.Player.GetName()} joined the game!");
    }
    
    private void OnEnemyDefeated(EnemyDefeatedEventArgs args)
    {
        _enemies.Remove(args.Enemy);
        UE.Log($"Enemy {args.Enemy.GetName()} defeated! Remaining: {_enemies.Count}");
    }
    
    private async Task OnAllEnemiesDefeated()
    {
        UE.Log("Victory! All enemies defeated!");
        
        // 播放胜利效果
        await PlayVictoryEffects();
        
        _gameStarted = false;
    }
    
    private async Task OnAllPlayersDefeated()
    {
        UE.Log("Game Over! All players defeated!");
        
        // 播放失败效果
        await PlayDefeatEffects();
        
        _gameStarted = false;
    }
    
    private async Task PlayVictoryEffects()
    {
        // 播放胜利音效和特效
        var audioComponent = GetComponentByClass<UAudioComponent>();
        audioComponent?.PlaySound("VictorySound");
        
        // 显示胜利UI
        var uiManager = GetWorld().GetGameInstance().GetSubsystem<UUIManager>();
        uiManager?.ShowVictoryScreen();
        
        await DelayAsync(3.0f);
    }
    
    private async Task PlayDefeatEffects()
    {
        // 播放失败音效和特效
        var audioComponent = GetComponentByClass<UAudioComponent>();
        audioComponent?.PlaySound("DefeatSound");
        
        // 显示失败UI
        var uiManager = GetWorld().GetGameInstance().GetSubsystem<UUIManager>();
        uiManager?.ShowDefeatScreen();
        
        await DelayAsync(3.0f);
    }
}

// 事件参数类
public class PlayerJoinedEventArgs : EventArgs
{
    public APlayer Player { get; set; }
    
    public PlayerJoinedEventArgs(APlayer player)
    {
        EventName = "PlayerJoined";
        Player = player;
    }
}

public class EnemyDefeatedEventArgs : EventArgs
{
    public AEnemy Enemy { get; set; }
    public APlayer DefeatedBy { get; set; }
    
    public EnemyDefeatedEventArgs(AEnemy enemy, APlayer defeatedBy)
    {
        EventName = "EnemyDefeated";
        Enemy = enemy;
        DefeatedBy = defeatedBy;
    }
}
```

## 5. 总结

这个UE5 API C#封装策略提供了：

1. **完整的类型系统**: 从UObject到具体的游戏类型的完整封装
2. **现代C#特性**: 支持async/await、LINQ、扩展方法等
3. **流畅的API设计**: 链式调用和配置模式
4. **性能优化**: 类型缓存、批量操作等优化策略
5. **事件驱动架构**: 完善的事件系统支持
6. **异步操作**: 原生支持异步游戏逻辑

通过这个封装策略，C#开发者可以用熟悉的语法和模式来开发UE5游戏，同时保持与原生C++代码相当的性能和功能完整性。