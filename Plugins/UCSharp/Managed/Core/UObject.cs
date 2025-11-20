using System;

namespace UCSharp.Core
{
	public class UObject
	{
		public virtual void BeginPlay() {}
		public virtual void Tick(float deltaTime) {}
		public virtual void EndPlay() {}
	}
}
