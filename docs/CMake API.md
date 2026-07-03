# The YUP CMake API

## System Requirements

- All project types require CMake 3.31 or higher.
- Android and Emscripten targets are not currently supported for audio plugins.

Most system package managers have packages for CMake, but we recommend using the most recent release
from https://cmake.org/download.

## Getting Started

### Using FetchContent

The recommended way to include YUP in your project:

```cmake
cmake_minimum_required (VERSION 3.31)

set (target_name my_plugin)
set (target_version "0.0.1")
project (${target_name} VERSION ${target_version})

include (FetchContent)

FetchContent_Declare (
    yup
    GIT_REPOSITORY https://github.com/kunitoki/yup.git
    GIT_TAG main)

set (YUP_BUILD_EXAMPLES OFF)
set (YUP_BUILD_TESTS OFF)
FetchContent_MakeAvailable (yup)

yup_audio_plugin (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_APP_ID "com.mycompany.${target_name}"
    TARGET_APP_NAMESPACE "com.mycompany"
    PLUGIN_CREATE_CLAP ON
    PLUGIN_CREATE_VST3 ON
    MODULES yup::yup_gui yup::yup_audio_processors)

file (GLOB sources "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
target_sources (${target_name}_shared PUBLIC ${sources})
```

### Platform-Specific Build Commands

**macOS:**
```bash
cmake -G "Xcode" . -B build
cmake --build build --config Release
```

**Windows:**
```bash
cmake -G "Visual Studio 17 2022" -A x64 . -B build
cmake --build build --config Release
```

**Linux:**
```bash
cmake -G "Ninja" . -B build
cmake --build build --config Release
```

### Building universal binaries for macOS

```bash
cmake -B build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

---

## CMake Cache Options

These flags can be enabled or disabled to change the behaviour of parts of the YUP build. They
would normally be configured by supplying an option in the form `-DNAME=ON/OFF` to the initial CMake
configuration call, or by calling `set(NAME ON/OFF)` before including YUP via `FetchContent`.

### `YUP_BUILD_EXAMPLES`

Controls whether targets are added for the example projects. OFF by default.

### `YUP_BUILD_TESTS`

Controls whether targets are added for test projects. OFF by default.

### `YUP_BUILD_BENCHMARKS`

Controls whether targets are added for benchmark projects. OFF by default.

### `YUP_ENABLE_COVERAGE`

Enables code coverage instrumentation. OFF by default.

### `YUP_ENABLE_PLUGINVAL`

Enables pluginval validation as a post-build step for plugin targets. OFF by default.

### `YUP_ENABLE_CLAP_VALIDATOR`

Enables CLAP validator as a post-build step for CLAP plugin targets. OFF by default.

### `YUP_ENABLE_VST3_VALIDATOR`

Enables the SMTG VST3 validator as a post-build step for VST3 targets. OFF by default.

### `YUP_ENABLE_AUVAL_VALIDATOR`

Enables auval validation as a post-build step for AU/AUv3 targets. OFF by default.

---

## Functions

### `yup_audio_plugin`

```cmake
yup_audio_plugin(
    [KEY value]...
)
```

Adds a shared code static library target with name `<TARGET_NAME>_shared`, along with extra targets
for each of the specified plugin formats. All arguments are optional unless noted otherwise. Keys
that are not specified will fall back to sensible defaults.

#### Target Globals

| Key | Type | Default | Description |
|---|---|---|---|
| `TARGET_NAME` | string | *(required)* | Base name for all generated targets. |
| `TARGET_VERSION` | string | *(required)* | Version in `major.minor.patch` format. |
| `TARGET_IDE_GROUP` | string | — | IDE folder group name (e.g. "Examples"). |
| `TARGET_APP_ID` | string | `org.kunitoki.yup.<TARGET_NAME>` | Bundle identifier. |
| `TARGET_APP_NAMESPACE` | string | — | App namespace (e.g. `com.mycompany`). |
| `TARGET_CXX_STANDARD` | integer | `20` | C++ standard version. |

#### Format Toggles

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_CREATE_CLAP` | ON\|OFF | OFF | Build a CLAP plugin target. |
| `PLUGIN_CREATE_VST3` | ON\|OFF | OFF | Build a VST3 plugin target. |
| `PLUGIN_CREATE_AU` | ON\|OFF | OFF | Build an AUv2 plugin target (macOS only). |
| `PLUGIN_CREATE_AUv3` | ON\|OFF | OFF | Build an AUv3 plugin target (macOS only). |
| `PLUGIN_CREATE_AAX` | ON\|OFF | OFF | Build an AAX plugin target. Requires AAX SDK. |
| `PLUGIN_CREATE_STANDALONE` | ON\|OFF | OFF | Build a standalone executable. |

