#include "UCSharpPropertyRegistry.h"
#include "UCSharpInterop.h"
#include "UCSharpLogs.h"

using namespace UCSharpInterop;

EInteropType DeduceStorage(const FProperty& Property)
{
	if (Property.IsA<FIntProperty>())
	{
		return EInteropType::Int32;
	}
	if (Property.IsA<FFloatProperty>())
	{
		return EInteropType::Float;
	}
	if (Property.IsA<FBoolProperty>())
	{
		return EInteropType::Bool;
	}
	if (Property.IsA<FStrProperty>())
	{
		return EInteropType::String;
	}
	if (Property.IsA<FObjectProperty>())
	{
		return EInteropType::Object;
	}
	return EInteropType::Unsupported;
}

void FUCSharpPropertyRegistry::RegisterProperty(UClass* Class, FPropertyId PropertyId, const FName& PropertyName)
{
	if (PropertyId == 0 || Class == nullptr || PropertyName.IsNone())
	{
		UE_LOG(LogUCSharp, Warning, TEXT("Invalid property registration request. Id=%u Class=%s Property=%s"),
			PropertyId,
			Class ? *Class->GetName() : TEXT("<null>"),
			*PropertyName.ToString());
		return;
	}

	const FProperty* Property = Class->FindPropertyByName(PropertyName);
	if (Property == nullptr)
	{
		UE_LOG(LogUCSharp, Warning, TEXT("Failed to locate property %s on class %s for UCSharp fast-path registration."),
			*PropertyName.ToString(),
			*Class->GetName());
		return;
	}

	const EInteropType InteropType = DeduceStorage(*Property);
	if (InteropType == EInteropType::Unsupported)
	{
		UE_LOG(LogUCSharp, Warning, TEXT("UCSharp fast-path currently does not support property %s (%s)."),
			*PropertyName.ToString(),
			*Property->GetClass()->GetName());
		return;
	}

	FWriteScopeLock ScopeLock(RegistryLock);
	FUSharpPropertyDesc& PropertyDesc = Properties.FindOrAdd(Class).Add(PropertyId);
	PropertyDesc.PropertyId = PropertyId;
	PropertyDesc.Property = Property;
	PropertyDesc.Type = InteropType;
}

void FUCSharpPropertyRegistry::UnregisterProperty(UClass* Class, FPropertyId PropertyId)
{
	FReadScopeLock ScopeLock(RegistryLock);
	Properties.FindOrAdd(Class).Remove(PropertyId);
}

FUSharpPropertyDesc* FUCSharpPropertyRegistry::FindProperty(UClass* Class, FPropertyId PropertyId)
{
	FReadScopeLock ScopeLock(RegistryLock);
	if (auto ClassProperties = Properties.Find(Class))
	{
		return ClassProperties->Find(PropertyId);
	}
	return nullptr;
}

void FUCSharpPropertyRegistry::Reset()
{
	FWriteScopeLock ScopeLock(RegistryLock);
	Properties.Empty();
}
