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
        bool visible = true;

        uint32 parent = EntityHandle::nullIndex;
        std::vector<uint32> children;

        Matrix4 worldMatrix;
        bool worldDirty = true;
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
    bool setParent (uint32 child, uint32 parent, int index);
    void unlink (uint32 child);
    bool isAncestorOf (uint32 ancestor, uint32 index) const noexcept;
    bool isActive (uint32 index, uint32 root) const noexcept;

    //==============================================================================
    void markWorldDirty (uint32 index) noexcept;

    /** Returns the cached world matrix, computing the dirty ones with localMatrixOf (uint32 index). */
    template <class LocalMatrixFn>
    const Matrix4& getWorldMatrix (uint32 index, LocalMatrixFn&& localMatrixOf)
    {
        auto& record = records[index];

        if (record.worldDirty)
        {
            auto world = localMatrixOf (index);

            if (record.parent != EntityHandle::nullIndex)
                world = world.followedBy (getWorldMatrix (record.parent, localMatrixOf));

            // The recursion doesn't add records, so the reference is still valid
            record.worldMatrix = world;
            record.worldDirty = false;
        }

        return record.worldMatrix;
    }

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
    std::vector<Record> records;
    uint64 uuidStreams[2] = {};
    std::vector<uint32> freeList;
    int numEntities = 0;
};

} // namespace yup
