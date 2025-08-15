using System;
using UCSharp.Core;

namespace UCSharp
{
    /// <summary>
    /// Main entry point for the C# scripting system
    /// This class provides the interface between UE5 C++ and C# scripts
    /// </summary>
    public static class Program
    {
        /// <summary>
        /// Initialize the C# scripting system
        /// Called from UE5 C++ code when the plugin starts
        /// </summary>
        public static void Initialize()
        {
            try
            {
                Console.WriteLine("=== UCSharp C# Scripting System ===");
                Console.WriteLine("Initializing C# scripting system...");
                
                // Initialize the script manager
                ScriptManager.Initialize();
                
                Console.WriteLine("C# scripting system initialized successfully!");
                Console.WriteLine("Ready to create and manage C# actors.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to initialize C# scripting system: {ex.Message}");
                Console.WriteLine($"Stack trace: {ex.StackTrace}");
            }
        }

        /// <summary>
        /// Shutdown the C# scripting system
        /// Called from UE5 C++ code when the plugin shuts down
        /// </summary>
        public static void Shutdown()
        {
            try
            {
                Console.WriteLine("Shutting down C# scripting system...");
                
                // Shutdown the script manager
                ScriptManager.Shutdown();
                
                Console.WriteLine("C# scripting system shutdown complete.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error during C# scripting system shutdown: {ex.Message}");
            }
        }

        /// <summary>
        /// Update all C# scripts (called every frame from UE5)
        /// </summary>
        /// <param name="deltaTime">Time since last frame in seconds</param>
        public static void Tick(float deltaTime)
        {
            try
            {
                ScriptManager.TickAllActors(deltaTime);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error during C# script tick: {ex.Message}");
            }
        }

        /// <summary>
        /// Create a new C# actor by type name
        /// </summary>
        /// <param name="actorTypeName">Name of the actor type to create</param>
        /// <returns>Actor ID, or -1 if failed</returns>
        public static int CreateActor(string actorTypeName)
        {
            try
            {
                return ScriptManager.CreateActorByName(actorTypeName);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error creating actor {actorTypeName}: {ex.Message}");
                return -1;
            }
        }

        /// <summary>
        /// Destroy a C# actor by ID
        /// </summary>
        /// <param name="actorId">ID of the actor to destroy</param>
        /// <returns>True if destroyed successfully</returns>
        public static bool DestroyActor(int actorId)
        {
            try
            {
                return ScriptManager.DestroyActor(actorId);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error destroying actor {actorId}: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Get the number of active C# actors
        /// </summary>
        /// <returns>Number of active actors</returns>
        public static int GetActiveActorCount()
        {
            try
            {
                return ScriptManager.GetActiveActorCount();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error getting active actor count: {ex.Message}");
                return 0;
            }
        }

        /// <summary>
        /// Run a demonstration of the C# scripting system
        /// This can be called from UE5 to test the system
        /// </summary>
        public static void RunDemo()
        {
            try
            {
                Console.WriteLine("\n=== Starting C# Scripting Demo ===");
                
                if (!ScriptManager.IsInitialized)
                {
                    Console.WriteLine("Script manager not initialized. Initializing now...");
                    Initialize();
                }
                
                ScriptManager.RunDemo();
                
                Console.WriteLine("=== C# Scripting Demo Complete ===");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error during demo: {ex.Message}");
                Console.WriteLine($"Stack trace: {ex.StackTrace}");
            }
        }

        /// <summary>
        /// Get version information about the C# scripting system
        /// </summary>
        /// <returns>Version string</returns>
        public static string GetVersion()
        {
            return "UCSharp C# Scripting System v1.0.0 - MVP Demo";
        }

        /// <summary>
        /// Test method to verify C# interop is working
        /// </summary>
        /// <param name="message">Test message</param>
        /// <returns>Processed message</returns>
        public static string TestInterop(string message)
        {
            string response = $"C# received: '{message}' at {DateTime.Now:HH:mm:ss.fff}";
            Console.WriteLine(response);
            return response;
        }

        /// <summary>
        /// Main method for standalone testing (not used when called from UE5)
        /// </summary>
        /// <param name="args">Command line arguments</param>
        public static void Main(string[] args)
        {
            Console.WriteLine("UCSharp C# Scripting System - Standalone Test Mode");
            Console.WriteLine("Note: This is for testing only. In production, this will be called from UE5.");
            Console.WriteLine();
            
            // Initialize and run demo
            Initialize();
            RunDemo();
            
            Console.WriteLine("\nPress any key to exit...");
            Console.ReadKey();
            
            Shutdown();
        }
    }
}