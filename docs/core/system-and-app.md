# System & Application

`yup_core` exposes platform information, unique identifiers, command-line
application scaffolding, runtime permissions, and the cross-platform macros that
keep the rest of the framework portable.

## SystemStats

`SystemStats` is a collection of static queries about the host machine, OS, and
process.

```cpp
String os      = SystemStats::getOperatingSystemName();
String osVer   = SystemStats::getOperatingSystemVersionString();
bool   is64    = SystemStats::isOperatingSystem64Bit();

int    cpus    = SystemStats::getNumCpus();
int    physical= SystemStats::getNumPhysicalCpus();
int    mhz     = SystemStats::getCpuSpeedInMegahertz();

String user    = SystemStats::getLogonName();
String machine = SystemStats::getComputerName();
String device  = SystemStats::getDeviceDescription();
String yup     = SystemStats::getYUPVersion();

String path    = SystemStats::getEnvironmentVariable ("PATH", {});
```

`getUniqueDeviceID()` returns a stable per-device identifier, and the
language/region getters (`getUserLanguage`, `getUserRegion`, `getDisplayLanguage`)
support localization.

## Uuid

`Uuid` generates and parses RFC-4122 universally-unique identifiers.

```cpp
Uuid id;                              // random UUID
String s      = id.toString();        // 32 hex chars
String dashed = id.toDashedString();  // 8-4-4-4-12
bool   empty  = id.isNull();

Uuid parsed ("550e8400e29b41d4a716446655440000");
```

## ConsoleApplication

`ConsoleApplication` builds command-line tools with sub-commands, help, and
version output — the same scaffolding the YUP command-line examples use.

```cpp
int main (int argc, char* argv[])
{
    ConsoleApplication app;
    app.addHelpCommand ("--help|-h", "Usage:", true);
    app.addVersionCommand ("--version|-v", "MyTool 1.0");

    app.addCommand ({ "--convert",
                      "--convert <in> <out>",
                      "Convert a file",
                      "Longer help text",
                      [] (const ArgumentList& args) { /* ... */ } });

    return app.findAndRunCommand (argc, argv);
}
```

The `ArgumentList` passed to each command parses options, values, and file
arguments (`getValueForOption`, `containsOption`, …).

```{seealso}
For GUI applications, use `YUPApplication` and `START_YUP_APPLICATION` from
`yup_events`/`yup_gui` instead — see [Getting Started](../getting-started/index.md).
```

## RuntimePermissions

On mobile platforms, gate access to protected resources (microphone, storage,
etc.) behind `RuntimePermissions`.

```cpp
if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
    && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
{
    RuntimePermissions::request (RuntimePermissions::recordAudio,
                                 [] (bool granted) { if (granted) startRecording(); });
}
```

On desktop platforms permissions are generally not required, so `isRequired`
returns false.

## Platform detection

`yup_core` defines a consistent set of preprocessor macros for conditional
compilation. Prefer these over ad-hoc checks:

```cpp
#if YUP_WINDOWS
#elif YUP_MAC
#elif YUP_IOS
#elif YUP_LINUX
#elif YUP_ANDROID
#elif YUP_WASM
#endif

#if YUP_DESKTOP   // Windows / macOS / Linux
#if YUP_MOBILE    // Android / iOS
```

These live in `system/yup_TargetPlatform.h`, alongside `yup_CompilerSupport.h`
(feature detection) and `yup_PlatformDefs.h` (`jassert`, `DBG`, and other core
macros).

## Other utilities

- **`ScopeGuard`** — run a lambda on scope exit (RAII cleanup without a custom
  class).
- **`Uuid`**, **`WindowsRegistry`** (Windows-only registry access).
- **`FlagSet`** / **`EnumHelpers`** — type-safe bitmask flags over enums.
- **`Functional`** / **`MetaProgramming`** — small function and template helpers.

```cpp
auto guard = ScopeGuard { [&] { cleanup(); } };   // runs at scope exit
```

## See also

- [Results & error handling](results-and-errors.md) — `Result` and assertions.
- [Getting Started](../getting-started/index.md) — GUI application entry points.
- [Build System](../build-system/index.md) — platform build configuration.
