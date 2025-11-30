#pragma once

#include "CoreMinimal.h"

/**
 * CSharp 运行时
 */
class UCSHARP_API FUCSharpRuntime
{
public:
	/**
	 * 初始化.Net运行时
	 */
	bool Initialize();
	void Shutdown();
};
