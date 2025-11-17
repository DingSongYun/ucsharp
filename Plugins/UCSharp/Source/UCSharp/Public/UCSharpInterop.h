#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "UObject/Object.h"

// Forward declarations
class UObject;

/**
 * Core interop types for C++/C# communication
 */
namespace UCSharpInterop
{
	/**
	 * Handle type for managed C# objects
	 */
	typedef void* ManagedObjectHandle;

	/**
	 * Handle type for native C++ objects
	 */
	typedef void* NativeObjectHandle;

	/**
	 * Function pointer type for C# method calls
	 */
	typedef void* (*ManagedMethodPtr)(void* instance, void** args, int32 argCount);

	/**
	 * Function pointer type for native method calls
	 */
	typedef void* (*NativeMethodPtr)(void* instance, void** args, int32 argCount);

	/**
	 * Basic type enumeration for marshaling
	 */
	enum class EInteropType : uint8
	{
		Void,
		Bool,
		Int8,
		UInt8,
		Int16,
		UInt16,
		Int32,
		UInt32,
		Int64,
		UInt64,
		Float,
		Double,
		String,
		Object,
		Array,
		Struct
	};

	/**
	 * Type information for marshaling
	 */
	struct FTypeInfo
	{
		EInteropType Type;
		FString TypeName;
		int32 Size;
		bool bIsPointer;
		bool bIsReference;

		FTypeInfo()
			: Type(EInteropType::Void)
			, Size(0)
			, bIsPointer(false)
			, bIsReference(false)
		{}

		FTypeInfo(EInteropType InType, const FString& InTypeName, int32 InSize, bool bInIsPointer = false, bool bInIsReference = false)
			: Type(InType)
			, TypeName(InTypeName)
			, Size(InSize)
			, bIsPointer(bInIsPointer)
			, bIsReference(bInIsReference)
		{}
	};

	/**
	 * Method signature information
	 */
	struct FMethodSignature
	{
		FString MethodName;
		FTypeInfo ReturnType;
		TArray<FTypeInfo> Parameters;
		bool bIsStatic;

		FMethodSignature()
			: bIsStatic(false)
		{}
	};

	/**
	 * Object binding information
	 */
	struct FObjectBinding
	{
		NativeObjectHandle NativeHandle;
		ManagedObjectHandle ManagedHandle;
		FString ClassName;
		bool bIsValid;

		FObjectBinding()
			: NativeHandle(nullptr)
			, ManagedHandle(nullptr)
			, bIsValid(false)
		{}
	};
}

/**
 * Core interop interface for C++/C# communication
 */
class UCSHARP_API IUCSharpInterop
{
public:
	virtual ~IUCSharpInterop() = default;

	/**
	 * Initialize the interop system
	 */
	virtual bool Initialize() = 0;

	/**
	 * Shutdown the interop system
	 */
	virtual void Shutdown() = 0;

	/**
	 * Check if the interop system is initialized
	 */
	virtual bool IsInitialized() const = 0;

	/**
	 * Create a managed object from a native object
	 */
	virtual UCSharpInterop::ManagedObjectHandle CreateManagedObject(UObject* NativeObject) = 0;

	/**
	 * Get native object from managed handle
	 */
	virtual UObject* GetNativeObject(UCSharpInterop::ManagedObjectHandle ManagedHandle) = 0;

	/**
	 * Register a type mapping
	 */
	virtual bool RegisterTypeMapping(const FString& NativeTypeName, const FString& ManagedTypeName) = 0;

	/**
	 * Call a managed method
	 */
	virtual void* CallManagedMethod(UCSharpInterop::ManagedObjectHandle Instance, const FString& MethodName, void** Args, int32 ArgCount) = 0;

	/**
	 * Call a native method from managed code
	 */
	virtual void* CallNativeMethod(UCSharpInterop::NativeObjectHandle Instance, const FString& MethodName, void** Args, int32 ArgCount) = 0;
};

/**
 * Global interop access functions
 */
namespace UCSharpInterop
{
	/**
	 * Get the global interop instance
	 */
	UCSHARP_API IUCSharpInterop* GetInterop();

	/**
	 * Destroy the global interop instance
	 */
	UCSHARP_API void DestroyInterop();
}