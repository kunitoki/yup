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

#pragma once

#if ! YUP_MODULE_AVAILABLE_yup_gui
#error This binding file requires adding the yup_gui module in the project
#else
#include <yup_gui/yup_gui.h>
#endif

#include "yup_YupCore_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#define YUP_PYTHON_INCLUDE_PYBIND11_IOSTREAM
#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#include "../utilities/yup_PyBind11Includes.h"

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <vector>
#include <utility>

namespace yup::Bindings
{

// =================================================================================================

void registerYupGuiBindings (pybind11::module_& m);

// =================================================================================================

/** Reports an exception that escaped into one of the YUP dispatch loops or worker threads.

    This is the single entry point behind PyYUPApplication::unhandledException, and it never
    throws: every caller is a YUP_CATCH_EXCEPTION handler running inside a C or Objective-C frame
    (an SDL event watch, a CFRunLoop timer, the render thread loop), where letting an exception
    escape leaves the platform locks in an unrecoverable state.

    When called from a thread other than the message thread it never acquires the GIL, and marshals
    the reporting to the message thread instead. The message thread can be holding the GIL while it
    joins the calling thread - tearing a window down joins the render thread - so acquiring the GIL
    here would deadlock the two threads against each other.

    That is necessary but not sufficient: the caller's catch handler still destroys the exception on
    the throwing thread, and pybind11's deleter takes the GIL when that was the last reference. The
    matching half of the fix is PyComponent's destructor, which releases the GIL across the join.

    @param application    The application to report to, may be nullptr.
    @param ex             The exception being reported, may be nullptr for unknown exceptions.
    @param sourceFilename The name of the file the exception was caught in.
    @param lineNumber     The line the exception was caught at.
*/
void reportUnhandledException (yup::YUPApplication* application,
                               const std::exception* ex,
                               const yup::String& sourceFilename,
                               int lineNumber) noexcept;

// =================================================================================================

struct Options
{
    std::atomic_bool catchExceptionsAndContinue = false;
    std::atomic_bool caughtKeyboardInterrupt = false;
    std::atomic_int messageManagerGranularityMilliseconds = 200;
};

Options& globalOptions() noexcept;

// =================================================================================================

struct PyYUPApplication : yup::YUPApplication
{
    yup::String getApplicationName() override
    {
        PYBIND11_OVERRIDE_PURE (yup::String, yup::YUPApplication, getApplicationName);
    }

    yup::String getApplicationVersion() override
    {
        PYBIND11_OVERRIDE_PURE (yup::String, yup::YUPApplication, getApplicationVersion);
    }

    bool moreThanOneInstanceAllowed() override
    {
        PYBIND11_OVERRIDE (bool, yup::YUPApplication, moreThanOneInstanceAllowed);
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::YUPApplication, initialise, commandLineParameters);
    }

    void shutdown() override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::YUPApplication, shutdown);
    }

    void anotherInstanceStarted (const yup::String& commandLine) override
    {
        PYBIND11_OVERRIDE (void, yup::YUPApplication, anotherInstanceStarted, commandLine);
    }

    void systemRequestedQuit() override
    {
        PYBIND11_OVERRIDE (void, yup::YUPApplication, systemRequestedQuit);
    }

    void suspended() override
    {
        PYBIND11_OVERRIDE (void, yup::YUPApplication, suspended);
    }

    void resumed() override
    {
        PYBIND11_OVERRIDE (void, yup::YUPApplication, resumed);
    }

    void unhandledException (const std::exception* ex, const yup::String& sourceFilename, int lineNumber) override
    {
        reportUnhandledException (this, ex, sourceFilename, lineNumber);
    }

    void memoryWarningReceived() override
    {
        PYBIND11_OVERRIDE (void, yup::YUPApplication, memoryWarningReceived);
    }

    bool backButtonPressed() override
    {
        PYBIND11_OVERRIDE (bool, yup::YUPApplication, backButtonPressed);
    }
};

// =================================================================================================

template <class Base = yup::MouseListener>
struct PyMouseListener : Base
{
    using Base::Base;

    void mouseMove (const yup::MouseEvent& event) override
    {
        PYBIND11_OVERRIDE (void, Base, mouseMove, event);
    }

    void mouseEnter (const yup::MouseEvent& event) override
    {
        {
            pybind11::gil_scoped_acquire gil;

            if (pybind11::function override_ = pybind11::get_override (static_cast<Base*> (this), "mouseEnter"))
            {
                override_ (event);
                return;
            }
        }

        //if constexpr (! std::is_same_v<Base, yup::TooltipWindow>)
        //    Base::mouseEnter (event);
    }

