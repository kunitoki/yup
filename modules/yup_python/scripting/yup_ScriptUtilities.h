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

#include "../utilities/yup_PyBind11Includes.h"

#include <optional>

namespace yup
{

//==============================================================================

/** Cast a python object to a C++ type.

    @param value The python object to cast.

    @return The casted value, or std::nullopt if the cast failed.
 */
template <class T>
std::optional<T> python_cast (const pybind11::object& value)
{
    try
    {
        return value.cast<T>();
    }
    catch (const pybind11::cast_error& e)
    {
        return std::nullopt;
    }
}

//==============================================================================

/** Redirect the standard output and error streams to the script engine.

    @param scriptEngine The script engine to redirect the streams to.
 */
struct YUP_API ScriptStreamRedirection
{
    ScriptStreamRedirection() noexcept;
    ~ScriptStreamRedirection() noexcept;

private:
    pybind11::object sys;
};

} // namespace yup
