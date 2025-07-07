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

#if YUP_PYTHON_EMBEDDED_INTERPRETER

#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

#include <iostream>
#include <string>

PYBIND11_EMBEDDED_MODULE(__yup__, m)
{
    namespace py = pybind11;

    struct CustomOutputStream
    {
        CustomOutputStream() = default;
        CustomOutputStream (const CustomOutputStream&) = default;
        CustomOutputStream (CustomOutputStream&&) = default;
    };

    py::class_<CustomOutputStream> classCustomOutputStream (m, "__stdout__");
    classCustomOutputStream.def_static ("write", [](py::object buffer) { std::cout << buffer.cast<std::string>(); });
    classCustomOutputStream.def_static ("flush", [] { std::cout << std::flush; });
    classCustomOutputStream.def_static ("isatty", [] { return true; });

    struct CustomErrorStream
    {
        CustomErrorStream() = default;
        CustomErrorStream (const CustomErrorStream&) = default;
        CustomErrorStream (CustomErrorStream&&) = default;
    };

    py::class_<CustomErrorStream> classCustomErrorStream (m, "__stderr__");
    classCustomErrorStream.def_static ("write", [](py::object buffer) { std::cerr << buffer.cast<std::string>(); });
    classCustomErrorStream.def_static ("flush", [] { std::cerr << std::flush; });
    classCustomErrorStream.def_static ("isatty", [] { return true; });

    m.def ("__redirect__", []
    {
        auto sys = py::module_::import ("sys");
        auto yupSys = py::module_::import ("__yup__");

        yupSys.attr ("__saved_stdout__") = sys.attr ("stdout");
        yupSys.attr ("__saved_stderr__") = sys.attr ("stderr");
        sys.attr ("stdout") = yupSys.attr ("__stdout__");
        sys.attr ("stderr") = yupSys.attr ("__stderr__");
    });

    m.def ("__restore__", []
    {
        auto sys = py::module_::import ("sys");
        auto yupSys = py::module_::import ("__yup__");

        sys.attr ("stdout") = yupSys.attr ("__saved_stdout__");
        sys.attr ("stderr") = yupSys.attr ("__saved_stderr__");
    });
}

#endif
