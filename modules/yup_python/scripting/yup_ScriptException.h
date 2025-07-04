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

#include <yup_core/yup_core.h>

#include <exception>

namespace yup
{

// =================================================================================================

/**
 * @brief Custom exception class for handling script-related errors.
 *
 * The `ScriptException` class is derived from `std::exception` and is designed to be used for throwing exceptions when errors
 * occur during script execution. It provides a way to encapsulate and convey error messages in a human-readable format.
 *
 * @warning It is important to note that this class should be used judiciously and only for exceptional situations in script execution.
 *
 * @param msg A string containing the error message to be associated with the exception.
 */
class YUP_API ScriptException : public std::exception
{
public:
    /**
     * @brief Constructs a `ScriptException` with the specified error message.
     *
     * This constructor initializes the `ScriptException` object with the provided error message.
     *
     * @param msg The error message to be associated with the exception.
     */
    ScriptException (StringRef msg)
        : message (msg)
    {
    }

    /**
     * @brief Returns a C-string representing the error message associated with the exception.
     *
     * The `what` method is overridden from the `std::exception` base class to provide access to the error message stored within
     * this `ScriptException`.
     *
     * @return A pointer to a C-string containing the error message.
     */
    const char* what() const noexcept override
    {
        return message.toUTF8();
    }

private:
    String message;
};

} // namespace yup
