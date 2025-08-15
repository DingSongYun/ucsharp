# UE5 C#插件调试和开发工具支持设计

## 1. 调试工具架构概述

### 1.1 调试系统整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    调试工具架构图                               │
├─────────────────────────────────────────────────────────────────┤
│  开发环境集成 (IDE Integration)                                │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  Visual Studio  │  │   VS Code       │  │   Rider         │ │
│  │  - 断点调试     │  │   - 语法高亮    │  │   - 智能提示    │ │
│  │  - 变量监视     │  │   - 代码补全    │  │   - 重构工具    │ │
│  │  - 调用堆栈     │  │   - 调试支持    │  │   - 性能分析    │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  调试协议层 (Debug Protocol Layer)                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  DAP适配器      │  │  调试服务器     │  │  通信协议       │ │
│  │  - 协议转换     │  │  - 会话管理     │  │  - TCP/WebSocket│ │
│  │  - 消息路由     │  │  - 状态同步     │  │  - JSON-RPC     │ │
│  │  - 事件处理     │  │  - 命令执行     │  │  - 消息序列化   │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  运行时调试引擎 (Runtime Debug Engine)                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  断点管理器     │  │  变量检查器     │  │  执行控制器     │ │
│  │  - 断点设置     │  │  - 变量枚举     │  │  - 单步执行     │ │
│  │  - 条件断点     │  │  - 值修改       │  │  - 继续执行     │ │
│  │  - 日志断点     │  │  - 表达式求值   │  │  - 异常处理     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  C#运行时集成 (C# Runtime Integration)                         │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  调试API       │  │  元数据提供     │  │  JIT集成        │ │
│  │  - 调试器接口   │  │  - 类型信息     │  │  - 调试符号     │ │
│  │  - 事件回调     │  │  - 成员信息     │  │  - 源码映射     │ │
│  │  - 状态查询     │  │  - 程序集信息   │  │  - 优化控制     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  UE5集成层 (UE5 Integration Layer)                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  编辑器集成     │  │  游戏内调试     │  │  日志系统       │ │
│  │  - 调试面板     │  │  - 实时监控     │  │  - 分类日志     │ │
│  │  │  - 脚本列表   │  │  - 性能指标     │  │  - 过滤搜索     │ │
│  │  - 属性检查器   │  │  - 内存使用     │  │  - 导出功能     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 核心调试引擎实现

### 2.1 调试引擎核心类

```cpp
// UCSharpDebugEngine.h
#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Containers/Queue.h"
#include "Sockets.h"

// 调试事件类型
ENUM_CLASS_FLAGS(EDebugEventType)
{
    None = 0,
    BreakpointHit = 1 << 0,
    StepComplete = 1 << 1,
    ExceptionThrown = 1 << 2,
    ThreadStarted = 1 << 3,
    ThreadExited = 1 << 4,
    ModuleLoaded = 1 << 5,
    ModuleUnloaded = 1 << 6,
    OutputMessage = 1 << 7,
    ProcessExited = 1 << 8
};

// 断点信息
struct UCSHARPRUNTIME_API FBreakpointInfo
{
    int32 Id;
    FString SourceFile;
    int32 LineNumber;
    int32 ColumnNumber;
    FString Condition;
    FString LogMessage;
    bool bEnabled;
    bool bIsLogPoint;
    int32 HitCount;
    int32 HitCountCondition; // 0=无条件, 1=等于, 2=大于等于, 3=是倍数
    
    FBreakpointInfo()
        : Id(0)
        , LineNumber(0)
        , ColumnNumber(0)
        , bEnabled(true)
        , bIsLogPoint(false)
        , HitCount(0)
        , HitCountCondition(0)
    {}
};

// 变量信息
struct UCSHARPRUNTIME_API FVariableInfo
{
    FString Name;
    FString Type;
    FString Value;
    bool bHasChildren;
    int32 ChildCount;
    TArray<FVariableInfo> Children;
    
    FVariableInfo()
        : bHasChildren(false)
        , ChildCount(0)
    {}
};

// 调用堆栈帧
struct UCSHARPRUNTIME_API FStackFrame
{
    int32 FrameId;
    FString FunctionName;
    FString SourceFile;
    int32 LineNumber;
    int32 ColumnNumber;
    TArray<FVariableInfo> LocalVariables;
    TArray<FVariableInfo> Parameters;
    
    FStackFrame()
        : FrameId(0)
        , LineNumber(0)
        , ColumnNumber(0)
    {}
};

// 调试事件
struct UCSHARPRUNTIME_API FDebugEvent
{
    EDebugEventType Type;
    int32 ThreadId;
    FString Message;
    TSharedPtr<FBreakpointInfo> Breakpoint;
    TArray<FStackFrame> CallStack;
    
    FDebugEvent(EDebugEventType InType = EDebugEventType::None)
        : Type(InType)
        , ThreadId(0)
    {}
};

// 调试命令
struct UCSHARPRUNTIME_API FDebugCommand
{
    enum class EType
    {
        Continue,
        StepOver,
        StepInto,
        StepOut,
        Pause,
        SetBreakpoint,
        RemoveBreakpoint,
        EnableBreakpoint,
        DisableBreakpoint,
        EvaluateExpression,
        GetVariables,
        GetCallStack,
        Terminate
    };
    
    EType Type;
    int32 RequestId;
    TMap<FString, FString> Parameters;
    
    FDebugCommand(EType InType = EType::Continue)
        : Type(InType)
        , RequestId(0)
    {}
};

// 调试引擎主类
class UCSHARPRUNTIME_API UCSharpDebugEngine
{
public:
    static UCSharpDebugEngine& Get();
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    
    // 调试会话管理
    bool StartDebugSession(int32 Port = 4711);
    void StopDebugSession();
    bool IsDebugging() const { return bIsDebugging; }
    
    // 断点管理
    int32 SetBreakpoint(const FString& SourceFile, int32 LineNumber, const FString& Condition = TEXT(""));
    bool RemoveBreakpoint(int32 BreakpointId);
    bool EnableBreakpoint(int32 BreakpointId, bool bEnable);
    void ClearAllBreakpoints();
    TArray<FBreakpointInfo> GetBreakpoints() const;
    
    // 执行控制
    void Continue();
    void StepOver();
    void StepInto();
    void StepOut();
    void Pause();
    void Terminate();
    
    // 变量和表达式
    TArray<FVariableInfo> GetLocalVariables(int32 FrameId = 0);
    TArray<FVariableInfo> GetGlobalVariables();
    FString EvaluateExpression(const FString& Expression, int32 FrameId = 0);
    bool SetVariableValue(const FString& VariableName, const FString& Value, int32 FrameId = 0);
    
    // 调用堆栈
    TArray<FStackFrame> GetCallStack(int32 ThreadId = 0);
    
    // 事件处理
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnDebugEvent, const FDebugEvent&);
    FOnDebugEvent OnDebugEvent;
    
    // 日志和输出
    void LogMessage(const FString& Message, const FString& Category = TEXT("CSharp"));
    void LogWarning(const FString& Message, const FString& Category = TEXT("CSharp"));
    void LogError(const FString& Message, const FString& Category = TEXT("CSharp"));
    
private:
    UCSharpDebugEngine() = default;
    
    // 网络通信
    bool StartDebugServer(int32 Port);
    void StopDebugServer();
    void HandleClientConnection(FSocket* ClientSocket);
    void ProcessDebugCommands();
    
    // 断点处理
    bool ShouldBreakAtLocation(const FString& SourceFile, int32 LineNumber);
    void OnBreakpointHit(const FBreakpointInfo& Breakpoint);
    
    // C#运行时集成
    bool AttachToRuntime();
    void DetachFromRuntime();
    void SetupRuntimeCallbacks();
    
    // 静态回调函数
    static void OnRuntimeBreakpoint(const char* SourceFile, int32 LineNumber, void* UserData);
    static void OnRuntimeException(const char* ExceptionMessage, const char* StackTrace, void* UserData);
    static void OnRuntimeStep(const char* SourceFile, int32 LineNumber, void* UserData);
    
    // 数据成员
    bool bIsDebugging;
    bool bIsPaused;
    FSocket* DebugServerSocket;
    TArray<FSocket*> ClientSockets;
    TMap<int32, FBreakpointInfo> Breakpoints;
    int32 NextBreakpointId;
    
    // 线程安全
    FCriticalSection DebugMutex;
    TQueue<FDebugCommand> CommandQueue;
    TQueue<FDebugEvent> EventQueue;
    
    // 运行时状态
    int32 CurrentThreadId;
    TArray<FStackFrame> CurrentCallStack;
    
    static UCSharpDebugEngine* Instance;
};
```

