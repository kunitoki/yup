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

#include <yup_python/yup_python.h>

#include "yup_PyBind11Includes.h"
#include "yup_ClassDemangling.h"

#include <functional>

//==============================================================================

PYBIND11_DECLARE_HOLDER_TYPE(T, yup::ReferenceCountedObjectPtr<T>, true)

namespace yup::Helpers {

//==============================================================================

inline void printPythonException (const pybind11::error_already_set& e)
{
    pybind11::print ("Traceback (most recent call last):");

    pybind11::module_::import ("traceback").attr ("print_tb") (e.trace());

    pybind11::print (e.what());
}

//==============================================================================

template <class T, class F>
auto makeRepr (F&& func)
{
    static_assert (std::is_invocable_v<F, const T&>);

    return [func](const T& instance)
    {
        String result;

        result
            << pythonizeCompoundClassName (PythonModuleName, typeid (instance).name())
            << "('" << std::invoke (func, instance) << "')";

        return result;
    };
}

//==============================================================================

template <class E>
pybind11::enum_<E> makeArithmeticEnum (pybind11::object& parent, const char* name)
{
    jassert (name != nullptr);

    using T = std::underlying_type_t<E>;

    pybind11::enum_<E> classEnum (parent, name);

    classEnum
        .def ("__and__", [](E lhs, E rhs) { return (static_cast<T> (lhs) & static_cast<T> (rhs)); })
        .def ("__eq__", [](E lhs, E rhs) { return (static_cast<T> (lhs) == static_cast<T> (rhs)); })
        .def ("__eq__", [](E lhs, T rhs) { return (static_cast<T> (lhs) == rhs); })
        .def ("__ge__", [](E lhs, E rhs) { return (static_cast<T> (lhs) >= static_cast<T> (rhs)); })
        .def ("__ge__", [](E lhs, T rhs) { return (static_cast<T> (lhs) >= rhs); })
        .def ("__gt__", [](E lhs, E rhs) { return (static_cast<T> (lhs) > static_cast<T> (rhs)); })
        .def ("__gt__", [](E lhs, T rhs) { return (static_cast<T> (lhs) > rhs); })
        .def ("__hash__", [](E lhs) { return static_cast<T> (lhs); })
        .def ("__int__", [](E lhs) { return static_cast<T> (lhs); })
        .def ("__invert__", [](E lhs) { return ~static_cast<T> (lhs); })
        .def ("__le__", [](E lhs, E rhs) { return (static_cast<T> (lhs) <= static_cast<T> (rhs)); })
        .def ("__le__", [](E lhs, T rhs) { return (static_cast<T> (lhs) <= rhs); })
        .def ("__lt__", [](E lhs, E rhs) { return (static_cast<T> (lhs) < static_cast<T> (rhs)); })
        .def ("__lt__", [](E lhs, T rhs) { return (static_cast<T> (lhs) < rhs); })
        .def ("__ne__", [](E lhs, E rhs) { return (static_cast<T> (lhs) != static_cast<T> (rhs)); })
        .def ("__ne__", [](E lhs, T rhs) { return (static_cast<T> (lhs) != rhs); })
        .def ("__or__", [](E lhs, E rhs) { return (static_cast<T> (lhs) | static_cast<T> (rhs)); })
        .def ("__xor__", [](E lhs, E rhs) { return (static_cast<T> (lhs) ^ static_cast<T> (rhs)); })
    ;

    return classEnum;
}

//==============================================================================

template <class T, class F>
auto makeVoidPointerAndSizeCallable (F&& func)
{
    if constexpr (std::is_invocable_v<F, T&, const void*, size_t>)
    {
        return [func](T* self, pybind11::buffer data)
        {
            const auto info = data.request();

            using return_value = std::invoke_result_t<F, T&, const void*, size_t>;

            if constexpr (std::is_void_v<return_value>)
                std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
            else
                return std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
        };
    }
    else if constexpr (std::is_invocable_v<F, const T&, const void*, size_t>)
    {
        return [func](const T* self, pybind11::buffer data)
        {
            const auto info = data.request();

            using return_value = std::invoke_result_t<F, const T&, const void*, size_t>;

            if constexpr (std::is_void_v<return_value>)
                std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
            else
                return std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
        };
    }
    else if constexpr (std::is_invocable_v<F, T&, void*, size_t>)
    {
        return [func](T* self, pybind11::buffer data)
        {
            auto info = data.request (true);

            using return_value = std::invoke_result_t<F, T&, void*, size_t>;

            if constexpr (std::is_void_v<return_value>)
                std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
            else
                return std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
        };
    }
    else if constexpr (std::is_invocable_v<F, const T&, void*, size_t>)
    {
        return [func](const T* self, pybind11::buffer data)
        {
            auto info = data.request (true);

            using return_value = std::invoke_result_t<F, T&, void*, size_t>;

            if constexpr (std::is_void_v<return_value>)
                std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
            else
                return std::invoke (func, self, info.ptr, static_cast<size_t> (info.size));
        };
    }
    else
    {
        return func;
    }
}

} // namespace yup::Helpers
