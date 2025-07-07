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

#include <string_view>
#include <typeinfo>
#include <tuple>

// =================================================================================================

namespace PYBIND11_NAMESPACE {

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

namespace yup::Bindings {

namespace py = pybind11;
using namespace py::literals;

// =================================================================================================

Options& globalOptions() noexcept
{
    static Options options = {};
    return options;
}

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
        .def ("setApplicationReturnValue", [](YUPApplication& self, int value) { self.setApplicationReturnValue (value); })
        .def ("getApplicationReturnValue", [](const YUPApplication& self) { return self.getApplicationReturnValue(); })
        .def_static ("isStandaloneApp", &YUPApplication::isStandaloneApp)
        .def ("isInitialising", &YUPApplication::isInitialising)
    ;
}

} // namespace yup::Bindings
