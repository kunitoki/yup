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

        int tag = 0;
        Entity* entityWhenAttached = nullptr;
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

    static Entity::Ptr makeEntity (const String& name = {})
    {
        return Entity::Ptr (new Entity (name));
    }

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
};

//==============================================================================

TEST_F (EntityTests, NewEntityIsAVisibleRootGroup)
{
    auto entity = makeEntity ("root");

    EXPECT_EQ (entity->getName(), "root");
    EXPECT_TRUE (entity->isVisible());
    EXPECT_EQ (entity->getParent(), nullptr);
    EXPECT_EQ (entity->getNumChildren(), 0);
    EXPECT_EQ (entity->getNumNodes<TaggedNode>(), 0);
    EXPECT_TRUE (entity->getLocalMatrix().isIdentity());
}

TEST_F (EntityTests, AddChildAppendsAndInserts)
{
    auto root = makeEntity();
    auto a = makeEntity ("a");
    auto b = makeEntity ("b");
    auto c = makeEntity ("c");

    root->addChild (a);
    root->addChild (b);
    root->addChild (c, 0);

    ASSERT_EQ (root->getNumChildren(), 3);
    EXPECT_EQ (root->getChild (0), c.get());
    EXPECT_EQ (root->getChild (1), a.get());
    EXPECT_EQ (root->getChild (2), b.get());
    EXPECT_EQ (root->getChild (3), nullptr);
    EXPECT_EQ (root->getChild (-1), nullptr);
    EXPECT_EQ (a->getParent(), root.get());

    const auto children = root->getChildren();
    ASSERT_EQ (children.size(), 3u);
    EXPECT_EQ (children[1].get(), a.get());
}

TEST_F (EntityTests, RemoveChildAndRemoveFromParent)
{
    auto root = makeEntity();
    auto a = makeEntity ("a");
    auto b = makeEntity ("b");
    root->addChild (a);
    root->addChild (b);

    EXPECT_TRUE (root->removeChild (a.get()));
    EXPECT_FALSE (root->removeChild (a.get()));
    EXPECT_EQ (a->getParent(), nullptr);
    EXPECT_EQ (root->getNumChildren(), 1);

    b->removeFromParent();
    EXPECT_EQ (b->getParent(), nullptr);
    EXPECT_EQ (root->getNumChildren(), 0);

    b->removeFromParent();
    EXPECT_EQ (b->getParent(), nullptr);
}

TEST_F (EntityTests, AddingAChildWithAParentReparentsIt)
{
    auto first = makeEntity();
    auto second = makeEntity();
    auto child = makeEntity ("child");

    first->addChild (child);
    second->addChild (child);

    EXPECT_EQ (first->getNumChildren(), 0);
    EXPECT_EQ (second->getNumChildren(), 1);
    EXPECT_EQ (child->getParent(), second.get());
}

TEST_F (EntityTests, AddingAnExistingChildAgainMovesIt)
{
    auto root = makeEntity();
    auto a = makeEntity ("a");
    auto b = makeEntity ("b");
    root->addChild (a);
    root->addChild (b);

    root->addChild (a);

    ASSERT_EQ (root->getNumChildren(), 2);
    EXPECT_EQ (root->getChild (0), b.get());
    EXPECT_EQ (root->getChild (1), a.get());
}

TEST_F (EntityTests, DISABLED_CyclesAreRejected)
{
    auto root = makeEntity();
    auto child = makeEntity();
    auto grandChild = makeEntity();
    root->addChild (child);
    child->addChild (grandChild);

    EXPECT_TRUE (root->isAncestorOf (grandChild.get()));
    EXPECT_FALSE (grandChild->isAncestorOf (root.get()));

#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Adding a cycle triggers an assertion when assertions are enabled or logged";
#endif

    grandChild->addChild (root);
    root->addChild (root);

    EXPECT_EQ (root->getParent(), nullptr);
    EXPECT_EQ (grandChild->getNumChildren(), 0);
    EXPECT_EQ (root->getNumChildren(), 1);
}

TEST_F (EntityTests, ChildAddressesStayStableWhenSiblingsChange)
{
    auto root = makeEntity();
    std::vector<Entity*> addresses;

    for (int i = 0; i < 64; ++i)
    {
        auto child = makeEntity (String (i));
        addresses.push_back (child.get());
        root->addChild (child, 0);
    }

    root->removeChild (addresses[10]);

    for (int i = 0; i < 64; ++i)
    {
        if (i != 10)
            EXPECT_EQ (root->findChild (String (i), false), addresses[(size_t) i]);
    }
}

TEST_F (EntityTests, ChildrenOutliveTheirParentWhenReferenced)
{
    auto child = makeEntity();

    {
        auto parent = makeEntity();
        parent->addChild (child);
    }

    EXPECT_EQ (child->getParent(), nullptr);
}

