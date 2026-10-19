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

    An entity has at most one MeshNode: use child entities to draw several meshes. The mesh
    can be shared with other entities.

    @see Mesh, Entity
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

    /** Returns where a world space ray enters the bounds of the mesh, placed by the entity.

        The box is tested in the space of the mesh, so it stays tight around rotated meshes.
        Visibility is ignored. See Ray::intersectBox() for the range of distances.

        @return The distance along the ray, or nothing without a mesh, without an entity, for a
                mesh flattened by a zero scale, or when the ray misses it.
    */
    std::optional<float> intersectRay (const Ray& ray,
                                       float minDistance = 0.0f,
                                       float maxDistance = std::numeric_limits<float>::max()) const noexcept;

    Mesh::Ptr mesh; ///< The mesh to draw, or nullptr to draw nothing.

    /** @internal The world matrix a SceneRenderer drew last frame, for motion vectors. */
    Matrix4 previousWorld;

    /** @internal The SceneRenderer frame stamp previousWorld belongs to, 0 for none. */
    uint64 previousStamp = 0;
};

} // namespace yup
