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

class SceneTests : public ::testing::Test
{
protected:
    class CountingNode : public Node
    {
    public:
        void update (double deltaSeconds) override
        {
            ++updates;
            totalTime += deltaSeconds;
        }

        int updates = 0;
        double totalTime = 0.0;
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

    template <int id>
    class LoggingSystem : public System
    {
    public:
        explicit LoggingSystem (std::vector<int>& logToUse)
            : log (logToUse)
        {
        }

        void update (Scene&, double) override { log.push_back (id); }

        std::vector<int>& log;
    };

    template <int id>
    class LoggingNode : public Node
    {
    public:
        explicit LoggingNode (std::vector<int>& logToUse)
            : log (logToUse)
        {
        }

        void update (double) override { log.push_back (id); }

        std::vector<int>& log;
    };

    class CallbackSystem : public System
    {
    public:
        explicit CallbackSystem (std::function<void (Scene&)> fn)
            : callback (std::move (fn))
        {
        }

        void update (Scene& scene, double) override { callback (scene); }

        std::function<void (Scene&)> callback;
    };

    std::vector<int> runLog()
    {
        log.clear();
        scene->update (0.0);
        return log;
    }

    /** Adds a child with a cube mesh spanning [-1, 1] around a position. */
    Entity addCube (const String& name, const Vector3<float>& position, Entity parent = {})
    {
        auto mesh = Mesh::Ptr (new Mesh (name));

        // One triangle whose bounds span the cube
        std::vector<Mesh::Vertex> vertices (3);
        vertices[0].position = { -1.0f, -1.0f, -1.0f };
        vertices[1].position = { 1.0f, 1.0f, 1.0f };
        vertices[2].position = { 1.0f, -1.0f, 1.0f };
        mesh->addPrimitive (vertices, {});

        auto entity = scene->createEntity (name, parent);
        entity.attach<MeshNode>().mesh = mesh;
        entity.attach<TransformNode> (position);
        return entity;
    }

