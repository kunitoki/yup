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
#include <optional>
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

namespace
{

/** Reports an unhandled exception through python, returning true if it was a KeyboardInterrupt.

    Must only be called on the message thread.
*/
bool reportToPython (YUPApplication* application,
                     const std::exception* ex,
                     const String& sourceFilename,
                     int lineNumber) noexcept
{
    bool wasInterrupt = false;

    try
    {
        py::gil_scoped_acquire gil;

        const auto* pyEx = dynamic_cast<const py::error_already_set*> (ex);
        auto traceback = py::module_::import ("traceback");

        auto override_ = (application != nullptr)
                           ? py::get_override (application, "unhandledException")
                           : py::function();

        if (override_)
        {
            if (pyEx != nullptr)
            {
                auto newPyEx = pyEx->type() (pyEx->value());
                PyException_SetTraceback (newPyEx.ptr(), pyEx->trace().ptr());

                override_ (newPyEx, sourceFilename, lineNumber);
            }
            else
            {
                auto runtimeError = py::module_::import (PYBIND11_BUILTINS_MODULE).attr ("RuntimeError");
                auto newPyEx = runtimeError (ex != nullptr ? ex->what() : "unknown exception");
                PyException_SetTraceback (newPyEx.ptr(), traceback.attr ("extract_stack")().ptr());

                override_ (newPyEx, sourceFilename, lineNumber);
            }
        }
        else if (pyEx != nullptr)
        {
            traceback.attr ("print_exception") (pyEx->type(),
                                                pyEx->value(),
                                                pyEx->trace() ? pyEx->trace() : py::none());
        }
        else
        {
            py::print (ex != nullptr ? ex->what() : "unknown exception");
            traceback.attr ("print_stack")();
        }

        if (pyEx != nullptr && pyEx->matches (PyExc_KeyboardInterrupt))
        {
            wasInterrupt = true;
            globalOptions().caughtKeyboardInterrupt = true;
        }
    }
    catch (...)
    {
        Logger::writeToLog ("Failed to report an unhandled exception at " + sourceFilename + ":" + String (lineNumber));
    }

    return wasInterrupt;
}

} // namespace

void reportUnhandledException (YUPApplication* application,
                               const std::exception* ex,
                               const String& sourceFilename,
                               int lineNumber) noexcept
{
    try
    {
        auto* mm = MessageManager::getInstanceWithoutCreating();
        const bool shouldStop = ! globalOptions().catchExceptionsAndContinue;

        if (mm != nullptr && mm->isThisTheMessageThread())
        {
            const bool wasInterrupt = reportToPython (application, ex, sourceFilename, lineNumber);

            if (shouldStop || wasInterrupt)
                mm->stopDispatchLoop();

            return;
        }

        bool posted = false;

        if (mm != nullptr)
        {
            if (const auto* pyEx = dynamic_cast<const py::error_already_set*> (ex))
            {
                posted = MessageManager::callAsync ([application, e = *pyEx, sourceFilename, lineNumber, shouldStop]
                {
                    const bool wasInterrupt = reportToPython (application, std::addressof (e), sourceFilename, lineNumber);

                    if (! shouldStop && ! wasInterrupt)
                        return;

                    if (auto* messageManager = MessageManager::getInstanceWithoutCreating())
                        messageManager->stopDispatchLoop();
                });
            }
        }

        if (! posted)
        {
            const auto description = (dynamic_cast<const py::error_already_set*> (ex) != nullptr)
                                       ? String ("python exception, traceback unavailable")
                                       : String (ex != nullptr ? ex->what() : "unknown exception");

            Logger::writeToLog ("Unhandled exception at " + sourceFilename + ":" + String (lineNumber) + " - " + description);

            if (shouldStop && mm != nullptr)
                mm->stopDispatchLoop();
        }
    }
    catch (...)
    {
        jassertfalse;
    }
}

// ============================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER
namespace
{

/** Polls python's signal handlers so that ctrl+c interrupts the message loop.

    The raised error is kept here and re-raised by the caller once the application has shut down:
    throwing it from the callback would unwind through the platform timer's C frame.
*/
struct PythonSignalCheckTimer final : public Timer
{
    void timerCallback() override
    {
        py::gil_scoped_acquire gil;

        if (PyErr_CheckSignals() == 0)
            return;

        pendingInterrupt.emplace(); // fetches and clears the error the signal handler raised
        globalOptions().caughtKeyboardInterrupt = true;

        stopTimer();

        if (auto* mm = MessageManager::getInstanceWithoutCreating())
            mm->stopDispatchLoop();
    }

    /** The error a python signal handler raised, if any. Only read it with the GIL held. */
    std::optional<py::error_already_set> pendingInterrupt;
};

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

    // ============================================================================================ yup::ApplicationTheme

    py::class_<ApplicationTheme, ReferenceCountedObjectPtr<ApplicationTheme>> (m, "ApplicationTheme")
        .def_static ("getGlobalTheme", [] () -> ApplicationTheme::Ptr
        {
            auto theme = ApplicationTheme::getGlobalTheme();
            if (theme == nullptr)
                throw py::value_error ("No global theme is set: the gui subsystem has not been initialised yet");

            return theme;
        })
        .def ("getDefaultFont", &ApplicationTheme::getDefaultFont)
        .def ("getDefaultIconFont", &ApplicationTheme::getDefaultIconFont)
        .def ("getDefaultMonospaceFont", &ApplicationTheme::getDefaultMonospaceFont);

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

    // ============================================================================================ yup::MouseEvent

    py::class_<MouseEvent> classMouseEvent (m, "MouseEvent");

    py::enum_<MouseEvent::Buttons> (classMouseEvent, "Buttons")
        .value ("noButtons", MouseEvent::noButtons)
        .value ("leftButton", MouseEvent::leftButton)
        .value ("middleButton", MouseEvent::middleButton)
        .value ("rightButton", MouseEvent::rightButton)
        .value ("allButtons", MouseEvent::allButtons)
        .export_values();

    classMouseEvent
        .def (py::init<>())
        .def (py::init<MouseEvent::Buttons, KeyModifiers, const Point<float>&>(),
              "buttons"_a, "modifiers"_a, "position"_a)
        .def (py::init<MouseEvent::Buttons, KeyModifiers, const Point<float>&, Component*>(),
              "buttons"_a, "modifiers"_a, "position"_a, "sourceComponent"_a)

        .def ("getButtons", &MouseEvent::getButtons)
        .def ("isLeftButtonDown", &MouseEvent::isLeftButtonDown)
        .def ("isMiddleButtonDown", &MouseEvent::isMiddleButtonDown)
        .def ("isRightButtonDown", &MouseEvent::isRightButtonDown)
        .def ("isAnyButtonDown", &MouseEvent::isAnyButtonDown)
        .def ("withButtons", &MouseEvent::withButtons, "buttonsToAdd"_a)
        .def ("withoutButtons", &MouseEvent::withoutButtons, "buttonsToRemove"_a)

        .def ("getModifiers", &MouseEvent::getModifiers)
        .def ("withModifiers", &MouseEvent::withModifiers, "newModifiers"_a)

        .def ("getPosition", &MouseEvent::getPosition)
        .def ("getScreenPosition", &MouseEvent::getScreenPosition)
        .def ("withPosition", &MouseEvent::withPosition, "newPosition"_a)
        .def ("withTranslatedPosition", &MouseEvent::withTranslatedPosition, "translation"_a)
        .def ("withRelativePositionTo", &MouseEvent::withRelativePositionTo, "targetComponent"_a)

        .def ("getLastMouseDownPosition", &MouseEvent::getLastMouseDownPosition)
        .def ("withLastMouseDownPosition", &MouseEvent::withLastMouseDownPosition, "newPosition"_a)
        .def ("getLastMouseDownTime", &MouseEvent::getLastMouseDownTime)
        .def ("withLastMouseDownTime", &MouseEvent::withLastMouseDownTime, "newTime"_a)

        // The event only borrows the component it refers to: the platform builds it around the
        // component being clicked, and only hands it out while the callback it was built for runs.
        .def ("getSourceComponent", &MouseEvent::getSourceComponent, py::return_value_policy::reference)
        .def ("withSourceComponent", &MouseEvent::withSourceComponent, "newComponent"_a)

        .def ("isTouch", &MouseEvent::isTouch)
        .def ("getTouchIndex", &MouseEvent::getTouchIndex)
        .def ("getPressure", &MouseEvent::getPressure)
        .def ("withTouchIndex", &MouseEvent::withTouchIndex, "newTouchIndex"_a)
        .def ("withPressure", &MouseEvent::withPressure, "newPressure"_a)

        .def (py::self == py::self)
        .def (py::self != py::self);

    // ============================================================================================ yup::MouseListener

    // Registered before Component, which derives from it in C++. The MouseListener overrides a
    // Python subclass writes are dispatched through PyMouseListener, which Component shares.
    py::class_<MouseListener, PyMouseListener<>> classMouseListener (m, "MouseListener");

