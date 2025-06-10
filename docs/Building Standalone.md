# Building Standalone Applications with YUP

This guide explains how to create standalone applications using the YUP framework. YUP provides a robust foundation for building cross-platform desktop applications with modern UI capabilities.

## Basic Application Structure

A basic YUP application consists of the following components:

1. A main application class that manages the application lifecycle
2. One or more window classes for the user interface
3. Application-specific resources and assets

Here's a minimal example of a standalone application:

```cpp
#include <juce_core/juce_core.h>
#include <yup_events/yup_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

class MainWindow : public yup::DocumentWindow
{
public:
    MainWindow()
        : DocumentWindow (yup::ComponentNative::Options(), {})
    {
        setTitle ("My Application");
        setSize (800, 600);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
        takeFocus();
    }

    void paint (yup::Graphics& g) override
    {
        g.fillAll (yup::Colours::black);

        //g.setColour (yup::Colours::white);
        //g.setFont (16.0f);
        //g.drawText ("Hello, YUP!", getLocalBounds(), yup::Justification::centred);
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

class MyApplication : public yup::YUPApplication
{
public:
    MyApplication() = default;

    const yup::String getApplicationName() override
    {
        return "My Application";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        window = std::make_unique<MainWindow>();
    }

    void shutdown() override
    {
        window.reset();
    }

private:
    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION (MyApplication)
```

## Building with CMake

Create a `CMakeLists.txt` file for your application:

```cmake
cmake_minimum_required (VERSION 3.28)

set (target_name my_app)
set (target_version "1.0.0")
project (${target_name} VERSION ${target_version})

include (FetchContent)

FetchContent_Declare(
  yup
  GIT_REPOSITORY https://github.com/kunitoki/yup.git
  GIT_TAG        main)

set (YUP_BUILD_EXAMPLES OFF)
set (YUP_BUILD_TESTS OFF)
FetchContent_MakeAvailable(yup)

# Create the standalone application target
yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "MyApp"
    TARGET_APP_ID "com.mycompany.${target_name}"
    TARGET_APP_NAMESPACE "com.mycompany"
    TARGET_CXX_STANDARD 17
    INITIAL_MEMORY 268435456  # 256MB initial memory
    MODULES
        juce_audio_devices
        yup_gui
        libpng
        libwebp)

# Add source files
file (GLOB sources "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})

# Add resources if needed
if (NOT YUP_TARGET_ANDROID)
    file (GLOB resources "${CMAKE_CURRENT_LIST_DIR}/resources/*")
    source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/resources/ FILES ${resources})
    target_sources (${target_name} PRIVATE ${resources})
endif()
```

## Application Resources

### Resource Management

YUP provides several ways to manage application resources:

1. **Embedded Resources**

```cmake
// In your CMakeLists.txt
yup_add_embedded_binary_resources (
    "${target_name}_binary_data"
    OUT_DIR BinaryData
    HEADER BinaryData.h
    NAMESPACE MyApp
    RESOURCE_NAMES
        Settings
        Logo
    RESOURCES
        "resources/config/settings.json"
        "resources/images/logo.png")

yup_standalone_app (
    # ...
    MODULES
        juce_audio_devices
        yup_gui
        libpng
        libwebp
        ${target_name}_binary_data) # << add this
```

```cpp
// In your code you can access these binaries
#include <BinaryData.h>

yup::MemoryInputStream is (MyApp::Settings_data, MyApp::Settings_size, false);
```

2. **File System Resources**
```cpp
// Get the application's data directory
auto dataDir = yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory)
    .getChildFile (getApplicationName());

// Create if it doesn't exist
if (! dataDir.exists())
    dataDir.createDirectory();
```

## Best Practices

1. **Window Management**
   - Use `DocumentWindow` for main windows
   - Implement proper window closing behavior
   - Handle window resizing and positioning

2. **Resource Management**
   - Use RAII for resource allocation
   - Cache frequently used resources
   - Clean up resources in destructors

3. **UI Design**
   - Follow platform-specific UI guidelines
   - Implement responsive layouts
   - Handle different DPI settings

4. **Performance**
   - Minimize allocations in UI thread
   - Use background threads for heavy operations
   - Profile your application

5. **Testing**
   - Test on all target platforms
   - Verify window management
   - Check resource cleanup
   - Test different screen resolutions

## Common Issues and Solutions

1. **Window Issues**
   - Handle window focus properly
   - Manage window state (minimized, maximized)
   - Handle multiple monitors

2. **Resource Problems**
   - Check file permissions
   - Verify resource paths
   - Handle missing resources gracefully

3. **Platform-Specific Issues**
   - Handle platform-specific menu systems
   - Manage platform-specific window decorations
   - Handle platform-specific file system operations

## Additional Resources

- [YUP Documentation](https://yup.github.io/docs)
- [YUP Examples](https://github.com/kunitoki/yup/tree/main/examples)
- [Platform-Specific Guidelines](https://yup.github.io/docs/platforms)