At least one format must be enabled, otherwise configuration fails with an error.

#### Plugin Metadata

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_ID` | string | — | Globally-unique plugin identifier (e.g. `com.mycompany.MyPlugin`). |
| `PLUGIN_NAME` | string | `<TARGET_NAME>` | Display name shown in DAWs. |
| `PLUGIN_VENDOR` | string | — | Vendor/company name. |
| `PLUGIN_EMAIL` | string | — | Contact email address. |
| `PLUGIN_VERSION` | string | `1` | Plugin version string. |
| `PLUGIN_DESCRIPTION` | string | — | Short description of the plugin. |
| `PLUGIN_URL` | string | — | Website URL for the plugin. |
| `PLUGIN_IS_SYNTH` | ON\|OFF | OFF | Whether the plugin is a synthesizer (affects format-specific defaults). |
| `PLUGIN_IS_MONO` | ON\|OFF | OFF | Whether the plugin is mono (affects CLAP feature defaults). |
| `PLUGIN_COPYRIGHT` | string | — | Copyright string added to Info.plist on macOS. |

#### AU-Specific

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_AU_SUBTYPE` | 4-char | `Dflt` | AudioUnit subtype code. Must be exactly 4 characters. |
| `PLUGIN_AU_MANUFACTURER` | 4-char | `Yup!` | AudioUnit manufacturer code. Must be exactly 4 characters. |
| `PLUGIN_AU_SANDBOX_SAFE` | ON\|OFF | ON | Sets `sandboxSafe` in the AU Info.plist. Set OFF if your plugin needs IOKit or other unsandboxed resources. |

#### AAX-Specific

**Required** when `PLUGIN_CREATE_AAX` is ON. If not provided, configuration fails with a fatal error.

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_AAX_MANUFACTURER_ID` | 4-char | *(required)* | AAX manufacturer ID (e.g. `YUPP`). |
| `PLUGIN_AAX_PRODUCT_ID` | 4-char | *(required)* | AAX product ID (e.g. `YpSy`). |
| `PLUGIN_AAX_PLUGIN_ID_NATIVE` | 4-char | *(required)* | AAX native plugin ID (e.g. `YpSN`). |
| `PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE` | 4-char | *(required)* | AAX AudioSuite plugin ID (e.g. `YpSA`). |
| `PLUGIN_AAX_CATEGORY` | string | *(auto)* | AAX plugin category (e.g. `AAX_ePlugInCategory_Example`). If omitted, auto-derived from `PLUGIN_IS_SYNTH`. |
| `PLUGIN_AAX_PAGE_TABLE_FILE` | path | — | Path to an AAX page table resource file. |

#### CLAP-Specific

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_CLAP_FEATURES` | list | *(auto)* | Space-separated list of CLAP plugin features. If omitted, auto-derived from `IS_SYNTH` and `IS_MONO`. |

Supported feature names:

