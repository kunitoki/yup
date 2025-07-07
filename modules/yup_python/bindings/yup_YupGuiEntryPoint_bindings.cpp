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

#include "yup_YupGuiEntryPoint_bindings.h"

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

namespace yup {

#if ! YUP_WINDOWS
extern const char* const* yup_argv;
extern int yup_argc;
#endif

namespace Bindings {

namespace py = pybind11;
using namespace py::literals;

namespace {

// ============================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER
void runApplication (YUPApplicationBase* application, int milliseconds)
{
    {
        py::gil_scoped_release release;

        if (! application->initialiseApp())
            return;
    }

    while (! MessageManager::getInstance()->hasStopMessageBeenSent())
    {
        try
        {
            py::gil_scoped_release release;

            MessageManager::getInstance()->runDispatchLoopUntil (milliseconds);
        }
        catch (const py::error_already_set& e)
        {
            if (globalOptions().catchExceptionsAndContinue)
            {
                Helpers::printPythonException (e);
            }
            else
            {
                throw e;
            }
        }

        if (globalOptions().caughtKeyboardInterrupt)
            break;

        if (PyErr_CheckSignals() != 0)
            throw py::error_already_set();
    }
}
#endif

} // namespace

void registerYupGuiEntryPointsBindings (py::module_& m)
{
#if ! YUP_PYTHON_EMBEDDED_INTERPRETER

    // =================================================================================================

    m.def ("START_YUP_APPLICATION", [](py::handle applicationType, bool catchExceptionsAndContinue)
    {
        globalOptions().catchExceptionsAndContinue = catchExceptionsAndContinue;
        globalOptions().caughtKeyboardInterrupt = false;

        py::scoped_ostream_redirect output;

        if (! applicationType)
            throw py::value_error("Argument must be a YUPApplication subclass");

        YUPApplicationBase* application = nullptr;

        auto sys = py::module_::import ("sys");
        auto systemExit = [sys, &application]
        {
            const int returnValue = application != nullptr ? application->shutdownApp() : 255;

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
        if (application == nullptr)
        {
            systemExit();
            return;
        }

        try
        {
            runApplication (application, globalOptions().messageManagerGranularityMilliseconds);
        }
        catch (const py::error_already_set& e)
        {
            Helpers::printPythonException (e);
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
                    throw py::value_error("Argument must be a YUPApplication subclass");

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

        void processEvents(int milliseconds = 20)
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
        .def ("__enter__", [](PyTestableApplication& self)
        {
            self.applicationScope = std::make_unique<PyTestableApplication::Scope> (self.applicationType);
            return std::addressof (self);
        }, py::return_value_policy::reference)
        .def ("__exit__", [](PyTestableApplication& self, const std::optional<py::type>&, const std::optional<py::object>&, const std::optional<py::object>&)
        {
            self.applicationScope.reset();
        })
        .def ("__next__", [](PyTestableApplication& self)
        {
            self.processEvents();
            return std::addressof (self);
        }, py::return_value_policy::reference)
    ;

#endif
}

} // namespace Bindings
} // namespace yup

// =================================================================================================

#if ! YUP_PYTHON_EMBEDDED_INTERPRETER && YUP_WINDOWS
BOOL APIENTRY DllMain(HANDLE instance, DWORD reason, LPVOID reserved)
{
    yup::ignoreUnused (reserved);

    if (reason == DLL_PROCESS_ATTACH)
        yup::Process::setCurrentModuleInstanceHandle (instance);

    return true;
}
#endif
