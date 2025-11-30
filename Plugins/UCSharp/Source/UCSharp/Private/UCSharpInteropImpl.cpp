#include "UCSharpInterop.h"
#include "Engine/Engine.h"
#include "TestActor.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "UCSharpLogs.h"
#include "UObject/UnrealType.h"
#include "UObject/PropertyAccessUtil.h"
#include "UCSharpPropertyRegistry.h"

using namespace UCSharpInterop;


namespace
{
	constexpr uint32 HashLiteral(const TCHAR* Text, uint32 Hash = 2166136261u)
	{
		return (*Text == 0) ? Hash : HashLiteral(Text + 1, (Hash ^ uint32(*Text)) * 16777619u);
	}

	constexpr uint32 MakePropertyId(const TCHAR* ClassName, const TCHAR* PropertyName)
	{
		return HashLiteral(PropertyName, HashLiteral(ClassName));
	}

	void RegisterPropertyMetadata()
	{
		IUCSharpInterop* Interop = UCSharpInterop::GetInterop();
		Interop->GetPropertyRegistry().RegisterProperty(
			ATestActor::StaticClass(),
			MakePropertyId(TEXT("ATestActor"), TEXT("Health")),
			TEXT("Health"));

		Interop->GetPropertyRegistry().RegisterProperty(
			ATestActor::StaticClass(),
			MakePropertyId(TEXT("ATestActor"), TEXT("Speed")),
			TEXT("Speed"));

		Interop->GetPropertyRegistry().RegisterProperty(
			ATestActor::StaticClass(),
			MakePropertyId(TEXT("ATestActor"), TEXT("Label")),
			TEXT("Label"));
	}
}


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
	virtual FUCSharpPropertyRegistry& GetPropertyRegistry() { return PropertyRegistry; }

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

	FUCSharpPropertyRegistry PropertyRegistry;

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

	RegisterPropertyMetadata();
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