    classMouseListener
        .def (py::init<>())
        .def ("mouseEnter", &MouseListener::mouseEnter, "event"_a)
        .def ("mouseExit", &MouseListener::mouseExit, "event"_a)
        .def ("mouseDown", &MouseListener::mouseDown, "event"_a)
        .def ("mouseMove", &MouseListener::mouseMove, "event"_a)
        .def ("mouseDrag", &MouseListener::mouseDrag, "event"_a)
        .def ("mouseUp", &MouseListener::mouseUp, "event"_a)
        .def ("mouseDoubleClick", &MouseListener::mouseDoubleClick, "event"_a)
        .def ("mouseWheel", &MouseListener::mouseWheel, "event"_a, "wheelData"_a);

    // ============================================================================================ yup::TextInputTarget

    py::class_<TextInputTarget, PyTextInputTarget<>> classTextInputTarget (m, "TextInputTarget");

    classTextInputTarget
        .def (py::init<>())
        .def ("getTextInputRect", &TextInputTarget::getTextInputRect,
              "Returns the screen rectangle being edited, so on-screen keyboards and IME windows\n"
              "can be moved out of the way. Subclasses must override it.")
        .def ("requestTextInput", &TextInputTarget::requestTextInput,
              "Asks the system to start accepting text input, typically from focusGained().")
        .def ("relinquishTextInput", &TextInputTarget::relinquishTextInput,
              "Tells the system this target no longer needs text input, typically from focusLost().")
        .def ("updateTextInputRect", &TextInputTarget::updateTextInputRect,
              "Pushes a moved caret or edited area to the system while text input is active.")
        .def ("isTextInputActive", &TextInputTarget::isTextInputActive);

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
        .def ("withMouseCapture", &ComponentNative::Options::withMouseCapture)
        .def ("withGraphicsApi", &ComponentNative::Options::withGraphicsApi)
        .def ("withFramerateRedraw", &ComponentNative::Options::withFramerateRedraw)
        .def ("withClearColor", &ComponentNative::Options::withClearColor)
        .def ("withDoubleClickTime", &ComponentNative::Options::withDoubleClickTime)
        .def ("withUpdateOnlyFocused", &ComponentNative::Options::withUpdateOnlyFocused)
        .def ("withVSync", &ComponentNative::Options::withVSync)
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

    // ============================================================================================ yup::DragAndDropData

    py::class_<DragAndDropData> classDragAndDropData (m, "DragAndDropData");

    classDragAndDropData
        .def (py::init<>())
        .def ("withFiles", [] (const DragAndDropData& self, py::iterable files)
        {
            Array<File> fileList;

            for (auto item : files)
            {
                py::detail::make_caster<File> conv;

                if (! conv.load (item, true))
                    throw py::type_error ("DragAndDropData.withFiles expects an iterable of File");

                fileList.add (py::detail::cast_op<File&&> (std::move (conv)));
            }

            return self.withFiles (fileList);
        }, "files"_a, "Returns a copy of this payload with the given files set.")
        .def ("withText", &DragAndDropData::withText, "text"_a, "Returns a copy of this payload with the given text set.")
        .def ("withUris", &DragAndDropData::withUris, "uris"_a, "Returns a copy of this payload with the given URIs set.")
        .def ("getFiles", &DragAndDropData::getFiles, "Returns the dropped files.")
        .def ("getText", &DragAndDropData::getText, "Returns the dropped text.")
        .def ("getUris", &DragAndDropData::getUris, "Returns the dropped URIs.")
        .def ("hasFiles", &DragAndDropData::hasFiles)
        .def ("hasText", &DragAndDropData::hasText)
        .def ("hasUris", &DragAndDropData::hasUris)
        .def ("isEmpty", &DragAndDropData::isEmpty)
        .def ("__repr__", [] (const DragAndDropData& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " files=" << self.getFiles().size()
                << " uris=" << self.getUris().size()
                << " text=\"" << self.getText() << "\">";
            return result;
        });

    // ============================================================================================ yup::ComponentPaintMetrics

    py::class_<ComponentPaintMetrics> classComponentPaintMetrics (m, "ComponentPaintMetrics");

    classComponentPaintMetrics
        .def (py::init<>())
        .def_readwrite ("selfTicks", &ComponentPaintMetrics::selfTicks)
        .def_readwrite ("childrenTicks", &ComponentPaintMetrics::childrenTicks)
        .def_readwrite ("frameworkTicks", &ComponentPaintMetrics::frameworkTicks)
        .def_readwrite ("totalTicks", &ComponentPaintMetrics::totalTicks)
        .def_readwrite ("componentBounds", &ComponentPaintMetrics::componentBounds)
        .def_readwrite ("repaintArea", &ComponentPaintMetrics::repaintArea)
        .def_readwrite ("renderContinuous", &ComponentPaintMetrics::renderContinuous)
        .def_readwrite ("selfPaintSkipped", &ComponentPaintMetrics::selfPaintSkipped);

    // ============================================================================================ yup::ComponentListener

    py::class_<ComponentListener, PyComponentListener<>> classComponentListener (m, "ComponentListener");

    classComponentListener
        .def (py::init<>())
        .def ("componentMoved", &ComponentListener::componentMoved, "component"_a)
        .def ("componentResized", &ComponentListener::componentResized, "component"_a)
        .def ("componentBeingDeleted", &ComponentListener::componentBeingDeleted, "component"_a)
        .def ("componentPaintCompleted", &ComponentListener::componentPaintCompleted, "component"_a, "metrics"_a);

    // ============================================================================================ yup::ComponentEffect

    py::class_<ComponentEffect, PyComponentEffect<>, ReferenceCountedObjectPtr<ComponentEffect>> classComponentEffect (m, "ComponentEffect");

    classComponentEffect
        .def (py::init<>())
        .def ("apply", &ComponentEffect::apply, "g"_a, "inputTexture"_a, "bounds"_a,
              "Composites the effect. The component subtree has already been rendered into\n"
              "inputTexture, and the result must be drawn into g at bounds.");

    // ============================================================================================ yup::Component

    // Component derives from MouseListener in C++, so declaring the base here gives Python
    // issubclass (Component, MouseListener) and lets a component be used as a mouse listener.
    py::class_<Component, MouseListener, PyComponent<>> classComponent (m, "Component");

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
        .def ("getSafeAreaBounds", &Component::getSafeAreaBounds)

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
        .def ("setPaintProfilingDisabled", &Component::setPaintProfilingDisabled, "shouldBeDisabled"_a)
        .def ("isPaintProfilingDisabled", &Component::isPaintProfilingDisabled)
        .def ("refreshDisplay", &Component::refreshDisplay, "lastFrameTimeSeconds"_a)
        .def ("repaint", py::overload_cast<>(&Component::repaint))
        .def ("repaint", py::overload_cast<const Rectangle<float>&>(&Component::repaint))
        .def ("repaint", py::overload_cast<float, float, float, float>(&Component::repaint))

        // Native component
        .def ("getNativeHandle", &Component::getNativeHandle, py::return_value_policy::reference_internal)
        .def ("getNativeComponent", py::overload_cast<>(&Component::getNativeComponent), py::return_value_policy::reference_internal)
        .def ("isOnDesktop", &Component::isOnDesktop)
        .def ("addToDesktop", &Component::addToDesktop, "nativeOptions"_a, "parent"_a = nullptr, py::call_guard<py::gil_scoped_release>())
        .def ("removeFromDesktop", &Component::removeFromDesktop, py::call_guard<py::gil_scoped_release>())
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
        .def ("getWantsKeyboardFocus", &Component::getWantsKeyboardFocus)
        .def ("setClickingGrabFocus", &Component::setClickingGrabFocus)
        .def ("getClickingGrabFocus", &Component::getClickingGrabFocus)
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

        // Mouse events. The listener list only holds weak references, so the component keeps the
        // Python listener alive for as long as it is registered.
        .def ("setWantsMouseEvents", &Component::setWantsMouseEvents)
        .def ("doesWantSelfMouseEvents", &Component::doesWantSelfMouseEvents)
        .def ("doesWantChildrenMouseEvents", &Component::doesWantChildrenMouseEvents)
        .def ("addMouseListener", &Component::addMouseListener, "listener"_a, py::keep_alive<1, 2>())
        .def ("removeMouseListener", &Component::removeMouseListener, "listener"_a)

        // Drag and drop. The platform delivers the payload through these virtuals, so a Python
        // subclass overrides them; binding them keeps the entry points callable from Python too.
        .def ("isInterestedInDrag", &Component::isInterestedInDrag, "data"_a)
        .def ("itemsDropped", &Component::itemsDropped, "position"_a, "data"_a)
        .def ("itemDragEnter", &Component::itemDragEnter, "data"_a, "position"_a)
        .def ("itemDragMove", &Component::itemDragMove, "data"_a, "position"_a)
        .def ("itemDragExit", &Component::itemDragExit, "data"_a)

        // Component listeners. The listener list only holds weak references, so the component
        // keeps the Python listener alive for as long as it is registered.
        .def ("addComponentListener", &Component::addComponentListener, "listener"_a, py::keep_alive<1, 2>())
        .def ("removeComponentListener", &Component::removeComponentListener, "listener"_a)

        // Style system
        .def ("setStyle", &Component::setStyle)
        .def ("getStyle", &Component::getStyle)
        .def ("setColor", &Component::setColor)
        .def ("getColor", &Component::getColor)
        .def ("findColor", &Component::findColor)