### 2.2 调试适配器协议(DAP)实现

```cpp
// UCSharpDebugAdapter.h
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// DAP消息类型
enum class EDAPMessageType : uint8
{
    Request,
    Response,
    Event
};

// DAP消息基类
struct UCSHARPRUNTIME_API FDAPMessage
{
    EDAPMessageType Type;
    int32 Seq;
    
    FDAPMessage(EDAPMessageType InType)
        : Type(InType)
        , Seq(0)
    {}
    
    virtual ~FDAPMessage() = default;
    virtual TSharedPtr<FJsonObject> ToJson() const = 0;
    virtual bool FromJson(const TSharedPtr<FJsonObject>& JsonObject) = 0;
};

// DAP请求消息
struct UCSHARPRUNTIME_API FDAPRequest : public FDAPMessage
{
    FString Command;
    TSharedPtr<FJsonObject> Arguments;
    
    FDAPRequest(const FString& InCommand = TEXT(""))
        : FDAPMessage(EDAPMessageType::Request)
        , Command(InCommand)
    {}
    
    virtual TSharedPtr<FJsonObject> ToJson() const override;
    virtual bool FromJson(const TSharedPtr<FJsonObject>& JsonObject) override;
};

// DAP响应消息
struct UCSHARPRUNTIME_API FDAPResponse : public FDAPMessage
{
    int32 RequestSeq;
    bool bSuccess;
    FString Message;
    FString Command;
    TSharedPtr<FJsonObject> Body;
    
    FDAPResponse()
        : FDAPMessage(EDAPMessageType::Response)
        , RequestSeq(0)
        , bSuccess(true)
    {}
    
    virtual TSharedPtr<FJsonObject> ToJson() const override;
    virtual bool FromJson(const TSharedPtr<FJsonObject>& JsonObject) override;
};

// DAP事件消息
struct UCSHARPRUNTIME_API FDAPEvent : public FDAPMessage
{
    FString Event;
    TSharedPtr<FJsonObject> Body;
    
    FDAPEvent(const FString& InEvent = TEXT(""))
        : FDAPMessage(EDAPMessageType::Event)
        , Event(InEvent)
    {}
    
    virtual TSharedPtr<FJsonObject> ToJson() const override;
    virtual bool FromJson(const TSharedPtr<FJsonObject>& JsonObject) override;
};

// DAP适配器
class UCSHARPRUNTIME_API UCSharpDebugAdapter
{
public:
    UCSharpDebugAdapter();
    ~UCSharpDebugAdapter();
    
    // 初始化和清理
    bool Initialize(int32 Port = 4711);
    void Shutdown();
    
    // 消息处理
    void ProcessMessages();
    void SendMessage(const FDAPMessage& Message);
    
    // 请求处理器
    void HandleInitializeRequest(const FDAPRequest& Request);
    void HandleLaunchRequest(const FDAPRequest& Request);
    void HandleAttachRequest(const FDAPRequest& Request);
    void HandleDisconnectRequest(const FDAPRequest& Request);
    void HandleSetBreakpointsRequest(const FDAPRequest& Request);
    void HandleContinueRequest(const FDAPRequest& Request);
    void HandleNextRequest(const FDAPRequest& Request);
    void HandleStepInRequest(const FDAPRequest& Request);
    void HandleStepOutRequest(const FDAPRequest& Request);
    void HandlePauseRequest(const FDAPRequest& Request);
    void HandleStackTraceRequest(const FDAPRequest& Request);
    void HandleScopesRequest(const FDAPRequest& Request);
    void HandleVariablesRequest(const FDAPRequest& Request);
    void HandleEvaluateRequest(const FDAPRequest& Request);
    
    // 事件发送
    void SendInitializedEvent();
    void SendStoppedEvent(const FString& Reason, int32 ThreadId = 0);
    void SendContinuedEvent(int32 ThreadId = 0);
    void SendTerminatedEvent();
    void SendOutputEvent(const FString& Output, const FString& Category = TEXT("console"));
    
private:
    // 网络处理
    bool StartServer(int32 Port);
    void StopServer();
    void HandleClientConnection();
    FString ReceiveMessage();
    void SendRawMessage(const FString& Message);
    
    // 消息解析
    TSharedPtr<FDAPMessage> ParseMessage(const FString& MessageText);
    FString SerializeMessage(const FDAPMessage& Message);
    
    // 数据转换
    TSharedPtr<FJsonObject> BreakpointToJson(const FBreakpointInfo& Breakpoint);
    TSharedPtr<FJsonObject> StackFrameToJson(const FStackFrame& Frame);
    TSharedPtr<FJsonObject> VariableToJson(const FVariableInfo& Variable);
    
    // 网络相关
    FSocket* ServerSocket;
    FSocket* ClientSocket;
    bool bIsConnected;
    
    // 消息序列号
    int32 NextSeq;
    
    // 调试引擎引用
    UCSharpDebugEngine* DebugEngine;
};
```

## 3. IDE集成支持

### 3.1 Visual Studio扩展

