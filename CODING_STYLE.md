# UCSharp 项目代码风格指南

本项目遵循虚幻引擎官方代码风格规范，以确保代码的一致性和可维护性。

## 1. 命名约定

### 1.1 类命名
- **模块类**: 以 `F` 开头，如 `FUCSharpModule`、`FUCSharpEditorModule`
- **UObject 派生类**: 以 `U` 开头，如 `UMyActor`
- **Actor 派生类**: 以 `A` 开头，如 `AMyGameMode`
- **Slate Widget**: 以 `S` 开头，如 `SMyWidget`
- **接口**: 以 `I` 开头，如 `IMyInterface`
- **枚举**: 以 `E` 开头，如 `EMyEnum`
- **结构体**: 以 `F` 开头，如 `FMyStruct`

### 1.2 变量命名
- **成员变量**: 使用 `PascalCase`，如 `bCSharpRuntimeInitialized`
- **布尔变量**: 以 `b` 开头，如 `bIsInitialized`
- **指针变量**: 可选择性地以类型前缀开头，如 `RuntimeHandle`
- **局部变量**: 使用 `PascalCase`，如 `ModuleName`
- **参数**: 使用 `PascalCase`，如 `InModuleName`

### 1.3 函数命名
- 使用 `PascalCase`，如 `StartupModule()`、`InitializeCSharpRuntime()`
- 布尔返回函数可以 `Is`、`Has`、`Can` 开头，如 `IsCSharpRuntimeInitialized()`

### 1.4 常量和宏
- 全大写，使用下划线分隔，如 `LOCTEXT_NAMESPACE`
- 日志类别：`LogUCSharp`、`LogUCSharpEditor`

## 2. 文件组织

### 2.1 头文件结构
```cpp
#pragma once

#include "CoreMinimal.h"
// 其他系统包含
// 项目包含

// 前向声明
class FMyClass;

// 日志声明
DECLARE_LOG_CATEGORY_EXTERN(LogUCSharp, Log, All);

/**
 * 类文档注释
 * 详细描述类的用途
 */
class UCSHARP_API FMyClass
{
    // 类内容
};
```

### 2.2 源文件结构
```cpp
#include "MyClass.h"
#include "Core.h"
// 其他包含文件

// 日志定义
DEFINE_LOG_CATEGORY(LogUCSharp);

// 本地化命名空间
#define LOCTEXT_NAMESPACE "FMyModule"

// 实现代码

#undef LOCTEXT_NAMESPACE
```

## 3. 代码格式

### 3.1 缩进和空格
- 使用 **Tab** 进行缩进（虚幻引擎标准）
- 函数参数列表中逗号后加空格
- 操作符前后加空格：`a = b + c`

### 3.2 大括号风格
- 使用 **Allman 风格**（大括号独占一行）：
```cpp
void FMyClass::MyFunction()
{
    if (Condition)
    {
        // 代码
    }
    else
    {
        // 代码
    }
}
```

### 3.3 行长度
- 建议每行不超过 120 字符
- 长参数列表可以换行对齐

## 4. 注释规范

### 4.1 文档注释
使用 `/** */` 格式为公共 API 编写文档：
```cpp
/**
 * Initialize the C# runtime
 * @return true if initialization succeeded, false otherwise
 */
bool InitializeCSharpRuntime();
```

### 4.2 行内注释
- 使用 `//` 进行行内注释
- 重要的代码段使用 `/* */` 块注释
- TODO 注释格式：`// TODO: 描述需要完成的工作`

## 5. 包含文件顺序

1. 对应的头文件（.cpp 文件中）
2. 虚幻引擎核心头文件
3. 虚幻引擎其他模块头文件
4. 第三方库头文件
5. 项目内其他头文件

示例：
```cpp
#include "UCSharp.h"                    // 对应头文件
#include "Core.h"                       // UE 核心
#include "Modules/ModuleManager.h"       // UE 其他模块
#include "Interfaces/IPluginManager.h"  // UE 接口
// 第三方库（如果有）
// 项目内其他头文件
```

## 6. 错误处理

### 6.1 日志记录
使用虚幻引擎日志系统：
```cpp
UE_LOG(LogUCSharp, Log, TEXT("信息日志"));
UE_LOG(LogUCSharp, Warning, TEXT("警告日志"));
UE_LOG(LogUCSharp, Error, TEXT("错误日志"));
```

### 6.2 断言
使用虚幻引擎断言宏：
```cpp
check(Condition);           // 发布版本中也会检查
checkSlow(Condition);       // 仅在调试版本中检查
ensure(Condition);          // 记录错误但继续执行
```

## 7. 内存管理

- 优先使用虚幻引擎的智能指针：`TSharedPtr`、`TWeakPtr`、`TUniquePtr`
- 对于 UObject 派生类，使用 `TObjectPtr` 或原始指针
- 避免使用 `new`/`delete`，使用 `MakeShared` 或虚幻引擎内存分配器

## 8. 平台兼容性

使用虚幻引擎的平台抽象：
```cpp
#if PLATFORM_WINDOWS
    // Windows 特定代码
#elif PLATFORM_MAC
    // Mac 特定代码
#endif
```

## 9. 本地化

使用虚幻引擎本地化系统：
```cpp
#define LOCTEXT_NAMESPACE "FUCSharpModule"

FText DisplayText = LOCTEXT("KeyName", "Default Text");
FText FormattedText = FText::Format(LOCTEXT("Format", "Value: {0}"), FText::AsNumber(Value));

#undef LOCTEXT_NAMESPACE
```

## 10. 性能考虑

### 10.1 内存分配
- 避免在游戏循环中频繁分配内存
- 使用对象池模式重用对象
- 优先使用栈分配而非堆分配

### 10.2 算法复杂度
- 选择合适的数据结构和算法
- 避免不必要的循环嵌套
- 使用缓存友好的数据布局

### 10.3 编译优化
- 在头文件中使用前向声明而不是包含
- 避免在头文件中包含大型头文件
- 使用 `const` 和 `constexpr` 适当地标记不可变数据
- 优先使用引用传递大型对象

## 11. 代码风格检查清单

### 11.1 命名检查
- [ ] 类名使用PascalCase，以F、U、A等前缀开头
- [ ] 函数名使用PascalCase
- [ ] 变量名使用camelCase，成员变量以b、n、f等前缀开头
- [ ] 常量使用UPPER_CASE
- [ ] 枚举值使用PascalCase

### 11.2 格式检查
- [ ] 使用Tab缩进（4个空格宽度）
- [ ] 大括号使用Allman风格
- [ ] 每行代码不超过120个字符
- [ ] 函数参数过多时换行对齐

### 11.3 注释检查
- [ ] 公共API有完整的文档注释
- [ ] 复杂逻辑有行内注释说明
- [ ] TODO注释包含负责人和时间
- [ ] 文件头包含版权信息

### 11.4 包含文件检查
- [ ] 头文件按正确顺序排列
- [ ] 使用前置声明减少依赖
- [ ] 避免循环包含
- [ ] 使用#pragma once

## 12. 自动化工具

### 12.1 代码格式化
建议使用以下工具保持代码风格一致：
- **UnrealBuildTool**: 自动检查模块依赖
- **clang-format**: 自动格式化C++代码
- **Visual Studio**: 配置代码风格设置

### 12.2 静态分析
- 启用编译器警告（/W4 或 -Wall）
- 使用静态分析工具检查潜在问题
- 定期运行代码质量检查

---

**注意**: 本风格指南基于虚幻引擎 5.6 的官方编码标准。所有团队成员都应遵循这些规范以确保代码质量和一致性。