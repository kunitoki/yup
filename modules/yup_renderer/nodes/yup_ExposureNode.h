/*
  ==============================================================================

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
/** A scene feature: the exposure multiplier applied before tone mapping.

    The visible ExposureNode nearest to the root wins, see
    SceneSettings::resolve(). Without one the exposure is 1.

    @see ToneMappingNode, SceneSettings
*/
class YUP_API ExposureNode : public Node
{
public:
    /** Creates an exposure of 1. */
    ExposureNode() = default;

    /** Creates an exposure of a multiplier. */
    explicit ExposureNode (float exposureMultiplier) noexcept
        : exposure (exposureMultiplier)
    {
    }

    float exposure = 1.0f; ///< The multiplier applied to the scene color before tone mapping.
};

} // namespace yup
