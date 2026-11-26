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
/** A node that draws triangles written as numbers: the `Mesh3D` of a `.yui` document.

    The geometry is given as flat arrays, one value after the other:

    - `vertices`: the x, y, z of each vertex.
    - `normals`: the x, y, z of the normal of each vertex. Without them each triangle gets flat
      normals, facing the side its counter-clockwise winding faces.
    - `uvs`: the u, v of each vertex, v pointing down.
    - `colors`: the linear r, g, b, a of each vertex, multiplied with the base color.
    - `indices`: three vertex indices for each triangle, counter-clockwise when front facing.
      Without them the vertices are taken three by three.

    @code
    Mesh3D {
        vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
        uvs: [0, 1,  1, 1,  1, 0,  0, 0];
        indices: [0, 1, 2,  0, 2, 3];

        Material3D { baseColor: #c8553d; roughness: 0.6; }
    }
    @endcode

    They make one Mesh with one primitive, on the MeshNode of the entity, drawn with the
    default material unless a `Material3D` is written inside the mesh or above it. The mesh is
    built again when an array changes: once for the arrays set together, when the values the
    document applies are all in, before the next frame or when the scene is painted before it,
    and by getMesh(). Arrays that do
    not fit together draw nothing and are reported in the diagnostics of the document: a
    number that is not finite, a length that is not a multiple of what each vertex takes,
    another number of normals, uvs or colors than of vertices, or an index that is not a whole
    number or not the one of a vertex.

    @see SceneNodeComponent, SceneMaterialComponent, Mesh
*/
class YUP_API SceneMeshComponent : public SceneNodeComponent
{
public:
    //==============================================================================
    /** Creates a mesh without geometry.

        @param componentID The ID of the component.
    */
    explicit SceneMeshComponent (StringRef componentID = {});

    //==============================================================================
    /** Returns the mesh made from the arrays, building it first when one changed since.

        @returns The mesh, or nullptr when there are no vertices or the arrays do not fit together.
    */
    Mesh::Ptr getMesh();

    //==============================================================================
    /** Returns the static type info describing the Mesh3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal Builds the mesh when an array changed since, and keeps what is wrong with the arrays for getMesh() to tell: what a scene painted before its next frame calls. */
    void updateMesh();

protected:
    //==============================================================================
    /** @internal */
    void entityChanged() override;

private:
    //==============================================================================
    void setGeometry (var SceneMeshComponent::* member, const var& newValue, const Identifier& property);
    void geometryChanged();
    Mesh::Ptr buildMesh (Identifier& problemProperty, String& problem);

    var vertices;
    var normals;
    var uvs;
    var colors;
    var indices;
    Mesh::Ptr mesh;
    bool isMeshOutdated = false;
    Identifier untoldProperty; // The array the mesh that was last built has a problem with, until it is told
    String untoldProblem;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneMeshComponent)
};

} // namespace yup
