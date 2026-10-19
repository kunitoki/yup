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

#include <gtest/gtest.h>

#include <yup_3d/yup_3d.h>

#include <deque>

using namespace yup;

class EntityTests : public ::testing::Test
{
protected:
    class TaggedNode : public Node
    {
    public:
        TaggedNode() = default;

        explicit TaggedNode (int newTag)
            : tag (newTag)
        {
        }

        void attachedToEntity() override { entityWhenAttached = getEntity(); }

        void detachedFromEntity() override { ++detachCalls; }

        int tag = 0;
        int detachCalls = 0;
        Entity entityWhenAttached;
    };

    class OtherNode : public Node
    {
    };

    class DestructionTracker : public Node
    {
    public:
        explicit DestructionTracker (bool& flagToSet)
            : flag (flagToSet)
        {
        }

        ~DestructionTracker() override { flag = true; }

        bool& flag;
    };

    static Mesh::Ptr makeUnitCubeMesh()
    {
        auto mesh = Mesh::Ptr (new Mesh ("cube"));

        std::vector<Mesh::Vertex> vertices (3);
        vertices[0].position = { -1.0f, -1.0f, -1.0f };
        vertices[1].position = { 1.0f, 1.0f, 1.0f };
        vertices[2].position = { 1.0f, -1.0f, 1.0f };
        mesh->addPrimitive (vertices, {});

        return mesh;
    }

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    /** Returns a flag for a DestructionTracker, alive until the scene destroys its nodes. */
    bool& makeFlag()
    {
        return flags.emplace_back (false);
    }

    std::deque<bool> flags; // destroyed after the scene, which may destroy the trackers
    Scene scene;
};

//==============================================================================

TEST_F (EntityTests, DefaultEntityIsInvalid)
{
    const Entity entity;

    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (static_cast<bool> (entity));
    EXPECT_EQ (entity.getScene(), nullptr);
    EXPECT_TRUE (entity.getHandle().isNull());
    EXPECT_EQ (entity.get<TaggedNode>(), nullptr);
    EXPECT_FALSE (entity.has<TaggedNode>());
}

TEST_F (EntityTests, NewEntityIsAVisibleChildOfTheRoot)
{
    const auto entity = scene.createEntity ("group");

    EXPECT_TRUE (entity.isValid());
    EXPECT_EQ (entity.getScene(), &scene);
    EXPECT_EQ (entity.getName(), "group");
    EXPECT_TRUE (entity.isVisible());
    EXPECT_TRUE (entity.isActive());
    EXPECT_EQ (entity.getParent(), scene.getRoot());
    EXPECT_EQ (entity.getNumChildren(), 0);
    EXPECT_TRUE (entity.getLocalMatrix().isIdentity());
}

TEST_F (EntityTests, CopiesReferToTheSameEntity)
{
    const auto entity = scene.createEntity();
    auto copy = entity;

    copy.setName ("renamed");

    EXPECT_EQ (copy, entity);
    EXPECT_EQ (entity.getName(), "renamed");
}

TEST_F (EntityTests, AddChildAppendsAndInserts)
{
    auto parent = scene.createEntity();
    const auto a = scene.createEntity ("a");
    const auto b = scene.createEntity ("b");
    const auto c = scene.createEntity ("c");

    EXPECT_TRUE (parent.addChild (a));
    EXPECT_TRUE (parent.addChild (c));
    EXPECT_TRUE (parent.addChild (b, 1));

    ASSERT_EQ (parent.getNumChildren(), 3);
    EXPECT_EQ (parent.getChild (0), a);
    EXPECT_EQ (parent.getChild (1), b);
    EXPECT_EQ (parent.getChild (2), c);
    EXPECT_FALSE (parent.getChild (3).isValid());
    EXPECT_EQ (b.getParent(), parent);

    std::vector<String> names;
    for (const auto child : parent.getChildren())
        names.push_back (child.getName());

    EXPECT_EQ (names, (std::vector<String> { "a", "b", "c" }));
}

