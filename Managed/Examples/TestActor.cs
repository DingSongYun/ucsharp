using System;
using System.Numerics;
using System.Threading.Tasks;

namespace UCSharp.Managed.Examples
{
    // 简化的结构体定义
    public struct Rotator
    {
        public float Pitch, Yaw, Roll;
        
        public Rotator(float pitch, float yaw, float roll)
        {
            Pitch = pitch; Yaw = yaw; Roll = roll;
        }
        
        public override string ToString()
        {
            return $"Pitch={Pitch}, Yaw={Yaw}, Roll={Roll}";
        }
    }
    /// <summary>
    /// 简化的测试用C#类，用于验证UCSharp框架的基本功能
    /// 注意：这是一个简化版本，实际的UCSharp框架需要更多的基础设施
    /// </summary>
    public class TestActor
    {
        #region 属性测试
        
        public float TestFloat { get; set; } = 42.0f;
        public int TestInt { get; set; } = 100;
        public string TestString { get; set; } = "Hello UCSharp";
        public bool TestBool { get; set; } = true;
        
        // 简化的向量和旋转器结构
        public Vector3 TestVector { get; set; } = new Vector3(1.0f, 2.0f, 3.0f);
        public Rotator TestRotator { get; set; } = new Rotator(0.0f, 90.0f, 0.0f);
        
        #endregion
        
        #region 简化的组件模拟
        
        public string MeshComponentName { get; private set; } = "TestMesh";
        public string RootComponentName { get; private set; } = "RootComponent";
        
        #endregion
        
        #region 生命周期方法
        
        public TestActor()
        {
            Console.WriteLine("TestActor constructed in C#");
        }
        
        public void BeginPlay()
        {
            Console.WriteLine($"TestActor BeginPlay - TestFloat: {TestFloat}");
            Console.WriteLine($"TestActor BeginPlay - TestString: {TestString}");
            
            // 测试属性访问
            TestPropertyAccess();
            
            // 测试方法调用
            TestMethodCalls();
        }
        
        public void EndPlay(string reason)
        {
            Console.WriteLine($"TestActor EndPlay - Reason: {reason}");
        }
        
        public void Tick(float deltaTime)
        {
            // 简单的旋转动画模拟
            var rotator = TestRotator;
            rotator.Yaw += deltaTime * 30.0f; // 每秒30度
            TestRotator = rotator;
        }
        
        #endregion
        
        #region 测试方法
        
        public void TestPropertyAccess()
        {
            Console.WriteLine("=== Property Access Test ===");
            
            // 读取属性
            Console.WriteLine($"TestFloat: {TestFloat}");
            Console.WriteLine($"TestInt: {TestInt}");
            Console.WriteLine($"TestString: {TestString}");
            Console.WriteLine($"TestBool: {TestBool}");
            Console.WriteLine($"TestVector: {TestVector}");
            Console.WriteLine($"TestRotator: {TestRotator}");
            
            // 修改属性
            TestFloat += 1.0f;
            TestInt += 1;
            TestString += " - Modified";
            TestBool = !TestBool;
            TestVector = new Vector3(TestVector.X + 0.1f, TestVector.Y + 0.1f, TestVector.Z + 0.1f);
            var rotator = TestRotator;
            rotator.Yaw += 1.0f;
            TestRotator = rotator;
            
            Console.WriteLine("Properties modified successfully");
        }
        
        public void TestMethodCalls()
        {
            Console.WriteLine("=== Method Call Test ===");
            
            // 测试简单方法调用
            var result = AddTwoNumbers(10, 20);
            Console.WriteLine($"AddTwoNumbers(10, 20) = {result}");
            
            // 测试字符串方法
            var greeting = CreateGreeting("UCSharp");
            Console.WriteLine($"CreateGreeting result: {greeting}");
            
            // 测试向量计算
            var distance = CalculateDistance(new Vector3(0, 0, 0), new Vector3(3, 4, 0));
            Console.WriteLine($"Distance calculation: {distance}");
            
            // 测试异步方法
            TestAsyncMethod();
        }
        
        public int AddTwoNumbers(int a, int b)
        {
            return a + b;
        }
        
        public string CreateGreeting(string name)
        {
            return $"Hello, {name}! Welcome to UCSharp.";
        }
        
        public float CalculateDistance(Vector3 pointA, Vector3 pointB)
        {
            return (pointA - pointB).Length();
        }
        
        public void TestAsyncMethod()
        {
            Console.WriteLine("Starting async operation...");
            
            // 模拟异步操作
            System.Threading.Tasks.Task.Run(async () =>
            {
                await System.Threading.Tasks.Task.Delay(1000);
                Console.WriteLine("Async operation completed!");
            });
        }
        
        public void TestMemoryAllocation()
        {
            Console.WriteLine("=== Memory Allocation Test ===");
            
            // 分配一些内存来测试垃圾回收
            var largeArray = new byte[1024 * 1024]; // 1MB
            for (int i = 0; i < largeArray.Length; i++)
            {
                largeArray[i] = (byte)(i % 256);
            }
            
            Console.WriteLine($"Allocated {largeArray.Length} bytes");
            
            // 强制垃圾回收
            GC.Collect();
            GC.WaitForPendingFinalizers();
            
            Console.WriteLine("Memory allocation test completed");
        }
        
        public void TestExceptionHandling()
        {
            Console.WriteLine("=== Exception Handling Test ===");
            
            try
            {
                // 故意触发异常
                int zero = 0;
                var result = 10 / zero;
            }
            catch (DivideByZeroException ex)
            {
                Console.WriteLine($"Caught expected exception: {ex.Message}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Caught unexpected exception: {ex.Message}");
            }
            
            Console.WriteLine("Exception handling test completed");
        }
        
        #endregion
        
        #region 事件测试
        
        public event Action<string> OnTestEvent;
        
        public void TriggerTestEvent()
        {
            OnTestEvent?.Invoke("Test event triggered from C#!");
            Console.WriteLine("Test event triggered from C#!");
        }
        
        #endregion
        
        #region 性能测试
        
        public void PerformanceTest(int iterations = 10000)
        {
            Console.WriteLine($"=== Performance Test ({iterations} iterations) ===");
            
            var stopwatch = System.Diagnostics.Stopwatch.StartNew();
            
            for (int i = 0; i < iterations; i++)
            {
                // 执行一些计算密集的操作
                var result = Math.Sin(i) * Math.Cos(i) + Math.Sqrt(i);
                TestFloat = (float)result;
            }
            
            stopwatch.Stop();
            
            var timePerIteration = stopwatch.ElapsedMilliseconds / (double)iterations;
            Console.WriteLine($"Performance test completed in {stopwatch.ElapsedMilliseconds}ms");
            Console.WriteLine($"Average time per iteration: {timePerIteration:F4}ms");
        }
        
        #endregion
    }
    

}