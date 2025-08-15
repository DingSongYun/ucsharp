using System;
using UCSharp.Core;

namespace UCSharp.Examples
{
    /// <summary>
    /// Example C# Actor demonstrating UCSharp integration
    /// This actor can be used in Blueprint and shows basic functionality
    /// </summary>
    [UClass(BlueprintName = "MyTestActor", Blueprintable = true, Category = "UCSharp Examples")]
    public class MyTestActor : AActor
    {
        #region Properties
        
        /// <summary>
        /// Health value that can be edited in Blueprint
        /// </summary>
        [UProperty(Category = "Health", EditAnywhere = true, BlueprintReadWrite = true, 
                   ClampMin = 0, ClampMax = 100, ToolTip = "Current health of the actor")]
        public float Health { get; set; } = 100.0f;
        
        /// <summary>
        /// Maximum health value
        /// </summary>
        [UProperty(Category = "Health", EditAnywhere = true, BlueprintReadOnly = true,
                   ClampMin = 1, ClampMax = 1000, ToolTip = "Maximum health of the actor")]
        public float MaxHealth { get; set; } = 100.0f;
        
        /// <summary>
        /// Actor's display name
        /// </summary>
        [UProperty(Category = "Info", EditAnywhere = true, BlueprintReadWrite = true,
                   ToolTip = "Display name for this actor")]
        public string DisplayName { get; set; } = "Test Actor";
        
        /// <summary>
        /// Whether the actor is invulnerable
        /// </summary>
        [UProperty(Category = "Health", EditAnywhere = true, BlueprintReadWrite = true,
                   ToolTip = "Whether this actor can take damage")]
        public bool IsInvulnerable { get; set; } = false;
        
        /// <summary>
        /// Movement speed
        /// </summary>
        [UProperty(Category = "Movement", EditAnywhere = true, BlueprintReadWrite = true,
                   ClampMin = 0, ClampMax = 2000, ToolTip = "Movement speed in units per second")]
        public float MovementSpeed { get; set; } = 600.0f;
        
        #endregion
        
        #region Lifecycle
        
        /// <summary>
        /// Called when the actor begins play
        /// </summary>
        protected override void BeginPlay()
        {
            base.BeginPlay();
            
            // Initialize actor
            Health = MaxHealth;
            
            // Log actor creation
            LogMessage($"MyTestActor '{DisplayName}' has begun play with {Health} health");
            
            // Call Blueprint event
            OnActorSpawned();
        }
        
        /// <summary>
        /// Called when the actor ends play
        /// </summary>
        protected override void EndPlay()
        {
            LogMessage($"MyTestActor '{DisplayName}' is ending play");
            
            base.EndPlay();
        }
        
        /// <summary>
        /// Called every frame
        /// </summary>
        /// <param name="deltaTime">Time since last frame</param>
        protected override void Tick(float deltaTime)
        {
            base.Tick(deltaTime);
            
            // Example: Regenerate health over time
            if (Health < MaxHealth && !IsInvulnerable)
            {
                float regenRate = 5.0f; // 5 health per second
                Health = Math.Min(MaxHealth, Health + regenRate * deltaTime);
            }
        }
        
        #endregion
        
        #region UFunctions
        
        /// <summary>
        /// Take damage and reduce health
        /// </summary>
        /// <param name="damageAmount">Amount of damage to take</param>
        /// <param name="damageSource">Source of the damage</param>
        /// <returns>Actual damage taken</returns>
        [UFunction(Category = "Health", BlueprintCallable = true,
                   ToolTip = "Apply damage to this actor")]
        public float TakeDamage(float damageAmount, string damageSource = "Unknown")
        {
            if (IsInvulnerable || damageAmount <= 0)
            {
                return 0.0f;
            }
            
            float actualDamage = Math.Min(damageAmount, Health);
            Health -= actualDamage;
            
            LogMessage($"'{DisplayName}' took {actualDamage} damage from {damageSource}. Health: {Health}/{MaxHealth}");
            
            // Call Blueprint event
            OnDamageTaken(actualDamage, damageSource);
            
            // Check if actor should be destroyed
            if (Health <= 0)
            {
                OnActorDied();
            }
            
            return actualDamage;
        }
        
        /// <summary>
        /// Heal the actor
        /// </summary>
        /// <param name="healAmount">Amount to heal</param>
        /// <returns>Actual amount healed</returns>
        [UFunction(Category = "Health", BlueprintCallable = true,
                   ToolTip = "Heal this actor")]
        public float Heal(float healAmount)
        {
            if (healAmount <= 0 || Health >= MaxHealth)
            {
                return 0.0f;
            }
            
            float actualHeal = Math.Min(healAmount, MaxHealth - Health);
            Health += actualHeal;
            
            LogMessage($"'{DisplayName}' healed for {actualHeal}. Health: {Health}/{MaxHealth}");
            
            // Call Blueprint event
            OnHealed(actualHeal);
            
            return actualHeal;
        }
        