```csharp
// VisualStudioExtension/UE5CSharpDebugger.cs
using Microsoft.VisualStudio.Debugger.Interop;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;

namespace UE5CSharpExtension
{
    /// <summary>
    /// UE5 C#调试引擎
    /// </summary>
    [ComVisible(true)]
    [Guid("12345678-1234-1234-1234-123456789012")]
    public class UE5CSharpDebugEngine : IDebugEngine2, IDebugEngineLaunch2
    {
        private IDebugEventCallback2 _eventCallback;
        private IDebugProcess2 _process;
        private readonly Dictionary<string, UE5Breakpoint> _breakpoints = new();
        
        public int Attach(IDebugProgram2[] rgpPrograms, IDebugProgramNode2[] rgpProgramNodes, 
                         uint celtPrograms, IDebugEventCallback2 pCallback, 
                         enum_ATTACH_REASON dwReason)
        {
            _eventCallback = pCallback;
            
            // 连接到UE5调试服务器
            var debugClient = new UE5DebugClient();
            debugClient.Connect("localhost", 4711);
            
            // 发送附加事件
            var attachEvent = new UE5EngineCreateEvent(this);
            _eventCallback.Event(this, null, rgpPrograms[0], null, attachEvent, 
                                ref attachEvent.EventGuid, attachEvent.EventAttributes);
            
            return VSConstants.S_OK;
        }
        
        public int CreatePendingBreakpoint(IDebugBreakpointRequest2 pBPRequest, 
                                          out IDebugPendingBreakpoint2 ppPendingBP)
        {
            ppPendingBP = new UE5PendingBreakpoint(pBPRequest, this);
            return VSConstants.S_OK;
        }
        
        public int ContinueFromSynchronousEvent(IDebugEvent2 pEvent)
        {
            return VSConstants.S_OK;
        }
        
        public int DestroyProgram(IDebugProgram2 pProgram)
        {
            return VSConstants.S_OK;
        }
        
        public int EnumPrograms(out IEnumDebugPrograms2 ppEnum)
        {
            ppEnum = null;
            return VSConstants.E_NOTIMPL;
        }
        
        public int GetEngineId(out Guid pguidEngine)
        {
            pguidEngine = new Guid("12345678-1234-1234-1234-123456789012");
            return VSConstants.S_OK;
        }
        
        public int RemoveAllSetExceptions(ref Guid guidType)
        {
            return VSConstants.S_OK;
        }
        
        public int RemoveSetException(EXCEPTION_INFO[] pException)
        {
            return VSConstants.S_OK;
        }
        
        public int SetException(EXCEPTION_INFO[] pException)
        {
            return VSConstants.S_OK;
        }
        
        public int SetLocale(ushort wLangID)
        {
            return VSConstants.S_OK;
        }
        
        public int SetMetric(string pszMetric, object varValue)
        {
            return VSConstants.S_OK;
        }
        
        public int SetRegistryRoot(string pszRegistryRoot)
        {
            return VSConstants.S_OK;
        }
        
        // IDebugEngineLaunch2 实现
        public int CanTerminateProcess(IDebugProcess2 pProcess)
        {
            return VSConstants.S_OK;
        }
        
        public int LaunchSuspended(string pszServer, IDebugPort2 pPort, 
                                  string pszExe, string pszArgs, string pszDir, 
                                  string bstrEnv, string pszOptions, 
                                  enum_LAUNCH_FLAGS dwLaunchFlags, 
                                  uint hStdInput, uint hStdOutput, uint hStdError, 
                                  IDebugEventCallback2 pCallback, 
                                  out IDebugProcess2 ppProcess)
        {
            _eventCallback = pCallback;
            
            // 启动UE5进程并附加调试器
            var processInfo = new UE5ProcessInfo(pszExe, pszArgs, pszDir);
            _process = new UE5DebugProcess(processInfo, pPort);
            ppProcess = _process;
            
            return VSConstants.S_OK;
        }
        
        public int ResumeProcess(IDebugProcess2 pProcess)
        {
            // 恢复进程执行
            var debugClient = UE5DebugClient.Instance;
            debugClient?.SendCommand("continue");
            
            return VSConstants.S_OK;
        }
        
        public int TerminateProcess(IDebugProcess2 pProcess)
        {
            // 终止调试会话
            var debugClient = UE5DebugClient.Instance;
            debugClient?.Disconnect();
            
            return VSConstants.S_OK;
        }
    }
    
    /// <summary>
    /// UE5调试客户端
    /// </summary>
    public class UE5DebugClient
    {
        private TcpClient _tcpClient;
        private NetworkStream _stream;
        private bool _isConnected;
        
        public static UE5DebugClient Instance { get; private set; }
        
        public bool Connect(string host, int port)
        {
            try
            {
                _tcpClient = new TcpClient();
                _tcpClient.Connect(host, port);
                _stream = _tcpClient.GetStream();
                _isConnected = true;
                
                Instance = this;
                
                // 启动消息处理线程
                Task.Run(ProcessMessages);
                
                return true;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Failed to connect to UE5 debug server: {ex.Message}");
                return false;
            }
        }
        
        public void Disconnect()
        {
            _isConnected = false;
            _stream?.Close();
            _tcpClient?.Close();
            Instance = null;
        }
        
        public void SendCommand(string command, object parameters = null)
        {
            if (!_isConnected) return;
            
            var message = new
            {
                type = "request",
                seq = GetNextSeq(),
                command = command,
                arguments = parameters
            };
            
            var json = JsonConvert.SerializeObject(message);
            var data = Encoding.UTF8.GetBytes($"Content-Length: {json.Length}\r\n\r\n{json}");
            
            _stream.Write(data, 0, data.Length);
        }
        
        private async Task ProcessMessages()
        {
            var buffer = new byte[4096];
            
            while (_isConnected)
            {
                try
                {
                    var bytesRead = await _stream.ReadAsync(buffer, 0, buffer.Length);
                    if (bytesRead > 0)
                    {
                        var message = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                        ProcessMessage(message);
                    }
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error processing message: {ex.Message}");
                    break;
                }
            }
        }
        
        private void ProcessMessage(string message)
        {
            // 解析DAP消息并处理
            // 实现消息解析和事件分发逻辑
        }
        
        private static int _seqCounter = 0;
        private static int GetNextSeq() => Interlocked.Increment(ref _seqCounter);
    }
}
```

### 3.2 VS Code扩展

```typescript
// vscode-extension/src/extension.ts
import * as vscode from 'vscode';
import { UE5DebugAdapterDescriptorFactory } from './debugAdapter';
import { UE5ConfigurationProvider } from './configurationProvider';

export function activate(context: vscode.ExtensionContext) {
    // 注册调试适配器
    const factory = new UE5DebugAdapterDescriptorFactory();
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('ue5-csharp', factory)
    );
    
    // 注册配置提供器
    const provider = new UE5ConfigurationProvider();
    context.subscriptions.push(
        vscode.debug.registerDebugConfigurationProvider('ue5-csharp', provider)
    );
    
    // 注册命令
    context.subscriptions.push(
        vscode.commands.registerCommand('ue5-csharp.startDebugging', () => {
            vscode.debug.startDebugging(undefined, {
                type: 'ue5-csharp',
                name: 'UE5 C# Debug',
                request: 'attach',
                host: 'localhost',
                port: 4711
            });
        })
    );
    
    // 注册语言服务
    const languageClient = new UE5LanguageClient(context);
    languageClient.start();
}

export function deactivate() {
    // 清理资源
}
```