        // Style metrics
        .def ("setMetric", &Component::setMetric, "metricId"_a, "metric"_a,
              "Overrides a themed metric on this component; pass None to remove the override.")
        .def ("getMetric", &Component::getMetric, "metricId"_a)
        .def ("findMetric", &Component::findMetric, "metricId"_a)

        // Cached to texture
        .def ("setCachedToTexture", &Component::setCachedToTexture, "shouldCache"_a)
        .def ("isCachedToTexture", &Component::isCachedToTexture)

        // Component effects
        .def ("setComponentEffect", &Component::setComponentEffect, "effect"_a,
              "Sets the effect applied after the component and its children are rendered; pass None to remove it.")
        .def ("getComponentEffect", &Component::getComponentEffect)

        // Snapshots
        .def ("snapshotToImage", &Component::snapshotToImage, "ctx"_a, "includeEffects"_a = true,
              "Renders the subtree offscreen and reads the pixels back. Returns an invalid Image\n"
              "when the context has no GPU or the component has no size.")
        .def ("snapshotToTexture", &Component::snapshotToTexture, "ctx"_a, "includeEffects"_a = true,
              "Like snapshotToImage, but returns the GPU texture without reading pixels back.")
    ;

    // ============================================================================================ yup::DocumentWindow

    py::class_<DocumentWindow, Component, PyDocumentWindow<>> classDocumentWindow (m, "DocumentWindow");

    classDocumentWindow
        .def (py::init<>())
        .def (py::init<const ComponentNative::Options&>())
        .def (py::init<const ComponentNative::Options&, const std::optional<Color>&>())
        .def ("centreWithSize", &DocumentWindow::centreWithSize)
    ;

    // ============================================================================================ yup::FlexItem

    py::class_<FlexItem> classFlexItem (m, "FlexItem");

    py::enum_<FlexItem::AlignSelf> (classFlexItem, "AlignSelf")
        .value ("autoAlign", FlexItem::AlignSelf::autoAlign)
        .value ("flexStart", FlexItem::AlignSelf::flexStart)
        .value ("flexEnd", FlexItem::AlignSelf::flexEnd)
        .value ("center", FlexItem::AlignSelf::center)
        .value ("stretch", FlexItem::AlignSelf::stretch)
        .value ("baseline", FlexItem::AlignSelf::baseline);

    classFlexItem
        .def (py::init<>())
        .def (py::init<Component&>(), "component"_a, py::keep_alive<1, 2>())
        .def (py::init<float, float>(), "width"_a, "height"_a)
        .def (py::init<Component&, float, float>(), "component"_a, "width"_a, "height"_a, py::keep_alive<1, 2>())

        // The FlexItem only stores a raw pointer, so the component must be kept alive by the
        // caller for as long as the item is used - see the note on FlexBox.performLayout.
        .def_property ("associatedComponent",
                       py::cpp_function ([] (const FlexItem& self)
                       {
                           return self.associatedComponent;
                       }, py::return_value_policy::reference),
                       py::cpp_function ([] (FlexItem& self, Component* newComponent)
                       {
                           self.associatedComponent = newComponent;
                       }, py::keep_alive<1, 2>()))

        .def_readwrite ("flexGrow", &FlexItem::flexGrow)
        .def_readwrite ("flexShrink", &FlexItem::flexShrink)
        .def_readwrite ("flexBasis", &FlexItem::flexBasis)
        .def_readwrite ("flexBasisPercent", &FlexItem::flexBasisPercent)
        .def_readwrite ("width", &FlexItem::width)
        .def_readwrite ("height", &FlexItem::height)
        .def_readwrite ("widthPercent", &FlexItem::widthPercent)
        .def_readwrite ("heightPercent", &FlexItem::heightPercent)
        .def_readwrite ("minWidth", &FlexItem::minWidth)
        .def_readwrite ("minHeight", &FlexItem::minHeight)
        .def_readwrite ("maxWidth", &FlexItem::maxWidth)
        .def_readwrite ("maxHeight", &FlexItem::maxHeight)
        .def_readwrite ("alignSelf", &FlexItem::alignSelf)
        .def_readwrite ("baseline", &FlexItem::baseline)
        .def_readwrite ("marginLeft", &FlexItem::marginLeft)
        .def_readwrite ("marginRight", &FlexItem::marginRight)
        .def_readwrite ("marginTop", &FlexItem::marginTop)
        .def_readwrite ("marginBottom", &FlexItem::marginBottom)
        .def_readwrite ("marginLeftAuto", &FlexItem::marginLeftAuto)
        .def_readwrite ("marginRightAuto", &FlexItem::marginRightAuto)
        .def_readwrite ("marginTopAuto", &FlexItem::marginTopAuto)
        .def_readwrite ("marginBottomAuto", &FlexItem::marginBottomAuto)
        .def_readwrite ("order", &FlexItem::order)

        .def ("withFlex", &FlexItem::withFlex, "flexGrow"_a)
        .def ("withFlexShrink", &FlexItem::withFlexShrink, "flexShrink"_a)
        .def ("withFlexBasis", &FlexItem::withFlexBasis, "flexBasis"_a)
        .def ("withFlexBasisPercent", &FlexItem::withFlexBasisPercent, "flexBasisPercent"_a)
        .def ("withWidth", &FlexItem::withWidth, "width"_a)
        .def ("withHeight", &FlexItem::withHeight, "height"_a)
        .def ("withWidthPercent", &FlexItem::withWidthPercent, "widthPercent"_a)
        .def ("withHeightPercent", &FlexItem::withHeightPercent, "heightPercent"_a)
        .def ("withMinWidth", &FlexItem::withMinWidth, "minWidth"_a)
        .def ("withMinHeight", &FlexItem::withMinHeight, "minHeight"_a)
        .def ("withMaxWidth", &FlexItem::withMaxWidth, "maxWidth"_a)
        .def ("withMaxHeight", &FlexItem::withMaxHeight, "maxHeight"_a)
        .def ("withMargin", &FlexItem::withMargin, "margin"_a)
        .def ("withAutoMargins", &FlexItem::withAutoMargins, "left"_a, "right"_a, "top"_a, "bottom"_a)
        .def ("withAlignSelf", &FlexItem::withAlignSelf, "alignSelf"_a)
        .def ("withBaseline", &FlexItem::withBaseline, "baseline"_a)
        .def ("withOrder", &FlexItem::withOrder, "order"_a)
    ;

    registerArray<Array, FlexItem> (m);

    // ============================================================================================ yup::FlexBox

    py::class_<FlexBox> classFlexBox (m, "FlexBox");

    py::enum_<FlexBox::Direction> (classFlexBox, "Direction")
        .value ("row", FlexBox::Direction::row)
        .value ("rowReverse", FlexBox::Direction::rowReverse)
        .value ("column", FlexBox::Direction::column)
        .value ("columnReverse", FlexBox::Direction::columnReverse);

    py::enum_<FlexBox::Wrap> (classFlexBox, "Wrap")
        .value ("noWrap", FlexBox::Wrap::noWrap)
        .value ("wrap", FlexBox::Wrap::wrap)
        .value ("wrapReverse", FlexBox::Wrap::wrapReverse);

    py::enum_<FlexBox::JustifyContent> (classFlexBox, "JustifyContent")
        .value ("flexStart", FlexBox::JustifyContent::flexStart)
        .value ("flexEnd", FlexBox::JustifyContent::flexEnd)
        .value ("center", FlexBox::JustifyContent::center)
        .value ("spaceBetween", FlexBox::JustifyContent::spaceBetween)
        .value ("spaceAround", FlexBox::JustifyContent::spaceAround)
        .value ("spaceEvenly", FlexBox::JustifyContent::spaceEvenly);

    py::enum_<FlexBox::AlignItems> (classFlexBox, "AlignItems")
        .value ("flexStart", FlexBox::AlignItems::flexStart)
        .value ("flexEnd", FlexBox::AlignItems::flexEnd)
        .value ("center", FlexBox::AlignItems::center)
        .value ("stretch", FlexBox::AlignItems::stretch)
        .value ("baseline", FlexBox::AlignItems::baseline);

    py::enum_<FlexBox::AlignContent> (classFlexBox, "AlignContent")
        .value ("flexStart", FlexBox::AlignContent::flexStart)
        .value ("flexEnd", FlexBox::AlignContent::flexEnd)
        .value ("center", FlexBox::AlignContent::center)
        .value ("spaceBetween", FlexBox::AlignContent::spaceBetween)
        .value ("spaceAround", FlexBox::AlignContent::spaceAround)
        .value ("spaceEvenly", FlexBox::AlignContent::spaceEvenly)
        .value ("stretch", FlexBox::AlignContent::stretch);

    classFlexBox
        .def (py::init<>())
        .def (py::init<FlexBox::Direction>(), "direction"_a)
        .def (py::init<FlexBox::Direction, FlexBox::Wrap, FlexBox::AlignItems, FlexBox::JustifyContent, FlexBox::AlignContent>(),
              "direction"_a, "wrap"_a, "alignItems"_a, "justifyContent"_a, "alignContent"_a)

