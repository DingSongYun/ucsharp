using System;
using System.Collections.Generic;
using System.Reflection;
using UCSharp.Examples;

namespace UCSharp.Core
{
    /// <summary>
    /// Manages C# scripts and their lifecycle
    /// </summary>
    public static class ScriptManager
    {
        /// <summary>
        /// Dictionary of active actors by their unique ID
        /// </summary>
        private static Dictionary<int, Actor> activeActors = new Dictionary<int, Actor>();

        /// <summary>
        /// Next available actor ID
        /// </summary>
        private static int nextActorId = 1;

        /// <summary>
        /// Whether the script manager has been initialized
        /// </summary>
        public static bool IsInitialized { get; private set; } = false;

        /// <summary>
        /// Initialize the script manager
        /// </summary>
        public static void Initialize()
        {
            if (IsInitialized)
            {
                Console.WriteLine("ScriptManager already initialized");
                return;
            }

            Console.WriteLine("Initializing C# Script Manager...");
            
            // Register available actor types
            RegisterActorTypes();
            
            IsInitialized = true;
            Console.WriteLine("C# Script Manager initialized successfully");
        }

        /// <summary>
        /// Shutdown the script manager
        /// </summary>
        public static void Shutdown()
        {
            if (!IsInitialized)
                return;

            Console.WriteLine("Shutting down C# Script Manager...");
            
            // Destroy all active actors
            foreach (var actor in activeActors.Values)
            {
                actor.EndPlay();
            }
            activeActors.Clear();
            
            IsInitialized = false;
            Console.WriteLine("C# Script Manager shutdown complete");
        }

        /// <summary>
        /// Create a new actor instance
        /// </summary>
        /// <typeparam name="T">Type of actor to create</typeparam>
        /// <returns>Actor ID, or -1 if failed</returns>
        public static int CreateActor<T>() where T : Actor, new()
        {
            if (!IsInitialized)
            {
                Console.WriteLine("ScriptManager not initialized");
                return -1;
            }

            try
            {
                T actor = new T();
                int actorId = nextActorId++;
                
                activeActors[actorId] = actor;
                actor.BeginPlay();
                
                Console.WriteLine($"Created actor {typeof(T).Name} with ID {actorId}");
                return actorId;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to create actor {typeof(T).Name}: {ex.Message}");
                return -1;
            }
        }

        /// <summary>
        /// Create a new actor instance by type name
        /// </summary>
        /// <param name="typeName">Name of the actor type</param>
        /// <returns>Actor ID, or -1 if failed</returns>
        public static int CreateActorByName(string typeName)
        {
            if (!IsInitialized)
            {
                Console.WriteLine("ScriptManager not initialized");
                return -1;
            }

            try
            {
                // Find the type in the current assembly
                Type actorType = Type.GetType($"UCSharp.Examples.{typeName}") ?? 
                                Type.GetType($"UCSharp.Core.{typeName}");
                
                if (actorType == null || !actorType.IsSubclassOf(typeof(Actor)))
                {
                    Console.WriteLine($"Actor type '{typeName}' not found or not derived from Actor");
                    return -1;
                }

                Actor actor = (Actor)Activator.CreateInstance(actorType);
                int actorId = nextActorId++;
                
                activeActors[actorId] = actor;
                actor.BeginPlay();
                
                Console.WriteLine($"Created actor {typeName} with ID {actorId}");
                return actorId;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to create actor {typeName}: {ex.Message}");
                return -1;
            }
        }

        /// <summary>
        /// Destroy an actor by ID
        /// </summary>
        /// <param name="actorId">ID of the actor to destroy</param>
        /// <returns>True if destroyed successfully</returns>
        public static bool DestroyActor(int actorId)
        {
            if (!activeActors.TryGetValue(actorId, out Actor actor))
            {
                Console.WriteLine($"Actor with ID {actorId} not found");
                return false;
            }

            actor.EndPlay();
            activeActors.Remove(actorId);
            
            Console.WriteLine($"Destroyed actor with ID {actorId}");
            return true;
        }

        /// <summary>
        /// Get an actor by ID
        /// </summary>
        /// <param name="actorId">Actor ID</param>
        /// <returns>Actor instance or null if not found</returns>
        public static Actor GetActor(int actorId)
        {
            activeActors.TryGetValue(actorId, out Actor actor);
            return actor;
        }

        /// <summary>
        /// Update all active actors (called every frame)
        /// </summary>
        /// <param name="deltaTime">Time since last frame</param>
        public static void TickAllActors(float deltaTime)
        {
            if (!IsInitialized)
                return;

            foreach (var actor in activeActors.Values)
            {
                if (actor.IsActive)
                {
                    actor.Tick(deltaTime);
                }
            }
        }

        /// <summary>
        /// Get the number of active actors
        /// </summary>
        /// <returns>Number of active actors</returns>
        public static int GetActiveActorCount()
        {
            return activeActors.Count;
        }

        /// <summary>
        /// Register available actor types for dynamic creation
        /// </summary>
        private static void RegisterActorTypes()
        {
            Console.WriteLine("Registering actor types:");
            
            // Get all types that inherit from Actor
            Assembly assembly = Assembly.GetExecutingAssembly();
            foreach (Type type in assembly.GetTypes())
            {
                if (type.IsSubclassOf(typeof(Actor)) && !type.IsAbstract)
                {
                    Console.WriteLine($"  - {type.Name}");
                }
            }
        }

        /// <summary>
        /// Demo method to create and test an Actor
        /// </summary>
        public static void RunDemo()
        {
            Console.WriteLine("\n=== Running C# Script Demo ===");
            
            // Create an actor by name
            int actorId = CreateActorByName("DemoActor");
            if (actorId == -1)
            {
                Console.WriteLine("Failed to create actor");
                return;
            }

            // Simulate some frame updates
            Console.WriteLine("\nSimulating 5 seconds of gameplay...");
            float deltaTime = 0.016f; // ~60 FPS
            float totalTime = 0.0f;
            
            while (totalTime < 5.0f)
            {
                TickAllActors(deltaTime);
                totalTime += deltaTime;
                
                // Sleep a bit to make the demo visible
                System.Threading.Thread.Sleep(16); // ~60 FPS
            }

            // Test basic actor functionality
            var actor = GetActor(actorId);
            if (actor != null)
            {
                Console.WriteLine("\nActor is active and running...");
                
                // Simulate a bit more
                totalTime = 0.0f;
                while (totalTime < 2.0f)
                {
                    TickAllActors(deltaTime);
                    totalTime += deltaTime;
                    System.Threading.Thread.Sleep(16);
                }
            }

            // Clean up
            DestroyActor(actorId);
            Console.WriteLine("\n=== Demo Complete ===");
        }
    }
}