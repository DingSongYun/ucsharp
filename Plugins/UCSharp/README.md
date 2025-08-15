# UCSharp - C# Scripting Support for Unreal Engine 5

## Project Overview

UCSharp is a plugin that enables C# scripting support in Unreal Engine 5, providing seamless integration between C# code and UE5's native systems including UObject, UFunction, and UProperty.

## Current Status: Phase 1 - Prototype Development

### Milestone M1.1: Development Environment Setup ✅

The basic project structure and development environment has been established:

- ✅ UE5 Plugin structure (`UCSharp.uplugin`)
- ✅ C++ Runtime module (`UCSharp`)
- ✅ C++ Editor module (`UCSharpEditor`)
- ✅ C# Managed library (`UCSharp.Managed`)
- ✅ Example C# Actor implementation

## Project Structure

```
UCSharp/
├── UCSharp.uplugin              # Plugin descriptor
├── Source/
│   ├── UCSharp/                 # Runtime module
│   │   ├── UCSharp.Build.cs     # Build configuration
│   │   ├── Public/UCSharp.h     # Module interface
│   │   └── Private/UCSharp.cpp  # Module implementation
│   └── UCSharpEditor/           # Editor module
│       ├── UCSharpEditor.Build.cs
│       ├── Public/UCSharpEditor.h
│       └── Private/UCSharpEditor.cpp
├── Managed/                     # C# code
│   ├── UCSharp.Managed.csproj   # C# project file
│   ├── Core/
│   │   ├── UObject.cs           # Base UObject implementation
│   │   └── Attributes.cs        # UFunction/UProperty attributes
│   └── Examples/
│       └── MyTestActor.cs       # Example C# Actor
├── Binaries/DotNet/             # Compiled C# assemblies (generated)
└── doc/                         # Technical documentation
```

## Key Features Implemented

### 1. UObject Integration
- Base `UObject` class in C# with lifecycle management
- Native pointer wrapping and object registry
- Automatic disposal and cleanup

### 2. Attribute System
- `[UFunction]` - Mark C# methods as Blueprint-callable functions
- `[UProperty]` - Mark C# properties/fields as Blueprint-accessible
- `[UClass]` - Mark C# classes as UE5 classes
- Full attribute configuration support

### 3. Example Implementation
- `MyTestActor` - Demonstrates complete C# Actor implementation
- Health system with damage/healing
- Blueprint events and function calls
- Property binding and editor integration

## Building the Project

### Prerequisites
- Unreal Engine 5.1 or later
- Visual Studio 2022 with C++ workload
- .NET 6.0 SDK or later

### Build Steps

1. **Generate Project Files**
   ```bash
   # In your UE5 project directory
   <UE5_ROOT>/Engine/Binaries/DotNET/UnrealBuildTool.exe -projectfiles -project="YourProject.uproject" -game -rocket -progress
   ```

2. **Build C# Managed Library**
   ```bash
   cd UCSharp/Managed
   dotnet build UCSharp.Managed.csproj
   ```

3. **Build UE5 Plugin**
   - Open your project in Visual Studio
   - Build the solution (this will compile the UCSharp modules)

4. **Enable Plugin**
   - Open your project in UE5 Editor
   - Go to Edit → Plugins
   - Find "UCSharp" and enable it
   - Restart the editor

## Usage Example

### Creating a C# Actor

```csharp
using UCSharp.Core;

[UClass(Blueprintable = true)]
public class MyGameActor : AActor
{
    [UProperty(EditAnywhere = true, BlueprintReadWrite = true)]
    public float Health { get; set; } = 100.0f;
    
    [UFunction(BlueprintCallable = true)]
    public void TakeDamage(float damage)
    {
        Health -= damage;
        if (Health <= 0)
        {
            OnActorDied();
        }
    }
    
    [UFunction(BlueprintImplementableEvent = true)]
    protected virtual void OnActorDied()
    {
        // Blueprint can implement this
    }
}
```

### Using in Blueprint
1. Create a new Blueprint class
2. Set parent class to your C# class (e.g., "MyGameActor")
3. Access C# properties in the Blueprint editor
4. Call C# functions from Blueprint graphs
5. Implement Blueprint events defined in C#

## Next Steps (Upcoming Milestones)

### M1.2: Core Binding Prototype
- Implement C++/C# interop framework
- Basic function call marshalling
- Property get/set operations
- Native object creation/destruction

### M1.3: MVP Demonstration
- Complete MyTestActor integration
- Blueprint inheritance from C# classes
- Function calls between Blueprint and C#
- Property synchronization

## Technical Architecture

The plugin uses a layered architecture:

1. **C++ Native Layer**: Interfaces with UE5 engine systems
2. **Interop Layer**: Marshals data between C++ and C#
3. **C# Managed Layer**: Provides .NET-friendly APIs
4. **Attribute System**: Enables declarative UE5 integration

## Development Guidelines

### C# Code Style
- Use PascalCase for public members
- Apply appropriate UE5 attributes
- Inherit from appropriate base classes (UObject, AActor, etc.)
- Implement proper disposal patterns

### Performance Considerations
- Minimize cross-boundary calls
- Use object pooling for frequently created objects
- Cache native pointers when possible
- Avoid boxing/unboxing in hot paths

## Troubleshooting

### Common Issues
1. **Plugin not loading**: Check UE5 logs for .NET runtime initialization errors
2. **C# classes not visible**: Ensure proper UClass attributes and build
3. **Function calls failing**: Verify UFunction signatures and parameters

### Debug Information
- Check Output Log for UCSharp messages
- Enable verbose logging in UCSharp.cpp
- Use Visual Studio debugger for C# code
- Use UE5 debugger for C++ code

## Contributing

This is currently a prototype in active development. The codebase will evolve significantly as we implement the core binding mechanisms and complete the MVP.

## License

TBD - License will be determined based on project requirements and distribution strategy.