        .def_readwrite ("flexDirection", &FlexBox::flexDirection)
        .def_readwrite ("flexWrap", &FlexBox::flexWrap)
        .def_readwrite ("alignItems", &FlexBox::alignItems)
        .def_readwrite ("justifyContent", &FlexBox::justifyContent)
        .def_readwrite ("alignContent", &FlexBox::alignContent)
        .def_readwrite ("gap", &FlexBox::gap)
        .def_readwrite ("rowGap", &FlexBox::rowGap)
        .def_readwrite ("columnGap", &FlexBox::columnGap)
        .def_readwrite ("paddingLeft", &FlexBox::paddingLeft)
        .def_readwrite ("paddingRight", &FlexBox::paddingRight)
        .def_readwrite ("paddingTop", &FlexBox::paddingTop)
        .def_readwrite ("paddingBottom", &FlexBox::paddingBottom)
        .def_readwrite ("items", &FlexBox::items)

        .def ("setPadding", py::overload_cast<float> (&FlexBox::setPadding), "padding"_a)
        .def ("setPadding", py::overload_cast<float, float> (&FlexBox::setPadding), "horizontal"_a, "vertical"_a)

        .def ("performLayout", py::overload_cast<Rectangle<float>> (&FlexBox::performLayout), "targetArea"_a,
              "Lays the items out inside the given area, calling setBounds on every associated component.\n\n"
              "The items only hold raw component pointers, so every component referenced by an item must\n"
              "be kept alive by the caller until this call returns.")
        .def ("performLayout", py::overload_cast<Rectangle<int>> (&FlexBox::performLayout), "targetArea"_a)
    ;

    // ============================================================================================ yup::GridItem

    py::class_<GridItem> classGridItem (m, "GridItem");

    py::enum_<GridItem::AlignSelf> (classGridItem, "AlignSelf")
        .value ("autoAlign", GridItem::AlignSelf::autoAlign)
        .value ("flexStart", GridItem::AlignSelf::flexStart)
        .value ("flexEnd", GridItem::AlignSelf::flexEnd)
        .value ("center", GridItem::AlignSelf::center)
        .value ("stretch", GridItem::AlignSelf::stretch)
        .value ("baseline", GridItem::AlignSelf::baseline);

    classGridItem
        .def (py::init<>())
        .def (py::init<Component&>(), "component"_a, py::keep_alive<1, 2>())

        .def_property_readonly_static ("autoPlace", [] (py::object)
        {
            return GridItem::autoPlace;
        })

        .def_property ("associatedComponent",
                       py::cpp_function ([] (const GridItem& self)
                       {
                           return self.associatedComponent;
                       }, py::return_value_policy::reference),
                       py::cpp_function ([] (GridItem& self, Component* newComponent)
                       {
                           self.associatedComponent = newComponent;
                       }, py::keep_alive<1, 2>()))

        .def_readwrite ("column", &GridItem::column)
        .def_readwrite ("row", &GridItem::row)
        .def_readwrite ("columnSpan", &GridItem::columnSpan)
        .def_readwrite ("rowSpan", &GridItem::rowSpan)
        .def_readwrite ("area", &GridItem::area)
        .def_readwrite ("columnStartName", &GridItem::columnStartName)
        .def_readwrite ("rowStartName", &GridItem::rowStartName)
        .def_readwrite ("width", &GridItem::width)
        .def_readwrite ("height", &GridItem::height)
        .def_readwrite ("widthPercent", &GridItem::widthPercent)
        .def_readwrite ("heightPercent", &GridItem::heightPercent)
        .def_readwrite ("minWidth", &GridItem::minWidth)
        .def_readwrite ("minHeight", &GridItem::minHeight)
        .def_readwrite ("maxWidth", &GridItem::maxWidth)
        .def_readwrite ("maxHeight", &GridItem::maxHeight)
        .def_readwrite ("justifySelf", &GridItem::justifySelf)
        .def_readwrite ("alignSelf", &GridItem::alignSelf)
        .def_readwrite ("marginLeft", &GridItem::marginLeft)
        .def_readwrite ("marginRight", &GridItem::marginRight)
        .def_readwrite ("marginTop", &GridItem::marginTop)
        .def_readwrite ("marginBottom", &GridItem::marginBottom)

        .def ("withColumn", &GridItem::withColumn, "column"_a)
        .def ("withRow", &GridItem::withRow, "row"_a)
        .def ("withColumnSpan", &GridItem::withColumnSpan, "span"_a)
        .def ("withRowSpan", &GridItem::withRowSpan, "span"_a)
        .def ("withMargin", &GridItem::withMargin, "margin"_a)
        .def ("withWidth", &GridItem::withWidth, "width"_a)
        .def ("withHeight", &GridItem::withHeight, "height"_a)
        .def ("withWidthPercent", &GridItem::withWidthPercent, "widthPercent"_a)
        .def ("withHeightPercent", &GridItem::withHeightPercent, "heightPercent"_a)
        .def ("withMinWidth", &GridItem::withMinWidth, "minWidth"_a)
        .def ("withMinHeight", &GridItem::withMinHeight, "minHeight"_a)
        .def ("withMaxWidth", &GridItem::withMaxWidth, "maxWidth"_a)
        .def ("withMaxHeight", &GridItem::withMaxHeight, "maxHeight"_a)
        .def ("withJustifySelf", &GridItem::withJustifySelf, "justifySelf"_a)
        .def ("withAlignSelf", &GridItem::withAlignSelf, "alignSelf"_a)
        .def ("withArea", &GridItem::withArea, "areaName"_a)
        .def ("withColumnStart", &GridItem::withColumnStart, "lineName"_a)
        .def ("withRowStart", &GridItem::withRowStart, "lineName"_a)
    ;

    registerArray<Array, GridItem> (m);

    // ============================================================================================ yup::Grid

    py::class_<Grid> classGrid (m, "Grid");
    py::class_<Grid::TrackInfo> classGridTrackInfo (classGrid, "TrackInfo");

    py::enum_<Grid::TrackInfo::SizeType> (classGridTrackInfo, "SizeType")
        .value ("pixels", Grid::TrackInfo::SizeType::pixels)
        .value ("percent", Grid::TrackInfo::SizeType::percent)
        .value ("fraction", Grid::TrackInfo::SizeType::fraction)
        .value ("autoSize", Grid::TrackInfo::SizeType::autoSize);

    py::class_<Grid::TrackInfo::SizingFunction> (classGridTrackInfo, "SizingFunction")
        .def (py::init<>())
        .def_readwrite ("type", &Grid::TrackInfo::SizingFunction::type)
        .def_readwrite ("value", &Grid::TrackInfo::SizingFunction::value);

    // TrackInfo is deliberately factory-only - its default constructor is private, so there is
    // no py::init here either and Python can only build one through px/percent/fr/auto_/minmax.
    classGridTrackInfo
        .def_static ("px", &Grid::TrackInfo::px, "pixelSize"_a)
        .def_static ("percent", &Grid::TrackInfo::percent, "percentage"_a)
        .def_static ("fr", &Grid::TrackInfo::fr, "fraction"_a)
        .def_static ("auto_", &Grid::TrackInfo::auto_)
        .def_static ("minmax", &Grid::TrackInfo::minmax, "minimum"_a, "maximum"_a)
        .def_static ("fitContent", &Grid::TrackInfo::fitContent, "maximumSize"_a)
        .def_readwrite ("minimum", &Grid::TrackInfo::minimum)
        .def_readwrite ("maximum", &Grid::TrackInfo::maximum)
        .def ("isFractional", &Grid::TrackInfo::isFractional)
    ;

    // Hand-rolled rather than registerArray, because that binds resize/fill/set, all of which
    // need a public default constructor for the element type - which TrackInfo does not have.
    using TrackInfoArray = Array<Grid::TrackInfo>;

    py::class_<TrackInfoArray> (m, "ArrayTrackInfo")
        .def (py::init<>())
        .def ("size", &TrackInfoArray::size)
        .def ("isEmpty", &TrackInfoArray::isEmpty)
        .def ("clear", &TrackInfoArray::clear)
        .def ("clearQuick", &TrackInfoArray::clearQuick)
        .def ("__len__", &TrackInfoArray::size)
        .def ("__getitem__", [] (const TrackInfoArray& self, int index)
        {
            if (! isPositiveAndBelow (index, self.size()))
                throw py::index_error();

            return self.getUnchecked (index);
        }, "index"_a)
        .def ("__iter__", [] (TrackInfoArray& self)
        {
            return py::make_iterator (self.begin(), self.end());
        }, py::keep_alive<0, 1>())
        .def ("add", [] (TrackInfoArray& self, const Grid::TrackInfo& track)
        {
            self.add (track);
        }, "track"_a)
        .def ("addArray", [] (TrackInfoArray& self, const TrackInfoArray& other)
        {
            for (const auto& track : other)
                self.add (track);
        }, "other"_a)
        .def ("remove", [] (TrackInfoArray& self, int index)
        {
            self.remove (index);
        }, "index"_a)
    ;

    py::enum_<Grid::AlignItems> (classGrid, "AlignItems")
        .value ("flexStart", Grid::AlignItems::flexStart)
        .value ("flexEnd", Grid::AlignItems::flexEnd)
        .value ("center", Grid::AlignItems::center)
        .value ("stretch", Grid::AlignItems::stretch)
        .value ("baseline", Grid::AlignItems::baseline);

