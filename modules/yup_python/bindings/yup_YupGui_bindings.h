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

} // namespace yup::Bindings