TEST_F (EntityTests, FindChildSearchesDirectOrRecursive)
{
    auto root = makeEntity();
    auto a = makeEntity ("a");
    auto deep = makeEntity ("deep");
    root->addChild (a);
    a->addChild (deep);

    EXPECT_EQ (root->findChild ("a"), a.get());
    EXPECT_EQ (root->findChild ("deep"), deep.get());
    EXPECT_EQ (root->findChild ("deep", false), nullptr);
    EXPECT_EQ (root->findChild ("missing"), nullptr);
}

//==============================================================================

TEST_F (EntityTests, AttachAndGetBySlot)
{
    auto entity = makeEntity();

    auto& first = entity->attach<TaggedNode> (1);
    auto& second = entity->attach<TaggedNode, 2> (3);

    EXPECT_EQ (entity->getNode<TaggedNode>(), &first);
    EXPECT_EQ ((entity->getNode<TaggedNode, 2>()), &second);
    EXPECT_EQ ((entity->getNode<TaggedNode, 1>()), nullptr);
    EXPECT_EQ (entity->getNodeAt<TaggedNode> (2), &second);
    EXPECT_EQ (entity->getNumNodes<TaggedNode>(), 2);
    EXPECT_EQ (first.getEntity(), entity.get());
    EXPECT_EQ (first.entityWhenAttached, entity.get());
    EXPECT_EQ (second.tag, 3);
}

TEST_F (EntityTests, PartsMatchTheExactTypeOnly)
{
    auto entity = makeEntity();
    entity->attach<TaggedNode>();

    EXPECT_EQ (entity->getNode<OtherNode>(), nullptr);
    EXPECT_EQ (entity->getNumNodes<OtherNode>(), 0);

    auto& other = entity->attach<OtherNode>();
    EXPECT_EQ (entity->getNode<OtherNode>(), &other);
    EXPECT_NE (static_cast<Node*> (entity->getNode<TaggedNode>()), static_cast<Node*> (&other));
}

TEST_F (EntityTests, RuntimeSlotFormsMatchTemplateForms)
{
    auto entity = makeEntity();

    auto& node = entity->attachAt<TaggedNode> (5, 42);

    EXPECT_EQ ((entity->getNode<TaggedNode, 5>()), &node);
    EXPECT_TRUE ((entity->detach<TaggedNode, 5>()));
    EXPECT_EQ (entity->getNodeAt<TaggedNode> (5), nullptr);

    entity->attach<TaggedNode, 7>();
    EXPECT_TRUE (entity->detachAt<TaggedNode> (7));
    EXPECT_FALSE (entity->detachAt<TaggedNode> (7));
}

TEST_F (EntityTests, AttachReplacesThePartInTheSameSlot)
{
    auto entity = makeEntity();
    bool firstDestroyed = false;

    entity->attach<DestructionTracker> (firstDestroyed);
    EXPECT_FALSE (firstDestroyed);

    bool secondDestroyed = false;
    auto& second = entity->attach<DestructionTracker> (secondDestroyed);

    EXPECT_TRUE (firstDestroyed);
    EXPECT_FALSE (secondDestroyed);
    EXPECT_EQ (entity->getNode<DestructionTracker>(), &second);
    EXPECT_EQ (entity->getNumNodes<DestructionTracker>(), 1);
}

TEST_F (EntityTests, DetachDestroysThePart)
{
    auto entity = makeEntity();
    bool destroyed = false;
    entity->attach<DestructionTracker> (destroyed);

    EXPECT_TRUE (entity->detach<DestructionTracker>());
    EXPECT_TRUE (destroyed);
    EXPECT_FALSE (entity->detach<DestructionTracker>());
}

TEST_F (EntityTests, PartAddressesStayStable)
{
    auto entity = makeEntity();
    auto& node = entity->attach<TaggedNode, 3>();

    for (int slot = 0; slot < 32; ++slot)
    {
        if (slot != 3)
            entity->attachAt<TaggedNode> (slot);

        entity->attachAt<OtherNode> (slot);
    }

    EXPECT_EQ ((entity->getNode<TaggedNode, 3>()), &node);
}

TEST_F (EntityTests, ForEachNodeVisitsSlotsInAscendingOrder)
{
    auto entity = makeEntity();
    entity->attach<TaggedNode, 4> (40);
    entity->attach<OtherNode>();
    entity->attach<TaggedNode, 0> (0);
    entity->attach<TaggedNode, 2> (20);

    std::vector<int> slots;
    std::vector<int> tags;
    entity->forEachNode<TaggedNode> ([&] (TaggedNode& node, int slot)
    {
        slots.push_back (slot);
        tags.push_back (node.tag);
    });

    EXPECT_EQ (slots, (std::vector<int> { 0, 2, 4 }));
    EXPECT_EQ (tags, (std::vector<int> { 0, 20, 40 }));

    int all = 0;
    entity->forEachAttachedNode ([&] (Node&) { ++all; });
    EXPECT_EQ (all, 4);
}

