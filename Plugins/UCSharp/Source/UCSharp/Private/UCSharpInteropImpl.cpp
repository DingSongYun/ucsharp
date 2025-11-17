#include "UCSharpInterop.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "UCSharpLogs.h"

/**
 * Default implementation of the UCSharp interop interface
 */
class FUCSharpInteropImpl : public IUCSharpInterop
{
public:
	FUCSharpInteropImpl();
	virtual ~FUCSharpInteropImpl();

	// IUCSharpInterop interface
	virtual bool Initialize() override;
	virtual void Shutdown() override;
	virtual bool IsInitialized() const override { return bIsInitialized; }
	virtual UCSharpInterop::ManagedObjectHandle CreateManagedObject(UObject* NativeObject) override;
	virtual UObject* GetNativeObject(UCSharpInterop::ManagedObjectHandle ManagedHandle) override;
	virtual bool RegisterTypeMapping(const FString& NativeTypeName, const FString& ManagedTypeName) override;
	virtual void* CallManagedMethod(UCSharpInterop::ManagedObjectHandle Instance, const FString& MethodName, void** Args, int32 ArgCount) override;
	virtual void* CallNativeMethod(UCSharpInterop::NativeObjectHandle Instance, const FString& MethodName, void** Args, int32 ArgCount) override;

private:
	/** Whether the interop system is initialized */
	bool bIsInitialized;

	/** Map of native objects to managed handles */
	TMap<UObject*, UCSharpInterop::ManagedObjectHandle> NativeToManagedMap;

	/** Map of managed handles to native objects */
	TMap<UCSharpInterop::ManagedObjectHandle, UObject*> ManagedToNativeMap;

	/** Type mapping registry */
	TMap<FString, FString> TypeMappings;

	/** Object binding registry */
	TArray<UCSharpInterop::FObjectBinding> ObjectBindings;

	/** Next available handle ID */
	uint64 NextHandleId;

	/**
	 * Generate a new managed handle
	 */
	UCSharpInterop::ManagedObjectHandle GenerateHandle();

	/**
	 * Validate a managed handle
	 */
	bool IsValidHandle(UCSharpInterop::ManagedObjectHandle Handle) const;

	/**
	 * Clean up invalid object bindings
	 */
	void CleanupInvalidBindings();
};

FUCSharpInteropImpl::FUCSharpInteropImpl()
	: bIsInitialized(false)
	, NextHandleId(1)
{
}

FUCSharpInteropImpl::~FUCSharpInteropImpl()
{
	Shutdown();
}

bool FUCSharpInteropImpl::Initialize()
{
	if (bIsInitialized)
	{
		return true;
	}

	UE_LOG(LogUCSharp, Log, TEXT("Initializing UCSharp interop system..."));

	// Initialize basic type mappings
	RegisterTypeMapping(TEXT("bool"), TEXT("System.Boolean"));
	RegisterTypeMapping(TEXT("int8"), TEXT("System.SByte"));
	RegisterTypeMapping(TEXT("uint8"), TEXT("System.Byte"));
	RegisterTypeMapping(TEXT("int16"), TEXT("System.Int16"));
	RegisterTypeMapping(TEXT("uint16"), TEXT("System.UInt16"));
	RegisterTypeMapping(TEXT("int32"), TEXT("System.Int32"));
	RegisterTypeMapping(TEXT("uint32"), TEXT("System.UInt32"));
	RegisterTypeMapping(TEXT("int64"), TEXT("System.Int64"));
	RegisterTypeMapping(TEXT("uint64"), TEXT("System.UInt64"));
	RegisterTypeMapping(TEXT("float"), TEXT("System.Single"));
	RegisterTypeMapping(TEXT("double"), TEXT("System.Double"));
	RegisterTypeMapping(TEXT("FString"), TEXT("System.String"));
	RegisterTypeMapping(TEXT("UObject"), TEXT("UnrealEngine.UObject"));

	bIsInitialized = true;
	UE_LOG(LogUCSharp, Log, TEXT("UCSharp interop system initialized successfully"));
	return true;
}

void FUCSharpInteropImpl::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	UE_LOG(LogUCSharp, Log, TEXT("Shutting down UCSharp interop system..."));

	// Clear all mappings and bindings
	NativeToManagedMap.Empty();
	ManagedToNativeMap.Empty();
	TypeMappings.Empty();
	ObjectBindings.Empty();

	bIsInitialized = false;
	UE_LOG(LogUCSharp, Log, TEXT("UCSharp interop system shut down"));
}