TEST_F (EntityTests, CreateChildAddsItLast)
{
    auto parent = scene.createEntity();
    parent.createChild ("first");
    const auto second = parent.createChild ("second");

    EXPECT_EQ (parent.getChild (1), second);
    EXPECT_EQ (second.getParent(), parent);
}

TEST_F (EntityTests, AddingAnExistingChildMovesIt)
{
    auto first = scene.createEntity();
    auto second = scene.createEntity();
    const auto child = first.createChild();

    EXPECT_TRUE (second.addChild (child));

    EXPECT_EQ (first.getNumChildren(), 0);
    EXPECT_EQ (child.getParent(), second);
}

TEST_F (EntityTests, RemoveChildAndRemoveFromParent)
{
    auto parent = scene.createEntity();
    auto a = parent.createChild();
    const auto b = parent.createChild();

    EXPECT_TRUE (parent.removeChild (b));
    EXPECT_FALSE (parent.removeChild (b));
    EXPECT_FALSE (b.getParent().isValid());
    EXPECT_TRUE (b.isValid());
    EXPECT_FALSE (b.isActive());

    a.removeFromParent();
    EXPECT_EQ (parent.getNumChildren(), 0);
    EXPECT_TRUE (scene.getRoot().addChild (a));
    EXPECT_TRUE (a.isActive());
}

TEST_F (EntityTests, IsAncestorOfFollowsTheHierarchy)
{
    auto parent = scene.createEntity();
    auto child = parent.createChild();
    const auto grandChild = child.createChild();

    EXPECT_TRUE (parent.isAncestorOf (grandChild));
    EXPECT_TRUE (scene.getRoot().isAncestorOf (grandChild));
    EXPECT_FALSE (grandChild.isAncestorOf (parent));
    EXPECT_FALSE (parent.isAncestorOf (parent));
}

// Disabled: the rejected calls hit a jassert in debug builds
TEST_F (EntityTests, DISABLED_CyclesAreRejected)
{
    auto parent = scene.createEntity();
    auto child = parent.createChild();

    EXPECT_FALSE (child.addChild (parent));
    EXPECT_FALSE (child.addChild (child));
    EXPECT_FALSE (child.addChild (scene.getRoot()));
    EXPECT_EQ (child.getParent(), parent);
}

TEST_F (EntityTests, FindChildSearchesDirectOrRecursive)
{
    auto parent = scene.createEntity();
    auto child = parent.createChild ("child");
    const auto deep = child.createChild ("deep");

    EXPECT_EQ (parent.findChild ("child"), child);
    EXPECT_EQ (parent.findChild ("deep"), deep);
    EXPECT_FALSE (parent.findChild ("deep", false).isValid());
    EXPECT_FALSE (parent.findChild ("missing").isValid());
}

TEST_F (EntityTests, HiddenAncestorsMakeEntitiesInactive)
{
    auto parent = scene.createEntity();
    const auto child = parent.createChild();

    parent.setVisible (false);

    EXPECT_TRUE (child.isVisible());
    EXPECT_FALSE (child.isActive());
}

//==============================================================================

TEST_F (EntityTests, AttachGetHasAndDetach)
{
    auto entity = scene.createEntity();

    auto& node = entity.attach<TaggedNode> (7);

    EXPECT_EQ (entity.get<TaggedNode>(), &node);
    EXPECT_TRUE (entity.has<TaggedNode>());
    EXPECT_FALSE (entity.has<OtherNode>());
    EXPECT_EQ (node.tag, 7);
    EXPECT_EQ (node.getEntity(), entity);
    EXPECT_EQ (node.entityWhenAttached, entity);

    EXPECT_TRUE (entity.detach<TaggedNode>());
    EXPECT_FALSE (entity.detach<TaggedNode>());
    EXPECT_FALSE (entity.has<TaggedNode>());
}

TEST_F (EntityTests, NodesMatchTheExactTypeOnly)
{
    class DerivedNode : public TaggedNode
    {
    };

    auto entity = scene.createEntity();
    entity.attach<DerivedNode>();

    EXPECT_FALSE (entity.has<TaggedNode>());
    EXPECT_TRUE (entity.has<DerivedNode>());
}

