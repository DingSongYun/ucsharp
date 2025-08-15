using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace UCSharp.Core
{
    /// <summary>
    /// Base class for all UE5 objects in C#
    /// Provides the foundation for UObject integration
    /// </summary>
    public abstract class UObject : IDisposable
    {
        #region Native Interop
        
        /// <summary>
        /// Native UObject pointer
        /// </summary>
        protected IntPtr NativePtr { get; private set; }
        
        /// <summary>
        /// Internal access to native pointer for ObjectRegistry
        /// </summary>
        internal IntPtr InternalNativePtr => NativePtr;
        
        /// <summary>
        /// Whether this object owns the native pointer
        /// </summary>
        protected bool bOwnsNativePtr { get; private set; }
        
        /// <summary>
        /// Whether this object has been disposed
        /// </summary>
        public bool IsDisposed { get; private set; }
        
        #endregion
        
        #region Constructors
        
        /// <summary>
        /// Default constructor for C# created objects
        /// </summary>
        protected UObject()
        {
            // Create native UObject
            NativePtr = CreateNativeObject(GetType());
            bOwnsNativePtr = true;
            
            // Register with object registry
            ObjectRegistry.RegisterObject(this);
        }
        
        /// <summary>
        /// Constructor for wrapping existing native objects
        /// </summary>
        /// <param name="nativePtr">Native UObject pointer</param>
        /// <param name="ownsPtr">Whether this wrapper owns the pointer</param>
        protected UObject(IntPtr nativePtr, bool ownsPtr = false)
        {
            if (nativePtr == IntPtr.Zero)
                throw new ArgumentException("Native pointer cannot be null", nameof(nativePtr));
                
            NativePtr = nativePtr;
            bOwnsNativePtr = ownsPtr;
            
            // Register with object registry
            ObjectRegistry.RegisterObject(this);
        }
        
        #endregion
        
        #region Object Lifecycle
        
        /// <summary>
        /// Called when the object is being initialized
        /// Override this in derived classes for custom initialization
        /// </summary>
        protected virtual void BeginPlay()
        {
            // Default implementation - can be overridden
        }
        
        /// <summary>
        /// Called when the object is being destroyed
        /// Override this in derived classes for custom cleanup
        /// </summary>
        protected virtual void EndPlay()
        {
            // Default implementation - can be overridden
        }
        
        /// <summary>
        /// Called every frame (for objects that support ticking)
        /// Override this in derived classes for per-frame logic
        /// </summary>
        /// <param name="deltaTime">Time since last frame</param>
        protected virtual void Tick(float deltaTime)
        {
            // Default implementation - can be overridden
        }
        
        #endregion
        
        #region UFunction Support
        
        /// <summary>
        /// Call a UFunction by name
        /// </summary>
        /// <param name="functionName">Name of the function to call</param>
        /// <param name="parameters">Function parameters</param>
        /// <returns>Function return value</returns>
        protected object? CallUFunction(string functionName, params object[] parameters)
        {
            if (IsDisposed)
                throw new ObjectDisposedException(GetType().Name);
                
            return CallNativeFunction(NativePtr, functionName, parameters);
        }
        
        /// <summary>
        /// Check if a UFunction exists
        /// </summary>
        /// <param name="functionName">Name of the function</param>
        /// <returns>True if function exists</returns>
        protected bool HasUFunction(string functionName)
        {
            if (IsDisposed)
                return false;
                
            return HasNativeFunction(NativePtr, functionName);
        }
        
        #endregion
        
        #region UProperty Support
        
        /// <summary>
        /// Get a UProperty value by name
        /// </summary>
        /// <typeparam name="T">Property type</typeparam>
        /// <param name="propertyName">Name of the property</param>
        /// <returns>Property value</returns>
        protected T? GetUProperty<T>(string propertyName)
        {
            if (IsDisposed)
                throw new ObjectDisposedException(GetType().Name);
                
            return GetProperty<T>(propertyName);
        }
        
        /// <summary>
        /// Set a UProperty value by name
        /// </summary>
        /// <typeparam name="T">Property type</typeparam>
        /// <param name="propertyName">Name of the property</param>
        /// <param name="value">Property value</param>
        protected void SetUProperty<T>(string propertyName, T value)
        {
            if (IsDisposed)
                throw new ObjectDisposedException(GetType().Name);
                
            SetProperty<T>(propertyName, value);
        }
        
        #endregion
        
        #region Object Information
        
        /// <summary>
        /// Get the UClass of this object
        /// </summary>
        /// <returns>UClass pointer</returns>
        public IntPtr GetClass()
        {
            if (IsDisposed)
                return IntPtr.Zero;
                
            return GetNativeClass(NativePtr);
        }
        
        /// <summary>
        /// Get the name of this object
        /// </summary>
        /// <returns>Object name</returns>
        public string GetName()
        {
            if (IsDisposed)
                return string.Empty;
                
            return GetNativeName(NativePtr) ?? string.Empty;
        }
        
        /// <summary>
        /// Check if this object is valid (not null and not pending kill)
        /// </summary>
        /// <returns>True if object is valid</returns>
        public bool IsValid()
        {
            return !IsDisposed && NativePtr != IntPtr.Zero && IsNativeValid(NativePtr);
        }
        
        #endregion
        
        #region IDisposable Implementation
        
        /// <summary>
        /// Dispose of this object
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        
        /// <summary>
        /// Protected dispose method
        /// </summary>
        /// <param name="disposing">Whether disposing from Dispose() call</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!IsDisposed)
            {
                if (disposing)
                {
                    // Call EndPlay for cleanup
                    EndPlay();
                    
                    // Unregister from object registry
                    ObjectRegistry.UnregisterObject(this);
                }
                
                // Clean up native resources
                if (bOwnsNativePtr && NativePtr != IntPtr.Zero)
                {
                    DestroyNativeObject(NativePtr);
                }
                
                NativePtr = IntPtr.Zero;
                IsDisposed = true;
            }
        }
        
        /// <summary>
        /// Finalizer
        /// </summary>
        ~UObject()
        {
            Dispose(false);
        }
        
        #endregion
        
        #region Native Interop Methods
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateNativeObject(Type objectType);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern void DestroyNativeObject(IntPtr objectPtr);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern object? CallNativeFunction(IntPtr objectPtr, string functionName, object[] parameters);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool HasNativeFunction(IntPtr objectPtr, string functionName);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern object? GetNativeProperty(IntPtr objectPtr, string propertyName);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SetNativeProperty(IntPtr objectPtr, string propertyName, object value);
        
        /// <summary>
        /// Generic wrapper for getting native properties with type safety
        /// </summary>
        protected T? GetProperty<T>(string propertyName)
        {
            var result = GetNativeProperty(NativePtr, propertyName);
            return result is T typedResult ? typedResult : default(T);
        }
        
        /// <summary>
        /// Generic wrapper for setting native properties with type safety
        /// </summary>
        protected void SetProperty<T>(string propertyName, T value)
        {
            SetNativeProperty(NativePtr, propertyName, value);
        }
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GetNativeClass(IntPtr objectPtr);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern string? GetNativeName(IntPtr objectPtr);
        
        [DllImport("UCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool IsNativeValid(IntPtr objectPtr);
        
        #endregion
    }
    
    /// <summary>
    /// Registry for tracking C# UObject instances
    /// </summary>
    internal static class ObjectRegistry
    {
        private static readonly Dictionary<IntPtr, WeakReference> NativePtrToObject = new();
        private static readonly Dictionary<UObject, IntPtr> ObjectToNativePtr = new();
        private static readonly object RegistryLock = new();
        
        /// <summary>
        /// Register a C# object with its native pointer
        /// </summary>
        public static void RegisterObject(UObject obj)
        {
            lock (RegistryLock)
            {
                if (obj.InternalNativePtr != IntPtr.Zero)
                {
                    NativePtrToObject[obj.InternalNativePtr] = new WeakReference(obj);
                    ObjectToNativePtr[obj] = obj.InternalNativePtr;
                }
            }
        }
        
        /// <summary>
        /// Unregister a C# object
        /// </summary>
        public static void UnregisterObject(UObject obj)
        {
            lock (RegistryLock)
            {
                if (ObjectToNativePtr.TryGetValue(obj, out IntPtr nativePtr))
                {
                    NativePtrToObject.Remove(nativePtr);
                    ObjectToNativePtr.Remove(obj);
                }
            }
        }
        
        /// <summary>
        /// Get C# object from native pointer
        /// </summary>
        public static UObject? GetObject(IntPtr nativePtr)
        {
            lock (RegistryLock)
            {
                if (NativePtrToObject.TryGetValue(nativePtr, out WeakReference? weakRef))
                {
                    if (weakRef.Target is UObject obj && obj.IsValid())
                    {
                        return obj;
                    }
                    else
                    {
                        // Clean up dead reference
                        NativePtrToObject.Remove(nativePtr);
                    }
                }
            }
            return null;
        }
    }
}