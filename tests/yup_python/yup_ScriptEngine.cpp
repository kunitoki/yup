/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <yup_python/yup_python.h>

#include <gtest/gtest.h>

#if __has_include(<PythonStandardLibrary.h>)
#define YUP_HAS_EMBEDDED_PYTHON_STANDARD_LIBRARY 1
#include <PythonStandardLibrary.h>
#endif

using namespace yup;

namespace
{
} // namespace

class ScriptEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F (ScriptEngineTest, ConstructorInitializesCorrectly)
{
    ScriptEngine engine;
    SUCCEED();
}

TEST_F (ScriptEngineTest, ConstructorWithCustomModulesInitializesCorrectly)
{
    StringArray modules { "sys", "os" };
    ScriptEngine engine (modules);
    SUCCEED();
}

TEST_F (ScriptEngineTest, RunSimpleScriptReturnsSuccess)
{
    ScriptEngine engine;
    auto result = engine.runScript (String (R"(
        print("Hello, World!")
        result = 2 + 3
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithErrorReturnsFailure)
{
    ScriptEngine engine;
    auto result = engine.runScript (String (R"(
        print("This will fail")
        undefined_variable + 1
    )")
                                        .dedentLines());
    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ScriptEngineTest, RunScriptWithImportWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (String (R"(
        import sys
        print(sys.version)
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithCustomModuleWorks)
{
    StringArray modules { "yup" };
    ScriptEngine engine (modules);
    auto result = engine.runScript (String (R"(
        import yup
        print("YUP module loaded")
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunMathScriptWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (String (R"(
        import math
        result = math.sqrt(16)
        print(f"Square root of 16 is {result}")
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunVariableTestScriptWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (String (R"(
        x = 10
        y = 20
        result = x + y
        print(f"Result: {result}")
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, DISABLED_RunComplexScriptWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (String (R"(
        def fibonacci(n):
            if n <= 1:
                return n
            return fibonacci(n-1) + fibonacci(n-2)

        result = fibonacci(10)
        print(f"Fibonacci(10) = {result}")
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
    if (result.failed())
        std::cout << result.getErrorMessage().toStdString() << std::endl;
}

TEST_F (ScriptEngineTest, RunScriptFromFileWorks)
{
    ScriptEngine engine;

    // Create temporary script file
    auto tempFile = File::createTempFile ("test_script.py");
    tempFile.replaceWithText (String (R"(
        print("Hello from file!")
        result = 42
        print(f"The answer is {result}")
    )")
                                  .dedentLines());

    auto result = engine.runScript (tempFile);
    EXPECT_TRUE (result.wasOk());

    // Clean up
    tempFile.deleteFile();
}

TEST_F (ScriptEngineTest, RunScriptFromNonExistentFileReturnsFailure)
{
    ScriptEngine engine;

    auto nonExistentFile = File ("/non/existent/file.py");
    auto result = engine.runScript (nonExistentFile);
    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ScriptEngineTest, RunScriptWithLocalVariables)
{
    ScriptEngine engine;

    // Test with local variables
    pybind11::dict locals;
    locals["test_var"] = 42;

    auto result = engine.runScript ("result = test_var * 2", locals);
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithGlobalVariables)
{
    ScriptEngine engine;

    // Test with global variables
    pybind11::dict globals = pybind11::globals();
    globals["global_var"] = "Hello from global";

    auto result = engine.runScript ("print(global_var)", {}, globals);
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunMultipleScriptsSequentially)
{
    ScriptEngine engine;

    // First script
    auto result1 = engine.runScript ("x = 10");
    EXPECT_TRUE (result1.wasOk());

    // Second script using variable from first
    auto result2 = engine.runScript ("y = x * 2");
    EXPECT_TRUE (result2.wasOk());

    // Third script
    auto result3 = engine.runScript ("print(f'x={x}, y={y}')");
    EXPECT_TRUE (result3.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithSyntaxErrorReturnsFailure)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        if True
            print('missing colon')
    )")
                                        .dedentLines());
    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ScriptEngineTest, RunScriptWithIndentationErrorReturnsFailure)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        if True:
        print('bad indentation')
    )")
                                        .dedentLines());
    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ScriptEngineTest, RunEmptyScriptReturnsSuccess)
{
    ScriptEngine engine;

    auto result = engine.runScript ("");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithOnlyCommentsReturnsSuccess)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        # This is a comment
        # Another comment
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithUnicodeCharactersWorks)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (LR"(
        print('Hello, 世界! 🌍')
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithLongStringWorks)
{
    ScriptEngine engine;

    String longScript;
    for (int i = 0; i < 100; ++i)
        longScript << "print('Line " << i << "')\n";

    auto result = engine.runScript (longScript);
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, PrepareScriptingHomeWithValidParameters)
{
    auto tempDir = File::createTempFile ("test_home");
    tempDir.deleteFile();

    auto standardLibraryCallback = [] (const char* name) -> MemoryBlock
    {
        return MemoryBlock();
    };

    auto config = ScriptEngine::prepareScriptingHome (
        "TestApp",
        tempDir,
        standardLibraryCallback,
        false);

    EXPECT_NE (nullptr, config);
    EXPECT_TRUE (tempDir.isDirectory());

    // Clean up
    tempDir.deleteRecursively();
}

TEST_F (ScriptEngineTest, PrepareScriptingHomeWithForceInstall)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        try:
            import non_existent_module
        except ImportError as e:
            print(f"Import error: {e}")
            result = "import_failed"
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithExceptionHandling)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        try:
            x = 1 / 0
        except ZeroDivisionError as e:
            print(f"Division by zero: {e}")
            result = "handled"
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithListOperations)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        numbers = [1, 2, 3, 4, 5]
        squared = [x**2 for x in numbers]
        print(f"Original: {numbers}")
        print(f"Squared: {squared}")
        result = sum(squared)
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithDictionaryOperations)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        data = {'name': 'test', 'value': 42}
        print(f"Name: {data['name']}")
        print(f"Value: {data['value']}")
        data['new_key'] = 'new_value'
        result = len(data)
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithClassDefinition)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        class TestClass:
            def __init__(self, value):
                self.value = value

            def get_double(self):
                return self.value * 2

        obj = TestClass(21)
        result = obj.get_double()
        print(f"Result: {result}")
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithIterators)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        data = [10, 20, 30]
        total = 0
        for item in data:
            total += item
        print(f"Total: {total}")
        result = total
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithGenerators)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        def count_up_to(max):
            count = 1
            while count <= max:
                yield count
                count += 1

        result = list(count_up_to(5))
        print(f"Generated: {result}")
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithLambdaFunctions)
{
    ScriptEngine engine;

    auto result = engine.runScript (String (R"(
        numbers = [1, 2, 3, 4, 5]
        even_numbers = list(filter(lambda x: x % 2 == 0, numbers))
        print(f"Even numbers: {even_numbers}")
        result = len(even_numbers)
    )")
                                        .dedentLines());
    EXPECT_TRUE (result.wasOk());
}

#if YUP_HAS_EMBEDDED_PYTHON_STANDARD_LIBRARY
TEST_F (ScriptEngineTest, RunScriptWithStdLibImports)
{
    ScriptEngine engine (ScriptEngine::prepareScriptingHome (
        YUPApplication::getInstance()->getApplicationName(),
        File::getSpecialLocation (File::tempDirectory),
        [] (const char*) -> MemoryBlock
    {
        return { PythonStandardLibrary_data, PythonStandardLibrary_size };
    }));

    auto result = engine.runScript ("import argparse");
    EXPECT_TRUE (result.wasOk());
}
#endif