TEST_F (EntityTests, AttachReplacesTheNodeOfTheSameType)
{
    auto entity = scene.createEntity();
    bool& destroyed = makeFlag();
    entity.attach<DestructionTracker> (destroyed);

    bool& replacementDestroyed = makeFlag();
    auto& replacement = entity.attach<DestructionTracker> (replacementDestroyed);

    EXPECT_TRUE (destroyed);
    EXPECT_FALSE (replacementDestroyed);
    EXPECT_EQ (entity.get<DestructionTracker>(), &replacement);
}

TEST_F (EntityTests, GetOrAttachAttachesOnlyWhenMissing)
{
    auto entity = scene.createEntity();
    bool& destroyed = makeFlag();

    auto& attached = entity.getOrAttach<DestructionTracker> (destroyed);
    EXPECT_EQ (entity.get<DestructionTracker>(), &attached);

    bool& unused = makeFlag();
    auto& existing = entity.getOrAttach<DestructionTracker> (unused);

    EXPECT_EQ (&existing, &attached);
    EXPECT_FALSE (destroyed);
}

TEST_F (EntityTests, DetachCallsTheNodeBackAndDestroysIt)
{
    auto entity = scene.createEntity();
    bool& destroyed = makeFlag();
    entity.attach<DestructionTracker> (destroyed);

    entity.detach<DestructionTracker>();

    EXPECT_TRUE (destroyed);
}

TEST_F (EntityTests, NodeAddressesStayStableAcrossOtherAttachments)
{
    auto entity = scene.createEntity();
    auto& node = entity.attach<TaggedNode> (1);

    std::vector<Entity> others;
    for (int i = 0; i < 500; ++i)
    {
        others.push_back (scene.createEntity());
        others.back().attach<TaggedNode> (i);
    }

    for (int i = 0; i < 500; i += 2)
        others[static_cast<size_t> (i)].detach<TaggedNode>();

    EXPECT_EQ (entity.get<TaggedNode>(), &node);
    EXPECT_EQ (node.tag, 1);
}

TEST_F (EntityTests, ForEachAttachedNodeVisitsEveryType)
{
    auto entity = scene.createEntity();
    entity.attach<TaggedNode>();
    entity.attach<OtherNode>();
    scene.createEntity().attach<MeshNode>();

    int count = 0;
    entity.forEachAttachedNode ([&] (Node& node)
    {
        EXPECT_EQ (node.getEntity(), entity);
        ++count;
    });

    EXPECT_EQ (count, 2);
}

//==============================================================================

TEST_F (EntityTests, DestroyRemovesTheSubtreeAndItsNodes)
{
    auto parent = scene.createEntity();
    auto child = parent.createChild();
    bool& parentNodeDestroyed = makeFlag();
    bool& childNodeDestroyed = makeFlag();
    parent.attach<DestructionTracker> (parentNodeDestroyed);
    auto& childNode = child.attach<TaggedNode>();
    child.attach<DestructionTracker> (childNodeDestroyed);
    const auto entitiesBefore = scene.getNumEntities();

    parent.destroy();

    EXPECT_FALSE (parent.isValid());
    EXPECT_FALSE (child.isValid());
    EXPECT_TRUE (parentNodeDestroyed);
    EXPECT_TRUE (childNodeDestroyed);
    EXPECT_EQ (scene.getNumEntities(), entitiesBefore - 2);
    EXPECT_EQ (scene.getRoot().getNumChildren(), 0);
    (void) childNode;
}

TEST_F (EntityTests, StaleEntitiesStayInvalidWhenTheIndexIsReused)
{
    auto entity = scene.createEntity();
    entity.attach<TaggedNode>();
    const auto stale = entity;
    entity.destroy();

    auto reused = scene.createEntity();
    reused.attach<TaggedNode>();

    EXPECT_EQ (reused.getHandle().index, stale.getHandle().index);
    EXPECT_NE (reused.getHandle().generation, stale.getHandle().generation);
    EXPECT_FALSE (stale.isValid());
    EXPECT_EQ (stale.get<TaggedNode>(), nullptr);
    EXPECT_NE (reused, stale);
}

//==============================================================================

