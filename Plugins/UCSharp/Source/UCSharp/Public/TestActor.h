#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UCSHARP_API ATestActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	int32 Health = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	FString Label;

	UFUNCTION(BlueprintCallable, Category="Test")
	void SetHealth(int32 Value);

	UFUNCTION(BlueprintCallable, Category="Test")
	int32 GetHealth() const;

	UFUNCTION(BlueprintCallable, Category="Test")
	void SetSpeed(float Value);

	UFUNCTION(BlueprintCallable, Category="Test")
	float GetSpeed() const;

	virtual void BeginPlay() override;
};

