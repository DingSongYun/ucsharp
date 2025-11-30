using System;
using UCSharp.Core;
using System.Runtime.InteropServices;

namespace UCSharp.Examples
{
	public class CS_TestActor
	{
		[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvStdcall) })]
		public static void OnBeginPlay(IntPtr owner)
		{
			var uobj = new UObject(owner);
			uobj.SetInt("Health", 100);
			uobj.SetFloat("Speed", 3.5f);
			uobj.SetString("Label", "ManagedBound");
		}
	}
}
