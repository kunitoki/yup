# AI Assistant Guidelines for YUP Project

This document provides directive guidelines for AI assistants working on the YUP project. Use these rules when generating, reviewing, or suggesting code changes.

## Project Context
- **Project Type:** C++ graphics/audio library
- **License:** ISC License
- **Copyright:** `Copyright (c) 2025 - kunitoki@gmail.com`
- **Based On:** Fork of JUCE7 ISC Modules
- **Build System:** CMake
- **Testing Framework:** Google Test
- **Primary Dependencies:** Rive, OpenGL/Metal/D3D

## Code Generation Rules

### 1. File Headers
**ALWAYS** start new files with this exact header:

```cpp
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
```

### 2. Module Headers
For main module headers (e.g., `yup_graphics.h`), include this declaration block after the file header:

```cpp
/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 module_name
    vendor:             yup
    version:            1.0.0
    name:               Module Display Name
    description:        Brief module description
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       yup_graphics [other_dependencies]
    searchpaths:        native
    enableARC:          1

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/
```

### 3. Formatting Rules (Allman Style)
```cpp
// Classes
class MyClass
{
public:
    MyClass();
    ~MyClass();

private:
    int memberVariable;
};

// Functions
void functionName()
{
    // implementation
}

// Control structures
if (condition)
{
    // code
}
else
{
    // code
}

for (int i = 0; i < count; ++i)
{
    // code
}

while (condition)
{
    // code
}
```

### 4. Naming Conventions
- **Classes:** `PascalCase` (e.g., `GraphicsContext`)
- **Functions:** `camelCase` (e.g., `createRenderer`)
- **Variables:** `camelCase` (e.g., `currentState`)
- **Constants:** `camelCase` (e.g., `defaultSize`)
- **Member variables:** `camelCase` (e.g., `bufferSize`)
- **Files:** `yup_ClassName.h/cpp` for classes, one file per main class

### 5. Include Order
```cpp
#pragma once

// 1. Own module header (if in .cpp file)
#include <yup_graphics/yup_graphics.h>

// 2. Standard library
#include <memory>
#include <vector>

// 3. External libraries (Rive, etc.)
#include <rive/rive.h>

// 4. Other project modules
#include "yup_core/yup_core.h"

// 5. Same module headers
#include "graphics/yup_Color.h"
#include "primitives/yup_Point.h"
```

### 6. Namespace Usage
```cpp
// NEVER use "using namespace"
// In test files: OK for widely used namespaces
using namespace yup;

// Prefer limited scope usage
TEST(MyClassTests, someFunction)
{
    using namespace std::chrono;
    // use chrono types without std::chrono:: prefix
}
```

## File Organization Patterns

