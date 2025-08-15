using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace UCSharp.Tests
{
    /// <summary>
    /// UCSharp测试框架 - C#端支持
    /// 提供与UE5自动化测试框架的集成
    /// </summary>
    public static class UCSharpTestFramework
    {
        private static readonly List<ITestCase> RegisteredTests = new List<ITestCase>();
        private static bool _isInitialized = false;

        /// <summary>
        /// 初始化测试框架
        /// </summary>
        public static void Initialize()
        {
            if (_isInitialized)
                return;

            Console.WriteLine("[UCSharp.Tests] Initializing test framework...");
            
            // 注册内置测试用例
            RegisterBuiltInTests();
            
            _isInitialized = true;
            Console.WriteLine($"[UCSharp.Tests] Test framework initialized with {RegisteredTests.Count} test cases");
        }

        /// <summary>
        /// 注册测试用例
        /// </summary>
        public static void RegisterTest(ITestCase testCase)
        {
            if (testCase == null)
                throw new ArgumentNullException(nameof(testCase));

            RegisteredTests.Add(testCase);
            Console.WriteLine($"[UCSharp.Tests] Registered test: {testCase.Name}");
        }

        /// <summary>
        /// 运行所有测试
        /// </summary>
        public static TestResults RunAllTests()
        {
            var results = new TestResults();
            var stopwatch = Stopwatch.StartNew();

            Console.WriteLine($"[UCSharp.Tests] Running {RegisteredTests.Count} tests...");

            foreach (var test in RegisteredTests)
            {
                var testResult = RunSingleTest(test);
                results.AddResult(testResult);
            }

            stopwatch.Stop();
            results.TotalExecutionTime = stopwatch.Elapsed;

            Console.WriteLine($"[UCSharp.Tests] All tests completed in {results.TotalExecutionTime.TotalMilliseconds:F2}ms");
            Console.WriteLine($"[UCSharp.Tests] Results: {results.PassedCount} passed, {results.FailedCount} failed");

            return results;
        }

        /// <summary>
        /// 运行指定名称的测试
        /// </summary>
        public static TestResult RunTest(string testName)
        {
            var test = RegisteredTests.Find(t => t.Name == testName);
            if (test == null)
            {
                return new TestResult
                {
                    TestName = testName,
                    Success = false,
                    ErrorMessage = $"Test '{testName}' not found",
                    ExecutionTime = TimeSpan.Zero
                };
            }

            return RunSingleTest(test);
        }

        private static TestResult RunSingleTest(ITestCase test)
        {
            var result = new TestResult { TestName = test.Name };
            var stopwatch = Stopwatch.StartNew();

            try
            {
                Console.WriteLine($"[UCSharp.Tests] Running test: {test.Name}");
                
                test.Setup();
                test.Execute();
                test.Teardown();
                
                result.Success = true;
                Console.WriteLine($"[UCSharp.Tests] Test '{test.Name}' PASSED");
            }
            catch (Exception ex)
            {
                result.Success = false;
                result.ErrorMessage = ex.Message;
                result.StackTrace = ex.StackTrace;
                Console.WriteLine($"[UCSharp.Tests] Test '{test.Name}' FAILED: {ex.Message}");
            }
            finally
            {
                stopwatch.Stop();
                result.ExecutionTime = stopwatch.Elapsed;
            }

            return result;
        }

        private static void RegisterBuiltInTests()
        {
            RegisterTest(new UObjectBindingTest());
            RegisterTest(new ActorLifecycleTest());
            RegisterTest(new PropertyAccessTest());
            RegisterTest(new MethodCallTest());
            RegisterTest(new MemoryManagementTest());
            RegisterTest(new PerformanceTest());
        }

        /// <summary>
        /// 获取所有注册的测试名称
        /// </summary>
        public static string[] GetTestNames()
        {
            var names = new string[RegisteredTests.Count];
            for (int i = 0; i < RegisteredTests.Count; i++)
            {
                names[i] = RegisteredTests[i].Name;
            }
            return names;
        }
    }

    /// <summary>
    /// 测试用例接口
    /// </summary>
    public interface ITestCase
    {
        string Name { get; }
        string Description { get; }
        void Setup();
        void Execute();
        void Teardown();
    }

    /// <summary>
    /// 测试结果
    /// </summary>
    public class TestResult
    {
        public string TestName { get; set; }
        public bool Success { get; set; }
        public string ErrorMessage { get; set; }
        public string StackTrace { get; set; }
        public TimeSpan ExecutionTime { get; set; }
    }

    /// <summary>
    /// 测试结果集合
    /// </summary>
    public class TestResults
    {
        private readonly List<TestResult> _results = new List<TestResult>();

        public void AddResult(TestResult result)
        {
            _results.Add(result);
        }

        public int TotalCount => _results.Count;
        public int PassedCount => _results.FindAll(r => r.Success).Count;
        public int FailedCount => _results.FindAll(r => !r.Success).Count;
        public TimeSpan TotalExecutionTime { get; set; }

        public TestResult[] GetResults() => _results.ToArray();
        public TestResult[] GetFailedResults() => _results.FindAll(r => !r.Success).ToArray();
    }

    /// <summary>
    /// 测试断言工具
    /// </summary>
    public static class Assert
    {
        public static void IsTrue(bool condition, string message = "")
        {
            if (!condition)
                throw new AssertionException($"Expected true but was false. {message}");
        }

        public static void IsFalse(bool condition, string message = "")
        {
            if (condition)
                throw new AssertionException($"Expected false but was true. {message}");
        }

        public static void IsNotNull(object obj, string message = "")
        {
            if (obj == null)
                throw new AssertionException($"Expected non-null value. {message}");
        }

        public static void IsNull(object obj, string message = "")
        {
            if (obj != null)
                throw new AssertionException($"Expected null value. {message}");
        }

        public static void AreEqual<T>(T expected, T actual, string message = "")
        {
            if (!Equals(expected, actual))
                throw new AssertionException($"Expected '{expected}' but was '{actual}'. {message}");
        }

        public static void AreNotEqual<T>(T expected, T actual, string message = "")
        {
            if (Equals(expected, actual))
                throw new AssertionException($"Expected values to be different but both were '{expected}'. {message}");
        }
    }

    /// <summary>
    /// 断言异常
    /// </summary>
    public class AssertionException : Exception
    {
        public AssertionException(string message) : base(message) { }
        public AssertionException(string message, Exception innerException) : base(message, innerException) { }
    }
}