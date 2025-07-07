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

#if !YUP_MODULE_AVAILABLE_yup_gui
#error This binding file requires adding the yup_gui module in the project
#else
#include <yup_gui/yup_gui.h>
#endif

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

namespace yup::Bindings {

// =================================================================================================

void registerYupGuiBindings (pybind11::module_& m);

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
        pybind11::gil_scoped_acquire gil;

        const auto* pyEx = dynamic_cast<const pybind11::error_already_set*> (ex);
        auto traceback = pybind11::module_::import ("traceback");

        if (pybind11::function override_ = pybind11::get_override (static_cast<yup::YUPApplication*> (this), "unhandledException"); override_)
        {
            if (pyEx != nullptr)
            {
                auto newPyEx = pyEx->type()(pyEx->value());
                PyException_SetTraceback (newPyEx.ptr(), pyEx->trace().ptr());

                override_ (newPyEx, sourceFilename, lineNumber);
            }
            else
            {
                auto runtimeError = pybind11::module_::import ("__builtins__").attr ("RuntimeError");
                auto newPyEx = runtimeError(ex != nullptr ? ex->what() : "unknown exception");
                PyException_SetTraceback (newPyEx.ptr(), traceback.attr ("extract_stack")().ptr());

                override_ (newPyEx, sourceFilename, lineNumber);
            }

            return;
        }

        if (pyEx != nullptr)
        {
            pybind11::print (ex->what());
            traceback.attr ("print_tb")(pyEx->trace());

            if (pyEx->matches (PyExc_KeyboardInterrupt) || PyErr_CheckSignals() != 0)
            {
                globalOptions().caughtKeyboardInterrupt = true;
                return;
            }
        }
        else
        {
            pybind11::print (ex->what());
            traceback.attr ("print_stack")();

            if (PyErr_CheckSignals() != 0)
            {
                globalOptions().caughtKeyboardInterrupt = true;
                return;
            }
        }

        if (! globalOptions().caughtKeyboardInterrupt)
            std::terminate();
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

            if (pybind11::function override_ = pybind11::get_override (static_cast<Base*> (this), "mouseWheelMove"))
            {
                override_ (event, wheel);
                return;
            }
        }

        //if constexpr (! std::is_same_v<Base, yup::TooltipWindow>)
        //    Base::mouseWheelMove (event, wheel);
    }

    //void mouseMagnify (const yup::MouseEvent& event, float scaleFactor) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, mouseMagnify, event, scaleFactor);
    //}
};

// =================================================================================================

template <class Base = yup::Component>
struct PyComponent : PyMouseListener<Base>
{
    using PyMouseListener<Base>::PyMouseListener;

    //void setTitle (const yup::String& newName) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, setName, newName);
    //}

    //void setVisible (bool shouldBeVisible) override
    //{
    //    PYBIND11_OVERRIDE (void, Base, setVisible, shouldBeVisible);
    //}

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

    //void alphaChanged() override
    //{
    //    PYBIND11_OVERRIDE (void, Base, alphaChanged);
    //}

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

    //bool keyPressed (const yup::KeyPress& key) override
    //{
    //    PYBIND11_OVERRIDE (bool, Base, keyPressed, key);
    //}

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

} // namespace yup::Bindings