    Scene::Ptr scene { new Scene() };
    std::vector<int> log;
};

//==============================================================================

TEST_F (SceneTests, NewSceneHasARootAndNoCamera)
{
    const auto root = scene->getRoot();

    EXPECT_TRUE (root.isValid());
    EXPECT_EQ (root.getName(), "root");
    EXPECT_FALSE (root.getParent().isValid());
    EXPECT_EQ (root.getNumChildren(), 0);
    EXPECT_EQ (scene->getNumEntities(), 1);
    EXPECT_EQ (scene->getActiveCamera(), nullptr);
    EXPECT_FALSE (scene->isIterating());
}

TEST_F (SceneTests, CreateEntityAddsUnderTheRootOrTheParent)
{
    const auto parent = scene->createEntity ("parent");
    const auto child = scene->createEntity ("child", parent);

    EXPECT_EQ (parent.getParent(), scene->getRoot());
    EXPECT_EQ (child.getParent(), parent);
    EXPECT_EQ (scene->getNumEntities(), 3);
}

TEST_F (SceneTests, LightsDoNotCastShadowsByDefault)
{
    const LightNode light;
    EXPECT_FALSE (light.castsShadows);
}

//==============================================================================

TEST_F (SceneTests, UpdateTicksNodesOfActiveEntities)
{
    auto parent = scene->createEntity();
    auto child = parent.createChild();
    auto hidden = scene->createEntity();

    auto& rootNode = scene->getRoot().attach<CountingNode>();
    auto& childNode = child.attach<CountingNode>();
    auto& hiddenNode = hidden.attach<CountingNode>();
    hidden.setVisible (false);

    scene->update (0.25);
    scene->update (0.5);

    EXPECT_EQ (rootNode.updates, 2);
    EXPECT_EQ (childNode.updates, 2);
    EXPECT_DOUBLE_EQ (childNode.totalTime, 0.75);
    EXPECT_EQ (hiddenNode.updates, 0);
}

TEST_F (SceneTests, UpdateSkipsEntitiesOutsideTheTree)
{
    auto entity = scene->createEntity();
    auto& node = entity.attach<CountingNode>();
    entity.removeFromParent();

    scene->update (0.1);

    EXPECT_EQ (node.updates, 0);
}

TEST_F (SceneTests, NodesAttachedWhileUpdatingStartOnTheNextUpdate)
{
    auto entity = scene->createEntity();

    scene->addSystem<CallbackSystem> (SystemOrder().beforeNodes(), [entity] (Scene&) mutable
    {
        entity.getOrAttach<CountingNode>();
    });

    scene->update (0.1);
    EXPECT_EQ (entity.get<CountingNode>()->updates, 0);

    scene->update (0.1);
    EXPECT_EQ (entity.get<CountingNode>()->updates, 1);
}

//==============================================================================

TEST_F (SceneTests, DestroyingWhileIteratingDefersTheNodeDestruction)
{
    auto entity = scene->createEntity();
    bool destroyed = false;
    entity.attach<DestructionTracker> (destroyed);

    {
        const IterationGuard guard (*scene);
        EXPECT_TRUE (scene->isIterating());

        entity.destroy();

        EXPECT_FALSE (entity.isValid());
        EXPECT_FALSE (destroyed);
    }

    EXPECT_FALSE (scene->isIterating());
    EXPECT_TRUE (destroyed);
}

TEST_F (SceneTests, NestedIterationsDestroyOnceTheOutermostEnds)
{
    auto entity = scene->createEntity();
    bool destroyed = false;
    entity.attach<DestructionTracker> (destroyed);

    {
        const IterationGuard outer (*scene);

        {
            const IterationGuard inner (*scene);
            entity.detach<DestructionTracker>();
        }

        EXPECT_FALSE (destroyed);
    }

    EXPECT_TRUE (destroyed);
}

TEST_F (SceneTests, OnDestroyIsEmittedParentFirst)
{
    auto parent = scene->createEntity ("parent");
    parent.createChild ("child");

    StringArray names;
    const auto connection = scene->onDestroy().connect ([&] (Entity entity) { names.add (entity.getName()); });

    parent.destroy();

    EXPECT_EQ (names, StringArray ({ "parent", "child" }));
}

TEST_F (SceneTests, AttachAndDetachSignalsCarryTheEntityAndTheNode)
{
    auto entity = scene->createEntity();
    int attached = 0;
    int detached = 0;

    const auto onAttach = scene->onAttach<CountingNode>().connect ([&] (Entity e, CountingNode& node)
    {
        EXPECT_EQ (e, entity);
        EXPECT_EQ (e.get<CountingNode>(), &node);
        ++attached;
    });

    const auto onDetach = scene->onDetach<CountingNode>().connect ([&] (Entity e, CountingNode&)
    {
        EXPECT_EQ (e, entity);
        EXPECT_FALSE (e.has<CountingNode>());
        ++detached;
    });

    entity.attach<CountingNode>();
    entity.attach<CountingNode>();
    entity.destroy();

    EXPECT_EQ (attached, 2);
    EXPECT_EQ (detached, 2);
}

//==============================================================================

TEST_F (SceneTests, ActiveCameraUsesTheChosenEntity)
{
    scene->createEntity().attach<CameraNode>();
    auto second = scene->createEntity();
    auto& chosen = second.attach<CameraNode>();

    scene->setActiveCamera (second);

    EXPECT_EQ (scene->getActiveCamera(), &chosen);
}

TEST_F (SceneTests, ActiveCameraFollowsAReplacedCamera)
{
    auto entity = scene->createEntity();
    entity.attach<CameraNode>();
    scene->setActiveCamera (entity);

    auto& replacement = entity.attach<CameraNode>();

    EXPECT_EQ (scene->getActiveCamera(), &replacement);
}

TEST_F (SceneTests, ActiveCameraFallsBackWhenTheEntityIsDestroyed)
{
    auto& fallbackCamera = scene->createEntity().attach<CameraNode>();
    auto chosen = scene->createEntity();
    chosen.attach<CameraNode>();
    scene->setActiveCamera (chosen);
    EXPECT_NE (scene->getActiveCamera(), &fallbackCamera);

    chosen.destroy();

    EXPECT_EQ (scene->getActiveCamera(), &fallbackCamera);
}

TEST_F (SceneTests, ActiveCameraFallsBackDepthFirstWhenTheEntityHasNoCamera)
{
    auto withoutCamera = scene->createEntity();
    auto& deepCamera = withoutCamera.createChild().attach<CameraNode>();
    scene->createEntity().attach<CameraNode>();

    scene->setActiveCamera (withoutCamera);

    EXPECT_EQ (scene->getActiveCamera(), &deepCamera);
}

TEST_F (SceneTests, FallbackSkipsHiddenEntities)
{
    auto hidden = scene->createEntity();
    hidden.attach<CameraNode>();
    auto& visibleCamera = scene->createEntity().attach<CameraNode>();
    hidden.setVisible (false);

    EXPECT_EQ (scene->getActiveCamera(), &visibleCamera);
}

TEST_F (SceneTests, CameraViewMatrixIsTheInverseOfItsWorldMatrix)
{
    auto entity = scene->createEntity();
    entity.attach<TransformNode> (Vector3<float> { 0.0f, 0.0f, 5.0f });
    auto& camera = entity.attach<CameraNode>();

    const auto p = camera.getViewMatrix().transformPoint ({ 0.0f, 0.0f, 0.0f });
    EXPECT_NEAR (p.getZ(), -5.0f, 1.0e-5f);

    EXPECT_TRUE (CameraNode().getViewMatrix().isIdentity());
}

TEST_F (SceneTests, CameraProjectionHandlesInfiniteFarPlane)
{
    CameraNode camera;
    camera.zNear = 0.5f;
    camera.zFar = 0.0f;

    const auto projection = camera.getProjectionMatrix (2.0f);

    // A point on the near plane maps to depth 0, a very far one approaches depth 1
    const auto nearClip = projection.transformPoint4 (0.0f, 0.0f, -0.5f, 1.0f);
    EXPECT_NEAR (nearClip[2] / nearClip[3], 0.0f, 1.0e-5f);

    const auto farClip = projection.transformPoint4 (0.0f, 0.0f, -1.0e6f, 1.0f);
    EXPECT_NEAR (farClip[2] / farClip[3], 1.0f, 1.0e-5f);

    camera.zFar = 100.0f;
    EXPECT_TRUE (camera.getProjectionMatrix (2.0f).approximatelyEqualTo (Matrix4::perspective (camera.yFov, 2.0f, 0.5f, 100.0f)));
}

//==============================================================================

TEST_F (SceneTests, SystemsRunByPhaseThenInAddingOrder)
{
    scene->addSystem<LoggingSystem<2>> (SystemOrder().afterNodes(), log);
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { 0, 1, 2 }));
}

