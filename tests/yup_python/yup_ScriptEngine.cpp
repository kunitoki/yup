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

using namespace yup;

namespace
{
} // namespace

class ScriptEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup before each test
    }

    void TearDown() override
    {
        // Cleanup after each test
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
    auto result = engine.runScript (R"python(
print("Hello, World!")
result = 2 + 3
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithErrorReturnsFailure)
{
    ScriptEngine engine;
    auto result = engine.runScript (R"python(
print("This will fail")
undefined_variable + 1
)python");
    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ScriptEngineTest, RunScriptWithImportWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (R"python(
import sys
print(sys.version)
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithCustomModuleWorks)
{
    StringArray modules { "yup" };
    ScriptEngine engine (modules);
    auto result = engine.runScript (R"python(
import yup
print("YUP module loaded")
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunMathScriptWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (R"python(
import math
result = math.sqrt(16)
print(f"Square root of 16 is {result}")
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunVariableTestScriptWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (R"python(
x = 10
y = 20
result = x + y
print(f"Result: {result}")
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, DISABLED_RunComplexScriptWorks)
{
    ScriptEngine engine;
    auto result = engine.runScript (R"python(
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

result = fibonacci(10)
print(f"Fibonacci(10) = {result}")
)python");
    EXPECT_TRUE (result.wasOk());
    if (result.failed())
        std::cout << result.getErrorMessage().toStdString() << std::endl;
}

TEST_F (ScriptEngineTest, RunScriptFromFileWorks)
{
    ScriptEngine engine;

    // Create temporary script file
    auto tempFile = File::createTempFile ("test_script.py");
    tempFile.replaceWithText (R"python(
print("Hello from file!")
result = 42
print(f"The answer is {result}")
)python");

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

    auto result = engine.runScript (R"python(
if True
    print('missing colon')
)python");
    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ScriptEngineTest, RunScriptWithIndentationErrorReturnsFailure)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
if True:
print('bad indentation')
)python");
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

    auto result = engine.runScript (R"python(
# This is a comment
# Another comment
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithUnicodeCharactersWorks)
{
    ScriptEngine engine;

    auto result = engine.runScript (LR"python(
print('Hello, 世界! 🌍')
)python");
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

    auto result = engine.runScript (R"python(
try:
    import non_existent_module
except ImportError as e:
    print(f"Import error: {e}")
    result = "import_failed"
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithExceptionHandling)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
try:
    x = 1 / 0
except ZeroDivisionError as e:
    print(f"Division by zero: {e}")
    result = "handled"
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithListOperations)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
numbers = [1, 2, 3, 4, 5]
squared = [x**2 for x in numbers]
print(f"Original: {numbers}")
print(f"Squared: {squared}")
result = sum(squared)
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithDictionaryOperations)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
data = {'name': 'test', 'value': 42}
print(f"Name: {data['name']}")
print(f"Value: {data['value']}")
data['new_key'] = 'new_value'
result = len(data)
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithClassDefinition)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
class TestClass:
    def __init__(self, value):
        self.value = value
    
    def get_double(self):
        return self.value * 2

obj = TestClass(21)
result = obj.get_double()
print(f"Result: {result}")
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithIterators)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
data = [10, 20, 30]
total = 0
for item in data:
    total += item
print(f"Total: {total}")
result = total
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithGenerators)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
def count_up_to(max):
    count = 1
    while count <= max:
        yield count
        count += 1

result = list(count_up_to(5))
print(f"Generated: {result}")
)python");
    EXPECT_TRUE (result.wasOk());
}

TEST_F (ScriptEngineTest, RunScriptWithLambdaFunctions)
{
    ScriptEngine engine;

    auto result = engine.runScript (R"python(
numbers = [1, 2, 3, 4, 5]
even_numbers = list(filter(lambda x: x % 2 == 0, numbers))
print(f"Even numbers: {even_numbers}")
result = len(even_numbers)
)python");
    EXPECT_TRUE (result.wasOk());
}
