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

namespace yup
{

class Scene;

//==============================================================================
/** Marks a Scene as iterating for as long as it lives.

    While a scene is iterating, the nodes that are detached, or whose entity is destroyed,
    are unlinked at once but only destroyed when the outermost iteration ends, so references
    held by a loop stay valid. NodeView, NodeCollection and Scene::update() hold one: use it
    for custom loops over nodes that may detach nodes.

    @see Scene::isIterating
*/
class YUP_API IterationGuard
{
public:
    /** Starts iterating a scene. */
    explicit IterationGuard (Scene& sceneToGuard) noexcept;

    /** Stops iterating, destroying the pending nodes if this was the outermost iteration. */
    ~IterationGuard();

    IterationGuard (IterationGuard&& other) noexcept;
    IterationGuard& operator= (IterationGuard&& other) = delete;

private:
    Scene* scene = nullptr;

    YUP_DECLARE_NON_COPYABLE (IterationGuard)
};

} // namespace yup
