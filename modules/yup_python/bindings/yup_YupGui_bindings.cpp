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
        .def ("withMouseCapture", &ComponentNative::Options::withMouseCapture)
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
