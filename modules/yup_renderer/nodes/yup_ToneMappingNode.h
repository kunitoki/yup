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
/** How the lit colors are mapped into the displayable range. */
enum class ToneMappingOperator
{
    reinhard, ///< Soft roll-off of the highlights, keeps the colors flat.
    aces,     ///< The ACES filmic curve: more contrast and saturation, photographic highlights.
    agx       ///< AgX: the filmic default of modern renderers, keeps saturated highlights from skewing.
};

//==============================================================================
/** A scene feature: the operator mapping the lit colors into the displayable range.

    The visible ToneMappingNode nearest to the root wins, see
    SceneSettings::resolve(). Without one the scene uses ToneMappingOperator::reinhard.

    @code
    scene->getRoot().attach<yup::ToneMappingNode> (yup::ToneMappingOperator::agx);
    @endcode

    @see ExposureNode, SceneSettings
*/
class YUP_API ToneMappingNode : public Node
{
public:
    /** Creates a Reinhard tone mapping. */
    ToneMappingNode() = default;

    /** Creates a tone mapping with an operator. */
    explicit ToneMappingNode (ToneMappingOperator toneMappingOperator) noexcept
        : type (toneMappingOperator)
    {
    }

    ToneMappingOperator type = ToneMappingOperator::reinhard; ///< The tone mapping operator.
};

} // namespace yup
