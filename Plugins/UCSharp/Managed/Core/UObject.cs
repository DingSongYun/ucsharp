using System;
using System.Runtime.InteropServices;

namespace UCSharp.Core
{
	public class UObject
	{
		public readonly IntPtr Handle;

		public UObject() => Handle = IntPtr.Zero;
		public UObject(IntPtr handle) => Handle = handle;

		[DllImport("UCSharp.Native", EntryPoint = "Native_SetIntProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int SetIntNative(IntPtr obj, uint propertyId, int value);
		[DllImport("UCSharp.Native", EntryPoint = "Native_GetIntProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int GetIntNative(IntPtr obj, uint propertyId, out int value);

		[DllImport("UCSharp.Native", EntryPoint = "Native_SetFloatProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int SetFloatNative(IntPtr obj, uint propertyId, float value);
		[DllImport("UCSharp.Native", EntryPoint = "Native_GetFloatProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int GetFloatNative(IntPtr obj, uint propertyId, out float value);

		[DllImport("UCSharp.Native", EntryPoint = "Native_SetBoolProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int SetBoolNative(IntPtr obj, uint propertyId, int value);
		[DllImport("UCSharp.Native", EntryPoint = "Native_GetBoolProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int GetBoolNative(IntPtr obj, uint propertyId, out int value);

		[DllImport("UCSharp.Native", EntryPoint = "Native_SetObjectProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int SetObjectNative(IntPtr obj, uint propertyId, IntPtr value);
		[DllImport("UCSharp.Native", EntryPoint = "Native_GetObjectProperty", CallingConvention = CallingConvention.StdCall)]
		private static extern int GetObjectNative(IntPtr obj, uint propertyId, out IntPtr value);

		[DllImport("UCSharp.Native", EntryPoint = "Native_SetStringProperty", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
		private static extern int SetStringNative(IntPtr obj, uint propertyId, string value);

		// ----  path wrappers ----
		public void SetInt(uint propertyId, int value)
			=> ThrowIfFailed(SetIntNative(Handle, propertyId, value), propertyId, "SetInt");

		public int GetInt(uint propertyId)
		{
			ThrowIfFailed(GetIntNative(Handle, propertyId, out var value), propertyId, "GetInt");
			return value;
		}

		public void SetFloat(uint propertyId, float value)
			=> ThrowIfFailed(SetFloatNative(Handle, propertyId, value), propertyId, "SetFloat");

		public float GetFloat(uint propertyId)
		{
			ThrowIfFailed(GetFloatNative(Handle, propertyId, out var value), propertyId, "GetFloat");
			return value;
		}

		public void SetBool(uint propertyId, bool value)
			=> ThrowIfFailed(SetBoolNative(Handle, propertyId, value ? 1 : 0), propertyId, "SetBool");

		public bool GetBool(uint propertyId)
		{
			ThrowIfFailed(GetBoolNative(Handle, propertyId, out var raw), propertyId, "GetBool");
			return raw != 0;
		}

		public void SetString(uint propertyId, string value)
			=> ThrowIfFailed(SetStringNative(Handle, propertyId, value ?? string.Empty), propertyId, "SetString");

		public IntPtr GetObject(uint propertyId)
		{
			ThrowIfFailed(GetObjectNative(Handle, propertyId, out var ptr), propertyId, "GetObject");
			return ptr;
		}

		public void SetObject(uint propertyId, IntPtr value)
			=> ThrowIfFailed(SetObjectNative(Handle, propertyId, value), propertyId, "SetObject");

		private static void ThrowIfFailed(int code, uint propertyId, string api)
		{
			if (code != 0)
			{
				throw new InvalidOperationException($"UCSharp {api} failed (PropertyId=0x{propertyId:X}, Code={code}).");
			}
		}
	}
}