TEST_F (EntityTests, LocalMatrixComesFromTheTransformNode)
{
    auto entity = scene.createEntity();
    EXPECT_TRUE (entity.getLocalMatrix().isIdentity());

    entity.attach<TransformNode> (Vector3<float> { 1.0f, 2.0f, 3.0f });
    expectNear (entity.getLocalMatrix().transformPoint ({}), { 1.0f, 2.0f, 3.0f });

    entity.detach<TransformNode>();
    EXPECT_TRUE (entity.getLocalMatrix().isIdentity());
}

TEST_F (EntityTests, WorldMatrixAccumulatesParents)
{
    auto root = scene.createEntity();
    auto child = root.createChild();
    auto grandChild = child.createChild();

    root.attach<TransformNode> (Vector3<float> { 0.0f, 0.0f, 5.0f });
    child.attach<TransformNode> (Vector3<float> {}, Quaternion(), Vector3<float> { 2.0f, 2.0f, 2.0f });
    grandChild.attach<TransformNode> (Vector3<float> { 1.0f, 0.0f, 0.0f });

    expectNear (grandChild.getWorldMatrix().transformPoint ({}), { 2.0f, 0.0f, 5.0f });
}

TEST_F (EntityTests, EntitiesWithoutTransformFollowTheirParent)
{
    auto parent = scene.createEntity();
    auto group = parent.createChild();
    auto leaf = group.createChild();
    parent.attach<TransformNode> (Vector3<float> { 3.0f, 0.0f, 0.0f });

    expectNear (leaf.getWorldMatrix().transformPoint ({}), { 3.0f, 0.0f, 0.0f });
}

TEST_F (EntityTests, WorldMatrixFollowsChangesAboveIt)
{
    auto parent = scene.createEntity();
    auto child = parent.createChild();
    auto& transform = parent.attach<TransformNode>();
    child.attach<TransformNode> (Vector3<float> { 1.0f, 0.0f, 0.0f });

    expectNear (child.getWorldMatrix().transformPoint ({}), { 1.0f, 0.0f, 0.0f });

    transform.setPosition ({ 0.0f, 10.0f, 0.0f });
    expectNear (child.getWorldMatrix().transformPoint ({}), { 1.0f, 10.0f, 0.0f });

    parent.detach<TransformNode>();
    expectNear (child.getWorldMatrix().transformPoint ({}), { 1.0f, 0.0f, 0.0f });

    auto other = scene.createEntity();
    other.attach<TransformNode> (Vector3<float> { 0.0f, 0.0f, -4.0f });
    other.addChild (child);
    expectNear (child.getWorldMatrix().transformPoint ({}), { 1.0f, 0.0f, -4.0f });
}

TEST_F (EntityTests, ComputeWorldBoundsAppliesNestedTransforms)
{
    auto root = scene.createEntity();
    auto child = root.createChild();
    root.attach<TransformNode> (Vector3<float> { 10.0f, 0.0f, 0.0f });
    child.attach<TransformNode> (Vector3<float> {}, Quaternion(), Vector3<float> { 1.0f, 2.0f, 3.0f });
    child.attach<MeshNode> (makeUnitCubeMesh());

    const auto bounds = root.computeWorldBounds();

    expectNear (bounds.getMin(), { 9.0f, -2.0f, -3.0f });
    expectNear (bounds.getMax(), { 11.0f, 2.0f, 3.0f });
}

TEST_F (EntityTests, ComputeWorldBoundsSkipsHiddenSubtrees)
{
    auto root = scene.createEntity();
    auto visibleChild = root.createChild();
    auto hiddenChild = root.createChild();
    auto hiddenGrandChild = hiddenChild.createChild();

    visibleChild.attach<MeshNode> (makeUnitCubeMesh());
    hiddenGrandChild.attach<MeshNode> (makeUnitCubeMesh());
    hiddenGrandChild.attach<TransformNode> (Vector3<float> { 100.0f, 0.0f, 0.0f });
    hiddenChild.setVisible (false);

    expectNear (root.computeWorldBounds().getMax(), { 1.0f, 1.0f, 1.0f });

    root.setVisible (false);
    EXPECT_TRUE (root.computeWorldBounds().isEmpty());
}

