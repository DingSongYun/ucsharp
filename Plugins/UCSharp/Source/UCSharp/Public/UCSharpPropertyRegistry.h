#pragma once

#include "UCSharpInterop.h"
#include "UCSharpPropertyRegistry.h"
#include "CoreMinimal.h"

typedef uint32 FPropertyId;

namespace UCSharpInterop
{
struct FUSharpPropertyDesc;
};

struct UCSHARP_API FUCSharpPropertyRegistry
{
public:
	FUCSharpPropertyRegistry() = default;

	void RegisterProperty(UClass* Class, FPropertyId PropertyId, const FName& PropertyName);
	void UnregisterProperty(UClass* Class, FPropertyId PropertyId);

	UCSharpInterop::FUSharpPropertyDesc* FindProperty(UClass* Class, FPropertyId PropertyId);

	void Reset();

private:
	TMap<UClass*, TMap<FPropertyId, UCSharpInterop::FUSharpPropertyDesc>> Properties;

	mutable FRWLock RegistryLock;
};
