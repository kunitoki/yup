# YUP: Cross-Platform Application And Plugin Development Library

<p float="left">
  <a href="https://kunitoki.github.io/yup/demos/web_render_0/" title="UI courtesy from https://www.drywestdesign.com/">
    <img src="./docs/demos/web_render_0.png" width="99%" /></a>
<p>

<p float="left">
  <a href="https://kunitoki.github.io/yup/demos/web_render_1/"><img src="./docs/demos/web_render_1.png" width="24%" /></a>
  <a href="https://kunitoki.github.io/yup/demos/web_render_2/"><img src="./docs/demos/web_render_2.png" width="24%" /></a>
  <a href="https://kunitoki.github.io/yup/demos/web_render_3/"><img src="./docs/demos/web_render_3.png" width="24%" /></a>
  <a href="https://kunitoki.github.io/yup/demos/web_render_4/"><img src="./docs/demos/web_render_4.png" width="23%" /></a>
</p>

Example Rive animation display ([source code](./examples/render/source/main.cpp)):
[Renderer Youtube Video](https://youtube.com/shorts/3XC4hyDlrVs)

[![Build And Test MacOS](https://github.com/kunitoki/yup/actions/workflows/build_macos.yml/badge.svg)](https://github.com/kunitoki/yup/actions/workflows/build_macos.yml)
[![Build And Test Windows](https://github.com/kunitoki/yup/actions/workflows/build_windows.yml/badge.svg)](https://github.com/kunitoki/yup/actions/workflows/build_windows.yml)
[![Build And Test Linux](https://github.com/kunitoki/yup/actions/workflows/build_linux.yml/badge.svg)](https://github.com/kunitoki/yup/actions/workflows/build_linux.yml)
[![Build And Test Wasm](https://github.com/kunitoki/yup/actions/workflows/build_wasm.yml/badge.svg)](https://github.com/kunitoki/yup/actions/workflows/build_wasm.yml)
[![Build And Test iOS](https://github.com/kunitoki/yup/actions/workflows/build_ios.yml/badge.svg)](https://github.com/kunitoki/yup/actions/workflows/build_ios.yml)
[![Build And Test Android](https://github.com/kunitoki/yup/actions/workflows/build_android.yml/badge.svg)](https://github.com/kunitoki/yup/actions/workflows/build_android.yml)

## Introduction
YUP is an open-source library dedicated to empowering developers with advanced tools for cross-platform application and plugin development, featuring state-of-the-art rendering and audio processing. Originating from a fork of [JUCE7](https://juce.com/)'s ISC-licensed modules, YUP builds on the robust, high-performance capabilities that made JUCE7 popular among audio and visual application developers. Unlike its successor JUCE8, which moved to a restrictive AGPL license and an even more costly commercial one, YUP maintains the more permissive ISC license and ensures that all of its dependencies are either liberally licensed or public domain, remaining a freely accessible and modifiable resource for developers worldwide.

> [!CAUTION]
> The project is still in embryonic stage, use it at your own risk!

## Features
YUP brings a suite of powerful features, including:
- **High-Performance Rendering:** From intricate visualizations to high-speed gaming graphics, YUP handles it all with ease and efficiency, relying on the open source [Rive](https://rive.app/) Renderer, backed by Metal, Direct3D, OpenGL, Vulkan and WebGPU.
- **Advanced Audio Processing:** Tailored for professionals, our audio toolkit delivers pristine sound quality with minimal latency, suitable for music production, live performance tools, and more. Based on the JUCE7 module for audio/midi input and output.
- **Open Source Audio Plugin Standards:** Facilitates the development of [CLAP](https://cleveraudio.org/) plugin abstractions, providing a framework for creating versatile and compatible audio plugins.
- **Cross-Platform Compatibility:** Consistent and reliable on Windows, macOS, Linux, Wasm (iOS and Android are in the pipe).
- **Extensive Testing Infrastructure:** Massive set of unit and integration tests to validate functionality.
- **Community-Driven Development:** As an open-source project, YUP thrives on contributions from developers around the globe.

## Supported Platforms
| **Windows**        | **macOS**          | **Linux**          | **WASM**           | **Android**        | **iOS**            |
|--------------------|:------------------:|:------------------:|:------------------:|:------------------:|:------------------:|
| :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: | :construction:     | :construction:     |

## Supported Rendering Backends
|                          | **Windows**        | **macOS**          | **Linux**          | **WASM**           | **Android**               | **iOS**               |
|--------------------------|:------------------:|:------------------:|:------------------:|:------------------:|:-------------------------:|:---------------------:|
| **OpenGL 4.2**           | :white_check_mark: |                    | :white_check_mark: |                    |                           |                       |
| **OpenGL ES3.0**         |                    |                    |                    |                    | :white_check_mark:        |                       |
| **WebGL2 (GLES3.0)**     |                    |                    |                    | :white_check_mark: |                           |                       |
| **Metal**                |                    | :white_check_mark: |                    |                    |                           | :white_check_mark:    |
| **Direct3D 11**          | :white_check_mark: |                    |                    |                    |                           |                       |
| **Vulkan**               | :construction:     |                    | :construction:     |                    | :construction:            |                       |
| **WebGPU**               | :construction:     | :construction:     | :construction:     | :construction:     | :construction:            | :construction:        |

## Supported Audio Backends
|                          | **Windows**        | **macOS**          | **Linux**          | **WASM**           | **Android**               | **iOS**               |
|--------------------------|:------------------:|:------------------:|:------------------:|:------------------:|:-------------------------:|:---------------------:|
| **CoreAudio**            |                    | :white_check_mark: |                    |                    |                           | :white_check_mark:    |
| **ASIO**                 | :white_check_mark: |                    |                    |                    |                           |                       |
| **DirectSound**          | :white_check_mark: |                    |                    |                    |                           |                       |
| **WASAPI**               | :white_check_mark: |                    |                    |                    |                           |                       |
| **ALSA**                 |                    |                    | :white_check_mark: |                    |                           |                       |
| **JACK**                 |                    |                    | :white_check_mark: |                    |                           |                       |
| **Oboe**                 |                    |                    |                    |                    | :white_check_mark:        |                       |
| **OpenSL**               |                    |                    |                    |                    | :white_check_mark:        |                       |
| **AudioWorklet**         |                    |                    |                    | :white_check_mark: |                           |                       |

## Prerequisites
Before building, ensure you have a:
- C++17-compliant compiler
- CMake 3.28 or later

### Windows
Visual Studio 2022

### macOS and iOS
Xcode 15.2 (and command-line tools).

### Linux
Required packages:

```bash
sudo apt-get update && sudo apt-get install -y \
    libasound2-dev libjack-jackd2-dev ladspa-sdk libcurl4-openssl-dev libfreetype6-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxcursor-dev libxext-dev libxi-dev libxinerama-dev \
    libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev
```

### Wasm
Emscripten SDK (at least version 3.1.45).

### Android
JDK 17, Android SDK, and NDK (at least r26d).

## Installation
Clone the YUP repository:

```bash
git clone https://github.com/kunitoki/yup.git
cd yup
```

## Preparing the build directory
Create a Dedicated Build Directory:

```bash
mkdir -p build
```

## Configure
Generate the build system files with CMake. For a standard desktop build with tests and examples enabled, run:
```bash
cmake . -B build -DYUP_ENABLE_TESTS=ON -DYUP_ENABLE_EXAMPLES=ON
```

For platform-specific targets, add extra flags:

### Android
```bash
cmake -G "Ninja Multi-Config" . -B build -DYUP_TARGET_ANDROID=ON -DYUP_ENABLE_TESTS=ON -DYUP_ENABLE_EXAMPLES=ON
```

### iOS
```bash
cmake -G "Ninja Multi-Config" . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM=OS64 -DYUP_ENABLE_TESTS=ON -DYUP_ENABLE_EXAMPLES=ON
```

### Wasm
Use Emscripten’s helper command, after having activated the emsdk (refer to https://emscripten.org/docs/getting_started/downloads.html how to install and activate Emscripten):
```bash
emcmake cmake -G "Ninja Multi-Config" . -B build -DYUP_ENABLE_TESTS=ON -DYUP_ENABLE_EXAMPLES=ON
```

## Building the Library
Once configuration is complete, compile YUP using your build system. For a Ninja-based build, for example:
```bash
cmake --build build --config Release --parallel 4
```

This command builds the project in Release mode. Replace `Release` with `Debug` if you need a debug build.

## Running Tests and Examples
After compilation, you can validate the build and explore YUP’s features:

- Run Tests:
Build and execute the yup_tests target to run the automated test suite.

- Build Examples:
Compile example targets like example_app, example_console, or example_render to see practical implementations.

## Running Your First Application
Here is a simple example of creating a basic window using YUP, save this as `main.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

class MyWindow : public yup::DocumentWindow
{
public:
    MyWindow()
        : yup::DocumentWindow (yup::ComponentNative::Options(), {})
    {
        setTitle ("MyWindow");

        takeFocus();
    }

    void paint (yup::Graphics& g) override
    {
        g.fillAll (0xffffffff);
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }
};

struct MyApplication : yup::YUPApplication
{
    MyApplication() = default;

    const yup::String getApplicationName() override
    {
        return "MyApplication";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        window = std::make_unique<MyWindow>();
        window->centreWithSize ({ 1080, 2400 });
        window->setVisible (true);
        window->toFront();
    }

    void shutdown() override
    {
        window.reset();
    }

private:
    std::unique_ptr<MyWindow> window;
};

START_JUCE_APPLICATION (MyApplication)
```

And add this as `CMakeLists.txt`:

```cmake
# TODO
```

## Documentation
For full documentation, including more detailed tutorials and comprehensive API references, please visit [YUP Documentation](https://yup.github.io/docs).

## Community Engagement
Join our growing community and contribute to the YUP project. Connect with us and other YUP developers:
- **GitHub:** [YUP Repository](https://github.com/kunitoki/yup)

> [!IMPORTANT]
> We are looking for collaborators to bring forward the framework!

## License
YUP is distributed under the ISC License, supporting both personal and commercial use, modification, and distribution without restrictions.

## Acknowledgments
YUP was born in response to JUCE8’s shift to a more restrictive licensing model. By forking JUCE7’s community-driven, ISC-licensed modules, we aim to preserve and continue a legacy of high-quality, freely accessible software development. We are grateful to the JUCE7 community for laying the groundwork for this initiative.
