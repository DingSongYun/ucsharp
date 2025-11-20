using System;

namespace UCSharp.Core
{
	public class Actor : UObject
	{
		public bool IsActive { get; private set; } = false;

		public override void BeginPlay()
		{
			IsActive = true;
		}

		public override void Tick(float deltaTime)
		{
		}

		public override void EndPlay()
		{
			IsActive = false;
		}
	}
}