TEST_F (EntityTests, PartsAreDestroyedWithTheEntity)
{
    bool destroyed = false;

    {
        auto entity = makeEntity();
        entity->attach<DestructionTracker> (destroyed);
    }

    EXPECT_TRUE (destroyed);
}

//==============================================================================

TEST_F (EntityTests, LocalMatrixAppliesScaleThenRotationThenTranslation)
{
    auto entity = makeEntity();
    entity->setScale ({ 2.0f, 2.0f, 2.0f });
    entity->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi));
    entity->setPosition ({ 10.0f, 0.0f, 0.0f });

    // (1, 0, 0) -> scaled (2, 0, 0) -> rotated (0, 2, 0) -> translated (10, 2, 0)
    expectNear (entity->getLocalMatrix().transformPoint ({ 1.0f, 0.0f, 0.0f }), { 10.0f, 2.0f, 0.0f });

    expectNear (entity->getPosition(), { 10.0f, 0.0f, 0.0f });
    expectNear (entity->getScale(), { 2.0f, 2.0f, 2.0f });
}

TEST_F (EntityTests, LocalMatrixFollowsTransformChanges)
{
    auto entity = makeEntity();
    entity->setPosition ({ 1.0f, 0.0f, 0.0f });
    expectNear (entity->getLocalMatrix().transformPoint ({}), { 1.0f, 0.0f, 0.0f });

    entity->setPosition ({ 0.0f, 3.0f, 0.0f });
    expectNear (entity->getLocalMatrix().transformPoint ({}), { 0.0f, 3.0f, 0.0f });
}

TEST_F (EntityTests, WorldMatrixAccumulatesParents)
{
    auto root = makeEntity();
    auto child = makeEntity();
    auto grandChild = makeEntity();
    root->addChild (child);
    child->addChild (grandChild);

    root->setPosition ({ 0.0f, 0.0f, 5.0f });
    child->setScale ({ 2.0f, 2.0f, 2.0f });
    grandChild->setPosition ({ 1.0f, 0.0f, 0.0f });

    expectNear (grandChild->getWorldMatrix().transformPoint ({}), { 2.0f, 0.0f, 5.0f });
}

TEST_F (EntityTests, ComputeWorldBoundsAppliesNestedTransforms)
{
    auto root = makeEntity();
    auto child = makeEntity();
    root->addChild (child);
    root->setPosition ({ 10.0f, 0.0f, 0.0f });
    child->setScale ({ 1.0f, 2.0f, 3.0f });
    child->attach<MeshNode> (makeUnitCubeMesh());

    const auto bounds = root->computeWorldBounds();

    expectNear (bounds.getMin(), { 9.0f, -2.0f, -3.0f });
    expectNear (bounds.getMax(), { 11.0f, 2.0f, 3.0f });
}

TEST_F (EntityTests, ComputeWorldBoundsSkipsHiddenSubtrees)
{
    auto root = makeEntity();
    auto visibleChild = makeEntity();
    auto hiddenChild = makeEntity();
    auto hiddenGrandChild = makeEntity();
    root->addChild (visibleChild);
    root->addChild (hiddenChild);
    hiddenChild->addChild (hiddenGrandChild);

    visibleChild->attach<MeshNode> (makeUnitCubeMesh());
    hiddenGrandChild->attach<MeshNode> (makeUnitCubeMesh());
    hiddenGrandChild->setPosition ({ 100.0f, 0.0f, 0.0f });
    hiddenChild->setVisible (false);

    const auto bounds = root->computeWorldBounds();
    expectNear (bounds.getMax(), { 1.0f, 1.0f, 1.0f });

    root->setVisible (false);
    EXPECT_TRUE (root->computeWorldBounds().isEmpty());
}

TEST_F (EntityTests, ComputeWorldBoundsIncludesEveryMeshSlot)
{
    auto entity = makeEntity();
    entity->attach<MeshNode> (makeUnitCubeMesh());

    auto farMesh = Mesh::Ptr (new Mesh());
    std::vector<Mesh::Vertex> vertices (3);
    vertices[0].position = { 5.0f, 0.0f, 0.0f };
    vertices[1].position = { 6.0f, 0.0f, 0.0f };
    vertices[2].position = { 5.0f, 1.0f, 0.0f };
    farMesh->addPrimitive (vertices, {});
    entity->attach<MeshNode, 1> (farMesh);
    entity->attach<MeshNode, 2>();

    expectNear (entity->computeWorldBounds().getMax(), { 6.0f, 1.0f, 1.0f });
}
