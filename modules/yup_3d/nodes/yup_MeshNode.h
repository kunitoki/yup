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
/** A part that draws a Mesh with the transform of its entity.

    Every MeshNode slot of an entity is drawn. The mesh can be shared with other entities.

    @see Mesh, EntityNode
*/
class YUP_API MeshNode : public Node
{
public:
    /** Creates a part without a mesh. */
    MeshNode() = default;

    /** Creates a part drawing a mesh. */
    explicit MeshNode (Mesh::Ptr meshToDraw)
        : mesh (std::move (meshToDraw))
    {
    }

    Mesh::Ptr mesh; ///< The mesh to draw, or nullptr to draw nothing.
};

} // namespace yup
