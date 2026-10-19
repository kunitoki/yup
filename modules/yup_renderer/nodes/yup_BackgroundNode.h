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
/** A scene feature: the color the background is cleared to.

    Scene features apply to the whole scene: the first one of each type found depth-first in the
    visible part of the tree wins, see SceneSettings::resolve(), so attach them to the root entity.
    Without a BackgroundNode the scene behaves as if it had a default-constructed one.

    @code
    scene->getRoot()->attach<yup::BackgroundNode> (yup::Colors::black);
    @endcode

    @see SceneSettings
*/
class YUP_API BackgroundNode : public Node
{
public:
    /** Creates a background of the default color. */
    BackgroundNode() = default;

    /** Creates a background of a color. */
    explicit BackgroundNode (Color backgroundColor) noexcept
        : color (backgroundColor)
    {
    }

    Color color { 0xff1a1a2e }; ///< The color the background is cleared to.
};

} // namespace yup