    void mouseExit (const yup::MouseEvent& event) override
    {
        {
            pybind11::gil_scoped_acquire gil;

            if (pybind11::function override_ = pybind11::get_override (static_cast<Base*> (this), "mouseExit"))
            {
                override_ (event);
                return;
            }
        }

        //if constexpr (! std::is_same_v<Base, yup::TooltipWindow>)
        //    Base::mouseExit (event);
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        {
            pybind11::gil_scoped_acquire gil;

            if (pybind11::function override_ = pybind11::get_override (static_cast<Base*> (this), "mouseDown"))
            {
                override_ (event);
                return;
            }
        }

        //if constexpr (! std::is_same_v<Base, yup::TooltipWindow>)
        //    Base::mouseDown (event);
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        PYBIND11_OVERRIDE (void, Base, mouseDrag, event);
    }

    void mouseUp (const yup::MouseEvent& event) override
    {
        PYBIND11_OVERRIDE (void, Base, mouseUp, event);
    }

    void mouseDoubleClick (const yup::MouseEvent& event) override
    {
        PYBIND11_OVERRIDE (void, Base, mouseDoubleClick, event);
    }

    void mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& wheel) override
    {
        {
            pybind11::gil_scoped_acquire gil;

            if (pybind11::function override_ = pybind11::get_override (static_cast<Base*> (this), "mouseWheel"))
            {
                override_ (event, wheel);
                return;
            }
        }

        //if constexpr (! std::is_same_v<Base, yup::TooltipWindow>)
        //    Base::mouseWheel (event, wheel);
    }

    //void mouseMagnify (const yup::MouseEvent& event, float scaleFactor) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, mouseMagnify, event, scaleFactor);
    //}
};

// =================================================================================================

/** Trampoline for TextInputTarget, whose getTextInputRect() is pure virtual. */
template <class Base = yup::TextInputTarget>
struct PyTextInputTarget : Base
{
    using Base::Base;

    yup::Rectangle<float> getTextInputRect() const override
    {
        PYBIND11_OVERRIDE_PURE (yup::Rectangle<float>, Base, getTextInputRect);
    }
};

// =================================================================================================

/** Trampoline for ComponentListener, so a Python subclass receives component lifecycle events. */
template <class Base = yup::ComponentListener>
struct PyComponentListener : Base
{
    using Base::Base;

    void componentMoved (yup::Component& component) override
    {
        PYBIND11_OVERRIDE (void, Base, componentMoved, component);
    }

    void componentResized (yup::Component& component) override
    {
        PYBIND11_OVERRIDE (void, Base, componentResized, component);
    }

    void componentBeingDeleted (yup::Component& component) override
    {
        PYBIND11_OVERRIDE (void, Base, componentBeingDeleted, component);
    }

    void componentPaintCompleted (yup::Component& component, const yup::ComponentPaintMetrics& metrics) override
    {
        PYBIND11_OVERRIDE (void, Base, componentPaintCompleted, component, metrics);
    }
};

// =================================================================================================

/** Trampoline for ComponentEffect, whose apply() is pure virtual. */
template <class Base = yup::ComponentEffect>
struct PyComponentEffect : Base
{
    using Base::Base;

    void apply (yup::Graphics& g, yup::GpuTexture::Ptr inputTexture, yup::Rectangle<float> bounds) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, apply, g, inputTexture, bounds);
    }
};

// =================================================================================================

template <class Base = yup::Component>
struct PyComponent : PyMouseListener<Base>
{
    using PyMouseListener<Base>::PyMouseListener;

    ~PyComponent()
    {
        if (! Base::isOnDesktop())
            return;

        pybind11::gil_scoped_release release;

        Base::removeFromDesktop();
    }

    void setTitle (const yup::String& newName) override
    {
        PYBIND11_OVERRIDE (void, Base, setTitle, newName);
    }

    void setVisible (bool shouldBeVisible) override
    {
        PYBIND11_OVERRIDE (void, Base, setVisible, shouldBeVisible);
    }

    void attachedToNative() override
    {
        PYBIND11_OVERRIDE (void, Base, attachedToNative);
    }

    void detachedFromNative() override
    {
        PYBIND11_OVERRIDE (void, Base, detachedFromNative);
    }