```typescript
// vscode-extension/src/debugAdapter.ts
import * as vscode from 'vscode';
import { DebugAdapterExecutable, DebugAdapterServer } from 'vscode';
import * as Net from 'net';

export class UE5DebugAdapterDescriptorFactory implements vscode.DebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(
        session: vscode.DebugSession,
        executable: DebugAdapterExecutable | undefined
    ): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        
        const config = session.configuration;
        
        if (config.request === 'attach') {
            // 连接到现有的调试服务器
            return new DebugAdapterServer(config.port || 4711, config.host || 'localhost');
        } else {
            // 启动新的调试会话
            return new DebugAdapterExecutable('ue5-debug-adapter', []);
        }
    }
}

export class UE5ConfigurationProvider implements vscode.DebugConfigurationProvider {
    resolveDebugConfiguration(
        folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration,
        token?: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        
        // 提供默认配置
        if (!config.type && !config.request && !config.name) {
            const editor = vscode.window.activeTextEditor;
            if (editor && editor.document.languageId === 'csharp') {
                config.type = 'ue5-csharp';
                config.name = 'UE5 C# Debug';
                config.request = 'attach';
                config.host = 'localhost';
                config.port = 4711;
            }
        }
        
        return config;
    }
}
```

## 4. 游戏内调试工具

### 4.1 调试控制台

```cpp
// UCSharpDebugConsole.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"

// 控制台命令
struct UCSHARPRUNTIME_API FConsoleCommand
{
    FString Command;
    FString Description;
    TFunction<FString(const TArray<FString>&)> Handler;
    
    FConsoleCommand(const FString& InCommand, const FString& InDescription, 
                   TFunction<FString(const TArray<FString>&)> InHandler)
        : Command(InCommand)
        , Description(InDescription)
        , Handler(InHandler)
    {}
};

// 调试控制台Widget
class UCSHARPRUNTIME_API SCSharpDebugConsole : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCSharpDebugConsole) {}
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    
    // 添加日志消息
    void AddLogMessage(const FString& Message, const FString& Category = TEXT("Info"));
    
    // 执行命令
    FString ExecuteCommand(const FString& Command);
    
    // 注册命令
    void RegisterCommand(const FString& Command, const FString& Description, 
                        TFunction<FString(const TArray<FString>&)> Handler);
    
    // 清空控制台
    void ClearConsole();
    
private:
    // UI事件处理
    FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    void OnTextCommitted(const FText& Text, ETextCommit::Type CommitType);
    void OnTextChanged(const FText& Text);
    
    // 命令处理
    TArray<FString> ParseCommand(const FString& CommandLine);
    TArray<FString> GetCommandSuggestions(const FString& PartialCommand);
    
    // UI组件
    TSharedPtr<SScrollBox> LogScrollBox;
    TSharedPtr<SEditableTextBox> CommandInputBox;
    TSharedPtr<STextBlock> SuggestionText;
    
    // 数据
    TArray<FString> LogMessages;
    TArray<FString> CommandHistory;
    TMap<FString, FConsoleCommand> RegisteredCommands;
    int32 HistoryIndex;
    
    // 样式
    FSlateFontInfo ConsoleFont;
    FSlateColor InfoColor;
    FSlateColor WarningColor;
    FSlateColor ErrorColor;
};

// 调试控制台管理器
class UCSHARPRUNTIME_API UCSharpDebugConsole
{
public:
    static UCSharpDebugConsole& Get();
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    
    // 控制台显示
    void ShowConsole();
    void HideConsole();
    void ToggleConsole();
    bool IsConsoleVisible() const;
    
    // 日志记录
    void LogInfo(const FString& Message, const FString& Category = TEXT("CSharp"));
    void LogWarning(const FString& Message, const FString& Category = TEXT("CSharp"));
    void LogError(const FString& Message, const FString& Category = TEXT("CSharp"));
    
    // 命令注册
    void RegisterCommand(const FString& Command, const FString& Description, 
                        TFunction<FString(const TArray<FString>&)> Handler);
    void UnregisterCommand(const FString& Command);
    
    // 执行命令
    FString ExecuteCommand(const FString& Command);
    
private:
    UCSharpDebugConsole() = default;
    
    // 内置命令
    void RegisterBuiltinCommands();
    FString HandleHelpCommand(const TArray<FString>& Args);
    FString HandleClearCommand(const TArray<FString>& Args);
    FString HandleListScriptsCommand(const TArray<FString>& Args);
    FString HandleReloadScriptCommand(const TArray<FString>& Args);
    FString HandleSetBreakpointCommand(const TArray<FString>& Args);
    FString HandleListBreakpointsCommand(const TArray<FString>& Args);
    FString HandleEvaluateCommand(const TArray<FString>& Args);
    FString HandleGCCommand(const TArray<FString>& Args);
    FString HandleMemoryStatsCommand(const TArray<FString>& Args);
    FString HandlePerformanceStatsCommand(const TArray<FString>& Args);
    
    // UI管理
    TSharedPtr<SCSharpDebugConsole> ConsoleWidget;
    TSharedPtr<SWindow> ConsoleWindow;
    bool bIsVisible;
    
    static UCSharpDebugConsole* Instance;
};
```

### 4.2 实时监控面板