//==============================================================================

TEST_F (EntityTests, UuidIsCreatedOnceAndStaysTheSame)
{
    auto entity = scene.createEntity();

    const auto uuid = entity.getUuid();

    EXPECT_FALSE (uuid.isNull());
    EXPECT_EQ (uuid.getTimeHighAndVersion() >> 12, 4);
    EXPECT_EQ (uuid.getClockSeqAndReserved() >> 6, 2);
    EXPECT_EQ (entity.getUuid(), uuid);
    EXPECT_EQ (Entity (entity).getUuid(), uuid);
}

TEST_F (EntityTests, EntitiesHaveDifferentUuids)
{
    const auto first = scene.createEntity();
    const auto second = scene.createEntity();

    EXPECT_NE (first.getUuid(), second.getUuid());
    EXPECT_NE (scene.getRoot().getUuid(), first.getUuid());
}

TEST_F (EntityTests, FindEntityReturnsTheEntityWithTheUuid)
{
    const auto entity = scene.createEntity();
    const auto uuid = entity.getUuid();

    EXPECT_EQ (scene.findEntity (uuid), entity);
    EXPECT_FALSE (scene.findEntity (Uuid()).isValid());
    EXPECT_FALSE (scene.findEntity (Uuid::null()).isValid());
}

TEST_F (EntityTests, SetUuidRestoresASavedIdentifier)
{
    auto entity = scene.createEntity();
    const auto previous = entity.getUuid();
    const Uuid saved ("8b1f6a8e-2c4d-4e5f-9a0b-1c2d3e4f5a6b");

    EXPECT_TRUE (entity.setUuid (saved));

    EXPECT_EQ (entity.getUuid(), saved);
    EXPECT_EQ (scene.findEntity (saved), entity);
    EXPECT_FALSE (scene.findEntity (previous).isValid());
}

TEST_F (EntityTests, SetUuidToTheCurrentOneSucceeds)
{
    auto entity = scene.createEntity();

    EXPECT_TRUE (entity.setUuid (entity.getUuid()));
}

TEST_F (EntityTests, DISABLED_SetUuidRejectsAnIdentifierUsedByAnotherEntity)
{
    const auto first = scene.createEntity();
    auto second = scene.createEntity();
    const auto secondUuid = second.getUuid();

    EXPECT_FALSE (second.setUuid (first.getUuid()));
    EXPECT_FALSE (second.setUuid (Uuid::null()));
    EXPECT_EQ (second.getUuid(), secondUuid);
}

TEST_F (EntityTests, DestroyedEntitiesCanNoLongerBeFound)
{
    auto parent = scene.createEntity();
    const auto child = parent.createChild();
    const auto parentUuid = parent.getUuid();
    const auto childUuid = child.getUuid();

    parent.destroy();

    EXPECT_FALSE (scene.findEntity (parentUuid).isValid());
    EXPECT_FALSE (scene.findEntity (childUuid).isValid());
}

TEST_F (EntityTests, ReusedSlotsGetANewUuid)
{
    auto entity = scene.createEntity();
    const auto uuid = entity.getUuid();
    entity.destroy();

    const auto reused = scene.createEntity();

    EXPECT_EQ (reused.getHandle().index, entity.getHandle().index);
    EXPECT_NE (reused.getUuid(), uuid);
}

TEST_F (EntityTests, CreateEntityRestoresTheUuid)
{
    const Uuid saved ("8b1f6a8e-2c4d-4e5f-9a0b-1c2d3e4f5a6b");

    const auto entity = scene.createEntity ("loaded", {}, saved);

    EXPECT_EQ (entity.getUuid(), saved);
    EXPECT_EQ (scene.findEntity (saved), entity);
    EXPECT_EQ (entity.getName(), "loaded");
}

TEST_F (EntityTests, SameUuidCanLiveInDifferentScenes)
{
    Scene other;
    const auto entity = scene.createEntity();

    const auto copy = other.createEntity ({}, {}, entity.getUuid());

    EXPECT_EQ (copy.getUuid(), entity.getUuid());
    EXPECT_EQ (other.findEntity (entity.getUuid()), copy);
}