    py::enum_<Grid::AlignContent> (classGrid, "AlignContent")
        .value ("flexStart", Grid::AlignContent::flexStart)
        .value ("flexEnd", Grid::AlignContent::flexEnd)
        .value ("center", Grid::AlignContent::center)
        .value ("spaceBetween", Grid::AlignContent::spaceBetween)
        .value ("spaceAround", Grid::AlignContent::spaceAround)
        .value ("spaceEvenly", Grid::AlignContent::spaceEvenly);

    py::enum_<Grid::AutoFlow> (classGrid, "AutoFlow")
        .value ("row", Grid::AutoFlow::row)
        .value ("column", Grid::AutoFlow::column)
        .value ("rowDense", Grid::AutoFlow::rowDense)
        .value ("columnDense", Grid::AutoFlow::columnDense);

    classGrid
        .def (py::init<>())

        .def_static ("repeat", &Grid::repeat, "count"_a, "track"_a)
        .def_static ("repeatToFill", &Grid::repeatToFill, "track"_a, "availableSize"_a, "gap"_a, "defaultSize"_a)

        .def_readwrite ("templateColumns", &Grid::templateColumns)
        .def_readwrite ("templateRows", &Grid::templateRows)
        .def_readwrite ("autoRows", &Grid::autoRows)
        .def_readwrite ("autoColumns", &Grid::autoColumns)
        .def_readwrite ("gap", &Grid::gap)
        .def_readwrite ("columnGap", &Grid::columnGap)
        .def_readwrite ("rowGap", &Grid::rowGap)
        .def_readwrite ("justifyItems", &Grid::justifyItems)
        .def_readwrite ("alignItems", &Grid::alignItems)
        .def_readwrite ("justifyContent", &Grid::justifyContent)
        .def_readwrite ("alignContent", &Grid::alignContent)
        .def_readwrite ("autoFlow", &Grid::autoFlow)
        .def_readwrite ("items", &Grid::items)

        .def ("setTemplateAreas", &Grid::setTemplateAreas, "rowPatterns"_a)
        .def ("setColumnLineName", &Grid::setColumnLineName, "lineIndex"_a, "name"_a)
        .def ("setRowLineName", &Grid::setRowLineName, "lineIndex"_a, "name"_a)

        .def ("performLayout", py::overload_cast<Rectangle<float>> (&Grid::performLayout), "targetArea"_a,
              "Lays the items out inside the given area, calling setBounds on every associated component.\n\n"
              "The items only hold raw component pointers, so every component referenced by an item must\n"
              "be kept alive by the caller until this call returns.")
        .def ("performLayout", py::overload_cast<Rectangle<int>> (&Grid::performLayout), "targetArea"_a)
    ;

    // ============================================================================================ yup::Button

    py::class_<Button, Component, PyButton<>> (m, "Button")
        .def (py::init<StringRef>(), "componentID"_a = StringRef())
        .def ("isButtonOver", &Button::isButtonOver)
        .def ("isButtonDown", &Button::isButtonDown)
        .def_readwrite ("onClick", &Button::onClick)
        .def ("paintButton", &Button::paintButton);

    // ============================================================================================ yup::TextButton

    py::class_<TextButton, Button, PyButton<TextButton>> (m, "TextButton")
        .def (py::init<StringRef>(), "componentID"_a = StringRef())
        .def ("getButtonText", &TextButton::getButtonText)
        .def ("setButtonText", &TextButton::setButtonText);

    // ============================================================================================ yup::ToggleButton

    py::class_<ToggleButton, Button, PyButton<ToggleButton>> (m, "ToggleButton")
        .def (py::init<StringRef>(), "componentID"_a = StringRef())
        .def ("getToggleState", &ToggleButton::getToggleState)
        .def ("setToggleState", &ToggleButton::setToggleState,
              "shouldBeToggled"_a, "notification"_a = sendNotification)
        .def ("getButtonText", &ToggleButton::getButtonText)
        .def ("setButtonText", &ToggleButton::setButtonText);

    // ============================================================================================ yup::Slider::SliderType

    py::enum_<Slider::SliderType> (m, "SliderType")
        .value ("LinearHorizontal", Slider::LinearHorizontal)
        .value ("LinearVertical", Slider::LinearVertical)
        .value ("LinearBarHorizontal", Slider::LinearBarHorizontal)
        .value ("LinearBarVertical", Slider::LinearBarVertical)
        .value ("Rotary", Slider::Rotary)
        .value ("RotaryHorizontalDrag", Slider::RotaryHorizontalDrag)
        .value ("RotaryVerticalDrag", Slider::RotaryVerticalDrag)
        .value ("IncDecButtons", Slider::IncDecButtons)
        .value ("TwoValueHorizontal", Slider::TwoValueHorizontal)
        .value ("TwoValueVertical", Slider::TwoValueVertical)
        .value ("ThreeValueHorizontal", Slider::ThreeValueHorizontal)
        .value ("ThreeValueVertical", Slider::ThreeValueVertical)
        .export_values();

    // Make SliderType accessible as Slider.SliderType via the class
    py::class_<Slider, Component, PySlider<>> (m, "Slider")
        .def (py::init<Slider::SliderType, StringRef>(),
              "sliderType"_a, "componentID"_a = StringRef())
        .def (py::init<Slider::SliderType>(),
              "sliderType"_a)
        .def ("setValue", &Slider::setValue,
              "newValue"_a, "notification"_a = sendNotification)
        .def ("getValue", &Slider::getValue)
        .def ("setValueNormalised", &Slider::setValueNormalised,
              "newValue"_a, "notification"_a = sendNotification)
        .def ("getValueNormalised", &Slider::getValueNormalised)
        .def ("setMinValue", &Slider::setMinValue,
              "newMinValue"_a, "notification"_a = sendNotification, "allowNudgingOfOtherValues"_a = false)
        .def ("getMinValue", &Slider::getMinValue)
        .def ("setMaxValue", &Slider::setMaxValue,
              "newMaxValue"_a, "notification"_a = sendNotification, "allowNudgingOfOtherValues"_a = false)
        .def ("getMaxValue", &Slider::getMaxValue)
        .def ("setRange", py::overload_cast<double, double, double> (&Slider::setRange),
              "minValue"_a, "maxValue"_a, "stepSize"_a = 0.0)
        .def ("setSkewFactor", &Slider::setSkewFactor)
        .def ("setSkewFactorFromMidpoint", &Slider::setSkewFactorFromMidpoint)
        .def ("setNumDecimalPlacesToDisplay", &Slider::setNumDecimalPlacesToDisplay)
        .def ("setTextBoxStyle", &Slider::setTextBoxStyle,
              "position"_a, "isReadOnly"_a = false, "textEntryBoxWidth"_a = 80, "textEntryBoxHeight"_a = 20)
        .def_readwrite ("onValueChanged", &Slider::onValueChanged)
        .def_readwrite ("onMinValueChanged", &Slider::onMinValueChanged)
        .def_readwrite ("onMaxValueChanged", &Slider::onMaxValueChanged)
        .def ("__repr__", [] (const Slider& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " value=" << self.getValue() << ">";
            return result;
        });

    // ============================================================================================ yup::Slider::TextEntryBoxPosition

    py::enum_<Slider::TextEntryBoxPosition> (m, "TextEntryBoxPosition")
        .value ("NoTextBox", Slider::NoTextBox)
        .value ("TextBoxLeft", Slider::TextBoxLeft)
        .value ("TextBoxRight", Slider::TextBoxRight)
        .value ("TextBoxAbove", Slider::TextBoxAbove)
        .value ("TextBoxBelow", Slider::TextBoxBelow)
        .export_values();

    // ============================================================================================ yup::Label

    py::class_<Label, Component, PyComponent<Label>> labelClass (m, "Label");

    labelClass
        .def (py::init<StringRef>(), "componentID"_a = StringRef())
        .def ("getText", &Label::getText)
        .def ("setText", &Label::setText,
              "newText"_a, "notification"_a = sendNotification)
        .def ("__repr__", [] (const Label& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " text=\"" << self.getText() << "\">";
            return result;
        });

    // The nested C++ Style struct, so Python can name the same identifiers the widget's theme
    // style reads, e.g. label.setColor (Label.Style.backgroundColorId, color). The struct only
    // holds static members, so it is intentionally not constructible from Python.
    py::class_<Label::Style> labelStyle (labelClass, "Style");
    labelStyle.attr ("textFillColorId") = Label::Style::textFillColorId;
    labelStyle.attr ("textStrokeColorId") = Label::Style::textStrokeColorId;
    labelStyle.attr ("backgroundColorId") = Label::Style::backgroundColorId;
    labelStyle.attr ("outlineColorId") = Label::Style::outlineColorId;
    labelStyle.attr ("textHeightProportionMetricId") = Label::Style::textHeightProportionMetricId;