```cpp
// UCSharpMonitorPanel.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"

// 监控数据项
struct UCSHARPRUNTIME_API FMonitorDataItem
{
    FString Name;
    FString Value;
    FString Category;
    FLinearColor Color;
    bool bIsExpanded;
    TArray<TSharedPtr<FMonitorDataItem>> Children;
    
    FMonitorDataItem(const FString& InName, const FString& InValue, 
                    const FString& InCategory = TEXT("General"))
        : Name(InName)
        , Value(InValue)
        , Category(InCategory)
        , Color(FLinearColor::White)
        , bIsExpanded(false)
    {}
};

// 性能图表数据
struct UCSHARPRUNTIME_API FPerformanceGraphData
{
    FString Name;
    TArray<float> Values;
    FLinearColor Color;
    float MinValue;
    float MaxValue;
    int32 MaxSamples;
    
    FPerformanceGraphData(const FString& InName, const FLinearColor& InColor, int32 InMaxSamples = 100)
        : Name(InName)
        , Color(InColor)
        , MinValue(0.0f)
        , MaxValue(1.0f)
        , MaxSamples(InMaxSamples)
    {
        Values.Reserve(MaxSamples);
    }
    
    void AddValue(float Value)
    {
        Values.Add(Value);
        if (Values.Num() > MaxSamples)
        {
            Values.RemoveAt(0);
        }
        
        MinValue = FMath::Min(MinValue, Value);
        MaxValue = FMath::Max(MaxValue, Value);
    }
};

// 监控面板Widget
class UCSHARPRUNTIME_API SCSharpMonitorPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCSharpMonitorPanel) {}
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    
    // 数据更新
    void UpdateData();
    void AddMonitorItem(TSharedPtr<FMonitorDataItem> Item);
    void RemoveMonitorItem(const FString& Name);
    void ClearMonitorItems();
    
    // 图表管理
    void AddPerformanceGraph(const FString& Name, const FLinearColor& Color);
    void UpdatePerformanceGraph(const FString& Name, float Value);
    void RemovePerformanceGraph(const FString& Name);
    
private:
    // UI生成
    TSharedRef<ITableRow> OnGenerateRowForMonitorItem(
        TSharedPtr<FMonitorDataItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnGetChildrenForMonitorItem(
        TSharedPtr<FMonitorDataItem> Item, TArray<TSharedPtr<FMonitorDataItem>>& OutChildren);
    
    // 图表绘制
    int32 OnPaintPerformanceGraph(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                                 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                                 int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;
    
    // 数据收集
    void CollectScriptData();
    void CollectMemoryData();
    void CollectPerformanceData();
    
    // UI组件
    TSharedPtr<STreeView<TSharedPtr<FMonitorDataItem>>> MonitorTreeView;
    TSharedPtr<SWidget> GraphContainer;
    
    // 数据
    TArray<TSharedPtr<FMonitorDataItem>> MonitorItems;
    TMap<FString, TSharedPtr<FPerformanceGraphData>> PerformanceGraphs;
    
    // 更新定时器
    FTimerHandle UpdateTimerHandle;
    float UpdateInterval;
};

// 监控面板管理器
class UCSHARPRUNTIME_API UCSharpMonitorPanel
{
public:
    static UCSharpMonitorPanel& Get();
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    
    // 面板显示
    void ShowPanel();
    void HidePanel();
    void TogglePanel();
    bool IsPanelVisible() const;
    
    // 监控项管理
    void AddMonitorItem(const FString& Name, const FString& Value, const FString& Category = TEXT("General"));
    void UpdateMonitorItem(const FString& Name, const FString& Value);
    void RemoveMonitorItem(const FString& Name);
    
    // 性能图表
    void AddPerformanceMetric(const FString& Name, const FLinearColor& Color = FLinearColor::White);
    void UpdatePerformanceMetric(const FString& Name, float Value);
    void RemovePerformanceMetric(const FString& Name);
    
    // 自动监控
    void EnableAutoMonitoring(bool bEnable);
    void SetUpdateInterval(float Interval);
    
private:
    UCSharpMonitorPanel() = default;
    
    // 内置监控项
    void SetupBuiltinMonitors();
    void UpdateBuiltinMonitors();
    
    // UI管理
    TSharedPtr<SCSharpMonitorPanel> PanelWidget;
    TSharedPtr<SWindow> PanelWindow;
    bool bIsVisible;
    bool bAutoMonitoring;
    
    // 定时器
    FTimerHandle MonitorTimerHandle;
    
    static UCSharpMonitorPanel* Instance;
};
```

## 5. 开发工具集成

### 5.1 代码生成工具

```cpp
// UCSharpCodeGenerator.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"

// 代码生成选项
struct UCSHARPRUNTIME_API FCodeGenerationOptions
{
    bool bGenerateProperties;
    bool bGenerateMethods;
    bool bGenerateEvents;
    bool bGenerateComments;
    bool bUseRegions;
    FString Namespace;
    FString BaseClass;
    TArray<FString> UsingStatements;
    
    FCodeGenerationOptions()
        : bGenerateProperties(true)
        , bGenerateMethods(true)
        , bGenerateEvents(true)
        , bGenerateComments(true)
        , bUseRegions(true)
        , Namespace(TEXT("UnrealEngine.Generated"))
        , BaseClass(TEXT("ScriptBase"))
    {
        UsingStatements.Add(TEXT("System"));
        UsingStatements.Add(TEXT("UnrealEngine"));
    }
};

// 代码生成器
class UCSHARPRUNTIME_API UCSharpCodeGenerator
{
public:
    static UCSharpCodeGenerator& Get();
    
    // 从UClass生成C#代码
    FString GenerateClassFromUClass(UClass* Class, const FCodeGenerationOptions& Options = FCodeGenerationOptions());
    
    // 从Blueprint生成C#代码
    FString GenerateClassFromBlueprint(UBlueprint* Blueprint, const FCodeGenerationOptions& Options = FCodeGenerationOptions());
    
    // 生成API绑定代码
    FString GenerateAPIBindings(const TArray<UClass*>& Classes, const FCodeGenerationOptions& Options = FCodeGenerationOptions());
    
    // 生成项目模板
    bool GenerateProjectTemplate(const FString& ProjectPath, const FString& ProjectName);
    
    // 生成脚本模板
    FString GenerateScriptTemplate(const FString& ClassName, const FString& BaseClass = TEXT("ScriptBase"));
    
private:
    UCSharpCodeGenerator() = default;
    
    // 代码生成辅助方法
    FString GenerateClassHeader(const FString& ClassName, const FString& BaseClass, 
                               const FCodeGenerationOptions& Options);
    FString GenerateProperties(UClass* Class, const FCodeGenerationOptions& Options);
    FString GenerateMethods(UClass* Class, const FCodeGenerationOptions& Options);
    FString GenerateEvents(UClass* Class, const FCodeGenerationOptions& Options);
    FString GenerateClassFooter(const FCodeGenerationOptions& Options);
    
    // 类型转换
    FString ConvertUPropertyToCS(UProperty* Property);
    FString ConvertUFunctionToCS(UFunction* Function);
    FString ConvertTypeToCS(const FString& UEType);
    
    // 文件操作
    bool WriteToFile(const FString& FilePath, const FString& Content);
    FString ReadFromFile(const FString& FilePath);
    
    static UCSharpCodeGenerator* Instance;
};
```

### 5.2 项目管理工具