UCSharpInterop::ManagedObjectHandle FUCSharpInteropImpl::CreateManagedObject(UObject* NativeObject)
{
	if (!bIsInitialized || !IsValid(NativeObject))
	{
		return nullptr;
	}

	// Check if we already have a mapping for this object
	if (UCSharpInterop::ManagedObjectHandle* ExistingHandle = NativeToManagedMap.Find(NativeObject))
	{
		return *ExistingHandle;
	}

	// Generate a new handle
	UCSharpInterop::ManagedObjectHandle NewHandle = GenerateHandle();

	// Create the mapping
	NativeToManagedMap.Add(NativeObject, NewHandle);
	ManagedToNativeMap.Add(NewHandle, NativeObject);

	// Create object binding
	UCSharpInterop::FObjectBinding Binding;
	Binding.NativeHandle = NativeObject;
	Binding.ManagedHandle = NewHandle;
	Binding.ClassName = NativeObject->GetClass()->GetName();
	Binding.bIsValid = true;
	ObjectBindings.Add(Binding);

	UE_LOG(LogUCSharp, VeryVerbose, TEXT("Created managed object handle for %s (Handle: %p)"), 
		*NativeObject->GetClass()->GetName(), NewHandle);

	return NewHandle;
}

UObject* FUCSharpInteropImpl::GetNativeObject(UCSharpInterop::ManagedObjectHandle ManagedHandle)
{
	if (!bIsInitialized || !IsValidHandle(ManagedHandle))
	{
		return nullptr;
	}

	if (UObject** NativeObject = ManagedToNativeMap.Find(ManagedHandle))
	{
		// Verify the object is still valid
		if (IsValid(*NativeObject))
		{
			return *NativeObject;
		}
		else
		{
			// Object has been garbage collected, clean up the mapping
			ManagedToNativeMap.Remove(ManagedHandle);
			NativeToManagedMap.Remove(*NativeObject);
			UE_LOG(LogUCSharp, Warning, TEXT("Native object for handle %p has been garbage collected"), ManagedHandle);
		}
	}

	return nullptr;
}

bool FUCSharpInteropImpl::RegisterTypeMapping(const FString& NativeTypeName, const FString& ManagedTypeName)
{
	if (!bIsInitialized)
	{
		return false;
	}

	TypeMappings.Add(NativeTypeName, ManagedTypeName);
	UE_LOG(LogUCSharp, VeryVerbose, TEXT("Registered type mapping: %s -> %s"), *NativeTypeName, *ManagedTypeName);
	return true;
}

void* FUCSharpInteropImpl::CallManagedMethod(UCSharpInterop::ManagedObjectHandle Instance, const FString& MethodName, void** Args, int32 ArgCount)
{
	if (!bIsInitialized)
	{
		return nullptr;
	}

	// TODO: Implement actual managed method calling
	// This is a placeholder implementation
	UE_LOG(LogUCSharp, Warning, TEXT("CallManagedMethod not yet implemented: %s"), *MethodName);
	return nullptr;
}

void* FUCSharpInteropImpl::CallNativeMethod(UCSharpInterop::NativeObjectHandle Instance, const FString& MethodName, void** Args, int32 ArgCount)
{
	if (!bIsInitialized)
	{
		return nullptr;
	}

	// TODO: Implement actual native method calling
	// This is a placeholder implementation
	UE_LOG(LogUCSharp, Warning, TEXT("CallNativeMethod not yet implemented: %s"), *MethodName);
	return nullptr;
}

UCSharpInterop::ManagedObjectHandle FUCSharpInteropImpl::GenerateHandle()
{
	return reinterpret_cast<UCSharpInterop::ManagedObjectHandle>(NextHandleId++);
}

bool FUCSharpInteropImpl::IsValidHandle(UCSharpInterop::ManagedObjectHandle Handle) const
{
	return Handle != nullptr && ManagedToNativeMap.Contains(Handle);
}

void FUCSharpInteropImpl::CleanupInvalidBindings()
{
	// Remove bindings for objects that have been garbage collected
	for (int32 i = ObjectBindings.Num() - 1; i >= 0; --i)
	{
		UCSharpInterop::FObjectBinding& Binding = ObjectBindings[i];
		UObject* NativeObject = static_cast<UObject*>(Binding.NativeHandle);
		
		if (!IsValid(NativeObject))
		{
			// Clean up mappings
			ManagedToNativeMap.Remove(Binding.ManagedHandle);
			NativeToManagedMap.Remove(NativeObject);
			
			// Remove binding
			ObjectBindings.RemoveAt(i);
			
			UE_LOG(LogUCSharp, VeryVerbose, TEXT("Cleaned up invalid binding for %s"), *Binding.ClassName);
		}
	}
}

// Global interop instance
static TUniquePtr<FUCSharpInteropImpl> GUCSharpInterop;

// Public interface functions
namespace UCSharpInterop
{
	IUCSharpInterop* GetInterop()
	{
		if (!GUCSharpInterop.IsValid())
		{
			GUCSharpInterop = MakeUnique<FUCSharpInteropImpl>();
		}
		return GUCSharpInterop.Get();
	}

	void DestroyInterop()
	{
		if (GUCSharpInterop.IsValid())
		{
			GUCSharpInterop->Shutdown();
			GUCSharpInterop.Reset();
		}
	}
}
