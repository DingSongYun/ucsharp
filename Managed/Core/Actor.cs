using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace UCSharp.Core
{
    /// <summary>
    /// Base class for all C# actors in the UCSharp system
    /// Provides basic functionality similar to UE5's AActor
    /// Inherits from UObject to integrate with UE5's object system
    /// </summary>
    public class Actor : UObject
    {
        /// <summary>
        /// Actor's world location
        /// </summary>
        public Vector3 Location { get; set; } = Vector3.Zero;

        /// <summary>
        /// Actor's world rotation (Euler angles in degrees)
        /// </summary>
        public Vector3 Rotation { get; set; } = Vector3.Zero;

        /// <summary>
        /// Actor's world scale
        /// </summary>
        public Vector3 Scale { get; set; } = Vector3.One;

        /// <summary>
        /// Whether this actor is currently active in the world
        /// </summary>
        public bool IsActive { get; set; } = true;

        /// <summary>
        /// Called when the actor is first created
        /// </summary>
        public virtual void BeginPlay()
        {
            Console.WriteLine($"Actor {GetType().Name} BeginPlay called");
        }

        /// <summary>
        /// Called every frame
        /// </summary>
        /// <param name="deltaTime">Time since last frame in seconds</param>
        public virtual void Tick(float deltaTime)
        {
            // Override in derived classes for per-frame logic
        }

        /// <summary>
        /// Called when the actor is being destroyed
        /// </summary>
        public virtual void EndPlay()
        {
            Console.WriteLine($"Actor {GetType().Name} EndPlay called");
        }

        /// <summary>
        /// Set the actor's world location
        /// </summary>
        /// <param name="newLocation">New world location</param>
        public void SetActorLocation(Vector3 newLocation)
        {
            Location = newLocation;
            // TODO: Call native UE5 function to update actual actor location
            Console.WriteLine($"Actor location set to: {newLocation}");
        }

        /// <summary>
        /// Set the actor's world rotation
        /// </summary>
        /// <param name="newRotation">New world rotation in degrees</param>
        public void SetActorRotation(Vector3 newRotation)
        {
            Rotation = newRotation;
            // TODO: Call native UE5 function to update actual actor rotation
            Console.WriteLine($"Actor rotation set to: {newRotation}");
        }

        /// <summary>
        /// Set the actor's world scale
        /// </summary>
        /// <param name="newScale">New world scale</param>
        public void SetActorScale(Vector3 newScale)
        {
            Scale = newScale;
            // TODO: Call native UE5 function to update actual actor scale
            Console.WriteLine($"Actor scale set to: {newScale}");
        }

        /// <summary>
        /// Get the distance to another actor
        /// </summary>
        /// <param name="otherActor">The other actor</param>
        /// <returns>Distance in world units</returns>
        public float GetDistanceTo(Actor otherActor)
        {
            if (otherActor == null)
                return float.MaxValue;

            return Vector3.Distance(Location, otherActor.Location);
        }

        /// <summary>
        /// Destroy this actor
        /// </summary>
        public void Destroy()
        {
            EndPlay();
            // TODO: Call native UE5 function to destroy the actor
            Console.WriteLine($"Actor {GetType().Name} destroyed");
        }
    }
}