```csharp
// UnrealEngine.Tools.ProjectManager.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

namespace UnrealEngine.Tools
{
    /// <summary>
    /// C#项目配置
    /// </summary>
    public class CSharpProjectConfig
    {
        public string ProjectName { get; set; }
        public string Version { get; set; }
        public string TargetFramework { get; set; } = "net6.0";
        public List<string> References { get; set; } = new List<string>();
        public List<string> PackageReferences { get; set; } = new List<string>();
        public Dictionary<string, string> Properties { get; set; } = new Dictionary<string, string>();
        public List<string> SourceDirectories { get; set; } = new List<string> { "Scripts" };
        public string OutputDirectory { get; set; } = "Binaries";
        public bool EnableHotReload { get; set; } = true;
        public bool EnableDebugging { get; set; } = true;
    }
    
    /// <summary>
    /// 项目管理器
    /// </summary>
    public static class ProjectManager
    {
        private static CSharpProjectConfig _currentProject;
        private static string _projectPath;
        
        /// <summary>
        /// 创建新项目
        /// </summary>
        public static bool CreateProject(string projectPath, string projectName)
        {
            try
            {
                var config = new CSharpProjectConfig
                {
                    ProjectName = projectName,
                    Version = "1.0.0"
                };
                
                // 创建项目目录结构
                Directory.CreateDirectory(projectPath);
                Directory.CreateDirectory(Path.Combine(projectPath, "Scripts"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Binaries"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Generated"));
                
                // 生成项目文件
                GenerateProjectFile(projectPath, config);
                GenerateConfigFile(projectPath, config);
                GenerateGitIgnore(projectPath);
                GenerateReadme(projectPath, projectName);
                
                // 生成示例脚本
                GenerateExampleScript(projectPath, projectName);
                
                _currentProject = config;
                _projectPath = projectPath;
                
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to create project: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 加载项目
        /// </summary>
        public static bool LoadProject(string projectPath)
        {
            try
            {
                var configPath = Path.Combine(projectPath, "csharp-project.json");
                if (!File.Exists(configPath))
                {
                    UE.LogError("Project configuration file not found");
                    return false;
                }
                
                var configJson = File.ReadAllText(configPath);
                _currentProject = JsonSerializer.Deserialize<CSharpProjectConfig>(configJson);
                _projectPath = projectPath;
                
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to load project: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 保存项目配置
        /// </summary>
        public static bool SaveProject()
        {
            if (_currentProject == null || string.IsNullOrEmpty(_projectPath))
                return false;
                
            try
            {
                var configPath = Path.Combine(_projectPath, "csharp-project.json");
                var configJson = JsonSerializer.Serialize(_currentProject, new JsonSerializerOptions
                {
                    WriteIndented = true
                });
                
                File.WriteAllText(configPath, configJson);
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to save project: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 编译项目
        /// </summary>
        public static bool CompileProject()
        {
            if (_currentProject == null || string.IsNullOrEmpty(_projectPath))
                return false;
                
            try
            {
                var projectFile = Path.Combine(_projectPath, $"{_currentProject.ProjectName}.csproj");
                
                // 使用dotnet CLI编译
                var process = new System.Diagnostics.Process
                {
                    StartInfo = new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = "dotnet",
                        Arguments = $"build \"{projectFile}\" --configuration Release",
                        WorkingDirectory = _projectPath,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true
                    }
                };
                
                process.Start();
                
                var output = process.StandardOutput.ReadToEnd();
                var error = process.StandardError.ReadToEnd();
                
                process.WaitForExit();
                
                if (process.ExitCode == 0)
                {
                    UE.LogInfo("Project compiled successfully");
                    UE.LogInfo(output);
                    return true;
                }
                else
                {
                    UE.LogError("Project compilation failed");
                    UE.LogError(error);
                    return false;
                }
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to compile project: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 添加脚本文件
        /// </summary>
        public static bool AddScript(string scriptName, string template = "default")
        {
            if (_currentProject == null || string.IsNullOrEmpty(_projectPath))
                return false;
                
            try
            {
                var scriptsDir = Path.Combine(_projectPath, "Scripts");
                var scriptPath = Path.Combine(scriptsDir, $"{scriptName}.cs");
                
                if (File.Exists(scriptPath))
                {
                    UE.LogWarning($"Script {scriptName} already exists");
                    return false;
                }
                
                var scriptContent = GenerateScriptFromTemplate(scriptName, template);
                File.WriteAllText(scriptPath, scriptContent);
                
                UE.LogInfo($"Script {scriptName} created successfully");
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to add script: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 获取项目脚本列表
        /// </summary>
        public static List<string> GetProjectScripts()
        {
            if (_currentProject == null || string.IsNullOrEmpty(_projectPath))
                return new List<string>();
                
            var scripts = new List<string>();
            
            foreach (var sourceDir in _currentProject.SourceDirectories)
            {
                var fullPath = Path.Combine(_projectPath, sourceDir);
                if (Directory.Exists(fullPath))
                {
                    var csFiles = Directory.GetFiles(fullPath, "*.cs", SearchOption.AllDirectories);
                    scripts.AddRange(csFiles.Select(f => Path.GetRelativePath(_projectPath, f)));
                }
            }
            
            return scripts;
        }
        
        private static void GenerateProjectFile(string projectPath, CSharpProjectConfig config)
        {
            var projectContent = $@"<Project Sdk=""Microsoft.NET.Sdk"">

  <PropertyGroup>
    <TargetFramework>{config.TargetFramework}</TargetFramework>
    <OutputType>Library</OutputType>
    <AssemblyName>{config.ProjectName}</AssemblyName>
    <RootNamespace>{config.ProjectName}</RootNamespace>
    <GenerateAssemblyInfo>false</GenerateAssemblyInfo>
  </PropertyGroup>

  <ItemGroup>
    <Reference Include=""UnrealEngine.Runtime"">
      <HintPath>$(UE5_CSHARP_RUNTIME_PATH)\UnrealEngine.Runtime.dll</HintPath>
    </Reference>
  </ItemGroup>

</Project>";
            
            var projectFile = Path.Combine(projectPath, $"{config.ProjectName}.csproj");
            File.WriteAllText(projectFile, projectContent);
        }
        
        private static void GenerateConfigFile(string projectPath, CSharpProjectConfig config)
        {
            var configJson = JsonSerializer.Serialize(config, new JsonSerializerOptions
            {
                WriteIndented = true
            });
            
            var configFile = Path.Combine(projectPath, "csharp-project.json");
            File.WriteAllText(configFile, configJson);
        }
        
        private static void GenerateGitIgnore(string projectPath)
        {
            var gitIgnoreContent = @"# Build outputs
Binaries/
obj/
bin/

# Generated files
Generated/

# IDE files
.vs/
.vscode/
*.user
*.suo

# Temporary files
*.tmp
*.temp
";
            
            var gitIgnoreFile = Path.Combine(projectPath, ".gitignore");
            File.WriteAllText(gitIgnoreFile, gitIgnoreContent);
        }
        
        private static void GenerateReadme(string projectPath, string projectName)
        {
            var readmeContent = $@"# {projectName}

This is a UE5 C# scripting project.

## Getting Started

1. Open the project in your preferred IDE
2. Write your C# scripts in the Scripts folder
3. Build the project using `dotnet build`
4. The compiled assembly will be loaded automatically by UE5

## Project Structure

- `Scripts/` - Your C# script files
- `Binaries/` - Compiled assemblies
- `Generated/` - Auto-generated binding code

## Debugging

To debug your scripts:

1. Start UE5 with debugging enabled
2. Attach your debugger to the UE5 process
3. Set breakpoints in your C# code
";
            
            var readmeFile = Path.Combine(projectPath, "README.md");
            File.WriteAllText(readmeFile, readmeContent);
        }
        
        private static void GenerateExampleScript(string projectPath, string projectName)
        {
            var exampleContent = $@"using System;
using UnrealEngine;

namespace {projectName}
{{
    /// <summary>
    /// Example C# script for UE5
    /// </summary>
    public class ExampleScript : ScriptBase
    {{
        protected override void OnStart()
        {{
            UE.LogInfo(""ExampleScript started!"");
        }}
        
        protected override void OnUpdate(float deltaTime)
        {{
            // Update logic here
        }}
        
        protected override void OnDestroy()
        {{
            UE.LogInfo(""ExampleScript destroyed!"");
        }}
    }}
}}
";
            
            var scriptFile = Path.Combine(projectPath, "Scripts", "ExampleScript.cs");
            File.WriteAllText(scriptFile, exampleContent);
        }
        
        private static string GenerateScriptFromTemplate(string scriptName, string template)
        {
            return template switch
            {
                "component" => GenerateComponentTemplate(scriptName),
                "actor" => GenerateActorTemplate(scriptName),
                "gamemode" => GenerateGameModeTemplate(scriptName),
                _ => GenerateDefaultTemplate(scriptName)
            };
        }
        
        private static string GenerateDefaultTemplate(string scriptName)
        {
            return $@"using System;
using UnrealEngine;

public class {scriptName} : ScriptBase
{{
    protected override void OnStart()
    {{
        UE.LogInfo($""{scriptName} started!"");
    }}
    
    protected override void OnUpdate(float deltaTime)
    {{
        // Update logic here
    }}
    
    protected override void OnDestroy()
    {{
        UE.LogInfo($""{scriptName} destroyed!"");
    }}
}}";
        }
        
        private static string GenerateComponentTemplate(string scriptName)
        {
            return $@"using System;
using UnrealEngine;

public class {scriptName} : ComponentScript
{{
    protected override void OnStart()
    {{
        UE.LogInfo($""{scriptName} component started!"");
    }}
    
    protected override void OnUpdate(float deltaTime)
    {{
        // Component update logic
    }}
}}";
        }
        
        private static string GenerateActorTemplate(string scriptName)
        {
            return $@"using System;
using UnrealEngine;

public class {scriptName} : ActorScript
{{
    protected override void OnBeginPlay()
    {{
        UE.LogInfo($""{scriptName} actor began play!"");
    }}
    
    protected override void OnTick(float deltaTime)
    {{
        // Actor tick logic
    }}
}}";
        }
        
        private static string GenerateGameModeTemplate(string scriptName)
        {
            return $@"using System;
using UnrealEngine;

public class {scriptName} : GameModeScript
{{
    protected override void OnGameStart()
    {{
        UE.LogInfo($""{scriptName} game mode started!"");
    }}
    
    protected override void OnPlayerJoined(PlayerController player)
    {{
        UE.LogInfo($""Player joined: {{player.GetName()}}"");
    }}
}}";
        }
    }
}
```

