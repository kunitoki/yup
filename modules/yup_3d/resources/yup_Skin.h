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
/** The joints a skinned mesh follows, and where each of them stood when the mesh was bound to it.

    The vertices of a skinned Mesh name the joints that move them by their index in this list,
    see Mesh::Influences. For every joint the skin holds an inverse bind matrix: it takes a
    vertex from the space of the mesh, in the pose the mesh was modeled in, to the space of the
    joint. Wherever the joint then goes, the vertex follows.

    A skin names its joints as indices into the nodes of its model, and holds no entity: it is
    shared by every instance of the model, and each instance has a DeformNode holding the
    entities its own joints became.

    @code
    // An arm of two joints, the second bound one unit up the first
    auto skin = yup::Skin::Ptr (new yup::Skin ("arm", { 0, 1 }, { yup::Matrix4(), yup::Matrix4::translation ({ 0.0f, -1.0f, 0.0f }) }));

    entity.attach<yup::MeshNode> (armMesh);
    entity.attach<yup::DeformNode> (armMesh, skin, std::vector<yup::Entity> { shoulder, elbow });
    @endcode

    GltfModel makes one skin per glTF skin.

    @see DeformNode, Mesh, GltfModel
*/
class YUP_API Skin : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Skin>;

    //==============================================================================
    /** Creates a skin.

        @param skinName             The name of the skin.
        @param jointNodes           The joints, as indices into the nodes of the model: for the
                                    skins of a GltfModel, the indices of the glTF nodes.
        @param inverseBindMatrices  One matrix per joint, in the order of the joints, or none:
                                    every matrix is then the identity, as in glTF, for joints
                                    bound where the mesh stands. Another number asserts, the
                                    matrices missing being the identity too.
    */
    Skin (const String& skinName, std::vector<int> jointNodes, std::vector<Matrix4> inverseBindMatrices = {})
        : name (skinName)
        , joints (std::move (jointNodes))
        , inverseBinds (std::move (inverseBindMatrices))
    {
        jassert (inverseBinds.empty() || inverseBinds.size() == joints.size());

        // One per joint, whatever was given: a new Matrix4 is the identity
        inverseBinds.resize (joints.size());
    }

    //==============================================================================
    /** Returns the name of the skin. */
    const String& getName() const noexcept { return name; }

    /** Returns the number of joints. */
    int getNumJoints() const noexcept { return static_cast<int> (joints.size()); }

    /** Returns the joints, as indices into the nodes of the model the skin belongs to. */
    Span<const int> getJoints() const noexcept { return joints; }

    /** Returns the inverse bind matrix of every joint, in the order of the joints. */
    Span<const Matrix4> getInverseBindMatrices() const noexcept { return inverseBinds; }

private:
    String name;
    std::vector<int> joints;
    std::vector<Matrix4> inverseBinds;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Skin)
};

} // namespace yup
