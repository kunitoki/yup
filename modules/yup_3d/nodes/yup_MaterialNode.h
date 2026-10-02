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

//==============================================================================
/** A part that overrides the material of every mesh in the subtree of its entity.

    The override applies to the entity's own meshes and to all of its descendants, and the
    nearest MaterialNode up the tree wins. Only the lowest slot is the override: other slots
    are free for application use, for example to keep alternative materials at hand.

    @see Material, EntityNode
*/
class YUP_API MaterialNode : public Node
{
public:
    /** Creates a part without a material, which overrides nothing. */
    MaterialNode() = default;

    /** Creates a part overriding with a material. */
    explicit MaterialNode (Material::Ptr overrideMaterial)
        : material (std::move (overrideMaterial))
    {
    }

    Material::Ptr material; ///< The override, or nullptr to override nothing.
};

} // namespace yup