## 6. 性能分析工具

### 6.1 性能分析器

```csharp
// UnrealEngine.Tools.Profiler.cs
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;

namespace UnrealEngine.Tools
{
    /// <summary>
    /// 性能分析数据
    /// </summary>
    public class ProfileData
    {
        public string Name { get; set; }
        public long ElapsedTicks { get; set; }
        public double ElapsedMilliseconds => ElapsedTicks * 1000.0 / Stopwatch.Frequency;
        public int CallCount { get; set; }
        public double AverageTime => CallCount > 0 ? ElapsedMilliseconds / CallCount : 0;
        public double MinTime { get; set; } = double.MaxValue;
        public double MaxTime { get; set; } = double.MinValue;
        public List<ProfileData> Children { get; set; } = new List<ProfileData>();
        
        public void AddSample(double time)
        {
            CallCount++;
            MinTime = Math.Min(MinTime, time);
            MaxTime = Math.Max(MaxTime, time);
        }
    }
    
    /// <summary>
    /// 性能分析器
    /// </summary>
    public static class Profiler
    {
        private static readonly Dictionary<string, ProfileData> _profileData = new();
        private static readonly Dictionary<int, Stack<(string name, Stopwatch stopwatch)>> _threadStacks = new();
        private static readonly object _lock = new object();
        private static bool _isEnabled = true;
        
        /// <summary>
        /// 启用/禁用性能分析
        /// </summary>
        public static bool IsEnabled
        {
            get => _isEnabled;
            set => _isEnabled = value;
        }
        
        /// <summary>
        /// 开始性能分析
        /// </summary>
        public static IDisposable BeginSample(string name)
        {
            if (!_isEnabled) return new EmptyDisposable();
            
            var threadId = Thread.CurrentThread.ManagedThreadId;
            var stopwatch = Stopwatch.StartNew();
            
            lock (_lock)
            {
                if (!_threadStacks.ContainsKey(threadId))
                {
                    _threadStacks[threadId] = new Stack<(string, Stopwatch)>();
                }
                
                _threadStacks[threadId].Push((name, stopwatch));
            }
            
            return new ProfileSample(name, stopwatch);
        }
        
        /// <summary>
        /// 结束性能分析
        /// </summary>
        internal static void EndSample(string name, Stopwatch stopwatch)
        {
            if (!_isEnabled) return;
            
            stopwatch.Stop();
            var threadId = Thread.CurrentThread.ManagedThreadId;
            
            lock (_lock)
            {
                if (_threadStacks.ContainsKey(threadId) && _threadStacks[threadId].Count > 0)
                {
                    var (stackName, _) = _threadStacks[threadId].Pop();
                    
                    if (stackName == name)
                    {
                        if (!_profileData.ContainsKey(name))
                        {
                            _profileData[name] = new ProfileData { Name = name };
                        }
                        
                        var data = _profileData[name];
                        data.ElapsedTicks += stopwatch.ElapsedTicks;
                        data.AddSample(stopwatch.ElapsedMilliseconds);
                    }
                }
            }
        }
        
        /// <summary>
        /// 获取性能分析数据
        /// </summary>
        public static Dictionary<string, ProfileData> GetProfileData()
        {
            lock (_lock)
            {
                return new Dictionary<string, ProfileData>(_profileData);
            }
        }
        
        /// <summary>
        /// 清空性能分析数据
        /// </summary>
        public static void Clear()
        {
            lock (_lock)
            {
                _profileData.Clear();
                _threadStacks.Clear();
            }
        }
        
        /// <summary>
        /// 生成性能报告
        /// </summary>
        public static string GenerateReport()
        {
            var report = new System.Text.StringBuilder();
            report.AppendLine("Performance Report");
            report.AppendLine("==================");
            report.AppendLine();
            
            lock (_lock)
            {
                foreach (var kvp in _profileData.OrderByDescending(x => x.Value.ElapsedMilliseconds))
                {
                    var data = kvp.Value;
                    report.AppendLine($"{data.Name}:");
                    report.AppendLine($"  Total Time: {data.ElapsedMilliseconds:F2} ms");
                    report.AppendLine($"  Call Count: {data.CallCount}");
                    report.AppendLine($"  Average Time: {data.AverageTime:F2} ms");
                    report.AppendLine($"  Min Time: {data.MinTime:F2} ms");
                    report.AppendLine($"  Max Time: {data.MaxTime:F2} ms");
                    report.AppendLine();
                }
            }
            
            return report.ToString();
        }
    }
    
    /// <summary>
    /// 性能分析样本
    /// </summary>
    internal class ProfileSample : IDisposable
    {
        private readonly string _name;
        private readonly Stopwatch _stopwatch;
        private bool _disposed;
        
        public ProfileSample(string name, Stopwatch stopwatch)
        {
            _name = name;
            _stopwatch = stopwatch;
        }
        
        public void Dispose()
        {
            if (!_disposed)
            {
                Profiler.EndSample(_name, _stopwatch);
                _disposed = true;
            }
        }
    }
    
    /// <summary>
    /// 空的Disposable实现
    /// </summary>
    internal class EmptyDisposable : IDisposable
    {
        public void Dispose() { }
    }
}
```

