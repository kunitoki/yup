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
/** The entities of a Scene: names, visibility, hierarchy and cached world matrices, indexed by slot.

    The data read every frame (world matrices, transforms and flags) lives in dense arrays
    next to the records, which hold the rest.

    Internal to the Scene, use Entity instead.
*/
class EntityStore
{
public:
    //==============================================================================
    struct Record
    {
        uint32 generation = 0;
        bool alive = false;

        String name;
        Uuid uuid = Uuid::null();

        uint32 parent = EntityHandle::nullIndex;
        std::vector<uint32> children;
    };

    //==============================================================================
    EntityStore();

    //==============================================================================
    EntityHandle create (const String& name);
    void release (uint32 index);

    Record* find (EntityHandle entity) noexcept;
    const Record* find (EntityHandle entity) const noexcept;
    Record& get (uint32 index) noexcept { return records[index]; }
    EntityHandle getHandle (uint32 index) const noexcept;
    int getNumEntities() const noexcept { return numEntities; }

    //==============================================================================
    /** Sets the entity that is active when visible, without a parent. Set once by the Scene. */
    void setRoot (uint32 index) noexcept { root = index; }

    bool setParent (uint32 child, uint32 parent, int index);
    void unlink (uint32 child);
    bool isAncestorOf (uint32 ancestor, uint32 index) const noexcept;

    bool isVisible (uint32 index) const noexcept { return (flags[index] & visibleFlag) != 0; }
    void setVisible (uint32 index, bool shouldBeVisible) noexcept;

    /** Returns true if the entity and all its parents are visible, up to the root. Cached. */
    bool isActive (uint32 index) noexcept;

    //==============================================================================
    /** Sets the TransformNode giving the local matrix of an entity, or nullptr for the identity. */
    void setTransform (uint32 index, const TransformNode* transform) noexcept;

    void markWorldDirty (uint32 index) noexcept;

    /** Returns the cached world matrix, computing it and its dirty parents first. */
    const Matrix4& getWorldMatrix (uint32 index);

    //==============================================================================
    /** Returns a new random (version 4) Uuid from fast streams seeded once by SecureRandom.

        Unique, not secret: the next identifiers can be predicted from the previous ones.
    */
    Uuid createUuid() noexcept;

    /** Returns a (version 8) Uuid derived from another one and a type id, the same for the same
        inputs. For one type id, different inputs never give the same result.
    */
    static Uuid deriveUuid (const Uuid& uuid, uint64 typeId) noexcept;

private:
    enum : uint8
    {
        visibleFlag = 1 << 0,
        worldDirtyFlag = 1 << 1,
        activeDirtyFlag = 1 << 2,
        activeFlag = 1 << 3
    };

    template <uint8 dirtyFlag>
    void markSubtree (uint32 index) noexcept;

    std::vector<Record> records;
    std::vector<Matrix4> worldMatrices;
    std::vector<const TransformNode*> transforms;
    std::vector<uint8> flags;
    std::vector<uint32> pendingChain;
    uint32 root = EntityHandle::nullIndex;
    uint64 uuidStreams[2] = {};
    std::vector<uint32> freeList;
    int numEntities = 0;
};

} // namespace yup