### Module Structure
```
modules/yup_module_name/
├── yup_module_name.h          // Main module header
├── yup_module_name.cpp        // Main module implementation
├── yup_module_name.mm         // Objective-C++ (Apple platforms)
├── subdirectory/              // Logical groupings
│   ├── yup_ClassName.h
│   └── yup_ClassName.cpp
└── native/                    // Platform-specific code
    ├── yup_ClassName_win32.cpp
    ├── yup_ClassName_linux.cpp
    └── yup_ClassName_apple.mm
```
Avoid going deeply nested into modules. Prefer a single subdirectory whenever possible for YUP modules (might be ok for thirdparties as we don't control the upstream structure).

**Headers and Implementation files are designed to be included through the main module header/implementation, so linter errors are expected when parsing the files in isolation.**

### Test Structure
```
tests/module_name/
├── ModuleClassName.cpp        // Test file per class
└── ModuleIntegration.cpp      // Integration tests
```

## Class Design Templates

### Basic Class Template
```cpp
class ClassName
{
public:
    ClassName();
    ~ClassName();

    // Copy/move constructors if needed
    ClassName(const ClassName& other) = delete;
    ClassName& operator=(const ClassName& other) = delete;

    // Public interface
    void doSomething();
    int getValue() const;
    bool isValid() const;

private:
    // Member variables
    int value;
    bool initialized;

    // Helper methods
    void initialize();
};
```

### YUP-Style Class (with leak detector)
```cpp
class YupStyleClass
{
public:
    YupStyleClass();
    ~YupStyleClass();

    void publicMethod();

private:
    int memberVar;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YupStyleClass)
};
```

## Testing Patterns

### Test File Template
```cpp
#include <gtest/gtest.h>

#include <module_name/ClassName.h>

using namespace yup;

namespace
{
    // Test helpers and constants
    constexpr int kTestValue = 42;

    class TestHelper
    {
    public:
        static void setupTestData() { /* ... */ }
    };
}

class ClassNameTests : public ::testing::Test
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

    // Test fixtures
    ClassName instance;
};

TEST_F(ClassNameTests, ConstructorInitializesCorrectly)
{
    EXPECT_TRUE(instance.isValid());
    EXPECT_EQ(0, instance.getValue());
}

TEST(ClassNameTests, StaticMethodBehavesCorrectly)
{
    auto result = ClassName::staticMethod();
    EXPECT_NE(nullptr, result.get());
}
```

## AI Decision Making Rules

### When implementing new features:
1. **Check existing patterns** in similar modules first
2. **Use YUP conventions** for similar functionality
3. **Prefer composition over inheritance**
4. **Make classes small and focused** (single responsibility)
5. **Use const-correctness** throughout
6. **Do not leak internal details**
7. **Follow the open-closed principle**
8. **Always provide extensive and useful doxygen documentation** for public APIs
9. **Never assume we use plain JUCE7 functionality, always check APIs** as they might have evolved

### When writing tests:
1. **Test primarily public interfaces only**
2. **Cover normal, edge, and error cases**
3. **Use descriptive test names** (e.g., `ReturnsNullForInvalidInput`)
4. **Group related tests** in test fixtures
5. **Keep tests independent** and deterministic
6. **Never Use C or C++ macros (like M_PI)** use yup alternatives

### When suggesting refactoring:
1. **Maintain existing API contracts**
2. **Follow established module patterns**
3. **Preserve platform-specific code organization**
4. **Update tests accordingly**
5. **Consider performance implications**
6. **Keep API usage simple and effective**

### Platform-specific code:
```cpp
#if YUP_WINDOWS
    // Windows implementation
#elif YUP_MAC
    // macOS implementation
#elif YUP_IOS
    // iOS implementation
#elif YUP_LINUX
    // Linux implementation
#elif YUP_ANDROID
    // Android implementation
#elif YUP_WASM
    // WebAssembly implementation
#endif
```

### Error handling patterns:
```cpp
// Use YUP Result or ResultValue<T> for operations that can fail
yup::Result performOperation()
{
    if (preconditionFailed)
        return yup::Result::fail("Precondition not met");

    return yup::Result::ok();
}

yup::ResultValue<int> maybeGetInteger()
{
    if (preconditionFailed)
        return yup::ResultValue<int>::fail("Precondition not met");

    return 1;
}

// Use assertions for programming errors
void publicMethod(int value)
{
    jassert(value >= 0); // Debug builds only

    if (value < 0)
        return; // Graceful handling in release
}
```

## Code Review Checklist for AI

Before suggesting code, verify:
- [ ] Proper file header with correct copyright
- [ ] Allman-style braces throughout
- [ ] Consistent naming conventions
- [ ] Proper include order and guards
- [ ] const-correctness where applicable
- [ ] Platform-specific code properly guarded
- [ ] Tests cover the new functionality
- [ ] No memory leaks (prefer RAII/smart pointers)
- [ ] Thread safety considerations if applicable
- [ ] Documentation for public APIs

## Common Patterns to Follow

### Resource Management
```cpp
// Prefer RAII and smart pointers
class ResourceManager
{
private:
    std::unique_ptr<Resource> resource;
    std::vector<std::shared_ptr<Item>> items;
};
```

### Optional Values
```cpp
// Use yup::var for dynamic types
// Use std::optional for optional values
std::optional<int> findValue(const String& key);
```

### String Handling
```cpp
// Use yup::String for most string operations
void processText(const yup::String& text);

// Use std::string only when interfacing with non-YUP code
```

This document should be referenced for every code generation, review, and suggestion task in the YUP project.