### 6.2 内存分析器

```csharp
// UnrealEngine.Tools.MemoryProfiler.cs
using System;
using System.Collections.Generic;
using System.Runtime;
using System.Threading;

namespace UnrealEngine.Tools
{
    /// <summary>
    /// 内存分析数据
    /// </summary>
    public class MemorySnapshot
    {
        public DateTime Timestamp { get; set; }
        public long TotalMemory { get; set; }
        public long UsedMemory { get; set; }
        public long FreeMemory => TotalMemory - UsedMemory;
        public int Gen0Collections { get; set; }
        public int Gen1Collections { get; set; }
        public int Gen2Collections { get; set; }
        public Dictionary<string, long> ObjectCounts { get; set; } = new();
        public Dictionary<string, long> ObjectSizes { get; set; } = new();
    }
    
    /// <summary>
    /// 内存分析器
    /// </summary>
    public static class MemoryProfiler
    {
        private static readonly List<MemorySnapshot> _snapshots = new();
        private static readonly Timer _timer;
        private static bool _isEnabled;
        
        static MemoryProfiler()
        {
            _timer = new Timer(TakeSnapshot, null, Timeout.Infinite, Timeout.Infinite);
        }
        
        /// <summary>
        /// 开始内存监控
        /// </summary>
        public static void StartMonitoring(int intervalMs = 1000)
        {
            _isEnabled = true;
            _timer.Change(0, intervalMs);
        }
        
        /// <summary>
        /// 停止内存监控
        /// </summary>
        public static void StopMonitoring()
        {
            _isEnabled = false;
            _timer.Change(Timeout.Infinite, Timeout.Infinite);
        }
        
        /// <summary>
        /// 手动拍摄内存快照
        /// </summary>
        public static MemorySnapshot TakeManualSnapshot()
        {
            var snapshot = CreateSnapshot();
            lock (_snapshots)
            {
                _snapshots.Add(snapshot);
            }
            return snapshot;
        }
        
        /// <summary>
        /// 获取内存快照历史
        /// </summary>
        public static List<MemorySnapshot> GetSnapshots()
        {
            lock (_snapshots)
            {
                return new List<MemorySnapshot>(_snapshots);
            }
        }
        
        /// <summary>
        /// 清空快照历史
        /// </summary>
        public static void ClearSnapshots()
        {
            lock (_snapshots)
            {
                _snapshots.Clear();
            }
        }
        
        /// <summary>
        /// 强制垃圾回收
        /// </summary>
        public static void ForceGC()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }
        
        /// <summary>
        /// 生成内存报告
        /// </summary>
        public static string GenerateReport()
        {
            var report = new System.Text.StringBuilder();
            report.AppendLine("Memory Analysis Report");
            report.AppendLine("=====================");
            report.AppendLine();
            
            lock (_snapshots)
            {
                if (_snapshots.Count == 0)
                {
                    report.AppendLine("No memory snapshots available.");
                    return report.ToString();
                }
                
                var latest = _snapshots[_snapshots.Count - 1];
                report.AppendLine($"Latest Snapshot ({latest.Timestamp:yyyy-MM-dd HH:mm:ss}):");
                report.AppendLine($"  Total Memory: {latest.TotalMemory / 1024 / 1024:F2} MB");
                report.AppendLine($"  Used Memory: {latest.UsedMemory / 1024 / 1024:F2} MB");
                report.AppendLine($"  Free Memory: {latest.FreeMemory / 1024 / 1024:F2} MB");
                report.AppendLine($"  Gen 0 Collections: {latest.Gen0Collections}");
                report.AppendLine($"  Gen 1 Collections: {latest.Gen1Collections}");
                report.AppendLine($"  Gen 2 Collections: {latest.Gen2Collections}");
                report.AppendLine();
                
                if (_snapshots.Count > 1)
                {
                    var first = _snapshots[0];
                    var memoryGrowth = latest.UsedMemory - first.UsedMemory;
                    var timeSpan = latest.Timestamp - first.Timestamp;
                    
                    report.AppendLine($"Memory Growth Analysis:");
                    report.AppendLine($"  Time Period: {timeSpan.TotalMinutes:F1} minutes");
                    report.AppendLine($"  Memory Growth: {memoryGrowth / 1024 / 1024:F2} MB");
                    report.AppendLine($"  Growth Rate: {memoryGrowth / timeSpan.TotalMinutes / 1024 / 1024:F2} MB/min");
                }
            }
            
            return report.ToString();
        }
        
        private static void TakeSnapshot(object state)
        {
            if (!_isEnabled) return;
            
            try
            {
                var snapshot = CreateSnapshot();
                lock (_snapshots)
                {
                    _snapshots.Add(snapshot);
                    
                    // 保持最近1000个快照
                    if (_snapshots.Count > 1000)
                    {
                        _snapshots.RemoveAt(0);
                    }
                }
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to take memory snapshot: {ex.Message}");
            }
        }
        
        private static MemorySnapshot CreateSnapshot()
        {
            return new MemorySnapshot
            {
                Timestamp = DateTime.Now,
                TotalMemory = GC.GetTotalMemory(false),
                UsedMemory = GC.GetTotalMemory(false),
                Gen0Collections = GC.CollectionCount(0),
                Gen1Collections = GC.CollectionCount(1),
                Gen2Collections = GC.CollectionCount(2)
            };
        }
    }
}
```

## 7. 总结

本设计文档详细阐述了UE5 C#插件的调试和开发工具支持方案，主要包括：

### 7.1 核心特性

1. **完整的调试支持**
   - 断点调试（条件断点、日志断点）
   - 变量监视和修改
   - 调用堆栈查看
   - 单步执行控制

2. **IDE集成**
   - Visual Studio扩展
   - VS Code扩展
   - JetBrains Rider支持
   - 调试适配器协议(DAP)实现

3. **游戏内工具**
   - 调试控制台
   - 实时监控面板
   - 性能图表显示
   - 内存使用监控

4. **开发工具**
   - 代码生成器
   - 项目管理工具
   - 性能分析器
   - 内存分析器

### 7.2 技术优势

- **标准协议支持**：使用DAP协议确保与主流IDE的兼容性
- **实时调试**：支持运行时断点设置和变量修改
- **性能监控**：提供详细的性能和内存分析工具
- **易于使用**：提供直观的UI和丰富的开发辅助功能

### 7.3 实施建议

1. **分阶段实现**：优先实现核心调试功能，再逐步添加高级特性
2. **测试驱动**：为每个组件编写完整的测试用例
3. **文档完善**：提供详细的用户手册和API文档
4. **社区反馈**：收集开发者反馈，持续改进工具体验

这套调试和开发工具将大大提升UE5 C#脚本开发的效率和体验，为开发者提供专业级的开发环境。