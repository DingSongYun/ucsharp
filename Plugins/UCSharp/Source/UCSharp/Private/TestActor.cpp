#include "TestActor.h"
#include "UCSharpRuntime.h"

void ATestActor::SetHealth(int32 Value)
{
	Health = Value;
}

int32 ATestActor::GetHealth() const
{
	return Health;
}

void ATestActor::SetSpeed(float Value)
{
	Speed = Value;
}

float ATestActor::GetSpeed() const
{
	return Speed;
}

void ATestActor::BeginPlay()
{
	Super::BeginPlay();
}

