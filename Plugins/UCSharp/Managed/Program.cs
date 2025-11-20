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
		public delegate int UCInitializeDelegate(IntPtr arg, int argSize);
        /// <summary>
        /// Initialize the C# scripting system
        /// Called from UE5 C++ code when the plugin starts
        /// </summary>
		public static void Initialize()
		{
			UCSharp.Core.ScriptManager.Initialize();
		}

		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static int InitializeUnmanaged(IntPtr arg, int argSize)
		{
			//try
			//{
			//	Initialize();
			//	return 0;
			//}
			//catch
			//{
			//	return -1;
			//}
			return 0;
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

		[UnmanagedCallersOnly]
		public static void Hello(IntPtr message)
		{
			Console.WriteLine($"Hello, world! from {nameof(Program)}");
		}
	}
}