TEST_F (SceneTests, SystemsRunAroundNodeUpdates)
{
    auto& node = scene->createEntity().attach<CountingNode>();
    std::vector<int> updatesSeen;

    scene->addSystem<CallbackSystem> (SystemOrder().beforeNodes(), [&] (Scene&) { updatesSeen.push_back (node.updates); });
    scene->addSystem<CallbackSystem> (SystemOrder().afterNodes(), [&] (Scene&) { updatesSeen.push_back (node.updates); });

    scene->update (0.0);

    EXPECT_EQ (updatesSeen, (std::vector<int> { 0, 1 }));
}

TEST_F (SceneTests, SystemRulesReorderAPhase)
{
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<2>> (SystemOrder().beforeNodes(), log);

    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<2>, LoggingSystem<0>>()));

    // Unconstrained systems keep their adding order
    EXPECT_EQ (runLog(), (std::vector<int> { 1, 2, 0 }));
}

TEST_F (SceneTests, SystemRulesAreTransitiveAndApplyToLaterSystems)
{
    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<2>, LoggingSystem<1>>()));
    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<1>, LoggingSystem<0>>()));

    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<2>> (SystemOrder().beforeNodes(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { 2, 0 }));

    scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { 2, 1, 0 }));
}

TEST_F (SceneTests, SystemRulesApplyToEverySystemOfAType)
{
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);

    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<1>, LoggingSystem<0>>()));

    EXPECT_EQ (runLog(), (std::vector<int> { 1, 0, 0 }));
}