    // =================================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER
    m.def ("START_YUP_APPLICATION", [] (py::handle applicationType, bool catchExceptionsAndContinue)
    {
        if (! applicationType)
            throw py::value_error ("Argument must be a YUPApplication subclass");

        auto& options = globalOptions();
        options.catchExceptionsAndContinue = catchExceptionsAndContinue;
        options.caughtKeyboardInterrupt = false;

#if YUP_MAC
        Process::setDockIconVisible (true);
#endif

        py::scoped_ostream_redirect output;

        auto sys = py::module_::import ("sys");

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

        auto pyApplication = applicationType();

        auto* application = pyApplication.cast<YUPApplication*>();
        if (application == nullptr)
            throw py::value_error ("Argument must be a YUPApplication subclass");

        int returnValue = 255;
        std::optional<py::error_already_set> pendingInterrupt;

        {
            PythonSignalCheckTimer signalCheckTimer;

            {
                py::gil_scoped_release release;

                YUP_TRY
                {
                    if (application->initialiseApp())
                    {
                        signalCheckTimer.startTimer (jmax (1, options.messageManagerGranularityMilliseconds.load()));

                        MessageManager::getInstance()->runDispatchLoop();
                    }
                }
                YUP_CATCH_EXCEPTION

                signalCheckTimer.stopTimer();

                returnValue = application->shutdownApp();
            }

            pendingInterrupt = std::move (signalCheckTimer.pendingInterrupt);
        }

        if (pendingInterrupt.has_value())
            throw *pendingInterrupt;

        if (options.caughtKeyboardInterrupt)
        {
            PyErr_SetNone (PyExc_KeyboardInterrupt);
            throw py::error_already_set();
        }

        sys.attr ("exit") (returnValue);
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

                previousCatchExceptionsAndContinue = globalOptions().catchExceptionsAndContinue;
                globalOptions().catchExceptionsAndContinue = true;

#if ! YUP_WINDOWS
                for (auto arg : py::module_::import ("sys").attr ("argv"))
                    arguments.add (arg.cast<String>());

                for (const auto& arg : arguments)
                    argv.add (arg.toRawUTF8());

                yup_argv = argv.getRawDataPointer();
                yup_argc = argv.size();
#endif

                pyApplication = applicationType();

                auto* application = pyApplication.cast<YUPApplication*>();
                if (application == nullptr)
                    return;

                if (! application->initialiseApp())
                    return;

                initialisedApp = true;
            }

            ~Scope()
            {
                if (initialisedApp)
                {
                    if (auto* application = pyApplication.cast<YUPApplication*>())
                        application->shutdownApp();
                }

                globalOptions().catchExceptionsAndContinue = previousCatchExceptionsAndContinue;
            }

        private:
#if ! YUP_WINDOWS
            StringArray arguments;
            Array<const char*> argv;
#endif
            py::object pyApplication;
            bool initialisedApp = false;
            bool previousCatchExceptionsAndContinue = false;
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

    // ============================================================================================ yup::KeyModifiers

    py::class_<KeyModifiers> classKeyModifiers (m, "KeyModifiers");

    classKeyModifiers
        .def (py::init<>())
        .def (py::init<int>(), "modifiers"_a)
        .def ("isShiftDown", &KeyModifiers::isShiftDown)
        .def ("isControlDown", &KeyModifiers::isControlDown)
        .def ("isCommandDown", &KeyModifiers::isCommandDown)
        .def ("isAltDown", &KeyModifiers::isAltDown);

    // ============================================================================================ yup::KeyPress

    py::class_<KeyPress> classKeyPress (m, "KeyPress");

    classKeyPress
        .def (py::init<>())
        .def (py::init<int>(), "key"_a)
        .def (py::init<int, KeyModifiers>(), "key"_a, "modifiers"_a)
        .def (py::init ([] (int key, KeyModifiers modifiers, uint32 scancode)
        {
            return KeyPress (key, modifiers, static_cast<char32_t> (scancode));
        }), "key"_a, "modifiers"_a, "scancode"_a)
        .def ("getKey", &KeyPress::getKey)
        .def ("getModifiers", &KeyPress::getModifiers)
        .def ("getTextCharacter", [] (const KeyPress& self)
        {
            return static_cast<uint32> (self.getTextCharacter());
        })
        .def (py::self == py::self)
        .def (py::self != py::self);

    // ============================================================================================ yup::MouseWheelData

    py::class_<MouseWheelData> classMouseWheelData (m, "MouseWheelData");

    classMouseWheelData
        .def (py::init<>())
        .def (py::init<float, float>(), "deltaX"_a, "deltaY"_a)
        .def ("getDeltaX", &MouseWheelData::getDeltaX)
        .def ("getDeltaY", &MouseWheelData::getDeltaY)
        .def ("setDeltaX", &MouseWheelData::setDeltaX, "deltaX"_a, py::return_value_policy::reference_internal)
        .def ("setDeltaY", &MouseWheelData::setDeltaY, "deltaY"_a, py::return_value_policy::reference_internal)
        .def ("withDeltaX", &MouseWheelData::withDeltaX, "deltaX"_a)
        .def ("withDeltaY", &MouseWheelData::withDeltaY, "deltaY"_a)
        .def (py::self == py::self)
        .def (py::self != py::self);

    // ============================================================================================ yup::ProgressBar

    py::class_<ProgressBar, Component, PyProgressBar<>> classProgressBar (m, "ProgressBar");

    classProgressBar
        .def (py::init<StringRef>(), "componentID"_a = StringRef())
        .def ("setProgress", &ProgressBar::setProgress,
              "newProgress"_a, "notification"_a = NotificationType::sendNotificationAsync)
        .def ("getProgress", &ProgressBar::getProgress)
        .def ("isIndeterminate", &ProgressBar::isIndeterminate)
        .def ("progressChanged", &ProgressBar::progressChanged)
        .def_readwrite ("onProgressChanged", &ProgressBar::onProgressChanged);

    // ============================================================================================ yup::SwitchButton

    py::class_<SwitchButton, Button, PySwitchButton<>> classSwitchButton (m, "SwitchButton");

    classSwitchButton
        .def (py::init<StringRef, bool>(), "componentID"_a = StringRef(), "isVertical"_a = false)
        .def ("getToggleState", &SwitchButton::getToggleState)
        .def ("setToggleState", &SwitchButton::setToggleState,
              "shouldBeToggled"_a, "notification"_a = NotificationType::sendNotification)
        .def ("setVertical", &SwitchButton::setVertical, "shouldBeVertical"_a)
        .def ("isVertical", &SwitchButton::isVertical)
        .def ("setMillisecondsToSpendMoving", &SwitchButton::setMillisecondsToSpendMoving, "newValue"_a)
        .def ("toggleStateChanged", &SwitchButton::toggleStateChanged);

    // ============================================================================================ yup::ScrollBar

    py::class_<ScrollBar, Component, PyComponent<ScrollBar>> classScrollBar (m, "ScrollBar");

    py::enum_<ScrollBar::Orientation> (classScrollBar, "Orientation")
        .value ("vertical", ScrollBar::Orientation::vertical)
        .value ("horizontal", ScrollBar::Orientation::horizontal)
        .export_values();

    py::enum_<ScrollBar::VisibilityMode> (classScrollBar, "VisibilityMode")
        .value ("alwaysVisible", ScrollBar::VisibilityMode::alwaysVisible)
        .value ("autoHide", ScrollBar::VisibilityMode::autoHide)
        .value ("alwaysHidden", ScrollBar::VisibilityMode::alwaysHidden)
        .export_values();

    classScrollBar
        .def (py::init<ScrollBar::Orientation>(), "orientation"_a = ScrollBar::Orientation::vertical)

        .def ("setOrientation", &ScrollBar::setOrientation, "newOrientation"_a)
        .def ("getOrientation", &ScrollBar::getOrientation)
        .def ("setVisibilityMode", &ScrollBar::setVisibilityMode, "mode"_a)
        .def ("getVisibilityMode", &ScrollBar::getVisibilityMode)

        .def ("setRangeLimits", &ScrollBar::setRangeLimits, "minimum"_a, "maximum"_a)
        .def ("getRangeMinimum", &ScrollBar::getRangeMinimum)
        .def ("getRangeMaximum", &ScrollBar::getRangeMaximum)

        .def ("setCurrentRange", &ScrollBar::setCurrentRange, "start"_a, "end"_a)
        .def ("getCurrentRangeStart", &ScrollBar::getCurrentRangeStart)
        .def ("getCurrentRangeEnd", &ScrollBar::getCurrentRangeEnd)
        .def ("getCurrentRangeSize", &ScrollBar::getCurrentRangeSize)
        .def ("setCurrentRangeStart", &ScrollBar::setCurrentRangeStart,
              "newPosition"_a, "notification"_a = NotificationType::sendNotification)
        .def ("scrollBy", &ScrollBar::scrollBy, "delta"_a, "notification"_a = NotificationType::sendNotification)

        .def ("setAutoHide", &ScrollBar::setAutoHide, "shouldAutoHide"_a)
        .def ("isAutoHide", &ScrollBar::isAutoHide)
        .def ("isScrollingNeeded", &ScrollBar::isScrollingNeeded)

        .def ("setScrollBarWidth", &ScrollBar::setScrollBarWidth, "newSize"_a)
        .def ("getScrollBarWidth", &ScrollBar::getScrollBarWidth)

        .def ("isDragging", &ScrollBar::isDragging)
        // ScrollBar also declares a private isThumbHovered (Point<float>) helper, so the
        // no-argument getter has to be selected explicitly - and being const, with py::const_.
        .def ("isThumbHovered", py::overload_cast<> (&ScrollBar::isThumbHovered, py::const_))
        .def ("getThumbBoundsForRendering", &ScrollBar::getThumbBoundsForRendering)
        .def ("getTrackBoundsForRendering", &ScrollBar::getTrackBoundsForRendering)

        .def_readwrite ("onScrollPositionChanged", &ScrollBar::onScrollPositionChanged,
                        "Called with the new scroll position when it changes.");

    // The nested Style struct only holds static members, so it is exposed for its ids only -
    // the same shape as Label.Style, so Python can name what the theme looks up.
    py::class_<ScrollBar::Style> scrollBarStyle (classScrollBar, "Style");
    scrollBarStyle.attr ("trackColorId") = ScrollBar::Style::trackColorId;
    scrollBarStyle.attr ("thumbColorId") = ScrollBar::Style::thumbColorId;
    scrollBarStyle.attr ("thumbHoverColorId") = ScrollBar::Style::thumbHoverColorId;
    scrollBarStyle.attr ("thumbDraggingColorId") = ScrollBar::Style::thumbDraggingColorId;

    // ============================================================================================ yup::ListBoxModel

    py::class_<ListBoxModel, PyListBoxModel<>> classListBoxModel (m, "ListBoxModel");

    classListBoxModel
        .def (py::init<>())
        .def ("getNumRows", &ListBoxModel::getNumRows,
              "Returns the number of rows in the list; a Python subclass must override it.")
        .def ("getRowHeight", &ListBoxModel::getRowHeight, "rowIndex"_a,
              "Returns 0 to use the ListBox's fixed row height.")
        .def ("getRowWidth", &ListBoxModel::getRowWidth, "rowIndex"_a,
              "Returns 0 to use the ListBox's fixed row width.")
        .def ("paintListBoxItem", &ListBoxModel::paintListBoxItem, "rowIndex"_a, "g"_a, "area"_a, "isSelected"_a)
        .def ("getRowText", &ListBoxModel::getRowText, "rowIndex"_a)
        .def ("getRowIcon", &ListBoxModel::getRowIcon, "rowIndex"_a)
        .def ("selectedRowsChanged", &ListBoxModel::selectedRowsChanged, "selectedRows"_a)
        .def ("rowClicked", &ListBoxModel::rowClicked, "rowIndex"_a, "event"_a)
        .def ("rowDoubleClicked", &ListBoxModel::rowDoubleClicked, "rowIndex"_a, "event"_a)
        .def ("returnKeyPressed", &ListBoxModel::returnKeyPressed, "lastSelectedRow"_a)
        .def ("deleteKeyPressed", &ListBoxModel::deleteKeyPressed, "selectedRows"_a)
        .def ("getDragSourceDescription", &ListBoxModel::getDragSourceDescription, "selectedRows"_a);

    // ============================================================================================ yup::ListBoxItem

    py::class_<ListBoxItem, Component, PyComponent<ListBoxItem>> classListBoxItem (m, "ListBoxItem");

    py::enum_<ListBoxItem::IconPosition> (classListBoxItem, "IconPosition")
        .value ("left", ListBoxItem::IconPosition::left)
        .value ("right", ListBoxItem::IconPosition::right)
        .value ("above", ListBoxItem::IconPosition::above)
        .value ("below", ListBoxItem::IconPosition::below)
        .export_values();

    // setIconDrawable/getIconDrawable are not bound: they traffic in std::shared_ptr<Drawable>,
    // while Drawable is registered with the default unique_ptr holder, so pybind11 cannot
    // convert either way. setIcon takes an Image and covers the same ground.
    classListBoxItem
        .def (py::init<>())
        .def ("setText", &ListBoxItem::setText, "newText"_a)
        .def ("getText", &ListBoxItem::getText)
        .def ("setIcon", &ListBoxItem::setIcon, "newIcon"_a)
        .def ("setIconPosition", &ListBoxItem::setIconPosition, "position"_a)
        .def ("getIconPosition", &ListBoxItem::getIconPosition)
        .def ("setSelected", &ListBoxItem::setSelected, "shouldBeSelected"_a)
        .def ("isSelected", &ListBoxItem::isSelected)
        .def ("setHovered", &ListBoxItem::setHovered, "shouldBeHovered"_a)
        .def ("isHovered", &ListBoxItem::isHovered)
        .def ("getTextBoundsForRendering", &ListBoxItem::getTextBoundsForRendering)
        .def ("getIconBoundsForRendering", &ListBoxItem::getIconBoundsForRendering);

    py::class_<ListBoxItem::Style> listBoxItemStyle (classListBoxItem, "Style");
    listBoxItemStyle.attr ("textColorId") = ListBoxItem::Style::textColorId;
    listBoxItemStyle.attr ("textColorSelectedId") = ListBoxItem::Style::textColorSelectedId;
    listBoxItemStyle.attr ("backgroundColorId") = ListBoxItem::Style::backgroundColorId;
    listBoxItemStyle.attr ("backgroundColorSelectedId") = ListBoxItem::Style::backgroundColorSelectedId;
    listBoxItemStyle.attr ("backgroundColorHoveredId") = ListBoxItem::Style::backgroundColorHoveredId;

    // ============================================================================================ yup::ListBox

    py::class_<ListBox, Component, PyComponent<ListBox>> classListBox (m, "ListBox");

    py::enum_<ListBox::Orientation> (classListBox, "Orientation")
        .value ("vertical", ListBox::Orientation::vertical)
        .value ("horizontal", ListBox::Orientation::horizontal)
        .export_values();

    py::enum_<ListBox::SelectionMode> (classListBox, "SelectionMode")
        .value ("none", ListBox::SelectionMode::none)
        .value ("single", ListBox::SelectionMode::single)
        .value ("multiple", ListBox::SelectionMode::multiple)
        .export_values();

    classListBox
        .def (py::init<StringRef, ListBox::Orientation>(),
              "componentID"_a = StringRef(), "orientation"_a = ListBox::Orientation::vertical)

        // The ListBox never owns its model and only holds it while it is set, so the model has to
        // outlive the ListBox - which is what keep_alive pins down for a Python model.
        .def ("setModel", &ListBox::setModel, "newModel"_a, py::keep_alive<1, 2>())
        .def ("getModel", &ListBox::getModel, py::return_value_policy::reference)

        .def ("setSelectionMode", &ListBox::setSelectionMode, "mode"_a)
        .def ("getSelectionMode", &ListBox::getSelectionMode)

        .def ("getSelectedRow", &ListBox::getSelectedRow)
        .def ("selectRow", &ListBox::selectRow,
              "rowIndex"_a, "scrollToShowRow"_a = true, "notification"_a = NotificationType::sendNotification)
        .def ("deselectRow", &ListBox::deselectRow, "rowIndex"_a, "notification"_a = NotificationType::sendNotification)
        .def ("deselectAllRows", &ListBox::deselectAllRows, "notification"_a = NotificationType::sendNotification)
        .def ("getSelectedRows", &ListBox::getSelectedRows)
        .def ("setSelectedRows", &ListBox::setSelectedRows,
              "rows"_a, "notification"_a = NotificationType::sendNotification)
        .def ("isRowSelected", &ListBox::isRowSelected, "rowIndex"_a)
        .def ("getNumSelectedRows", &ListBox::getNumSelectedRows)

        .def ("updateContent", &ListBox::updateContent)
        .def ("repaintRow", &ListBox::repaintRow, "rowIndex"_a)
        .def ("scrollToEnsureRowIsVisible", &ListBox::scrollToEnsureRowIsVisible, "rowIndex"_a)

        .def ("setOrientation", &ListBox::setOrientation, "newOrientation"_a)
        .def ("getOrientation", &ListBox::getOrientation)
        .def ("setRowHeight", &ListBox::setRowHeight, "newHeight"_a)
        .def ("setRowWidth", &ListBox::setRowWidth, "newWidth"_a)
        .def ("getRowHeight", &ListBox::getRowHeight)
        .def ("getRowWidth", &ListBox::getRowWidth)
        .def ("setVariableHeightEnabled", &ListBox::setVariableHeightEnabled, "enabled"_a)
        .def ("setVariableWidthEnabled", &ListBox::setVariableWidthEnabled, "enabled"_a)
        .def ("isVariableHeightEnabled", &ListBox::isVariableHeightEnabled)
        .def ("isVariableWidthEnabled", &ListBox::isVariableWidthEnabled)
        .def ("setMinimumContentSize", &ListBox::setMinimumContentSize, "minSize"_a)
        .def ("getMinimumContentSize", &ListBox::getMinimumContentSize)

        .def ("setVerticalScrollBarVisibility", &ListBox::setVerticalScrollBarVisibility, "mode"_a)
        .def ("setHorizontalScrollBarVisibility", &ListBox::setHorizontalScrollBarVisibility, "mode"_a)
        .def ("getVerticalScrollBar", &ListBox::getVerticalScrollBar, py::return_value_policy::reference_internal)
        .def ("getHorizontalScrollBar", &ListBox::getHorizontalScrollBar, py::return_value_policy::reference_internal)

        .def ("getVisibleRowsCount", &ListBox::getVisibleRowsCount)
        .def ("getVisibleRowRange", &ListBox::getVisibleRowRange)
        .def ("getRowAt", &ListBox::getRowAt, "position"_a)
        .def ("getComponentForRow", &ListBox::getComponentForRow, "rowIndex"_a, py::return_value_policy::reference_internal)
        .def ("getRowBounds", &ListBox::getRowBounds, "rowIndex"_a)

        .def_readwrite ("onRowClicked", &ListBox::onRowClicked, "Called with the row the user clicked.")
        .def_readwrite ("onRowDoubleClicked", &ListBox::onRowDoubleClicked, "Called with the row the user double-clicked.")
        .def_readwrite ("onSelectionChanged", &ListBox::onSelectionChanged, "Called when the selected rows change.");

    py::class_<ListBox::Style> listBoxStyle (classListBox, "Style");
    listBoxStyle.attr ("backgroundColorId") = ListBox::Style::backgroundColorId;
    listBoxStyle.attr ("outlineColorId") = ListBox::Style::outlineColorId;
    listBoxStyle.attr ("rowBackgroundColorId") = ListBox::Style::rowBackgroundColorId;
    listBoxStyle.attr ("selectedRowBackgroundColorId") = ListBox::Style::selectedRowBackgroundColorId;
    listBoxStyle.attr ("hoveredRowBackgroundColorId") = ListBox::Style::hoveredRowBackgroundColorId;

    // ============================================================================================ yup::ComboBox

    py::class_<ComboBox, Component, PyComboBox<>> classComboBox (m, "ComboBox");

    classComboBox
        .def (py::init<StringRef>(), "componentID"_a = StringRef())

        .def ("addItem", &ComboBox::addItem, "newItemText"_a, "newItemId"_a)
        .def ("addItemList", &ComboBox::addItemList, "itemsToAdd"_a, "firstItemId"_a)
        .def ("addSeparator", &ComboBox::addSeparator)
        .def ("clear", &ComboBox::clear)

        .def ("getNumItems", &ComboBox::getNumItems)
        .def ("getItemText", &ComboBox::getItemText, "index"_a)
        .def ("getItemId", &ComboBox::getItemId, "index"_a)
        .def ("changeItemText", &ComboBox::changeItemText, "index"_a, "newText"_a)

        .def ("getSelectedItemIndex", &ComboBox::getSelectedItemIndex)
        .def ("getSelectedId", &ComboBox::getSelectedId)
        .def ("getText", &ComboBox::getText)
        .def ("setSelectedItemIndex", &ComboBox::setSelectedItemIndex,
              "newItemIndex"_a, "notification"_a = NotificationType::sendNotification)
        .def ("setSelectedId", &ComboBox::setSelectedId,
              "newItemId"_a, "notification"_a = NotificationType::sendNotification)
        .def ("setTextWhenNothingSelected", &ComboBox::setTextWhenNothingSelected, "newPlaceholderText"_a)
        .def ("getTextWhenNothingSelected", &ComboBox::getTextWhenNothingSelected)

        .def ("setEditableText", &ComboBox::setEditableText, "isEditable"_a)
        .def ("isTextEditable", &ComboBox::isTextEditable)
        .def ("isPopupShown", &ComboBox::isPopupShown)

        .def ("selectedItemChanged", &ComboBox::selectedItemChanged,
              "Called when the selected item changes; a Python subclass overrides it.")
        .def_readwrite ("onSelectedItemChanged", &ComboBox::onSelectedItemChanged);

    py::class_<ComboBox::Style> comboBoxStyle (classComboBox, "Style");
    comboBoxStyle.attr ("backgroundColorId") = ComboBox::Style::backgroundColorId;
    comboBoxStyle.attr ("textColorId") = ComboBox::Style::textColorId;
    comboBoxStyle.attr ("borderColorId") = ComboBox::Style::borderColorId;
    comboBoxStyle.attr ("arrowColorId") = ComboBox::Style::arrowColorId;
    comboBoxStyle.attr ("focusedBorderColorId") = ComboBox::Style::focusedBorderColorId;

    // ============================================================================================ yup::TextEditor

    py::class_<TextEditor, Component, PyTextEditor<>> classTextEditor (m, "TextEditor");

    classTextEditor
        .def (py::init<StringRef>(), "componentID"_a = StringRef())

        .def ("getText", &TextEditor::getText)
        .def ("setText", &TextEditor::setText, "newText"_a, "notification"_a = NotificationType::sendNotification)
        .def ("insertText", &TextEditor::insertText,
              "textToInsert"_a, "notification"_a = NotificationType::sendNotification)

        .def ("isMultiLine", &TextEditor::isMultiLine)
        .def ("setMultiLine", &TextEditor::setMultiLine, "shouldBeMultiLine"_a)
        .def ("isReadOnly", &TextEditor::isReadOnly)
        .def ("setReadOnly", &TextEditor::setReadOnly, "shouldBeReadOnly"_a)

        .def ("getCaretPosition", &TextEditor::getCaretPosition)
        .def ("setCaretPosition", &TextEditor::setCaretPosition, "newPosition"_a)
        .def ("isCaretVisible", &TextEditor::isCaretVisible)
        .def ("moveCaretUp", &TextEditor::moveCaretUp, "extendSelection"_a = false)
        .def ("moveCaretDown", &TextEditor::moveCaretDown, "extendSelection"_a = false)
        .def ("moveCaretLeft", &TextEditor::moveCaretLeft, "extendSelection"_a = false)
        .def ("moveCaretRight", &TextEditor::moveCaretRight, "extendSelection"_a = false)
        .def ("moveCaretToStartOfLine", &TextEditor::moveCaretToStartOfLine, "extendSelection"_a = false)
        .def ("moveCaretToEndOfLine", &TextEditor::moveCaretToEndOfLine, "extendSelection"_a = false)
        .def ("moveCaretToStart", &TextEditor::moveCaretToStart, "extendSelection"_a = false)
        .def ("moveCaretToEnd", &TextEditor::moveCaretToEnd, "extendSelection"_a = false)

        .def ("getSelection", &TextEditor::getSelection)
        .def ("setSelection", &TextEditor::setSelection, "newSelection"_a)
        .def ("selectAll", &TextEditor::selectAll)
        .def ("hasSelection", &TextEditor::hasSelection)
        .def ("getSelectedText", &TextEditor::getSelectedText)
        .def ("getSelectedTextAreas", &TextEditor::getSelectedTextAreas)
        .def ("deleteSelectedText", &TextEditor::deleteSelectedText, "notification"_a = NotificationType::sendNotification)

        .def ("copy", &TextEditor::copy)
        .def ("cut", &TextEditor::cut)
        .def ("paste", &TextEditor::paste)

        .def ("getFont", &TextEditor::getFont, "Returns None while the editor uses the theme font.")
        .def ("setFont", &TextEditor::setFont, "newFont"_a)
        .def ("resetFont", &TextEditor::resetFont)
        .def ("getFontSize", &TextEditor::getFontSize, "Returns None while the editor uses the theme font size.")
        .def ("setFontSize", &TextEditor::setFontSize, "newFontSize"_a)
        .def ("resetFontSize", &TextEditor::resetFontSize)

        .def ("getTextBounds", &TextEditor::getTextBounds)
        .def ("getCaretBounds", &TextEditor::getCaretBounds)
        .def ("getScrollOffset", &TextEditor::getScrollOffset)

        .def_readwrite ("onTextChange", &TextEditor::onTextChange);

    // TextEditor implements TextInputTarget, but pybind11 allows only one Python base per class,
    // so the interface's methods are re-exposed here with an explicit TextEditor receiver. Taking
    // the member functions directly would not do: the four that TextInputTarget only declares
    // (everything but the getTextInputRect override) would be registered as TextInputTarget
    // methods, and pybind11 cannot convert a TextEditor instance to TextInputTarget.
    classTextEditor
        .def ("getTextInputRect", [] (const TextEditor& self) { return self.getTextInputRect(); },
              "The screen rectangle being edited, so on-screen keyboards avoid covering it.")
        .def ("requestTextInput", [] (TextEditor& self) { self.requestTextInput(); },
              "Asks the system to start accepting text input, typically from focusGained().")
        .def ("relinquishTextInput", [] (TextEditor& self) { self.relinquishTextInput(); },
              "Tells the system this editor no longer needs text input, typically from focusLost().")
        .def ("updateTextInputRect", [] (TextEditor& self) { self.updateTextInputRect(); },
              "Pushes a moved caret or edited area to the system while text input is active.")
        .def ("isTextInputActive", [] (const TextEditor& self) { return self.isTextInputActive(); });

    py::class_<TextEditor::Style> textEditorStyle (classTextEditor, "Style");
    textEditorStyle.attr ("backgroundColorId") = TextEditor::Style::backgroundColorId;
    textEditorStyle.attr ("textColorId") = TextEditor::Style::textColorId;
    textEditorStyle.attr ("caretColorId") = TextEditor::Style::caretColorId;
    textEditorStyle.attr ("selectionColorId") = TextEditor::Style::selectionColorId;
    textEditorStyle.attr ("outlineColorId") = TextEditor::Style::outlineColorId;
    textEditorStyle.attr ("focusedOutlineColorId") = TextEditor::Style::focusedOutlineColorId;
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
