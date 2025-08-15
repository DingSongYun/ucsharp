using System;

namespace UCSharp.Core
{
    /// <summary>
    /// Attribute to mark a C# method as a UFunction
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = true)]
    public class UFunctionAttribute : Attribute
    {
        /// <summary>
        /// Function name in Blueprint (defaults to method name)
        /// </summary>
        public string? BlueprintName { get; set; }
        
        /// <summary>
        /// Whether this function can be called from Blueprint
        /// </summary>
        public bool BlueprintCallable { get; set; } = true;
        
        /// <summary>
        /// Whether this function can be implemented in Blueprint
        /// </summary>
        public bool BlueprintImplementableEvent { get; set; } = false;
        
        /// <summary>
        /// Whether this function is a Blueprint native event
        /// </summary>
        public bool BlueprintNativeEvent { get; set; } = false;
        
        /// <summary>
        /// Whether this function is pure (no side effects)
        /// </summary>
        public bool BlueprintPure { get; set; } = false;
        
        /// <summary>
        /// Category for Blueprint organization
        /// </summary>
        public string Category { get; set; } = "Default";
        
        /// <summary>
        /// Keywords for Blueprint search
        /// </summary>
        public string Keywords { get; set; } = "";
        
        /// <summary>
        /// Tooltip text for Blueprint
        /// </summary>
        public string ToolTip { get; set; } = "";
        
        /// <summary>
        /// Whether this function can be called on the server
        /// </summary>
        public bool Server { get; set; } = false;
        
        /// <summary>
        /// Whether this function can be called on clients
        /// </summary>
        public bool Client { get; set; } = false;
        
        /// <summary>
        /// Whether this function requires validation
        /// </summary>
        public bool WithValidation { get; set; } = false;
        
        /// <summary>
        /// Whether this function is reliable for networking
        /// </summary>
        public bool Reliable { get; set; } = true;
    }
    
    /// <summary>
    /// Attribute to mark a C# property or field as a UProperty
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false, Inherited = true)]
    public class UPropertyAttribute : Attribute
    {
        /// <summary>
        /// Property name in Blueprint (defaults to property name)
        /// </summary>
        public string? BlueprintName { get; set; }
        
        /// <summary>
        /// Whether this property is visible in Blueprint
        /// </summary>
        public bool BlueprintReadOnly { get; set; } = false;
        
        /// <summary>
        /// Whether this property can be written in Blueprint
        /// </summary>
        public bool BlueprintReadWrite { get; set; } = true;
        
        /// <summary>
        /// Whether this property is editable in the editor
        /// </summary>
        public bool EditAnywhere { get; set; } = true;
        
        /// <summary>
        /// Whether this property is editable only in defaults
        /// </summary>
        public bool EditDefaultsOnly { get; set; } = false;
        
        /// <summary>
        /// Whether this property is editable only on instances
        /// </summary>
        public bool EditInstanceOnly { get; set; } = false;
        
        /// <summary>
        /// Whether this property is visible in the editor
        /// </summary>
        public bool VisibleAnywhere { get; set; } = true;
        
        /// <summary>
        /// Whether this property is visible only in defaults
        /// </summary>
        public bool VisibleDefaultsOnly { get; set; } = false;
        
        /// <summary>
        /// Whether this property is visible only on instances
        /// </summary>
        public bool VisibleInstanceOnly { get; set; } = false;
        
        /// <summary>
        /// Category for editor organization
        /// </summary>
        public string Category { get; set; } = "Default";
        
        /// <summary>
        /// Tooltip text for editor
        /// </summary>
        public string ToolTip { get; set; } = "";
        
        /// <summary>
        /// Display name in editor
        /// </summary>
        public string DisplayName { get; set; } = "";
        
        /// <summary>
        /// Whether this property should be saved
        /// </summary>
        public bool SaveGame { get; set; } = false;
        
        /// <summary>
        /// Whether this property should replicate
        /// </summary>
        public bool Replicated { get; set; } = false;
        
        /// <summary>
        /// Replication condition
        /// </summary>
        public string ReplicatedUsing { get; set; } = "";
        
        /// <summary>
        /// Meta data for the property
        /// </summary>
        public string Meta { get; set; } = "";
        
        /// <summary>
        /// Minimum value for numeric properties
        /// </summary>
        public double ClampMin { get; set; } = double.MinValue;
        
        /// <summary>
        /// Maximum value for numeric properties
        /// </summary>
        public double ClampMax { get; set; } = double.MaxValue;
        
        /// <summary>
        /// UI minimum value for numeric properties
        /// </summary>
        public double UIMin { get; set; } = double.MinValue;
        
        /// <summary>
        /// UI maximum value for numeric properties
        /// </summary>
        public double UIMax { get; set; } = double.MaxValue;
    }
    
    /// <summary>
    /// Attribute to mark a C# class as a UClass
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public class UClassAttribute : Attribute
    {
        /// <summary>
        /// Class name in Blueprint (defaults to class name)
        /// </summary>
        public string? BlueprintName { get; set; }
        
        /// <summary>
        /// Whether this class can be used as a Blueprint base
        /// </summary>
        public bool Blueprintable { get; set; } = true;
        
        /// <summary>
        /// Whether this class is Blueprint type
        /// </summary>
        public bool BlueprintType { get; set; } = true;
        
        /// <summary>
        /// Whether this class is abstract
        /// </summary>
        public bool Abstract { get; set; } = false;
        
        /// <summary>
        /// Whether this class is deprecated
        /// </summary>
        public bool Deprecated { get; set; } = false;
        
        /// <summary>
        /// Class flags
        /// </summary>
        public EClassFlags ClassFlags { get; set; } = EClassFlags.None;
        
        /// <summary>
        /// Category for editor organization
        /// </summary>
        public string Category { get; set; } = "Default";
        
        /// <summary>
        /// Keywords for Blueprint search
        /// </summary>
        public string Keywords { get; set; } = "";
        
        /// <summary>
        /// Tooltip text for Blueprint
        /// </summary>
        public string ToolTip { get; set; } = "";
        
        /// <summary>
        /// Display name in editor
        /// </summary>
        public string DisplayName { get; set; } = "";
    }
    
    /// <summary>
    /// Class flags enumeration
    /// </summary>
    [Flags]
    public enum EClassFlags : uint
    {
        None = 0,
        Abstract = 1 << 0,
        DefaultConfig = 1 << 1,
        Config = 1 << 2,
        Transient = 1 << 3,
        Parsed = 1 << 4,
        MatchedSerializers = 1 << 5,
        ProjectUserConfig = 1 << 6,
        Native = 1 << 7,
        NoExport = 1 << 8,
        NotPlaceable = 1 << 9,
        PerObjectConfig = 1 << 10,
        ReplicationDataIsSetUp = 1 << 11,
        EditInlineNew = 1 << 12,
        CollapseCategories = 1 << 13,
        Interface = 1 << 14,
        CustomConstructor = 1 << 15,
        Const = 1 << 16,
        LayoutChanging = 1 << 17,
        CompiledFromBlueprint = 1 << 18,
        MinimalAPI = 1 << 19,
        RequiredAPI = 1 << 20,
        DefaultToInstanced = 1 << 21,
        TokenStreamAssembled = 1 << 22,
        HasInstancedReference = 1 << 23,
        Hidden = 1 << 24,
        Deprecated = 1 << 25,
        HideDropDown = 1 << 26,
        GlobalUserConfig = 1 << 27,
        Intrinsic = 1 << 28,
        Constructed = 1 << 29,
        ConfigDoNotCheckDefaults = 1 << 30,
        NewerVersionExists = 1u << 31
    }
    
    /// <summary>
    /// Attribute to mark a parameter as output parameter
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, AllowMultiple = false, Inherited = false)]
    public class OutAttribute : Attribute
    {
    }
    
    /// <summary>
    /// Attribute to mark a parameter as reference parameter
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, AllowMultiple = false, Inherited = false)]
    public class RefAttribute : Attribute
    {
    }
    
    /// <summary>
    /// Attribute to specify Blueprint category for functions and properties
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false, Inherited = true)]
    public class CategoryAttribute : Attribute
    {
        public string Category { get; }
        
        public CategoryAttribute(string category)
        {
            Category = category;
        }
    }
    
    /// <summary>
    /// Attribute to specify meta data
    /// </summary>
    [AttributeUsage(AttributeTargets.All, AllowMultiple = true, Inherited = true)]
    public class MetaAttribute : Attribute
    {
        public string Key { get; }
        public string Value { get; }
        
        public MetaAttribute(string key, string value)
        {
            Key = key;
            Value = value;
        }
    }
}