    void displayChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, displayChanged);
    }

    void contentScaleChanged (float dpiScale) override
    {
        PYBIND11_OVERRIDE (void, Base, contentScaleChanged, dpiScale);
    }

    void opacityChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, opacityChanged);
    }

    void transformChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, transformChanged);
    }

    void visibilityChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, visibilityChanged);
    }

    void userTriedToCloseWindow() override
    {
        PYBIND11_OVERRIDE (void, Base, userTriedToCloseWindow);
    }

    //void minimisationStateChanged(bool isNowMinimised) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, minimisationStateChanged, isNowMinimised);
    //}

    //float getDesktopScaleFactor() const override
    //{
    //    PYBIND11_OVERRIDE (float, Base, getDesktopScaleFactor);
    //}

    void parentHierarchyChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, parentHierarchyChanged);
    }

    void childrenChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, childrenChanged);
    }

    //bool hitTest (int x, int y) override
    //{
    //    PYBIND11_OVERRIDE (bool, Base, hitTest, x, y);
    //}

    //void lookAndFeelChanged() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, lookAndFeelChanged);
    //}

    void enablementChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, enablementChanged);
    }

    void safeAreaChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, safeAreaChanged);
    }

    //void alphaChanged() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, alphaChanged);
    //}

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        PYBIND11_OVERRIDE (void, Base, refreshDisplay, lastFrameTimeSeconds);
    }

    void paint (yup::Graphics& g) override
    {
        {
            pybind11::gil_scoped_acquire gil;

            if (pybind11::function override_ = pybind11::get_override (static_cast<const Base*> (this), "paint"); override_)
            {
                override_ (std::addressof (g));
                return;
            }
        }

        //if constexpr (! std::is_same_v<Base, yup::TooltipWindow>)
        //    Base::paint (g);
    }

    void paintOverChildren (yup::Graphics& g) override
    {
        {
            pybind11::gil_scoped_acquire gil;

            if (pybind11::function override_ = pybind11::get_override (static_cast<const Base*> (this), "paintOverChildren"); override_)
            {
                override_ (std::addressof (g));
                return;
            }
        }

        Base::paintOverChildren (g);
    }

    void styleChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, styleChanged);
    }

    bool isInterestedInDrag (const DragAndDropData& data) override
    {
        PYBIND11_OVERRIDE (bool, Base, isInterestedInDrag, data);
    }

    bool itemsDropped (const Point<float>& position, const DragAndDropData& data) override
    {
        PYBIND11_OVERRIDE (bool, Base, itemsDropped, position, data);
    }

    void itemDragEnter (const DragAndDropData& data, const Point<float>& position) override
    {
        PYBIND11_OVERRIDE (void, Base, itemDragEnter, data, position);
    }

    void itemDragMove (const DragAndDropData& data, const Point<float>& position) override
    {
        PYBIND11_OVERRIDE (void, Base, itemDragMove, data, position);
    }

    void itemDragExit (const DragAndDropData& data) override
    {
        PYBIND11_OVERRIDE (void, Base, itemDragExit, data);
    }

    void keyDown (const yup::KeyPress& key, const Point<float>& position) override
    {
        PYBIND11_OVERRIDE (void, Base, keyDown, key, position);
    }

    void keyUp (const yup::KeyPress& key, const Point<float>& position) override
    {
        PYBIND11_OVERRIDE (void, Base, keyUp, key, position);
    }

    void textInput (const String& text) override
    {
        PYBIND11_OVERRIDE (void, Base, textInput, text);
    }

    //bool keyStateChanged (bool isDown) override
    //{
    //    PYBIND11_OVERRIDE (bool, Base, keyStateChanged, isDown);
    //}

    //void modifierKeysChanged (const yup::ModifierKeys& modifiers) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, modifierKeysChanged, modifiers);
    //}

    void focusGained() override
    {
        PYBIND11_OVERRIDE (void, Base, focusGained);
    }

    void focusLost() override
    {
        PYBIND11_OVERRIDE (void, Base, focusLost);
    }

    //void focusOfChildComponentChanged (yup::Component::FocusChangeType cause) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, focusOfChildComponentChanged, cause);
    //}

    void resized() override
    {
        PYBIND11_OVERRIDE (void, Base, resized);
    }

    void moved() override
    {
        PYBIND11_OVERRIDE (void, Base, moved);
    }

    //void childBoundsChanged (yup::Component* child) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, childBoundsChanged, child);
    //}

    //void parentSizeChanged() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, parentSizeChanged);
    //}

    //void broughtToFront() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, broughtToFront);
    //}

    //void colourChanged () override
    //{
    //    PYBIND11_OVERRIDE (void, Base, colourChanged);
    //}
};

// ============================================================================================

template <class Base = yup::DocumentWindow>
struct PyDocumentWindow : PyComponent<Base>
{
    using PyComponent<Base>::PyComponent;

    //void closeButtonPressed() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, closeButtonPressed);
    //}

    //void minimiseButtonPressed() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, minimiseButtonPressed);
    //}

    //void maximiseButtonPressed() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, maximiseButtonPressed);
    //}

    void userTriedToCloseWindow() override
    {
        PYBIND11_OVERRIDE (void, Base, userTriedToCloseWindow);
    }
};

