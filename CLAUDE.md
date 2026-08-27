# AI Assistant Guidelines for YUP

Directive rules for generating, reviewing and suggesting code in YUP. Apply them on every task.

## Project

C++20 graphics/audio library, ISC licensed, forked from the JUCE7 ISC modules. CMake build, Google Test, Rive + OpenGL/Metal/D3D.
Copyright line for new files: `Copyright (c) 2026 - kunitoki@gmail.com`.

## Hard Rules

- **Never run bash commands to configure, compile or test.** Say what should be tested; the user runs it and reports back.
- **Code that changed without you noticing is the user's doing, not a linter's.** Acknowledge it, never revert it.
- Headers and implementation files are compiled through the main module header/cpp - linter errors when parsing them in isolation are expected.

## Design Rules

1. Use C++20 and the standard library, unless the feature is unsupported on a YUP platform.
2. Check existing patterns in similar modules first; reuse YUP infrastructure instead of reinventing or duplicating it.
3. Never assume plain JUCE7 APIs - verify them, they may have evolved (see *Differences with JUCE*).
4. Prefer less code. If 200 lines could be 50, rewrite it. Nothing speculative, no abstraction for single-use code.
5. Composition over inheritance. Small single-responsibility classes, open-closed, no leaked internals.
6. Const-correct throughout. Flat code with early exits over deep nesting.
7. RAII and smart pointers, no raw ownership. Consider thread safety where it applies.
8. Adapt or replace an existing implementation - never copy-and-modify and leave both behind.
9. Don't pollute implementation files with obvious comments.
10. Extensive Doxygen docs on public APIs and public build-system methods.
11. Test-first where practical - a bug fix starts with a failing test. New code is always tested; refactors keep tests passing, API contracts intact, platform-specific layout preserved and performance in mind.
12. Update `docs/` with effective and user targeted documentation (no extensive internal details if not needed) and add a **brief** `CHANGELOG.md` entry when the change warrants it.
13. Surgical edits: every changed line traces to the request. Don't reformat or refactor adjacent code; do clean up orphans your own change created.
14. Avoid the use of em-dashes, just use `-`.

## Style

Formatting is enforced by `.clang-format` (Allman braces, 4-space indent, no column limit, `Type* ptr` alignment, space before non-empty parens: `foo (x)`, `TEST_F (Fixture, name)`).

- `PascalCase` types; `camelCase` for functions, variables, members and constants.
- One main class per file, named `yup_ClassName.h` / `yup_ClassName.cpp`.
- `using namespace` only in test files; elsewhere scope it to the smallest block.
- Include order: own module header → other YUP modules → same-module headers → external libraries (Rive) → standard library.

## Files and Layout

- New files start with the ISC header - copy it from `modules/yup_dsp_jit/yup_dsp_jit.h`, which already carries the correct `2026` year (many older files still say `2024`, and ported JUCE files carry an extra JUCE attribution block that must not be reused). Headers then open with `#pragma once`.
- Module headers add the `BEGIN_YUP_MODULE_DECLARATION` block right after it (ID, vendor `yup`, version, name, description, website, license `ISC`, dependencies, `searchpaths: native`). Same file is the exemplar; all `yup_*` modules share one version number. Details in `docs/build-system/module-format.md`.
- Module layout: `modules/yup_module_name/` holding `yup_module_name.h` / `.cpp` / `.mm`, one level of subdirectory for logical groups, and `native/` for platform code named `yup_ClassName_<platform>.cpp` - `android`, `windows`, `linux`, `wasm`, `emscripten`, plus `mac` / `ios` / `apple` as `.mm`. Avoid deep nesting (third-party trees excepted, we don't control them).
- Modules are unity builds assembled by files in the root which should also resolve global includes, subfolder files are just included there and should not include anything on their own.
- Tests live in `tests/module_name/yup_ClassName.cpp`, one per class, plus `yup_ModuleIntegration.cpp` for integration tests.

## Class Template

```cpp
class YupStyleClass
{
public:
    YupStyleClass();
    ~YupStyleClass();

    void publicMethod();

private:
    int memberVar;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YupStyleClass)
};
```

## Testing

Mirror the structure of an existing test, e.g. `tests/yup_dsp/yup_KMeterState.cpp`.

- Include the module header and `<gtest/gtest.h>`, then `using namespace yup;`.
- Test the public interface only, covering normal, edge and error cases.
- Descriptive names: `TEST_F (ClassNameTests, ReturnsNullForInvalidInput)`.
- Group related tests in a fixture; keep them independent and deterministic.
- Keep helpers and constants inside the fixture rather than at file scope - unity builds make file-scope names clash.
- Never use C/C++ macros such as `M_PI`; use the YUP alternatives.

## Platform Guards

`YUP_WINDOWS`, `YUP_MAC`, `YUP_IOS`, `YUP_LINUX`, `YUP_ANDROID`, `YUP_WASM` (any WebAssembly), `YUP_EMSCRIPTEN` (Emscripten only), `YUP_DESKTOP`, `YUP_MOBILE`.

## Error Handling

- Fallible operations return `yup::Result` (`Result::ok()` / `Result::fail ("...")`) or `yup::ResultValue<T>` (`makeResultValueOk (v)` / `makeResultValueFail ("...")`; a plain `T` converts implicitly).
- `jassert` for programming errors, paired with a graceful early return for release builds.

## Differences with JUCE

- American English: `center` not `centred`, `Color` not `Colour`.
- Check the YUP `Graphics` API - do not assume JUCE's.
- Graphics primitives convert with the template `.to<float>()`, not `toFloat()`.
- Fonts come from `ApplicationTheme`, never instantiated inline.
