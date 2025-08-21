

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

namespace yup
{

//==============================================================================
/**
    Filter coefficient storage for state variable filters.
*/
template <typename CoeffType = double>
struct StateVariableCoefficients
{
    CoeffType k = static_cast<CoeffType> (1.0);
    CoeffType g = static_cast<CoeffType> (1.0);
    CoeffType damping = static_cast<CoeffType> (1.0);

    StateVariableCoefficients() = default;

    StateVariableCoefficients (CoeffType k_, CoeffType g_, CoeffType damping_) noexcept
        : k (k_)
        , g (g_)
        , damping (damping_)
    {
    }
};

} // namespace yup
