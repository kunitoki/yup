/*
  ===============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================
inline bool isFloatComparison (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::ltF:
        case YdspIrOp::leF:
        case YdspIrOp::gtF:
        case YdspIrOp::geF:
            return true;

        default:
            return false;
    }
}

inline bool isIntComparison (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqI:
        case YdspIrOp::neI:
        case YdspIrOp::ltI:
        case YdspIrOp::ltUI:
        case YdspIrOp::leI:
        case YdspIrOp::gtI:
        case YdspIrOp::geI:
            return true;

        default:
            return false;
    }
}

inline bool isInlineFastMathTranscendental (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::atanF:
        case YdspIrOp::tanhF:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
        case YdspIrOp::powF:
        case YdspIrOp::atan2F:
            return true;

        default:
            return false;
    }
}

} // namespace yup
