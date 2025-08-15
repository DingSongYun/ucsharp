# UCSharp Test Project

这是一个用于测试UCSharp插件的UE5测试项目。

## 项目结构

```
UCSharpTest/
├── UCSharpTest.uproject          # 主项目文件
├── Source/                       # C++源代码
│   ├── UCSharpTest/             # 主游戏模块
│   ├── UCSharpTestTarget.cs     # 游戏构建目标
│   └── UCSharpTestEditor.Target.cs # 编辑器构建目标
├── Config/                      # 项目配置文件
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   └── DefaultInput.ini
├── Content/                     # 游戏内容资源
│   └── Maps/
│       └── TestLevel.umap       # 测试关卡
└── GenerateProjectFiles.bat     # 项目文件生成脚本
```

## 使用说明

### 1. 前置条件

- 安装Unreal Engine 5.1或更高版本
- 安装Visual Studio 2019/2022（包含C++开发工具）
- 确保UCSharp插件已正确放置在项目根目录的上级目录中

### 2. 生成项目文件

#### 方法一：使用UE5编辑器（推荐）
1. 右键点击`UCSharpTest.uproject`文件
2. 选择"Generate Visual Studio project files"
3. 等待生成完成

#### 方法二：使用批处理脚本
1. 运行`GenerateProjectFiles.bat`
2. 按照提示操作

#### 方法三：手动使用UnrealBuildTool
```bash
"[UE5安装路径]\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="UCSharpTest.uproject" -game -rocket -progress
```

### 3. 编译项目

1. 打开生成的`UCSharpTest.sln`解决方案文件
2. 在Visual Studio中选择"Development Editor"配置
3. 构建解决方案（Ctrl+Shift+B）

### 4. 运行项目

1. 在Visual Studio中设置`UCSharpTest`为启动项目
2. 按F5运行，或者直接双击`UCSharpTest.uproject`文件

### 5. 测试UCSharp插件

1. 在UE5编辑器中，检查"工具"菜单下是否有"C# Scripting"选项
2. 尝试使用UCSharp插件的各项功能：
   - 打开C#编辑器
   - 创建C#脚本
   - 构建C#脚本
   - 重载C#程序集

## 插件集成

本测试项目已配置为使用UCSharp插件：

- 插件路径：`../`（相对于项目根目录）
- 模块依赖：项目已添加对`UCSharp`模块的依赖
- 构建配置：已配置适当的构建规则

## 故障排除

### 编译错误

1. **找不到UCSharp模块**
   - 确保UCSharp插件位于正确的相对路径（项目上级目录）
   - 检查插件的.uplugin文件是否存在且格式正确

2. **缺少.NET运行时**
   - 确保已安装.NET 6.0 Runtime
   - 检查UCSharp插件的托管代码是否已正确编译

3. **UE5路径问题**
   - 确认UE5安装路径
   - 更新GenerateProjectFiles.bat中的路径

### 运行时错误

1. **插件加载失败**
   - 检查输出日志中的错误信息
   - 确认所有依赖的DLL文件都存在

2. **C#脚本执行错误**
   - 查看UE5编辑器的输出日志
   - 检查C#代码的语法和逻辑错误

## 开发建议

1. **调试C++代码**：使用Visual Studio的调试功能
2. **调试C#代码**：通过UE5编辑器的日志输出进行调试
3. **性能分析**：使用UE5内置的性能分析工具
4. **版本控制**：建议将生成的项目文件添加到.gitignore中

## 下一步

完成基础测试后，可以：

1. 创建更复杂的C# Actor示例
2. 测试蓝图与C#的互操作
3. 验证性能和内存管理
4. 测试编辑器工具的功能

## 支持

如遇到问题，请检查：

1. UCSharp插件的文档
2. UE5官方文档
3. 项目的输出日志和错误信息