TEST_F (SceneTests, RepeatedSystemRulesAreAccepted)
{
    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<0>, LoggingSystem<1>>()));
    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<0>, LoggingSystem<1>>()));
}

// Disabled: the rejected rules hit a jassert in debug builds
TEST_F (SceneTests, DISABLED_CyclicSystemRulesAreRejected)
{
    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<0>, LoggingSystem<1>>()));
    EXPECT_TRUE ((scene->addSystemRule<LoggingSystem<1>, LoggingSystem<2>>()));

    EXPECT_FALSE ((scene->addSystemRule<LoggingSystem<2>, LoggingSystem<0>>()));
    EXPECT_FALSE ((scene->addSystemRule<LoggingSystem<0>, LoggingSystem<0>>()));
}

TEST_F (SceneTests, AddSystemConstraintsOrderTheSystem)
{
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);
    scene->addSystem<LoggingSystem<2>> (SystemOrder()
                                            .beforeNodes()
                                            .before<LoggingSystem<1>>()
                                            .after<LoggingSystem<0>>(),
                                        log);

    EXPECT_EQ (runLog(), (std::vector<int> { 0, 2, 1 }));
}

TEST_F (SceneTests, SystemOrderBuilderKeepsPhaseAndConstraints)
{
    const auto order = SystemOrder().afterNodes().before<LoggingSystem<1>>().after<CountingNode>();

    EXPECT_EQ (order.getPhase(), SystemOrder::Phase::afterNodes);
    ASSERT_EQ (order.getConstraints().size(), 2u);
    EXPECT_EQ (order.getConstraints()[0].typeId, NodeTypeId::get<LoggingSystem<1>>());
    EXPECT_TRUE (order.getConstraints()[0].runsBefore);
    EXPECT_EQ (order.getConstraints()[1].typeId, NodeTypeId::get<CountingNode>());
    EXPECT_FALSE (order.getConstraints()[1].runsBefore);
    EXPECT_EQ (SystemOrder().getPhase(), SystemOrder::Phase::beforeNodes);
}

TEST_F (SceneTests, AddSystemConstraintsHoldForLaterSystems)
{
    scene->addSystem<LoggingSystem<0>> (SystemOrder().after<LoggingSystem<1>>(), log);
    scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { 1, 0 }));
}

TEST_F (SceneTests, DefaultOrderRunsBeforeNodesAndForwardsEveryArgument)
{
    auto& node = scene->createEntity().attach<CountingNode>();
    int updatesSeen = -1;

    scene->addSystem<CallbackSystem> (SystemOrder(), [&] (Scene&) { updatesSeen = node.updates; });

    scene->update (0.0);

    EXPECT_EQ (updatesSeen, 0);
}

TEST_F (SceneTests, NodeTypesUpdateInFirstUseOrder)
{
    scene->createEntity().attach<LoggingNode<1>> (log);
    scene->createEntity().attach<LoggingNode<0>> (log);

    EXPECT_EQ (runLog(), (std::vector<int> { 1, 0 }));
}

TEST_F (SceneTests, SystemRulesOrderNodeTypes)
{
    scene->createEntity().attach<LoggingNode<1>> (log);
    scene->createEntity().attach<LoggingNode<0>> (log);

    EXPECT_TRUE ((scene->addSystemRule<LoggingNode<0>, LoggingNode<1>>()));

    EXPECT_EQ (runLog(), (std::vector<int> { 0, 1 }));
}

TEST_F (SceneTests, SystemRulesOrderNodeTypesUsedLater)
{
    EXPECT_TRUE ((scene->addSystemRule<LoggingNode<0>, LoggingNode<1>>()));

    scene->createEntity().attach<LoggingNode<1>> (log);
    EXPECT_EQ (runLog(), (std::vector<int> { 1 }));

    scene->createEntity().attach<LoggingNode<0>> (log);
    EXPECT_EQ (runLog(), (std::vector<int> { 0, 1 }));
}

TEST_F (SceneTests, ConstraintsAgainstNodeTypesMatchingThePhaseAreAccepted)
{
    scene->createEntity().attach<LoggingNode<5>> (log);

    scene->addSystem<LoggingSystem<0>> (SystemOrder().before<LoggingNode<5>>(), log);
    scene->addSystem<LoggingSystem<1>> (SystemOrder().afterNodes().after<LoggingNode<5>>(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { 0, 5, 1 }));
}

