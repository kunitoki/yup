# The YUP Module Format

A YUP module is a collection of header and source files which can be added to a project to provide a set of classes and libraries or related functionality.

Their structure is designed to make it as simple as possible for modules to be added to user projects on many platforms, either via automated tools, or by manual inclusion.

Each module may have dependencies on other modules, but should be otherwise self-contained.

## File structure

Each module lives inside a folder whose name is the same as the name of the module. The YUP convention for naming modules is lower-case with underscores, e.g.

    yup_core
    yup_events
    yup_graphics

But any name that is a valid C++ identifier is OK.

Inside the root of this folder, there must be a set of public header and source files which the user's' project will include. The module may have as many other internal source files as it needs, but these must all be inside sub-folders!


### Master header file

In this root folder there must be ONE master header file, which includes all the necessary header files for the module. This header must have the same name as the module, with a .h/.hh/.hpp/.hxx suffix. E.g.

    yup_core/yup_core.h

IMPORTANT! All code within a module that includes other files from within its own subfolders must do so using RELATIVE paths!
A module must be entirely relocatable on disk, and it must not rely on the user's project having any kind of include path set up correctly for it to work. Even if the user has no include paths whatsoever and includes the module's master header via an absolute path, it must still correctly find all of its internally included sub-files.

This master header file must also contain a comment with a BEGIN_YUP_MODULE_DECLARATION block which defines the module's requirements - the syntax for this is described later on..


### Module CPP files

A module consists of a single header file and zero or more .c/.cc/.cpp/.cxx/.m/.mm files. Fewer is better!

Ideally, a module could be header-only module, so that a project can use it by simply including the master header file.

For various reasons it's usually necessary or preferable to have a simpler header and some .cpp files that the user's project should compile as stand-alone compile units. In this case you should ideally provide just a single cpp file in the module's root folder, and this should internally include all your other cpps from their sub-folders, so that only a single cpp needs to be added to the user's project in order to completely compile the module.