extern "C" UCSHARP_API int32 __stdcall UCSharp_NativeAdd(int32 A, int32 B)
{
	return A + B;
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

template <typename PropertyType, typename ValueType>
int32 SetFastValue(UObject* Obj, uint32 PropertyId, ValueType InValue, EInteropType ExpectedType)
{
	if (!Obj)
	{
		return -1;
	}

	IUCSharpInterop* Interop = GetInterop();
	check(Interop);
	const FUSharpPropertyDesc* Desc = Interop->GetPropertyRegistry().FindProperty(Obj->GetClass(), PropertyId);
	if (!Desc || Desc->Type != ExpectedType)
	{
		return -2;
	}

	const PropertyType* TypedProperty = CastField<PropertyType>(Desc->Property);
	if (!TypedProperty)
	{
		return -3;
	}

	void* Dest = TypedProperty->ContainerPtrToValuePtr<void>(Obj);
	TypedProperty->SetPropertyValue(Dest, InValue);
	return 0;
}

template <typename PropertyType, typename ValueType>
int32 GetFastValue(UObject* Obj, uint32 PropertyId, ValueType& OutValue, EInteropType ExpectedType)
{
	if (!Obj)
	{
		return -1;
	}

	IUCSharpInterop* Interop = GetInterop();
	check(Interop);
	const FUSharpPropertyDesc* Desc = Interop->GetPropertyRegistry().FindProperty(Obj->GetClass(), PropertyId);
	if (!Desc || Desc->Type != ExpectedType)
	{
		return -2;
	}

	const PropertyType* TypedProperty = CastField<PropertyType>(Desc->Property);
	if (!TypedProperty)
	{
		return -3;
	}

	void* Src = TypedProperty->ContainerPtrToValuePtr<void>(Obj);
	OutValue = TypedProperty->GetPropertyValue(Src);
	return 0;
}

extern "C" UCSHARP_API int32 __stdcall Native_SetIntProperty(UObject* Obj, uint32 PropertyId, int32 Value)
{
	return SetFastValue<FIntProperty>(Obj, PropertyId, Value, EInteropType::Int32);
}

extern "C" UCSHARP_API int32 __stdcall Native_GetIntProperty(UObject* Obj, uint32 PropertyId, int32* OutValue)
{
	if (!OutValue)
	{
		return -10;
	}
	return GetFastValue<FIntProperty>(Obj, PropertyId, *OutValue, EInteropType::Int32);
}

extern "C" UCSHARP_API int32 __stdcall Native_SetFloatProperty(UObject* Obj, uint32 PropertyId, float Value)
{
	return SetFastValue<FFloatProperty>(Obj, PropertyId, Value, EInteropType::Float);
}

extern "C" UCSHARP_API int32 __stdcall Native_GetFloatProperty(UObject* Obj, uint32 PropertyId, float* OutValue)
{
	if (!OutValue)
	{
		return -10;
	}
	return GetFastValue<FFloatProperty>(Obj, PropertyId, *OutValue, EInteropType::Float);
}

extern "C" UCSHARP_API int32 __stdcall Native_SetBoolProperty(UObject* Obj, uint32 PropertyId, int32 Value)
{
	const bool bValue = Value != 0;
	return SetFastValue<FBoolProperty>(Obj, PropertyId, bValue, EInteropType::Bool);
}

extern "C" UCSHARP_API int32 __stdcall Native_GetBoolProperty(UObject* Obj, uint32 PropertyId, int32* OutValue)
{
	if (!OutValue)
	{
		return -10;
	}

	bool bValue = false;
	const int32 Result = GetFastValue<FBoolProperty>(Obj, PropertyId, bValue, EInteropType::Bool);
	if (Result == 0)
	{
		*OutValue = bValue ? 1 : 0;
	}
	return Result;
}

extern "C" UCSHARP_API int32 __stdcall Native_SetStringProperty(UObject* Obj, uint32 PropertyId, const wchar_t* Value)
{
	FString Temp(Value ? Value : TEXT(""));
	return SetFastValue<FStrProperty>(Obj, PropertyId, Temp, EInteropType::String);
}

extern "C" UCSHARP_API int32 __stdcall Native_SetObjectProperty(UObject* Obj, uint32 PropertyId, UObject* Value)
{
	return SetFastValue<FObjectProperty>(Obj, PropertyId, Value, EInteropType::Object);
}

extern "C" UCSHARP_API int32 __stdcall Native_GetObjectProperty(UObject* Obj, uint32 PropertyId, UObject** OutValue)
{
	if (!OutValue)
	{
		return -10;
	}
	return GetFastValue<FObjectProperty>(Obj, PropertyId, *OutValue, EInteropType::Object);
}


extern "C" UCSHARP_API int32 __stdcall Native_CallFunction(UObject* Obj, const wchar_t* FuncName, const void* ArgsData, const int32* ArgTypes, const int32* ArgSizes, int32 ArgCount, void* RetData, int32 RetType, int32 RetSize)
{
	if (!Obj || !FuncName) return -1;
	UFunction* Func = Obj->FindFunction(FName(FuncName));
	if (!Func) return -2;
	uint8* Params = (uint8*)FMemory::Malloc(Func->ParmsSize);
	FMemory::Memzero(Params, Func->ParmsSize);
	const uint8* Cursor = (const uint8*)ArgsData;
	int32 Index = 0;
	for (TFieldIterator<FProperty> It(Func); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Parm) || Prop->HasAnyPropertyFlags(CPF_ReturnParm)) continue;
		void* Dest = Prop->ContainerPtrToValuePtr<void>(Params);
		if (Index >= ArgCount) { FMemory::Free(Params); return -3; }
		int32 Sz = ArgSizes[Index];
		int32 Tc = ArgTypes[Index];
		if (CastField<FIntProperty>(Prop)) { *reinterpret_cast<int32*>(Dest) = *reinterpret_cast<const int32*>(Cursor); }
		else if (CastField<FFloatProperty>(Prop)) { *reinterpret_cast<float*>(Dest) = *reinterpret_cast<const float*>(Cursor); }
		else if (CastField<FBoolProperty>(Prop)) { bool V = (*reinterpret_cast<const int32*>(Cursor)) != 0; *reinterpret_cast<uint8*>(Dest) = V ? 1 : 0; }
		else if (auto PObj = CastField<FObjectProperty>(Prop)) { UObject* V = *reinterpret_cast<UObject* const*>(Cursor); PObj->SetObjectPropertyValue(Dest, V); }
		else if (auto PStr = CastField<FStrProperty>(Prop)) { const wchar_t* S = reinterpret_cast<const wchar_t*>(Cursor); FString FS(S); PStr->SetPropertyValue(Dest, FS); }
		else { FMemory::Free(Params); return -4; }
		Cursor += Sz;
		Index++;
	}
	Obj->ProcessEvent(Func, Params);
	for (TFieldIterator<FProperty> It(Func); It; ++It)
	{
		FProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			void* Src = Prop->ContainerPtrToValuePtr<void>(Params);
			if (CastField<FIntProperty>(Prop)) { if (RetData) *reinterpret_cast<int32*>(RetData) = *reinterpret_cast<int32*>(Src); }
			else if (CastField<FFloatProperty>(Prop)) { if (RetData) *reinterpret_cast<float*>(RetData) = *reinterpret_cast<float*>(Src); }
			else if (CastField<FBoolProperty>(Prop)) { if (RetData) *reinterpret_cast<int32*>(RetData) = (*reinterpret_cast<uint8*>(Src)) ? 1 : 0; }
			else if (auto PObj = CastField<FObjectProperty>(Prop)) { if (RetData) *reinterpret_cast<UObject**>(RetData) = PObj->GetObjectPropertyValue(Src); }
			else { }
			break;
		}
	}
	FMemory::Free(Params);
	return 0;
}