        /// <summary>
        /// Get current health percentage
        /// </summary>
        /// <returns>Health as percentage (0-1)</returns>
        [UFunction(Category = "Health", BlueprintCallable = true, BlueprintPure = true,
                   ToolTip = "Get health as a percentage")]
        public float GetHealthPercentage()
        {
            return MaxHealth > 0 ? Health / MaxHealth : 0.0f;
        }
        
        /// <summary>
        /// Check if the actor is alive
        /// </summary>
        /// <returns>True if health > 0</returns>
        [UFunction(Category = "Health", BlueprintCallable = true, BlueprintPure = true,
                   ToolTip = "Check if the actor is alive")]
        public bool IsAlive()
        {
            return Health > 0;
        }
        
        /// <summary>
        /// Set invulnerability state
        /// </summary>
        /// <param name="invulnerable">Whether to make invulnerable</param>
        [UFunction(Category = "Health", BlueprintCallable = true,
                   ToolTip = "Set invulnerability state")]
        public void SetInvulnerable(bool invulnerable)
        {
            IsInvulnerable = invulnerable;
            LogMessage($"'{DisplayName}' invulnerability set to {invulnerable}");
            
            // Call Blueprint event
            OnInvulnerabilityChanged(invulnerable);
        }
        
        /// <summary>
        /// Log a message (can be called from Blueprint)
        /// </summary>
        /// <param name="message">Message to log</param>
        [UFunction(Category = "Debug", BlueprintCallable = true,
                   ToolTip = "Log a debug message")]
        public void LogMessage(string message)
        {
            // This would call UE5's logging system
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] {GetName()}: {message}");
        }
        
        #endregion
        
        #region Blueprint Events
        
        /// <summary>
        /// Blueprint implementable event called when actor spawns
        /// </summary>
        [UFunction(BlueprintImplementableEvent = true, Category = "Events",
                   ToolTip = "Called when the actor is spawned")]
        protected virtual void OnActorSpawned()
        {
            // Blueprint can implement this
        }
        
        /// <summary>
        /// Blueprint implementable event called when damage is taken
        /// </summary>
        /// <param name="damageAmount">Amount of damage taken</param>
        /// <param name="damageSource">Source of damage</param>
        [UFunction(BlueprintImplementableEvent = true, Category = "Events",
                   ToolTip = "Called when the actor takes damage")]
        protected virtual void OnDamageTaken(float damageAmount, string damageSource)
        {
            // Blueprint can implement this
        }
        
        /// <summary>
        /// Blueprint implementable event called when healed
        /// </summary>
        /// <param name="healAmount">Amount healed</param>
        [UFunction(BlueprintImplementableEvent = true, Category = "Events",
                   ToolTip = "Called when the actor is healed")]
        protected virtual void OnHealed(float healAmount)
        {
            // Blueprint can implement this
        }
        
        /// <summary>
        /// Blueprint implementable event called when actor dies
        /// </summary>
        [UFunction(BlueprintImplementableEvent = true, Category = "Events",
                   ToolTip = "Called when the actor dies")]
        protected virtual void OnActorDied()
        {
            // Blueprint can implement this
        }
        
        /// <summary>
        /// Blueprint implementable event called when invulnerability changes
        /// </summary>
        /// <param name="isInvulnerable">New invulnerability state</param>
        [UFunction(BlueprintImplementableEvent = true, Category = "Events",
                   ToolTip = "Called when invulnerability state changes")]
        protected virtual void OnInvulnerabilityChanged(bool isInvulnerable)
        {
            // Blueprint can implement this
        }
        
        #endregion
    }
    
    /// <summary>
    /// Base AActor class for C# actors
    /// This would be defined in the core UCSharp library
    /// </summary>
    public abstract class AActor : UObject
    {
        /// <summary>
        /// Constructor for C# created actors
        /// </summary>
        protected AActor() : base()
        {
        }
        
        /// <summary>
        /// Constructor for wrapping existing native actors
        /// </summary>
        /// <param name="nativePtr">Native actor pointer</param>
        /// <param name="ownsPtr">Whether this wrapper owns the pointer</param>
        protected AActor(IntPtr nativePtr, bool ownsPtr = false) : base(nativePtr, ownsPtr)
        {
        }
    }
}