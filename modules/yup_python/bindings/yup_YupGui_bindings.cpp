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

#include "yup_YupGui_bindings.h"

#include "../scripting/yup_ScriptBindings.h"
#include "../utilities/yup_ClassDemangling.h"
#include "../utilities/yup_PythonInterop.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_IOSTREAM
#include "../utilities/yup_PyBind11Includes.h"

#if YUP_WINDOWS
#include "../utilities/yup_WindowsIncludes.h"
#endif

#include <functional>
#include <string_view>
#include <typeinfo>
#include <tuple>

// =================================================================================================

namespace PYBIND11_NAMESPACE
{

template <>
struct polymorphic_type_hook<yup::Component>
{
    static const void* get (const yup::Component* src, const std::type_info*& type)
    {
        if (src == nullptr)
            return src;

        auto& map = yup::Bindings::getComponentTypeMap();
        auto demangledName = yup::Helpers::demangleClassName (typeid (*src).name());

        auto it = map.typeMap.find (demangledName);
        if (it != map.typeMap.end())
            return it->second (src, type);

        return src;
    }
};

} // namespace PYBIND11_NAMESPACE

namespace yup
{

// clang-format off

#if ! YUP_WINDOWS
extern const char* const* yup_argv;
extern int yup_argc;
#endif

namespace Bindings
{

namespace py = pybind11;
using namespace py::literals;

// =================================================================================================

Options& globalOptions() noexcept
{
    static Options options = {};
    return options;
}

// ============================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER
namespace
{
void runApplication (YUPApplicationBase* application, int milliseconds)
{
    try
    {
        py::gil_scoped_release release;

        if (! application->initialiseApp())
            return;

        MessageManager::getInstance()->runDispatchLoop();
    }
    catch (const py::error_already_set& e)
    {
        if (globalOptions().caughtKeyboardInterrupt)
            return;

        if (globalOptions().catchExceptionsAndContinue)
        {
            Helpers::printPythonException (e);
        }
        else
        {
            throw e;
        }
    }

    if (PyErr_CheckSignals() != 0)
        throw py::error_already_set();
}
} // namespace
#endif

// ============================================================================================

void registerYupGuiBindings (py::module_& m)
{
    // ============================================================================================ yup::YUPApplication

    py::class_<YUPApplication, PyYUPApplication> classYUPApplication (m, "YUPApplication");

    classYUPApplication
        .def (py::init<>())
        .def_static ("getInstance", &YUPApplication::getInstance, py::return_value_policy::reference)
        .def ("getApplicationName", &YUPApplication::getApplicationName)
        .def ("getApplicationVersion", &YUPApplication::getApplicationVersion)
        .def ("moreThanOneInstanceAllowed", &YUPApplication::moreThanOneInstanceAllowed)
        .def ("initialise", &YUPApplication::initialise, "commandLineParameters"_a)
        .def ("shutdown", &YUPApplication::shutdown)
        .def ("anotherInstanceStarted", &YUPApplication::anotherInstanceStarted)
        .def ("systemRequestedQuit", &YUPApplication::systemRequestedQuit)
        .def ("suspended", &YUPApplication::suspended)
        .def ("resumed", &YUPApplication::resumed)
        .def ("unhandledException", &YUPApplication::unhandledException)
        .def ("memoryWarningReceived", &YUPApplication::memoryWarningReceived)
        .def_static ("quit", &YUPApplication::quit)
        .def_static ("getCommandLineParameterArray", &YUPApplication::getCommandLineParameterArray)
        .def_static ("getCommandLineParameters", &YUPApplication::getCommandLineParameters)
        .def ("setApplicationReturnValue", [] (YUPApplication& self, int value) { self.setApplicationReturnValue (value); })
        .def ("getApplicationReturnValue", [] (const YUPApplication& self) { return self.getApplicationReturnValue(); })
        .def_static ("isStandaloneApp", &YUPApplication::isStandaloneApp)
        .def ("isInitialising", &YUPApplication::isInitialising);

    // ============================================================================================ yup::MouseCursor

    py::class_<MouseCursor> classMouseCursor (m, "MouseCursor");

    py::enum_<MouseCursor::Type> (classMouseCursor, "Type")
        .value ("None", MouseCursor::Type::None)
        .value ("Default", MouseCursor::Type::Default)
        .value ("Arrow", MouseCursor::Type::Arrow)
        .value ("Text", MouseCursor::Type::Text)
        .value ("Wait", MouseCursor::Type::Wait)
        .value ("WaitArrow", MouseCursor::Type::WaitArrow)
        .value ("Hand", MouseCursor::Type::Hand)
        .value ("Crosshair", MouseCursor::Type::Crosshair)
        .value ("Crossbones", MouseCursor::Type::Crossbones)
        .value ("ResizeLeftRight", MouseCursor::Type::ResizeLeftRight)
        .value ("ResizeUpDown", MouseCursor::Type::ResizeUpDown)
        .value ("ResizeTopLeftRightBottom", MouseCursor::Type::ResizeTopLeftRightBottom)
        .value ("ResizeBottomLeftRightTop", MouseCursor::Type::ResizeBottomLeftRightTop)
        .value ("ResizeAll", MouseCursor::Type::ResizeAll)
        .export_values();

    classMouseCursor
        .def (py::init<>())
        .def (py::init<MouseCursor::Type>(), "type"_a);

    // ============================================================================================ yup::ComponentNative

    py::class_<ComponentNative> classComponentNative (m, "ComponentNative");

    py::class_<ComponentNative::Options> classComponentNativeOptions (classComponentNative, "Options");

    classComponentNativeOptions
        .def (py::init<>())
        .def ("withFlags", &ComponentNative::Options::withFlags)
        .def ("withDecoration", &ComponentNative::Options::withDecoration)
        .def ("withResizableWindow", &ComponentNative::Options::withResizableWindow)
        .def ("withRenderContinuous", &ComponentNative::Options::withRenderContinuous)
        .def ("withAllowedHighDensityDisplay", &ComponentNative::Options::withAllowedHighDensityDisplay)
        //.def ("withGraphicsApi", &ComponentNative::Options::withGraphicsApi)
        .def ("withFramerateRedraw", &ComponentNative::Options::withFramerateRedraw)
        .def ("withClearColor", &ComponentNative::Options::withClearColor)
        .def ("withDoubleClickTime", &ComponentNative::Options::withDoubleClickTime)
        .def ("withUpdateOnlyFocused", &ComponentNative::Options::withUpdateOnlyFocused)
    ;

    classComponentNative
        .def ("setTitle", &ComponentNative::setTitle)
        .def ("getTitle", &ComponentNative::getTitle)
        .def ("setVisible", &ComponentNative::setVisible)
        .def ("isVisible", &ComponentNative::isVisible)
        .def ("setSize", &ComponentNative::setSize)
        .def ("getSize", &ComponentNative::getSize)
        .def ("getContentSize", &ComponentNative::getContentSize)
        .def ("getPosition", &ComponentNative::getPosition)
        .def ("setPosition", &ComponentNative::setPosition)
        .def ("getBounds", &ComponentNative::getBounds)
        .def ("setBounds", &ComponentNative::setBounds)
        .def ("setFullScreen", &ComponentNative::setFullScreen)
        .def ("isFullScreen", &ComponentNative::isFullScreen)
        .def ("isDecorated", &ComponentNative::isDecorated)
        .def ("setOpacity", &ComponentNative::setOpacity)
        .def ("getOpacity", &ComponentNative::getOpacity)
        .def ("setFocusedComponent", &ComponentNative::setFocusedComponent)
        .def ("getFocusedComponent", &ComponentNative::getFocusedComponent)
        .def ("isContinuousRepaintingEnabled", &ComponentNative::isContinuousRepaintingEnabled)
        .def ("enableContinuousRepainting", &ComponentNative::enableContinuousRepainting)
        .def ("isAtomicModeEnabled", &ComponentNative::isAtomicModeEnabled)
        .def ("enableAtomicMode", &ComponentNative::enableAtomicMode)
        .def ("isWireframeEnabled", &ComponentNative::isWireframeEnabled)
        .def ("enableWireframe", &ComponentNative::enableWireframe)
        .def ("repaint", py::overload_cast<> (&ComponentNative::repaint))
        .def ("repaint", py::overload_cast<const Rectangle<float>&> (&ComponentNative::repaint))
        .def ("getRepaintAreas", &ComponentNative::getRepaintAreas)
        .def ("getScaleDpi", &ComponentNative::getScaleDpi)
        .def ("getCurrentFrameRate", &ComponentNative::getCurrentFrameRate)
        .def ("getDesiredFrameRate", &ComponentNative::getDesiredFrameRate)
        .def ("getNativeHandle", &ComponentNative::getNativeHandle, py::return_value_policy::reference_internal)
        .def_static ("createFor", &ComponentNative::createFor, "component"_a, "options"_a, "parent"_a = nullptr)
    ;

    // ============================================================================================ yup::Component

    py::class_<Component, PyComponent<>> classComponent (m, "Component");

    classComponent
        // Construction and identification
        .def (py::init_alias<>())
        .def (py::init_alias<StringRef>(), "componentID"_a)
        .def ("getComponentID", &Component::getComponentID)

        // Basic state properties
        .def ("isEnabled", &Component::isEnabled)
        .def ("setEnabled", &Component::setEnabled)
        .def ("isVisible", &Component::isVisible)
        .def ("setVisible", &Component::setVisible)
        .def ("isShowing", &Component::isShowing)
        .def ("getTitle", &Component::getTitle)
        .def ("setTitle", &Component::setTitle)

        // Position and size
        .def ("getPosition", &Component::getPosition)
        .def ("setPosition", &Component::setPosition)
        .def ("getScreenPosition", &Component::getScreenPosition)
        .def ("getX", &Component::getX)
        .def ("getY", &Component::getY)
        .def ("getLeft", &Component::getLeft)
        .def ("getTop", &Component::getTop)
        .def ("getRight", &Component::getRight)
        .def ("getBottom", &Component::getBottom)
        .def ("getTopLeft", &Component::getTopLeft)
        .def ("setTopLeft", &Component::setTopLeft)
        .def ("getBottomLeft", &Component::getBottomLeft)
        .def ("setBottomLeft", &Component::setBottomLeft)
        .def ("getTopRight", &Component::getTopRight)
        .def ("setTopRight", &Component::setTopRight)
        .def ("getBottomRight", &Component::getBottomRight)
        .def ("setBottomRight", &Component::setBottomRight)
        .def ("getCenter", &Component::getCenter)
        .def ("setCenter", &Component::setCenter)
        .def ("getCenterX", &Component::getCenterX)
        .def ("setCenterX", &Component::setCenterX)
        .def ("getCenterY", &Component::getCenterY)
        .def ("setCenterY", &Component::setCenterY)
        .def ("getSize", &Component::getSize)
        .def ("setSize", py::overload_cast<float, float>(&Component::setSize))
        .def ("setSize", py::overload_cast<const Size<float>&>(&Component::setSize))
        .def ("getWidth", &Component::getWidth)
        .def ("getHeight", &Component::getHeight)
        .def ("proportionOfWidth", &Component::proportionOfWidth)
        .def ("proportionOfHeight", &Component::proportionOfHeight)

        // Bounds
        .def ("setBounds", py::overload_cast<float, float, float, float>(&Component::setBounds))
        .def ("setBounds", py::overload_cast<const Rectangle<float>&>(&Component::setBounds))
        .def ("getBounds", &Component::getBounds)
        .def ("getLocalBounds", &Component::getLocalBounds)
        .def ("getBoundsRelativeToTopLevelComponent", &Component::getBoundsRelativeToTopLevelComponent)
        .def ("getScreenBounds", &Component::getScreenBounds)

        // Coordinate conversion
        .def ("localToScreen", py::overload_cast<const Point<float>&>(&Component::localToScreen, py::const_))
        .def ("screenToLocal", py::overload_cast<const Point<float>&>(&Component::screenToLocal, py::const_))
        .def ("localToScreen", py::overload_cast<const Rectangle<float>&>(&Component::localToScreen, py::const_))
        .def ("screenToLocal", py::overload_cast<const Rectangle<float>&>(&Component::screenToLocal, py::const_))
        .def ("getLocalPoint", &Component::getLocalPoint)
        .def ("getLocalArea", &Component::getLocalArea)
        .def ("getRelativePoint", &Component::getRelativePoint)
        .def ("getRelativeArea", &Component::getRelativeArea)

        // Transform
        .def ("setTransform", &Component::setTransform)
        .def ("getTransform", &Component::getTransform)
        .def ("isTransformed", &Component::isTransformed)
        .def ("getTransformToComponent", &Component::getTransformToComponent)
        .def ("getTransformFromComponent", &Component::getTransformFromComponent)
        .def ("getTransformToScreen", &Component::getTransformToScreen)

        // Full screen
        .def ("setFullScreen", &Component::setFullScreen)
        .def ("isFullScreen", &Component::isFullScreen)

        // Scale and display
        .def ("getScaleDpi", &Component::getScaleDpi)

        // Opacity and rendering
        .def ("getOpacity", &Component::getOpacity)
        .def ("setOpacity", &Component::setOpacity)
        .def ("isOpaque", &Component::isOpaque)
        .def ("setOpaque", &Component::setOpaque)
        .def ("enableRenderingUnclipped", &Component::enableRenderingUnclipped)
        .def ("isRenderingUnclipped", &Component::isRenderingUnclipped)
        .def ("refreshDisplay", &Component::refreshDisplay, "lastFrameTimeSeconds"_a)
        .def ("repaint", py::overload_cast<>(&Component::repaint))
        .def ("repaint", py::overload_cast<const Rectangle<float>&>(&Component::repaint))
        .def ("repaint", py::overload_cast<float, float, float, float>(&Component::repaint))

        // Native component
        .def ("getNativeHandle", &Component::getNativeHandle, py::return_value_policy::reference_internal)
        .def ("getNativeComponent", py::overload_cast<>(&Component::getNativeComponent), py::return_value_policy::reference_internal)
        .def ("isOnDesktop", &Component::isOnDesktop)
        .def ("addToDesktop", &Component::addToDesktop, "nativeOptions"_a, "parent"_a = nullptr)
        .def ("removeFromDesktop", &Component::removeFromDesktop)
        .def ("userTriedToCloseWindow", &Component::userTriedToCloseWindow)

        // Z-order
        .def ("toFront", &Component::toFront)
        .def ("toBack", &Component::toBack)
        .def ("raiseAbove", &Component::raiseAbove, py::return_value_policy::reference_internal)
        .def ("lowerBelow", &Component::lowerBelow, py::return_value_policy::reference_internal)
        .def ("raiseBy", &Component::raiseBy)
        .def ("lowerBy", &Component::lowerBy)

        // Mouse cursor
        .def ("setMouseCursor", &Component::setMouseCursor)
        .def ("getMouseCursor", &Component::getMouseCursor)

        // Keyboard focus
        .def ("setWantsKeyboardFocus", &Component::setWantsKeyboardFocus)
        .def ("takeKeyboardFocus", &Component::takeKeyboardFocus)
        .def ("leaveKeyboardFocus", &Component::leaveKeyboardFocus)
        .def ("hasKeyboardFocus", &Component::hasKeyboardFocus)

        // Hierarchy
        .def ("hasParent", &Component::hasParent)
        .def ("getParentComponent", py::overload_cast<>(&Component::getParentComponent), py::return_value_policy::reference_internal)
        .def ("addChildComponent", py::overload_cast<Component&, int>(&Component::addChildComponent), "component"_a, "index"_a = -1, py::return_value_policy::reference_internal)
        .def ("addChildComponent", py::overload_cast<Component*, int>(&Component::addChildComponent), "component"_a, "index"_a = -1, py::return_value_policy::reference_internal)
        .def ("addAndMakeVisible", py::overload_cast<Component&, int>(&Component::addAndMakeVisible), "component"_a, "index"_a = -1, py::return_value_policy::reference_internal)
        .def ("addAndMakeVisible", py::overload_cast<Component*, int>(&Component::addAndMakeVisible), "component"_a, "index"_a = -1, py::return_value_policy::reference_internal)
        .def ("removeChildComponent", py::overload_cast<Component&>(&Component::removeChildComponent))
        .def ("removeChildComponent", py::overload_cast<Component*>(&Component::removeChildComponent))
        .def ("removeChildComponent", py::overload_cast<int>(&Component::removeChildComponent))
        .def ("removeAllChildren", &Component::removeAllChildren)
        .def ("getNumChildComponents", &Component::getNumChildComponents)
        .def ("getChildComponent", &Component::getChildComponent, py::return_value_policy::reference_internal)
        .def ("getIndexOfChildComponent", &Component::getIndexOfChildComponent)
        .def ("findComponentAt", &Component::findComponentAt, py::return_value_policy::reference_internal)
        .def ("getTopLevelComponent", &Component::getTopLevelComponent, py::return_value_policy::reference_internal)

        // Properties
        .def ("getProperties", py::overload_cast<>(&Component::getProperties), py::return_value_policy::reference_internal)

        // Mouse events
        .def ("setWantsMouseEvents", &Component::setWantsMouseEvents)
        .def ("doesWantSelfMouseEvents", &Component::doesWantSelfMouseEvents)
        .def ("doesWantChildrenMouseEvents", &Component::doesWantChildrenMouseEvents)
        .def ("addMouseListener", &Component::addMouseListener, py::return_value_policy::reference_internal)
        .def ("removeMouseListener", &Component::removeMouseListener)

        // Style system
        .def ("setStyle", &Component::setStyle)
        .def ("getStyle", &Component::getStyle)
        .def ("setColor", &Component::setColor)
        .def ("getColor", &Component::getColor)
        .def ("findColor", &Component::findColor)
        .def ("setStyleProperty", &Component::setStyleProperty)
        .def ("getStyleProperty", &Component::getStyleProperty)
        .def ("findStyleProperty", &Component::findStyleProperty)
    ;

    // ============================================================================================ yup::DocumentWindow

    py::class_<DocumentWindow, Component, PyDocumentWindow<>> classDocumentWindow (m, "DocumentWindow");

    classDocumentWindow
        .def (py::init<>())
        .def (py::init<const ComponentNative::Options&>())
        .def (py::init<const ComponentNative::Options&, const std::optional<Color>&>())
        .def ("centreWithSize", &DocumentWindow::centreWithSize)
    ;

    // =================================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER
    m.def ("START_YUP_APPLICATION", [] (py::handle applicationType, bool catchExceptionsAndContinue)
    {
        globalOptions().catchExceptionsAndContinue = catchExceptionsAndContinue;
        globalOptions().caughtKeyboardInterrupt = false;

        py::scoped_ostream_redirect output;

        if (! applicationType)
            throw py::value_error ("Argument must be a YUPApplication subclass");

        YUPApplicationBase* application = nullptr;

        auto sys = py::module_::import ("sys");
        auto systemExit = [sys, &application]
        {
            int returnValue = 255;

            {
                py::gil_scoped_release release;

                if (application != nullptr)
                    returnValue = application->shutdownApp();
            }

            sys.attr ("exit") (returnValue);
        };

#if ! YUP_WINDOWS
        StringArray arguments;
        for (auto arg : sys.attr ("argv"))
            arguments.add (arg.cast<String>());

        Array<const char*> argv;
        for (const auto& arg : arguments)
            argv.add (arg.toRawUTF8());

        yup_argv = argv.getRawDataPointer();
        yup_argc = argv.size();
#endif

        auto pyApplication = applicationType(); // TODO - error checking (python)

        application = pyApplication.cast<YUPApplication*>();
        if (application != nullptr)
        {
            try
            {
                runApplication (application, globalOptions().messageManagerGranularityMilliseconds);
            }
            catch (const py::error_already_set& e)
            {
                Helpers::printPythonException (e);
            }
        }

        systemExit();
    }, "applicationType"_a, "catchExceptionsAndContinue"_a = false);

    // =================================================================================================

    struct PyTestableApplication
    {
        struct Scope
        {
            Scope (py::handle applicationType)
            {
                if (! applicationType)
                    throw py::value_error ("Argument must be a YUPApplication subclass");

                YUPApplicationBase* application = nullptr;

#if ! YUP_WINDOWS
                for (auto arg : py::module_::import ("sys").attr ("argv"))
                    arguments.add (arg.cast<String>());

                for (const auto& arg : arguments)
                    argv.add (arg.toRawUTF8());

                yup_argv = argv.getRawDataPointer();
                yup_argc = argv.size();
#endif

                auto pyApplication = applicationType();

                application = pyApplication.cast<YUPApplication*>();
                if (application == nullptr)
                    return;

                if (! application->initialiseApp())
                    return;
            }

            ~Scope()
            {
            }

        private:
#if ! YUP_WINDOWS
            StringArray arguments;
            Array<const char*> argv;
#endif
        };

        PyTestableApplication (py::handle applicationType)
            : applicationType (applicationType)
        {
        }

        void processEvents (int milliseconds = 20)
        {
            try
            {
                YUP_TRY
                {
                    py::gil_scoped_release release;

                    if (MessageManager::getInstance()->hasStopMessageBeenSent())
                        return;

                    MessageManager::getInstance()->runDispatchLoopUntil (milliseconds);
                }
                YUP_CATCH_EXCEPTION

                bool isErrorSignalInFlight = PyErr_CheckSignals() != 0;
                if (isErrorSignalInFlight)
                    throw py::error_already_set();
            }
            catch (const py::error_already_set& e)
            {
                py::print (e.what());
            }
            catch (...)
            {
                py::print ("unhandled runtime error");
            }
        }

        py::handle applicationType;
        std::unique_ptr<Scope> applicationScope;
    };

    py::class_<PyTestableApplication> classTestableApplication (m, "TestApplication");
    classTestableApplication
        .def (py::init<py::handle>())
        .def ("processEvents", &PyTestableApplication::processEvents, "milliseconds"_a = 20)
        .def ("__enter__", [] (PyTestableApplication& self)
        {
            self.applicationScope = std::make_unique<PyTestableApplication::Scope> (self.applicationType);
            return std::addressof (self);
        }, py::return_value_policy::reference)
        .def ("__exit__", [] (PyTestableApplication& self, const std::optional<py::type>&, const std::optional<py::object>&, const std::optional<py::object>&)
        {
            self.applicationScope.reset();
        })
        .def ("__next__", [] (PyTestableApplication& self)
        {
            self.processEvents();
            return std::addressof (self);
        }, py::return_value_policy::reference);
#endif
}

} // namespace Bindings

// clang-format on

} // namespace yup

// =================================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER && YUP_WINDOWS
BOOL APIENTRY DllMain (HANDLE instance, DWORD reason, LPVOID reserved)
{
    yup::ignoreUnused (reserved);

    if (reason == DLL_PROCESS_ATTACH)
        yup::Process::setCurrentModuleInstanceHandle (instance);

    return true;
}
#endif