| Value | CLAP Constant |
|---|---|
| `instrument` | `CLAP_PLUGIN_FEATURE_INSTRUMENT` |
| `audio-effect` | `CLAP_PLUGIN_FEATURE_AUDIO_EFFECT` |
| `note-effect` | `CLAP_PLUGIN_FEATURE_NOTE_EFFECT` |
| `note-detector` | `CLAP_PLUGIN_FEATURE_NOTE_DETECTOR` |
| `analyzer` | `CLAP_PLUGIN_FEATURE_ANALYZER` |
| `synthesizer` | `CLAP_PLUGIN_FEATURE_SYNTHESIZER` |
| `mono` | `CLAP_PLUGIN_FEATURE_MONO` |
| `stereo` | `CLAP_PLUGIN_FEATURE_STEREO` |
| `surround` | `CLAP_PLUGIN_FEATURE_SURROUND` |
| `utility` | `CLAP_PLUGIN_FEATURE_UTILITY` |

#### VST3-Specific

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_VST3_CATEGORIES` | list | *(auto)* | Space-separated list of VST3 subcategories. Pipe-joined at compile time. If omitted, auto-derived from `IS_SYNTH` (`Instrument|Synth` vs `Fx`). |
| `PLUGIN_VST3_AUTO_MANIFEST` | ON\|OFF | ON | Generate `moduleinfo.json` as a post-build step. Set OFF if the plugin needs custom post-processing before it can be loaded. |

Common VST3 categories: `Fx`, `Instrument`, `Analyzer`, `Delay`, `Distortion`, `Drum`, `Dynamics`, `EQ`, `Filter`, `Generator`, `Mastering`, `Modulation`, `Reverb`, `Sampler`, `Spatial`, `Stereo`, `Surround`, `Synth`, `Tools`.

Multiple categories are pipe-joined automatically. Example: `PLUGIN_VST3_CATEGORIES Fx Stereo` produces `"Fx|Stereo"`.

#### Codesigning & Entitlements

All codesigning options are macOS-only and have no effect on other platforms.

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_CODESIGN_IDENTITY` | string | `-` | Codesigning identity. Default `-` means ad-hoc signing. Set to `"Developer ID Application: ..."` for distribution. |
| `PLUGIN_CODESIGN_TEAM` | string | — | 10-character Apple Developer Team ID. When set, configures `DEVELOPMENT_TEAM` Xcode attribute. |
| `PLUGIN_APPLE_ENTITLEMENTS` | path | *(built-in)* | Path to a custom entitlements plist file. If omitted, the built-in AUv3 entitlements are used for AUv3 targets; other formats have no entitlements by default. |
| `PLUGIN_HARDENED_RUNTIME` | ON\|OFF | OFF | Enable macOS hardened runtime (required for notarization). Adds `--options runtime` to codesign. |
| `PLUGIN_HARDENED_RUNTIME_OPTIONS` | list | — | Space-separated list of entitlement keys to add to the entitlements plist when hardened runtime is enabled (e.g. `com.apple.security.device.audio-input`). |

#### Copy & Install

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_COPY_AFTER_BUILD` | ON\|OFF | ON | Automatically copy/symlink the built plugin to the system plugin directory after building. |

#### Icons

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_ICON_BIG` | path | — | Path to a large icon image file. |
| `PLUGIN_ICON_SMALL` | path | — | Path to a small icon image file. |

#### Custom Plist Injection

| Key | Type | Default | Description |
|---|---|---|---|
| `PLUGIN_CUSTOM_PLIST` | list | — | Additional plist key-value strings to merge into the target's Info.plist. |

#### Dependencies

| Key | Type | Default | Description |
|---|---|---|---|
| `MODULES` | list | *(required)* | YUP module dependencies (e.g. `yup::yup_gui yup::yup_audio_processors`). |
| `DEFINITIONS` | list | — | Additional compile definitions applied to all format targets. |
| `OPTIONS` | list | — | Additional compile options applied to all format targets. |
| `LINK_OPTIONS` | list | — | Additional linker options applied to all format targets. |

---

### `yup_standalone_app`

```cmake
yup_standalone_app(
    [KEY value]...
)
```

Creates a standalone executable application. Used internally by `PLUGIN_CREATE_STANDALONE` but can
also be used independently for non-plugin GUI applications.

#### Arguments

