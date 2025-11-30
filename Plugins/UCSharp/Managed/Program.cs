using System;
using UCSharp.Core;
using System.Runtime.InteropServices;

namespace UCSharp
{
	/// <summary>
	/// Main entry point for the C# scripting system
	/// This class provides the interface between UE5 C++ and C# scripts
	/// </summary>
	public static class Program
	{
		/// <summary>
		/// Initialize the C# scripting system
		/// Called from UE5 C++ code when the plugin starts
		/// </summary>
		public static void Initialize()
		{
			Console.WriteLine("------------------------------------------");
			Console.WriteLine("* Hello C# !!!");
			Console.WriteLine("------------------------------------------");
			Console.WriteLine("\n\n");

			//UCSharp.Core.ScriptManager.Initialize();

			NativeLibrary.SetDllImportResolver(typeof(Program).Assembly, (libraryName, assembly, searchPath) =>
			{
				if (libraryName == "UCSharp.Native")
				{
					IntPtr h;
					if (NativeLibrary.TryLoad("UnrealEditor-UCSharp", out h)) return h;
					if (NativeLibrary.TryLoad("UCSharp", out h)) return h;
				}
				return IntPtr.Zero;
			});

			var r = NativeAdd(2, 3);
			Console.WriteLine($"NativeAdd(2,3) = {r}");
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static int InitializeUnmanaged(IntPtr arg, int argSize)
		{
			try
			{
				Initialize();
				return 0;
			}
			catch
			{
				return -1;
			}
			return 0;
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static int InvokeStaticAdd(int a, int b)
		{
			return UCSharp.Examples.Math.Add(a, b);
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static void SetActorHealth(IntPtr owner, int value)
		{
			ActorState.SetHealth(owner, value);
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static void SetActorSpeed(IntPtr owner, float value)
		{
			ActorState.SetSpeed(owner, value);
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static void SetActorActive(IntPtr owner, int value)
		{
			ActorState.SetActive(owner, value != 0);
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static void SetActorLabel(IntPtr owner, IntPtr wcharStr)
		{
			var s = Marshal.PtrToStringUni(wcharStr) ?? string.Empty;
			ActorState.SetLabel(owner, s);
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static int GetActorHealth(IntPtr owner)
		{
			return ActorState.Get(owner).Health;
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static float GetActorSpeed(IntPtr owner)
		{
			return ActorState.Get(owner).Speed;
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static int GetActorActive(IntPtr owner)
		{
			return ActorState.Get(owner).Active ? 1 : 0;
		}

		/// <summary>
		/// Shutdown the C# scripting system
		/// Called from UE5 C++ code when the plugin shuts down
		/// </summary>
		public static void Shutdown()
		{
		}

		/// <summary>
		/// Main method for standalone testing (not used when called from UE5)
		/// </summary>
		/// <param name="args">Command line arguments</param>
		public static void Main(string[] args)
		{
			Console.WriteLine("UCSharp C# Scripting System - Standalone Test Mode");
			Console.WriteLine("Note: This is for testing only. In production, this will be called from UE5.");
			Console.WriteLine();
		}

		[DllImport("UCSharp.Native", EntryPoint = "UCSharp_NativeAdd", CallingConvention = CallingConvention.StdCall)]
		private static extern int NativeAdd(int a, int b);
	}
}
