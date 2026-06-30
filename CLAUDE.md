# AI Assistant Guidelines for YUP Project

This document provides directive guidelines for AI assistants working on the YUP project. Use these rules when generating, reviewing, or suggesting code changes.

## Project Context
- **Project Type:** C++ graphics/audio library
- **License:** ISC License
- **Copyright:** `Copyright (c) 2026 - kunitoki@gmail.com`
- **Based On:** Fork of JUCE7 ISC Modules
- **Build System:** CMake
- **Testing Framework:** Google Test
- **Primary Dependencies:** Rive, OpenGL/Metal/D3D
- **C++ Standard**: >= C++20

## Code Generation Rules

**NEVER EVER run bash commands to configure, compile or test the implementation, acknowledge that we should test and we'll run and report any issue.**

## AI Decision Making Rules

### Always:
1. **Rely on the C++20 language and standard library** so use it (unless the feature is not supported in all YUP's platforms)
2. **Check existing patterns** in similar modules first
3. **Use YUP conventions** for similar functionality
4. **Use YUP infrastructure** instead of reinventing the wheel
5. **If the same functionality can be provided with less code and complexity** prefer less code
6. **Always prefer reusing code than creating duplicated code**
7. **Prefer composition over inheritance**
8. **Make classes small and focused** (single responsibility)
9. **Use const-correctness** throughout
10. **Do not leak internal details**
11. **Follow the open-closed principle**
12. **Never assume we use plain JUCE7 functionality, always check APIs** as they might have evolved

### When implementing new features:
1. **Always provide extensive and useful doxygen documentation** for public APIs
2. **Make sure new code is always tested**

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

### 1. File Headers
**ALWAYS** start new files with this exact header:

```cpp
/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

    dependencies:       yup_graphics [other_dependencies]
    searchpaths:        native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/
```

Refer to `./docs/YUP Module Format.md` for more info if needed.

### 3. Formatting Rules (Allman Style)

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

// 2. Other project modules
#include "yup_core/yup_core.h"

// 3. Same module headers
#include "graphics/yup_Color.h"
#include "primitives/yup_Point.h"

// 5. External libraries (Rive, etc.)
#include <rive/rive.h>

// 4. Standard library
#include <memory>
#include <vector>
```

### 6. Namespace Usage
```cpp
// NEVER use "using namespace" except in test files
using namespace yup;

// Prefer limited scope usage
TEST (MyClassTests, someFunction)
{
    using namespace std::chrono;
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
    ├── yup_ClassName_android.cpp
    ├── yup_ClassName_windows.cpp
    ├── yup_ClassName_linux.cpp
    ├── yup_ClassName_wasm.cpp
    ├── yup_ClassName_emscripten.cpp
    ├── yup_ClassName_mac.mm
    ├── yup_ClassName_ios.mm
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

### YUP-Style Class (with leak detector)
```cpp
class YupStyleClass
{
public:
    YupStyleClass();
    ~YupStyleClass();

    void publicMethod();

private:
    void privateMethod();

    int memberVar;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YupStyleClass)
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

// Test helpers and constants, prefer move them into fixtures so they don't clash in unity builds
constexpr int kTestValue = 42;

class TestHelper
{
public:
    static void setupTestData() { /* ... */ }
};

} // namespace

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

TEST_F (ClassNameTests, ConstructorInitializesCorrectly)
{
    EXPECT_TRUE (instance.isValid());
    EXPECT_EQ (0, instance.getValue());
}

TEST (ClassNameTests, StaticMethodBehavesCorrectly)
{
    auto result = ClassName::staticMethod();
    EXPECT_NE (nullptr, result.get());
}
```

### Platform-specific code:
```cpp
#if YUP_WINDOWS // Windows
#elif YUP_MAC // macOS
#elif YUP_IOS // iOS
#elif YUP_LINUX // Linux
#elif YUP_ANDROID // Android
#elif YUP_WASM // WebAssembly (including emscripten)
#elif YUP_EMSCRIPTEN // WebAssembly (only emscripten)
#elif YUP_DESKTOP // Windows/macOS/Linux
#elif YUP_MOBILE // Android/iOS
#endif
```

### Error handling patterns:
```cpp
// Use YUP Result or ResultValue<T> for operations that can fail
yup::Result performOperation()
{
    if (preconditionFailed)
        return yup::Result::fail ("Precondition not met");

    return yup::Result::ok();
}

yup::ResultValue<int> maybeGetInteger()
{
    if (preconditionFailed)
        return yup::makeResultValueFail ("Precondition not met");

    return 1; // or yup::makeResultValueOk (1)
}

// Use assertions for programming errors
void publicMethod (int value)
{
    jassert (value >= 0); // Debug builds only
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
- [ ] Const-correctness whenever applicable
- [ ] Prefer flatter code and early exits over overly indented code
- [ ] Aim at simplifying and removing duplicated code, prefer removing rather than adding
- [ ] When changing implementation, don't copy it and change it, adapt the existing or remove the old one once the new is in place and working
- [ ] Platform-specific code properly guarded
- [ ] Proper TDD and ensure tests cover new functionality
- [ ] No memory leaks (prefer RAII/smart pointers)
- [ ] Thread safety considerations if applicable
- [ ] Documentation for public APIs

## Differences with JUCE

- We use American english in YUP, so it's `center` and not `centred`, or `Color` and not `Colour`
- Always check the available API in the Graphics class, don't assume we use JUCE Graphics classes
- Graphics primitives have a template `.to<float>` method not `toFloat`
- Fonts are obtained via ApplicationTheme, don't try to instantiate fonts inline

This document should be referenced for every code generation, review, and suggestion task in the YUP project.