| Key | Type | Default | Description |
|---|---|---|---|
| `TARGET_NAME` | string | *(required)* | Target name for the executable. |
| `TARGET_VERSION` | string | *(required)* | Version in `major.minor.patch` format. |
| `TARGET_IDE_GROUP` | string | — | IDE folder group name. |
| `TARGET_APP_NAMESPACE` | string | — | App namespace for bundle identifier generation. |
| `TARGET_CXX_STANDARD` | integer | `20` | C++ standard version. |
| `TARGET_ICON` | path | *(built-in)* | Path to an application icon image. |
| `DEFINITIONS` | list | — | Additional compile definitions. |
| `COMPILE_OPTIONS` | list | — | Additional compile options. |
| `MODULES` | list | — | YUP module dependencies. |
| `SOURCES` | list | — | Additional source files. |
| `LINK_OPTIONS` | list | — | Additional linker options. |

---

## Plugin Format-Specific Notes

### CLAP Features

When `PLUGIN_CLAP_FEATURES` is not specified, features are auto-derived:
- If `PLUGIN_IS_SYNTH ON`: `instrument`, `synthesizer`
- If `PLUGIN_IS_SYNTH OFF`: `audio-effect`
- If `PLUGIN_IS_MONO ON`: `mono`
- If `PLUGIN_IS_MONO OFF`: `stereo`

When `PLUGIN_CLAP_FEATURES` IS specified, the auto-derived features are bypassed entirely and only
the specified features are used.

### VST3 Categories

When `PLUGIN_VST3_CATEGORIES` is not specified, the category is auto-derived:
- If `PLUGIN_IS_SYNTH ON`: `Instrument|Synth`
- If `PLUGIN_IS_SYNTH OFF`: `Fx`

When `PLUGIN_VST3_CATEGORIES` IS specified, the auto-derived category is bypassed.

### AAX SDK & IDs

AAX plugin support requires the AAX SDK. Set `YUP_AAX_SDK_ROOT` (CMake variable or environment
variable) to the root of the AAX SDK installation. If `YUP_AAX_SDK_ROOT` is not set, AAX targets
are silently skipped.

All four AAX ID values (`PLUGIN_AAX_MANUFACTURER_ID`, `PLUGIN_AAX_PRODUCT_ID`,
`PLUGIN_AAX_PLUGIN_ID_NATIVE`, `PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE`) are required when AAX is enabled.
If any is missing, configuration fails with a fatal error.

### AU Subtype & Manufacturer

The AU subtype and manufacturer codes must be exactly 4 characters each. These are used in the
`AudioComponents` Info.plist entry for AUv2 and in the `NSExtensionAttributes` for AUv3.

### AUv3 Entitlements

AUv3 plugins require an entitlements plist. By default, YUP provides a built-in entitlements file
at `cmake/platforms/mac/AudioUnitV3_Entitlements.plist` with:
- App Sandbox enabled
- Audio input access
- MIDI access

Override with `PLUGIN_APPLE_ENTITLEMENTS` pointing to a custom `.plist` file.

### Codesigning for Distribution

For local development, ad-hoc signing (`-`) is sufficient. For distribution (notarization, App
Store), you must set:

```cmake
PLUGIN_CODESIGN_IDENTITY "Developer ID Application: Your Name (TEAMID)"
PLUGIN_CODESIGN_TEAM "ABCDE12345"
PLUGIN_HARDENED_RUNTIME ON
```

On macOS, `PLUGIN_CODESIGN_TEAM` also sets the `DEVELOPMENT_TEAM` Xcode attribute on the target.

### Plugin Install Locations

Plugins are automatically copied/symlinked after building when `PLUGIN_COPY_AFTER_BUILD` is ON (the default).

| Format | macOS |
|---|---|
| CLAP | `~/Library/Audio/Plug-Ins/CLAP/` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| AU | `~/Library/Audio/Plug-Ins/Components/` |
| AUv3 | `~/Library/Audio/Plug-Ins/AppExtensions/` |
| AAX | `~/Library/Application Support/Avid/Audio/Plug-Ins/` |