In some cases (e.g. if your module internally relies on 3rd-party code which can't be easily combined into a single compile-unit) then you may have more than one source file here, but avoid this if possible, as it will add a burden for users who are manually adding these files to their projects.

The names of these source files must begin with the name of the module, but they can have a number or other suffix if there is more than one.

In order to specify that a source file should only be compiled for a specific platform, then the filename can be suffixed with one of the following (case insensitive) strings:

    _apple          <- compiled for Apple platforms
    _mac            <- compiled for macOS only
    _ios            <- compiled for iOS only
    _msft           <- compiled for Microsoft platforms only
    _uwp            <- compiled for Universal Windows Platform only
    _windows        <- compiled for Windows desktop only (MSVC)
    _linux          <- compiled for Linux and FreeBSD only
    _android        <- compiled for Android only
    _posix          <- compiled for Posix platforms only
    _mobile         <- compiled for Mobile platforms (Android and iOS) only
    _wasm           <- compiled for Web platforms (Emscripten compiled)

e.g.
    yup_mymodule/yup_mymodule_1.cpp         <- compiled for all platforms
    yup_mymodule/yup_mymodule_2.cpp         <- compiled for all platforms
    yup_mymodule/yup_mymodule_mac.cpp       <- compiled for macOS platforms only
    yup_mymodule/yup_mymodule_windows.cpp     <- compiled for Windows platforms only

Often this isn't necessary, as in most cases you can easily add checks inside the files to do different things depending on the platform, but this may be handy just to avoid clutter in user projects where files aren't needed.

To simplify the use of obj-C++ there's also a special-case rule: If the folder contains both a .mm and a .cpp file whose names are otherwise identical, then on macOS/iOS the .mm will be used and the cpp ignored. (And vice-versa for other platforms, of course).


### Precompiled libraries

Precompiled libraries can be included in a module by placing them in a libs/ subdirectory. The following directories are automatically added to the library search paths, and libraries placed in these directories can be linked with projects via the OSXLibs, iOSLibs, windowsLibs and linuxLibs keywords in the module declaration (see the following section).

- OS X
  - libs/MacOSX - to support multiple architectures, you may place libraries built as universal binaries at this location. When building with CMake, only libraries built as universal binaries are supported and the arch subfolders are ignored.

- Visual Studio
  - libs/VisualStudio{year}/{arch}/{run-time}, where {year} is the four digit year of the Visual Studio release, arch is the target architecture in Visual Studio ("x64" or "Win32", for example), and {runtime} is the type of the run-time library indicated by the corresponding compiler flag ("MD", "MDd", "MT", "MTd").

- Linux
  - libs/Linux/{arch}, where {arch} is the architecture you are targeting with the compiler. Some common examples of {arch} are "x86_64", "i386" and "armv6".

- MinGW
  - libs/MinGW/{arch}, where {arch} can take the same values as Linux.

- iOS
  - libs/iOS - to support multiple architectures, you may place libraries built as universal binaries at this location. When building with CMake, only libraries built as universal binaries are supported and the arch subfolders are ignored.

- Android
  - libs/Android/{arch}, where {arch} is the architecture provided by the Android Studio variable "${ANDROID_ABI}" ("x86", "armeabi-v7a", "mips", for example).

## The BEGIN_YUP_MODULE_DECLARATION block

This block of text needs to go inside the module's main header file. It should be commented-out and perhaps inside an `#if 0` block too.

The block needs a corresponding END_YUP_MODULE_DECLARATION to finish the block. These should both be on a line of their own.

Inside the block, the parser will expect to find a list of value definitions, one-per-line, with the very simple syntax

    value_name:   value

The value_name must be one of the items listed below, and is case-sensitive. Whitespace on the line is ignored. Some values are compulsory and must be supplied, but others are optional. The order in which they're declared doesn't matter.

Possible values:

- ID
  - (Compulsory) This ID must match the name of the file and folder, e.g. yup_core. The main reason for also including it here is as a sanity-check.

- vendor
  - (Compulsory) A unique ID for the vendor, e.g. "yup". This should be short and shouldn't contain any spaces.

- version
  - (Compulsory) A version number for the module.

- name
  - (Compulsory) A short description of the module.

- description
  - (Compulsory) A longer description (but still only one line of text, please!).

- dependencies
  - (Optional) A list (space or comma-separated) of other modules that are required by this one.

- website
  - (Optional) A URL linking to useful info about the module.

- license
  - (Optional) A description of the type of software license that applies.

- minimumCppStandard
  - (Optional) A number indicating the minimum C++ language standard that is required for this module. This must be just the standard number with no prefix e.g. 20 for C++20.

- defines
  - (Optional) A list (space or comma-separated) of macro defines needed by this module.

- searchpaths
  - (Optional) A space-separated list of internal include paths, relative to the module's parent folder, which need to be added to a project's header search path.

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]CppStandard
  - (Optional) A number indicating the minimum C++ language standard that is required for this module and this platform exclusively. This must be just the standard number with no prefix e.g. 20 for C++20.

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]Deps
  - (Optional) A list (space or comma-separated) of other modules that are required by this one.

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]Defines
  - (Optional) A list (space or comma-separated) of macro defines needed by this module in a build.

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]Libs
  - (Optional) A list (space or comma-separated) of static or dynamic libs that should be linked in a build (these are passed to the linker via the -l flag).

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]Options
  - (Optional) A list (space or comma-separated) of compile options needed by this module in a build.

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]LinkOptions
  - (Optional) A list (space or comma-separated) of link options needed by this module in a build.

- [android|apple|ios|linux|mobile|msft|osx|wasm|win32|windows]Searchpaths
  - (Optional) A space-separated list of internal include paths, relative to the module's parent folder, which need to be added to a project's header search path.

- [ios|osx|apple]Frameworks
  - (Optional) A list (space or comma-separated) of iOS/macOS/Apple frameworks that are needed by this module.

- [ios|osx|apple]WeakFrameworks
  - (Optional) A list (space or comma-separated) of weak linked iOS/macOS/Apple frameworks that are needed by this module.

- linuxPackages
  - (Optional) A list (space or comma-separated) pkg-config packages that should be used to pass compiler (CFLAGS) and linker (LDFLAGS) flags.

Here's an example block:

    BEGIN_YUP_MODULE_DECLARATION

     ID:               yup_audio_devices
     vendor:           yup
     version:          1.0.0
     name:             YUP audio and MIDI I/O device classes
     description:      Classes to play and record from audio and MIDI I/O devices
     website:          https://github.com/kunitoki/yup
     license:          ISC

     dependencies:     yup_audio_basics yup_audio_formats yup_events
     osxFrameworks:    CoreAudio CoreMIDI DiscRecording
     iosFrameworks:    CoreAudio CoreMIDI AudioToolbox AVFoundation
     linuxLibs:        asound

    END_YUP_MODULE_DECLARATION