// Disabled: a constraint contradicting the phases hits a jassert in debug builds
TEST_F (SceneTests, DISABLED_ConstraintsContradictingThePhaseAssert)
{
    scene->createEntity().attach<LoggingNode<5>> (log);
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes().after<LoggingNode<5>>(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { 0, 5 }));
}

TEST_F (SceneTests, RemoveSystemStopsIt)
{
    scene->addSystem<LoggingSystem<0>> (SystemOrder().beforeNodes(), log);
    auto& removed = scene->addSystem<LoggingSystem<1>> (SystemOrder().beforeNodes(), log);

    EXPECT_TRUE (scene->removeSystem (removed));
    EXPECT_EQ (runLog(), (std::vector<int> { 0 }));
}

TEST_F (SceneTests, SystemsCanRemoveSystemsWhileUpdating)
{
    System* later = nullptr;
    auto& self = scene->addSystem<CallbackSystem> (SystemOrder().beforeNodes(), nullptr);
    self.callback = [&] (Scene& s)
    {
        log.push_back (-1);
        EXPECT_TRUE (s.removeSystem (*later));
        EXPECT_TRUE (s.removeSystem (self));
        EXPECT_FALSE (s.removeSystem (self));
    };

    later = &scene->addSystem<LoggingSystem<1>> (SystemOrder().afterNodes(), log);

    EXPECT_EQ (runLog(), (std::vector<int> { -1 }));
    EXPECT_EQ (runLog(), (std::vector<int> {}));
}

TEST_F (SceneTests, SystemsAddedWhileUpdatingRunFromTheNextUpdate)
{
    scene->addSystem<CallbackSystem> (SystemOrder().beforeNodes(), [this] (Scene& s)
    {
        if (log.empty())
            s.addSystem<LoggingSystem<5>> (SystemOrder().afterNodes(), log);

        log.push_back (0);
    });

    EXPECT_EQ (runLog(), (std::vector<int> { 0 }));
    EXPECT_EQ (runLog(), (std::vector<int> { 0, 5 }));
}

//==============================================================================

TEST_F (SceneTests, RaycastReturnsTheNearestEntity)
{
    const auto back = addCube ("back", { 0.0f, 0.0f, -5.0f });
    const auto front = addCube ("front", { 0.0f, 0.0f, 0.0f });

    const auto hit = scene->raycast (Ray ({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, -1.0f }));

    ASSERT_TRUE (hit.has_value());
    EXPECT_EQ (hit->entity, front);
    EXPECT_FLOAT_EQ (hit->distance, 9.0f);

    EXPECT_FALSE (scene->raycast (Ray ({ 5.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, -1.0f })).has_value());
    EXPECT_EQ (scene->raycast (Ray ({ 0.0f, 0.0f, -10.0f }, { 0.0f, 0.0f, 1.0f }))->entity, back);
}

TEST_F (SceneTests, RaycastSkipsInactiveAndFilteredEntities)
{
    auto group = scene->createEntity ("group");
    const auto back = addCube ("back", { 0.0f, 0.0f, -5.0f });
    const auto front = addCube ("front", { 0.0f, 0.0f, 0.0f }, group);

    const Ray ray ({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, -1.0f });

    EXPECT_EQ (scene->raycast (ray, 0.0f, [&] (const Entity& entity) { return entity != front; })->entity, back);
    EXPECT_FALSE (scene->raycast (ray, 0.0f, [] (const Entity&) { return false; }).has_value());

    group.setVisible (false);
    EXPECT_EQ (scene->raycast (ray)->entity, back);
}

TEST_F (SceneTests, RaycastStartsAtTheMinDistance)
{
    addCube ("front", { 0.0f, 0.0f, 0.0f });

    EXPECT_FLOAT_EQ (scene->raycast (Ray ({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, -1.0f }), 10.0f).value_or (Scene::RaycastHit {}).distance, 10.0f);
    EXPECT_FALSE (scene->raycast (Ray ({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, -1.0f }), 12.0f).has_value());
}