// ============================================================================================

template <class Base = yup::Button>
struct PyButton : PyComponent<Base>
{
    using PyComponent<Base>::PyComponent;

    void paintButton (yup::Graphics& g) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, paintButton, g);
    }
};

// ============================================================================================

template <class Base = yup::Slider>
struct PySlider : PyComponent<Base>
{
    using PyComponent<Base>::PyComponent;

    void valueChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, valueChanged);
    }

    void minValueChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, minValueChanged);
    }

    void maxValueChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, maxValueChanged);
    }
};

// ============================================================================================

template <class Base = yup::ProgressBar>
struct PyProgressBar : PyComponent<Base>
{
    using PyComponent<Base>::PyComponent;

    void progressChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, progressChanged);
    }
};

// ============================================================================================

template <class Base = yup::SwitchButton>
struct PySwitchButton : PyButton<Base>
{
    using PyButton<Base>::PyButton;

    void toggleStateChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, toggleStateChanged);
    }
};

// ============================================================================================

template <class Base = yup::ComboBox>
struct PyComboBox : PyComponent<Base>
{
    using PyComponent<Base>::PyComponent;

    void selectedItemChanged() override
    {
        PYBIND11_OVERRIDE (void, Base, selectedItemChanged);
    }
};

// ============================================================================================

/** Trampoline for ListBoxModel, which is entirely virtual and is never owned by a ListBox.

    refreshComponentForRow is deliberately absent: it hands the ListBox ownership of a raw
    Component*, which pybind11 cannot take away from a Python-owned instance without risking a
    double free. A Python model paints its rows through paintListBoxItem/getRowText/getRowIcon.
*/
template <class Base = yup::ListBoxModel>
struct PyListBoxModel : Base
{
    // ListBoxModel keeps its own constructor protected, so the trampoline exposes one.
    PyListBoxModel() = default;

    int getNumRows() override
    {
        PYBIND11_OVERRIDE_PURE (int, Base, getNumRows);
    }

    int getRowHeight (int rowIndex) override
    {
        PYBIND11_OVERRIDE (int, Base, getRowHeight, rowIndex);
    }

    int getRowWidth (int rowIndex) override
    {
        PYBIND11_OVERRIDE (int, Base, getRowWidth, rowIndex);
    }

    void paintListBoxItem (int rowIndex, yup::Graphics& g, yup::Rectangle<float> area, bool isSelected) override
    {
        PYBIND11_OVERRIDE (void, Base, paintListBoxItem, rowIndex, g, area, isSelected);
    }

    yup::String getRowText (int rowIndex) override
    {
        PYBIND11_OVERRIDE (yup::String, Base, getRowText, rowIndex);
    }

    yup::Image getRowIcon (int rowIndex) override
    {
        PYBIND11_OVERRIDE (yup::Image, Base, getRowIcon, rowIndex);
    }

    void selectedRowsChanged (const yup::Array<int>& selectedRows) override
    {
        PYBIND11_OVERRIDE (void, Base, selectedRowsChanged, selectedRows);
    }

    void rowClicked (int rowIndex, const yup::MouseEvent& event) override
    {
        PYBIND11_OVERRIDE (void, Base, rowClicked, rowIndex, event);
    }

    void rowDoubleClicked (int rowIndex, const yup::MouseEvent& event) override
    {
        PYBIND11_OVERRIDE (void, Base, rowDoubleClicked, rowIndex, event);
    }

    void returnKeyPressed (int lastSelectedRow) override
    {
        PYBIND11_OVERRIDE (void, Base, returnKeyPressed, lastSelectedRow);
    }

    void deleteKeyPressed (const yup::Array<int>& selectedRows) override
    {
        PYBIND11_OVERRIDE (void, Base, deleteKeyPressed, selectedRows);
    }

    yup::var getDragSourceDescription (const yup::Array<int>& selectedRows) override
    {
        PYBIND11_OVERRIDE (yup::var, Base, getDragSourceDescription, selectedRows);
    }
};

// ============================================================================================

/** Trampoline for TextEditor, which implements TextInputTarget's pure virtual.

    The bindings can only declare Component as TextEditor's Python base, so the TextInputTarget
    half is exposed as plain methods on TextEditor, and this override is what routes a Python
    subclass's getTextInputRect back into the text input system.
*/
template <class Base = yup::TextEditor>
struct PyTextEditor : PyComponent<Base>
{
    using PyComponent<Base>::PyComponent;

    yup::Rectangle<float> getTextInputRect() const override
    {
        PYBIND11_OVERRIDE (yup::Rectangle<float>, Base, getTextInputRect);
    }
};

} // namespace yup::Bindings
