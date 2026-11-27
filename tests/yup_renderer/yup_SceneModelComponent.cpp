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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

/** The `Model3D` of a `.yui` document: a node instantiating a glTF model read through the
    document, the `Node3D` bound to the entities of its instance, and the clips it plays.
*/
class SceneModelComponentTests : public SceneDocumentTests
{
protected:
    void SetUp() override
    {
        SceneDocumentTests::SetUp();
        files["device.gltf"] = modelFile();
        files["robot.gltf"] = robotFile();
    }

    //==============================================================================
    /** The buffers, buffer views and accessors of a glTF file: one buffer and one view for each
        block of data, and the accessors in the order they are added.
    */
    struct ModelData
    {
        template <class Value>
        void add (std::initializer_list<Value> values, int componentType, const char* type, int count)
        {
            const std::vector<Value> data (values);
            const auto numBytes = String (static_cast<int> (data.size() * sizeof (Value)));

            buffers.add (R"({ "uri": "data:application/octet-stream;base64,)" + Base64::toBase64 (data.data(), data.size() * sizeof (Value)) + R"(", "byteLength": )" + numBytes + " }");
            views.add (R"({ "buffer": )" + String (views.size()) + R"(, "byteLength": )" + numBytes + " }");
            accessors.add (R"({ "bufferView": )" + String (accessors.size()) + R"(, "componentType": )" + String (componentType) + R"(, "count": )" + String (count) + R"(, "type": ")" + type + "\" }");
        }

        String toJson() const
        {
            return R"("buffers": [ )" + buffers.joinIntoString (", ") + R"( ], "bufferViews": [ )" + views.joinIntoString (", ") + R"( ], "accessors": [ )" + accessors.joinIntoString (", ") + " ]";
        }

        StringArray buffers, views, accessors;
    };

    /** A glTF model holding everything as data URIs, with two scenes and two clips.

        The scene Main has Strip, a strip facing +z, 1 wide and 2 high, skinned to two joints: its
        foot follows Root, at the origin, and its top Tip, the child of Root 1 up, where the skin
        has it bound. It also has Face, a triangle with two morph targets its mesh weights 0.5 and
        0.25: the first lifts its last vertex by 1, the second moves its second vertex by 2 along
        x. The scene Extra only has Spare.

        The clip Bend turns Tip a quarter turn about x in 1 second, or in the time given, and the
        clip Smile takes the weights of Face from 0 and 0 to 1 and 0.5 in 2 seconds.
    */
    static MemoryBlock robotFile (float bendSeconds = 1.0f)
    {
        ModelData data;

        // The positions (0), the normals (1), the joints (2) and the weights (3) of the four vertices of
        // the strip, the inverse bind matrices of its joints (4) and its indices (5)
        data.add<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 1.0f, 2.0f, 0.0f }, 5126, "VEC3", 4);
        data.add<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, 5126, "VEC3", 4);
        data.add<uint8> ({ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 }, 5121, "VEC4", 4);
        data.add<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }, 5126, "VEC4", 4);
        data.add<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                           1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f },
                         5126,
                         "MAT4",
                         2);
        data.add<uint16> ({ 0, 1, 2, 1, 3, 2 }, 5123, "SCALAR", 6);

        // The keys of Bend: the times (6) of no turn and of a quarter turn about x (7)
        data.add<float> ({ 0.0f, bendSeconds }, 5126, "SCALAR", 2);
        data.add<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.70710678f, 0.0f, 0.0f, 0.70710678f }, 5126, "VEC4", 2);

        // The positions (8) and the normals (9) of the three vertices of the face, and what its two morph
        // targets add to the positions (10 and 11)
        data.add<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }, 5126, "VEC3", 3);
        data.add<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, 5126, "VEC3", 3);
        data.add<float> ({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }, 5126, "VEC3", 3);
        data.add<float> ({ 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 5126, "VEC3", 3);

        // The keys of Smile: the times 0 and 2 (12) and the two weights of each (13)
        data.add<float> ({ 0.0f, 2.0f }, 5126, "SCALAR", 2);
        data.add<float> ({ 0.0f, 0.0f, 1.0f, 0.5f }, 5126, "SCALAR", 4);

        return bytesOf (String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Main", "nodes": [ 0, 1, 3 ] }, { "name": "Extra", "nodes": [ 4 ] } ],
            "nodes": [ { "name": "Strip", "mesh": 0, "skin": 0 },
                       { "name": "Root", "children": [ 2 ] },
                       { "name": "Tip", "translation": [ 0, 1, 0 ] },
                       { "name": "Face", "mesh": 1 },
                       { "name": "Spare" } ],
            "skins": [ { "name": "Bend", "joints": [ 1, 2 ], "inverseBindMatrices": 4 } ],
            "meshes": [ { "name": "StripMesh", "primitives": [ { "attributes": { "POSITION": 0, "NORMAL": 1, "JOINTS_0": 2, "WEIGHTS_0": 3 }, "indices": 5 } ] },
                        { "name": "FaceMesh", "primitives": [ { "attributes": { "POSITION": 8, "NORMAL": 9 }, "targets": [ { "POSITION": 10 }, { "POSITION": 11 } ] } ], "weights": [ 0.5, 0.25 ] } ],
            "animations": [ { "name": "Bend",
                              "channels": [ { "sampler": 0, "target": { "node": 2, "path": "rotation" } } ],
                              "samplers": [ { "input": 6, "output": 7 } ] },
                            { "name": "Smile",
                              "channels": [ { "sampler": 0, "target": { "node": 3, "path": "weights" } } ],
                              "samplers": [ { "input": 12, "output": 13 } ] } ],
            $DATA
        })")
                            .replace ("$DATA", data.toJson()));
    }

    /** A document showing the robot, with what is written for its model, and after it in the scene. */
    static String robotWith (const String& properties, const String& after = {})
    {
        return "Scene3D { id: view; Model3D { id: robot; source: \"robot.gltf\"; " + properties + " } " + after + " }";
    }

    //==============================================================================
    /** Counts the display frames a scene was updated for, and the time they took: an update that
        only shows a pose again takes none.
    */
    struct FrameCounter final : Node
    {
        void update (double deltaSeconds) override
        {
            if (deltaSeconds <= 0.0)
                return;

            ++numFrames;
            seconds += deltaSeconds;
        }

        int numFrames = 0;
        double seconds = 0.0;
    };

    /** Counts how many times a component tells that one of its properties changed. */
    struct ChangeCounter final : ComponentListener
    {
        ChangeCounter (Component& componentToHear, const char* propertyToCount)
            : component (componentToHear)
            , property (propertyToCount)
        {
            component.addComponentListener (this);
        }

        ~ChangeCounter() override
        {
            component.removeComponentListener (this);
        }

        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            if (name == property)
                ++count;
        }

        Component& component;
        const Identifier property;
        int count = 0;
    };

    //==============================================================================
    /** How far the clip Bend has turned Tip, the joint the top of the strip follows. */
    static Quaternion tipRotation (const SceneModelComponent& model)
    {
        return rotationOfEntity (entityAt (model, { "Root", "Tip" }));
    }

    static Quaternion rotationOfEntity (const Entity& entity)
    {
        const auto* transform = entity.isValid() ? entity.get<TransformNode>() : nullptr;
        return transform != nullptr ? transform->getRotation() : Quaternion();
    }

    /** The turn of Tip at a time of the clip Bend. */
    static Quaternion bendAt (float seconds)
    {
        return Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, MathConstants<float>::halfPi * seconds);
    }

    /** Where a vertex of the mesh an entity of the model draws is, deformed or not. */
    static Vector3<float> vertexOf (const SceneModelComponent& model, const char* name, size_t index)
    {
        const auto instance = model.getInstance();
        const auto mesh = meshOf (instance.isValid() ? instance.findChild (name) : Entity());

        if (mesh == nullptr || mesh->getNumPrimitives() < 1 || mesh->getPrimitive (0).vertices.size() <= index)
        {
            ADD_FAILURE() << "No vertex " << index << " in the mesh of " << name;
            return {};
        }

        return mesh->getPrimitive (0).vertices[index].position;
    }

    /** The weights the morph targets of Face have. */
    static void expectWeights (const SceneModelComponent& model, std::initializer_list<float> expected, float tolerance = 1.0e-5f)
    {
        const auto instance = model.getInstance();
        ASSERT_TRUE (instance.isValid());

        const auto* deform = instance.findChild ("Face").get<DeformNode>();
        ASSERT_NE (deform, nullptr);

        const auto weights = deform->getWeights();
        ASSERT_EQ (weights.size(), expected.size());

        size_t index = 0;

        for (const auto weight : expected)
        {
            EXPECT_NEAR (weights[index], weight, tolerance) << "weight " << index;
            ++index;
        }
    }

    /** The entity a path of names leads to from the root of the instance of a model. */
    static Entity entityAt (const SceneModelComponent& model, std::initializer_list<const char*> names)
    {
        auto entity = model.getInstance();

        for (const auto* name : names)
            entity = entity.isValid() ? entity.findChild (name, false) : Entity();

        return entity;
    }

    static Mesh::Ptr meshOf (const Entity& entity)
    {
        const auto* node = entity.isValid() ? entity.get<MeshNode>() : nullptr;
        return node != nullptr ? node->mesh : Mesh::Ptr();
    }

    /** The scene and the model of a document made with robotWith(), shown as a window would. */
    struct Robot
    {
        explicit Robot (YuiDocument& document)
            : view (document.findById<SceneComponent> ("view"))
            , model (document.findById<SceneModelComponent> ("robot"))
        {
            if (view != nullptr)
                showScene (*view);
        }

        bool isValid() const
        {
            return view != nullptr && model != nullptr && model->getInstance().isValid();
        }

        SceneComponent* view;
        SceneModelComponent* model;
    };
};

TEST_F (SceneModelComponentTests, InstantiatesTheModelUnderItsEntity)
{
    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Model3D { id: model; source: "device.gltf"; position: [0, 1, 0]; }
        })yui");

    auto* model = document.findById<SceneModelComponent> ("model");
    ASSERT_NE (model, nullptr);

    const auto instance = model->getInstance();
    ASSERT_TRUE (instance.isValid());

    // The node places the model: its instance is the only entity under the one of the node
    EXPECT_EQ (instance.getParent(), model->getEntity());
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);
    EXPECT_EQ (instance.getName(), "Main");
    expectVector (positionOf (*model), 0.0f, 1.0f, 0.0f);

    ASSERT_EQ (instance.getNumChildren(), 2);
    EXPECT_EQ (instance.getChild (0).getName(), "Body");
    EXPECT_EQ (instance.getChild (1).getName(), "Lid");

    const auto knob = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (knob.isValid());
    ASSERT_NE (meshOf (knob), nullptr);
    EXPECT_EQ (meshOf (knob)->getNumPrimitives(), 1);
    expectVector (knob.get<TransformNode>()->getPosition(), 1.0f, 2.0f, 3.0f);
}

TEST_F (SceneModelComponentTests, TheModelsOfOneFileShareWhatItHolds)
{
    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Model3D { id: one; source: "device.gltf"; }
            Node3D { Model3D { id: other; source: "device.gltf"; position: [3, 0, 0]; } }
        })yui");

    auto* one = document.findById<SceneModelComponent> ("one");
    auto* other = document.findById<SceneModelComponent> ("other");

    ASSERT_NE (one, nullptr);
    ASSERT_NE (other, nullptr);

    // Two instances, with entities of their own, of a file read once
    ASSERT_TRUE (one->getInstance().isValid());
    ASSERT_TRUE (other->getInstance().isValid());
    EXPECT_NE (one->getInstance(), other->getInstance());
    EXPECT_EQ (reads["device.gltf"], 1);

    const auto mesh = meshOf (entityAt (*one, { "Body", "Knob" }));
    ASSERT_NE (mesh, nullptr);
    EXPECT_EQ (meshOf (entityAt (*other, { "Body", "Knob" })), mesh);
}

TEST_F (SceneModelComponentTests, ANodeIsBoundToAnEntityOfTheModelByNameOrByPath)
{
    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Model3D {
                id: model;
                source: "device.gltf";

                Node3D { id: knob; node: "Knob"; rotation: [0, 90deg, 0]; }
                Node3D { id: lidKnob; node: "Lid/Knob"; scale: 2; }
                Node3D { id: lid; node: "Lid"; visible: false;  Light3D { id: lamp; type: point; } }
            }
        })yui");

    auto* model = document.findById<SceneModelComponent> ("model");
    auto* knob = document.findById<SceneNodeComponent> ("knob");
    auto* lidKnob = document.findById<SceneNodeComponent> ("lidKnob");
    auto* lid = document.findById<SceneNodeComponent> ("lid");
    auto* lamp = document.findById<SceneLightComponent> ("lamp");

    ASSERT_NE (model, nullptr);
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (lidKnob, nullptr);
    ASSERT_NE (lid, nullptr);
    ASSERT_NE (lamp, nullptr);

    // A name is the first entity of that name, depth first: a path tells the others apart
    ASSERT_TRUE (knob->getEntity().isValid());
    EXPECT_EQ (knob->getEntity(), entityAt (*model, { "Body", "Knob" }));
    EXPECT_EQ (lidKnob->getEntity(), entityAt (*model, { "Lid", "Knob" }));
    EXPECT_EQ (lid->getEntity(), entityAt (*model, { "Lid" }));

    // A bound node makes no entity: the instance is still the only one under the model
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);

    // It only moves what it sets: the rest stays as the model has it
    expectRotation (rotationOf (*knob), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));
    expectVector (positionOf (*knob), 1.0f, 2.0f, 3.0f);
    expectVector (scaleOf (*knob), 1.0f, 1.0f, 1.0f);
    EXPECT_TRUE (knob->getProperty (Identifier ("position")).isVoid());

    expectVector (scaleOf (*lidKnob), 2.0f, 2.0f, 2.0f);
    expectVector (positionOf (*lidKnob), 0.0f, 5.0f, 0.0f);

    // It shows or hides the entity, and what is written in it is made under that entity
    EXPECT_FALSE (lid->getEntity().isVisible());
    ASSERT_TRUE (lamp->getEntity().isValid());
    EXPECT_EQ (lamp->getEntity().getParent(), lid->getEntity());
    EXPECT_NE (lamp->getEntity().get<LightNode>(), nullptr);
}

TEST_F (SceneModelComponentTests, ABoundNodeOnlyWritesWhatItSets)
{
    YuiDocument document;
    serve (document);
    load (document, "Scene3D { Model3D { source: \"device.gltf\"; Node3D { id: knob; node: \"Knob\"; rotation: [0, 1, 0]; } } }");

    auto* knob = document.findById<SceneNodeComponent> ("knob");
    ASSERT_NE (knob, nullptr);

    auto* transform = knob->getEntity().get<TransformNode>();
    ASSERT_NE (transform, nullptr);

    // Something else moves the entity, as a clip of the model would
    transform->setPosition ({ 5.0f, 5.0f, 5.0f });

    knob->setProperty (Identifier ("rotation"), numbers ({ 0.0, 2.0, 0.0 }));
    expectRotation (transform->getRotation(), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 2.0f));
    expectVector (transform->getPosition(), 5.0f, 5.0f, 5.0f);

    // What the node stops setting goes back to what the model had, and nothing else does
    knob->setProperty (Identifier ("scale"), 3.0);
    expectVector (transform->getScale(), 3.0f, 3.0f, 3.0f);

    knob->setProperty (Identifier ("scale"), var());
    expectVector (transform->getScale(), 1.0f, 1.0f, 1.0f);
    expectVector (transform->getPosition(), 5.0f, 5.0f, 5.0f);
    expectRotation (transform->getRotation(), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 2.0f));
}

TEST_F (SceneModelComponentTests, ABoundNodeThatGoesGivesBackWhatTheModelHad)
{
    const auto text = [] (const String& inside)
    {
        return "Scene3D { Model3D { id: model; source: \"device.gltf\"; " + inside + " } }";
    };

    YuiDocument document;
    serve (document);
    load (document, text ("Node3D { id: knob; node: \"Knob\"; position: [9, 9, 9]; visible: false; }"));

    auto* model = document.findById<SceneModelComponent> ("model");
    ASSERT_NE (model, nullptr);

    const auto knob = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (knob.isValid());
    expectVector (knob.get<TransformNode>()->getPosition(), 9.0f, 9.0f, 9.0f);
    EXPECT_FALSE (knob.isVisible());

    load (document, text (""));
    ASSERT_EQ (document.findById<SceneModelComponent> ("model"), model);

    // The entity is the model's: it stays, as the model made it
    ASSERT_TRUE (knob.isValid());
    EXPECT_EQ (entityAt (*model, { "Body", "Knob" }), knob);
    expectVector (knob.get<TransformNode>()->getPosition(), 1.0f, 2.0f, 3.0f);
    EXPECT_TRUE (knob.isVisible());
}

TEST_F (SceneModelComponentTests, ANameTheModelDoesNotHaveIsReported)
{
    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    Model3D {\n"
                                                 "        source: \"device.gltf\";\n"
                                                 "        Node3D { id: missing; node: \"Handle\"; }\n"
                                                 "        Node3D { id: wrong; node: \"Lid/Body\"; }\n"
                                                 "    }\n"
                                                 "    Node3D { id: outside; node: \"Knob\"; }\n"
                                                 "    Light3D { id: lamp; node: \"Knob\"; }\n"
                                                 "}");

    finishLoading (document);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    expectDiagnostic (document, "Node3D.node: the model has no entity 'Handle'");
    expectDiagnostic (document, "Node3D.node: the model has no entity 'Lid/Body'");
    expectDiagnostic (document, "Node3D.node: a node is bound to an entity of the Model3D it is in");
    expectDiagnostic (document, "Light3D.node: only a Node3D can be bound");
    EXPECT_EQ (document.getDiagnostics().size(), 4u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* diagnostic = findDiagnostic (document, "'Handle'");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 4);

    // A node that is bound to nothing has no entity, a type that can't be bound keeps its own
    auto* missing = document.findById<SceneNodeComponent> ("missing");
    auto* outside = document.findById<SceneNodeComponent> ("outside");
    auto* lamp = document.findById<SceneNodeComponent> ("lamp");

    ASSERT_NE (missing, nullptr);
    ASSERT_NE (outside, nullptr);
    ASSERT_NE (lamp, nullptr);

    EXPECT_FALSE (missing->getEntity().isValid());
    EXPECT_FALSE (outside->getEntity().isValid());
    EXPECT_TRUE (lamp->getEntity().isValid());
}

TEST_F (SceneModelComponentTests, TheModelIsLoadedAgainWhenItsFileChanges)
{
    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Model3D {
                id: model;
                source: "device.gltf";

                Node3D { id: knob; node: "Knob"; scale: 2;  Light3D { id: lamp; } }
            }

            Model3D { id: other; source: "device.gltf"; }
        })yui");

    auto* model = document.findById<SceneModelComponent> ("model");
    auto* other = document.findById<SceneModelComponent> ("other");
    auto* knob = document.findById<SceneNodeComponent> ("knob");
    auto* lamp = document.findById<SceneLightComponent> ("lamp");

    ASSERT_NE (model, nullptr);
    ASSERT_NE (other, nullptr);
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (lamp, nullptr);

    const Scene::Ptr scene = model->getEntity().getScene();
    const auto instance = model->getInstance();
    const auto bound = knob->getEntity();
    const auto mesh = meshOf (bound);

    ASSERT_NE (scene, nullptr);
    ASSERT_TRUE (instance.isValid());
    ASSERT_TRUE (bound.isValid());
    ASSERT_NE (mesh, nullptr);

    // A file that is not the one of the model changes nothing
    document.notifyAssetChanged ("other.gltf");
    EXPECT_EQ (model->getInstance(), instance);
    EXPECT_EQ (reads["device.gltf"], 1);

    files["device.gltf"] = modelFile ("4, 5, 6");
    document.notifyAssetChanged ("device.gltf");
    finishLoading (document);

    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // Read once more, for the two models that share it
    EXPECT_EQ (reads["device.gltf"], 2);
    EXPECT_FALSE (instance.isValid());
    EXPECT_FALSE (bound.isValid());
    ASSERT_TRUE (model->getInstance().isValid());
    ASSERT_TRUE (other->getInstance().isValid());
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);

    // The node is bound to the entity of the new instance, and drives it as before
    const auto again = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (again.isValid());
    EXPECT_EQ (knob->getEntity(), again);
    expectVector (positionOf (*knob), 4.0f, 5.0f, 6.0f);
    expectVector (scaleOf (*knob), 2.0f, 2.0f, 2.0f);
    EXPECT_NE (meshOf (again), mesh);
    EXPECT_EQ (meshOf (entityAt (*other, { "Body", "Knob" })), meshOf (again));

    // What is written in the node went with the entity it was under, and is made again under the new one
    ASSERT_TRUE (lamp->getEntity().isValid());
    EXPECT_EQ (lamp->getEntity().getParent(), again);
    EXPECT_NE (lamp->getEntity().get<LightNode>(), nullptr);
}

TEST_F (SceneModelComponentTests, AFileThatCanNotBeLoadedIsReported)
{
    files["broken.gltf"] = bytesOf ("this is not a model");

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    Model3D { id: missing; source: \"nowhere.gltf\"; }\n"
                                                 "    Model3D { id: broken; source: \"broken.gltf\"; }\n"
                                                 "    Model3D { id: third; source: \"device.gltf\"; scene: 3; }\n"
                                                 "    Model3D { id: none; }\n"
                                                 "}");

    finishLoading (document);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    expectDiagnostic (document, "Model3D.source: the file 'nowhere.gltf' can't be read");
    expectDiagnostic (document, "Model3D.scene: the model has no scene 3");
    EXPECT_EQ (document.getDiagnostics().size(), 3u) << DataTreeDiagnostic::join (document.getDiagnostics());

    // The loader tells what is wrong with the file it was given
    const auto broken = std::find_if (document.getDiagnostics().begin(), document.getDiagnostics().end(), [] (const DataTreeDiagnostic& diagnostic)
    {
        return diagnostic.line == 3;
    });

    ASSERT_NE (broken, document.getDiagnostics().end());
    EXPECT_TRUE (broken->message.startsWith ("Model3D.source: ")) << broken->message;
    EXPECT_TRUE (broken->isError());

    for (const auto* name : { "missing", "broken", "third", "none" })
    {
        auto* model = document.findById<SceneModelComponent> (name);

        ASSERT_NE (model, nullptr) << name;
        ASSERT_TRUE (model->getEntity().isValid()) << name;
        EXPECT_FALSE (model->getInstance().isValid()) << name;
        EXPECT_EQ (model->getEntity().getNumChildren(), 0) << name;
    }

    // A source that is right again shows the model
    files["nowhere.gltf"] = modelFile();
    document.notifyAssetChanged ("nowhere.gltf");
    finishLoading (document);

    auto* missing = document.findById<SceneModelComponent> ("missing");
    ASSERT_NE (missing, nullptr);
    EXPECT_TRUE (missing->getInstance().isValid());
}

TEST_F (SceneModelComponentTests, ChangingTheSourceOrTheSceneInstantiatesAgain)
{
    files["other.gltf"] = modelFile ("7, 8, 9");

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { Model3D { id: model; source: \"device.gltf\"; } }");

    auto* model = document.findById<SceneModelComponent> ("model");
    ASSERT_NE (model, nullptr);

    const auto scene = model->getEntity().getScene();
    const auto first = model->getInstance();
    ASSERT_TRUE (first.isValid());

    load (document, "Scene3D { Model3D { id: model; source: \"other.gltf\"; } }");
    ASSERT_EQ (document.findById<SceneModelComponent> ("model"), model);
    ASSERT_EQ (model->getEntity().getScene(), scene);

    EXPECT_FALSE (first.isValid());
    ASSERT_TRUE (model->getInstance().isValid());
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);

    const auto knob = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (knob.isValid());
    expectVector (knob.get<TransformNode>()->getPosition(), 7.0f, 8.0f, 9.0f);

    // Without a source there is no model
    model->setSource ({});
    EXPECT_FALSE (model->getInstance().isValid());
    EXPECT_EQ (model->getEntity().getNumChildren(), 0);

    // Another scene of the file is another instance
    model->setSource ("robot.gltf");
    finishLoading (document);

    const auto main = model->getInstance();
    ASSERT_TRUE (main.isValid());
    EXPECT_EQ (main.getName(), "Main");

    model->setSceneIndex (1);
    EXPECT_FALSE (main.isValid());

    const auto extra = model->getInstance();
    ASSERT_TRUE (extra.isValid());
    EXPECT_EQ (extra.getName(), "Extra");
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);
    ASSERT_EQ (extra.getNumChildren(), 1);
    EXPECT_EQ (extra.getChild (0).getName(), "Spare");

    // The scene it shows already is not instantiated again
    model->setSceneIndex (1);
    EXPECT_TRUE (extra.isValid());
    EXPECT_EQ (model->getInstance(), extra);

    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneModelComponentTests, DestroyingTheModelDestroysItsInstance)
{
    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; Model3D { id: model; source: \"device.gltf\"; } Node3D { } }");

    auto* view = document.findById<SceneComponent> ("view");
    auto* model = document.findById<SceneModelComponent> ("model");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (model, nullptr);

    const auto scene = view->getScene();
    const auto instance = model->getInstance();
    const auto knob = entityAt (*model, { "Body", "Knob" });

    ASSERT_TRUE (instance.isValid());
    ASSERT_TRUE (knob.isValid());

    load (document, "Scene3D { id: view; Node3D { } }");

    EXPECT_EQ (document.findById ("model"), nullptr);
    EXPECT_FALSE (instance.isValid());
    EXPECT_FALSE (knob.isValid());
    EXPECT_EQ (scene->getRoot().getNumChildren(), 1);
}

//==============================================================================

TEST_F (SceneModelComponentTests, AModelIsParsedAwayFromTheSceneAndShownOnceItIsThere)
{
    // A state, as the others a style rule matches
    const auto* state = SceneComponent::getClassTypeInfo().findProperty (Identifier ("loading"));
    ASSERT_NE (state, nullptr);
    EXPECT_TRUE (state->isState);

    // What hears the state reads it as it is told
    struct Recorder final : ComponentListener
    {
        explicit Recorder (Component& componentToHear)
            : component (componentToHear)
        {
            component.addComponentListener (this);
        }

        ~Recorder() override
        {
            component.removeComponentListener (this);
        }

        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            if (name == Identifier ("loading"))
                told.push_back (static_cast<bool> (component.getProperty (name)));
        }

        Component& component;
        std::vector<bool> told;
    };

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; Model3D { id: model; Node3D { id: knob; node: \"Knob\"; } } }");

    auto* view = showSceneOf (document);
    auto* model = document.findById<SceneModelComponent> ("model");
    auto* knob = document.findById<SceneNodeComponent> ("knob");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (model, nullptr);
    ASSERT_NE (knob, nullptr);
    EXPECT_FALSE (view->isLoading());

    Recorder recorder (*view);

    // The file is read by the call, and parsed elsewhere: the model is not there when the call returns, and nothing
    // is wrong with it
    model->setSource ("device.gltf");

    EXPECT_EQ (reads["device.gltf"], 1);
    EXPECT_TRUE (view->isLoading());
    EXPECT_TRUE (static_cast<bool> (view->getProperty (Identifier ("loading"))));
    EXPECT_FALSE (model->getInstance().isValid());
    EXPECT_EQ (model->getEntity().getNumChildren(), 0);
    EXPECT_FALSE (knob->getEntity().isValid());
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // What a message loop does once the model is parsed: its users are told, and the scene is painted with it
    Internals::takeRepaintRequest (*view);
    view->finishLoading();

    EXPECT_FALSE (view->isLoading());
    EXPECT_FALSE (static_cast<bool> (view->getProperty (Identifier ("loading"))));
    EXPECT_EQ (recorder.told, std::vector<bool> ({ true, false }));
    EXPECT_TRUE (Internals::takeRepaintRequest (*view));

    ASSERT_TRUE (model->getInstance().isValid());
    EXPECT_EQ (model->getInstance().getName(), "Main");
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);
    ASSERT_TRUE (knob->getEntity().isValid());
    EXPECT_EQ (knob->getEntity(), entityAt (*model, { "Body", "Knob" }));
    EXPECT_EQ (reads["device.gltf"], 1);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // With nothing left to load there is nothing to wait for, nor to tell
    view->finishLoading();
    EXPECT_EQ (recorder.told.size(), 2u);
}

TEST_F (SceneModelComponentTests, AModelStaysAsItWasWhileItsFileIsLoadedAgain)
{
    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; Model3D { id: model; source: \"device.gltf\"; } }");

    auto* view = document.findById<SceneComponent> ("view");
    auto* model = document.findById<SceneModelComponent> ("model");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (model, nullptr);

    const auto instance = model->getInstance();
    ASSERT_TRUE (instance.isValid());

    ChangeCounter clipChanges (*model, "clips");

    // The file is read again at once. Its model is shown as it was meanwhile: nothing goes missing, and nothing is told
    files["device.gltf"] = modelFile ("4, 5, 6");
    document.notifyAssetChanged ("device.gltf");

    EXPECT_EQ (reads["device.gltf"], 2);
    EXPECT_TRUE (view->isLoading());
    EXPECT_TRUE (instance.isValid());
    EXPECT_EQ (model->getInstance(), instance);
    EXPECT_EQ (clipChanges.count, 0);

    view->finishLoading();

    // Instantiated again once, from the file as it then is
    EXPECT_FALSE (view->isLoading());
    EXPECT_FALSE (instance.isValid());
    EXPECT_EQ (clipChanges.count, 1);
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);

    const auto knob = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (knob.isValid());
    expectVector (knob.get<TransformNode>()->getPosition(), 4.0f, 5.0f, 6.0f);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneModelComponentTests, ALoadOfTheDocumentWaitsForAFileThatChangedAndListsWhatItThenHasWrong)
{
    const String text ("Scene3D {\n"
                       "    id: view;\n"
                       "    Model3D { id: model; source: \"device.gltf\"; }\n"
                       "}");

    YuiDocument document;
    serve (document);
    load (document, text);

    auto* view = document.findById<SceneComponent> ("view");
    auto* model = document.findById<SceneModelComponent> ("model");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (model, nullptr);
    ASSERT_TRUE (model->getInstance().isValid());

    // What a loader watching its files does when one is saved: the file is told, then the document is loaded again
    files["device.gltf"] = bytesOf ("this is not a model");
    document.notifyAssetChanged ("device.gltf");
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    ASSERT_EQ (document.findById<SceneModelComponent> ("model"), model);

    // The file was waited for, and the load lists what is wrong with it as it is
    EXPECT_FALSE (view->isLoading());
    EXPECT_FALSE (model->getInstance().isValid());
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics().front().message.startsWith ("Model3D.source: ")) << document.getDiagnostics().front().message;
    EXPECT_EQ (document.getDiagnostics().front().line, 3);

    // And no longer once the file is a model again
    files["device.gltf"] = modelFile();
    document.notifyAssetChanged ("device.gltf");
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    ASSERT_EQ (document.findById<SceneModelComponent> ("model"), model);

    EXPECT_FALSE (view->isLoading());
    EXPECT_TRUE (model->getInstance().isValid());
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (reads["device.gltf"], 3);
}

TEST_F (SceneModelComponentTests, ATimeGivenWhileTheFileIsLoadedIsKeptFromASliderLimitedToTheDuration)
{
    YuiDocument document;
    serve (document);

    ASSERT_TRUE (document.loadFromString (robotWith ("clip: \"Smile\"; time: 1.5;", "Slider { id: scrub; maximum: robot.duration; value <=> robot.time; }")).wasOk());

    auto* view = document.findById<SceneComponent> ("view");
    auto* model = document.findById<SceneModelComponent> ("robot");
    auto* scrub = document.findById<Slider> ("scrub");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (model, nullptr);
    ASSERT_NE (scrub, nullptr);

    // A model has no clip yet: it is nowhere and lasts nothing, so the slider has no time to limit and give back
    EXPECT_TRUE (view->isLoading());
    EXPECT_DOUBLE_EQ (model->getDuration(), 0.0);
    EXPECT_DOUBLE_EQ (model->getTime(), 0.0);
    EXPECT_DOUBLE_EQ (scrub->getValue(), 0.0);
    EXPECT_TRUE (model->getClipNames().isEmpty());

    view->finishLoading();

    // The clip is where the document put it, and the slider with it
    EXPECT_DOUBLE_EQ (model->getDuration(), 2.0);
    EXPECT_DOUBLE_EQ (model->getTime(), 1.5);
    EXPECT_DOUBLE_EQ (scrub->getValue(), 1.5);
    EXPECT_DOUBLE_EQ (scrub->getRange().end, 2.0);
    expectWeights (*model, { 0.75f, 0.375f });
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneModelComponentTests, ASourceSetAgainBeforeItsModelIsThereShowsTheLastOneOnly)
{
    files["other.gltf"] = modelFile ("7, 8, 9");

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; Model3D { id: model; } }");

    auto* view = document.findById<SceneComponent> ("view");
    auto* model = document.findById<SceneModelComponent> ("model");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (model, nullptr);

    model->setSource ("device.gltf");
    model->setSource ("other.gltf");
    EXPECT_FALSE (model->getInstance().isValid());

    view->finishLoading();

    // One instance, of the file that was named last
    ASSERT_TRUE (model->getInstance().isValid());
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);

    auto knob = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (knob.isValid());
    expectVector (knob.get<TransformNode>()->getPosition(), 7.0f, 8.0f, 9.0f);

    // A file that changes while it is being loaded is read again: what was made of it as it was is not shown
    files["late.gltf"] = modelFile ("4, 5, 6");
    model->setSource ("late.gltf");

    files["late.gltf"] = modelFile ("1, 1, 1");
    document.notifyAssetChanged ("late.gltf");

    EXPECT_TRUE (view->isLoading());
    EXPECT_FALSE (model->getInstance().isValid());

    view->finishLoading();

    EXPECT_FALSE (view->isLoading());
    ASSERT_TRUE (model->getInstance().isValid());
    EXPECT_EQ (model->getEntity().getNumChildren(), 1);
    EXPECT_EQ (reads["late.gltf"], 2);

    knob = entityAt (*model, { "Body", "Knob" });
    ASSERT_TRUE (knob.isValid());
    expectVector (knob.get<TransformNode>()->getPosition(), 1.0f, 1.0f, 1.0f);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneModelComponentTests, ASceneOrAModelThatGoesWhileAFileIsLoadedLeavesNothingBehind)
{
    {
        // The scene goes with its document, the files of its two models still with its thread
        YuiDocument document;
        serve (document);

        ASSERT_TRUE (document.loadFromString (robotWith ("playing: true;", "Model3D { source: \"device.gltf\"; }")).wasOk());

        auto* view = document.findById<SceneComponent> ("view");
        ASSERT_NE (view, nullptr);
        EXPECT_TRUE (view->isLoading());
    }

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; Node3D { } }");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);

    {
        // A model goes alone: its file is loaded all the same, for nothing to show
        SceneModelComponent model;
        view->addAndMakeVisible (model);
        model.setSource ("device.gltf");

        EXPECT_TRUE (view->isLoading());
    }

    view->finishLoading();

    EXPECT_FALSE (view->isLoading());
    EXPECT_EQ (view->getScene()->getRoot().getNumChildren(), 1);
    EXPECT_EQ (reads["device.gltf"], 2);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneModelComponentTests, AFileThatIsNoModelIsReportedOnceItIsLoaded)
{
    files["broken.gltf"] = bytesOf ("this is not a model");

    const String text ("Scene3D {\n"
                       "    id: view;\n"
                       "    Model3D { id: broken; source: \"broken.gltf\"; }\n"
                       "}");

    YuiDocument document;
    serve (document);

    int told = 0;

    const DiagnosticListener listener (document, [&told] (const DataTreeDiagnostic&)
    {
        ++told;
    });

    ASSERT_TRUE (document.loadFromString (text).wasOk());

    auto* view = document.findById<SceneComponent> ("view");
    auto* broken = document.findById<SceneModelComponent> ("broken");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (broken, nullptr);

    // Read, and not parsed yet: nothing is known to be wrong with it
    EXPECT_TRUE (view->isLoading());
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);

    view->finishLoading();

    // Told once, where the file is named, by the loader that could not parse it
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics().front().message.startsWith ("Model3D.source: ")) << document.getDiagnostics().front().message;
    EXPECT_EQ (document.getDiagnostics().front().line, 3);
    EXPECT_TRUE (document.getDiagnostics().front().isError());
    EXPECT_EQ (told, 1);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);
    EXPECT_FALSE (broken->getInstance().isValid());

    // And again by the loads that keep the model, which do not read its file again
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);

    EXPECT_FALSE (view->isLoading());
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (reads["broken.gltf"], 1);
}

TEST_F (SceneModelComponentTests, AFileThatNoElementNamesAnymoreIsLetGoOfByTheNextLoad)
{
    files["other.gltf"] = modelFile ("7, 8, 9");

    const auto text = [] (const char* source)
    {
        return "Scene3D { Model3D { id: model; source: \"" + String (source) + "\"; } Model3D { source: \"robot.gltf\"; } }";
    };

    YuiDocument document;
    serve (document);
    load (document, text ("device.gltf"));

    EXPECT_EQ (reads["device.gltf"], 1);
    EXPECT_EQ (reads["robot.gltf"], 1);

    // The first model shows another file: nothing names the one it showed once the document has loaded
    load (document, text ("other.gltf"));

    // Named again, that file is read again. The one that was named all along was kept
    load (document, text ("device.gltf"));

    auto* model = document.findById<SceneModelComponent> ("model");
    ASSERT_NE (model, nullptr);
    ASSERT_TRUE (model->getInstance().isValid());

    EXPECT_EQ (reads["device.gltf"], 2);
    EXPECT_EQ (reads["other.gltf"], 1);
    EXPECT_EQ (reads["robot.gltf"], 1);
}

//==============================================================================

TEST_F (SceneModelComponentTests, ClipPropertiesAreGivenToTheAnimationAndReadBack)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("clip: \"Smile\"; speed: 1.5; loop: true; time: 0.5;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    const auto* animation = robot.model->getInstance().get<AnimationNode>();
    ASSERT_NE (animation, nullptr);

    EXPECT_EQ (animation->getCurrentClip(), 1);
    EXPECT_FLOAT_EQ (animation->getSpeed(), 1.5f);
    EXPECT_TRUE (animation->isLooping());
    EXPECT_FALSE (animation->isPlaying());
    EXPECT_DOUBLE_EQ (animation->getTime(), 0.5);

    EXPECT_EQ (robot.model->getClip(), "Smile");
    EXPECT_FLOAT_EQ (robot.model->getSpeed(), 1.5f);
    EXPECT_TRUE (robot.model->isLooping());
    EXPECT_FALSE (robot.model->isPlaying());
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
    EXPECT_DOUBLE_EQ (robot.model->getDuration(), 2.0);
    EXPECT_EQ (robot.model->getClipNames().joinIntoString (", "), "Bend, Smile");

    // They are properties like the others
    EXPECT_EQ (robot.model->getProperty (Identifier ("clip")).toString(), "Smile");
    EXPECT_DOUBLE_EQ (static_cast<double> (robot.model->getProperty (Identifier ("speed"))), 1.5);
    EXPECT_TRUE (static_cast<bool> (robot.model->getProperty (Identifier ("loop"))));
    EXPECT_FALSE (static_cast<bool> (robot.model->getProperty (Identifier ("playing"))));
    EXPECT_DOUBLE_EQ (static_cast<double> (robot.model->getProperty (Identifier ("time"))), 0.5);
    EXPECT_DOUBLE_EQ (static_cast<double> (robot.model->getProperty (Identifier ("duration"))), 2.0);

    const auto clips = robot.model->getProperty (Identifier ("clips"));
    const auto* names = clips.getArray();
    ASSERT_NE (names, nullptr);
    ASSERT_EQ (names->size(), 2);
    EXPECT_EQ ((*names)[0].toString(), "Bend");
    EXPECT_EQ ((*names)[1].toString(), "Smile");

    // A clip that is named or moved poses the model at once, a quarter of the way here, with no frame updating the scene
    expectWeights (*robot.model, { 0.25f, 0.125f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.25f, 0.0f);
    expectVector (vertexOf (*robot.model, "Face", 1), 1.25f, 0.0f, 0.0f);
}

TEST_F (SceneModelComponentTests, APlayingClipAdvancesWithTheFramesOfItsScene)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("playing: true;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto& counter = robot.view->getScene()->getRoot().attach<FrameCounter>();

    // Without a name the model plays its first clip, from the pose of its file
    EXPECT_TRUE (robot.model->isPlaying());
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);
    EXPECT_DOUBLE_EQ (robot.model->getDuration(), 1.0);
    expectRotation (tipRotation (*robot.model), Quaternion());

    Internals::takeRepaintRequest (*robot.view);
    robot.view->refreshDisplay (0.25);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    expectRotation (tipRotation (*robot.model), bendAt (0.25f));
    EXPECT_TRUE (Internals::takeRepaintRequest (*robot.view));

    robot.view->refreshDisplay (0.25);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
    expectRotation (tipRotation (*robot.model), bendAt (0.5f));
    EXPECT_TRUE (Internals::takeRepaintRequest (*robot.view));

    // The strip follows its joint in the update that turned it: its top left corner, 1 above Tip, leans halfway back
    expectVector (vertexOf (*robot.model, "Strip", 2), 0.0f, 1.0f + std::sqrt (0.5f), std::sqrt (0.5f));
    expectVector (vertexOf (*robot.model, "Strip", 0), 0.0f, 0.0f, 0.0f);

    // The scene was updated once for each frame, with the time of the frame
    EXPECT_EQ (counter.numFrames, 2);
    EXPECT_DOUBLE_EQ (counter.seconds, 0.5);
}

TEST_F (SceneModelComponentTests, AModelThatDoesNotPlayAsksNothingOfTheFrames)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith (""));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto& counter = robot.view->getScene()->getRoot().attach<FrameCounter>();

    robot.view->refreshDisplay (0.25);
    Internals::takeRepaintRequest (*robot.view);

    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 0);
    EXPECT_FALSE (Internals::takeRepaintRequest (*robot.view));
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);

    // Playing, the frames move the clip
    robot.model->setPlaying (true);
    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 1);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);

    // Paused, it stays where it is
    robot.model->setPlaying (false);
    Internals::takeRepaintRequest (*robot.view);

    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 1);
    EXPECT_FALSE (Internals::takeRepaintRequest (*robot.view));
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);

    // A model that is not shown is not updated: its clip stands still, and the scene has nothing to draw again
    robot.model->setPlaying (true);
    robot.model->setVisible (false);
    Internals::takeRepaintRequest (*robot.view);

    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 1);
    EXPECT_FALSE (Internals::takeRepaintRequest (*robot.view));
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);

    robot.model->setVisible (true);
    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 2);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
}

TEST_F (SceneModelComponentTests, APlayingClipTakesTheFramesOfItsWindowUntilItStops)
{
    Component window;
    auto& native = makeWindow (window);

    YuiDocument document;
    serve (document);
    load (document, robotWith (""));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    window.addChildComponent (*robot.view);

    // A model that stands still leaves the window free to skip its frames
    sendFrames (native, 3);
    ASSERT_FALSE (native.hasFrameListeners());

    robot.model->setPlaying (true);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.25);
    native.callFrameListeners (0.25);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
    EXPECT_TRUE (native.hasFrameListeners());

    // The clip stops at its end, 1 second in: nothing is left for the frames to move
    native.callFrameListeners (0.75);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 1.0);

    sendFrames (native, 3);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (SceneModelComponentTests, TheSpeedScalesTheTimeOfTheFramesAndANegativeOnePlaysBackwards)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("speed: 2; playing: true;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    int numEnded = 0;
    robot.model->onClipEnded = [&numEnded]
    {
        ++numEnded;
    };

    robot.view->refreshDisplay (0.125);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    expectRotation (tipRotation (*robot.model), bendAt (0.25f));

    // Turned around, the clip goes back the way it came
    robot.model->setProperty (Identifier ("speed"), -1.0);
    EXPECT_FLOAT_EQ (robot.model->getSpeed(), -1.0f);

    robot.view->refreshDisplay (0.125);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.125);
    expectRotation (tipRotation (*robot.model), bendAt (0.125f));
    EXPECT_EQ (numEnded, 0);

    // Its start is then the end it plays towards, where it stops
    robot.view->refreshDisplay (0.5);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);
    expectRotation (tipRotation (*robot.model), Quaternion());
    EXPECT_EQ (numEnded, 1);
}

TEST_F (SceneModelComponentTests, AModelAskedToPlayBackwardsStartsAtTheEndOfItsClip)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("speed: -1; playing: true;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 1.0);

    robot.view->refreshDisplay (0.25);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.75);
    expectRotation (tipRotation (*robot.model), bendAt (0.75f));
}

TEST_F (SceneModelComponentTests, AClipThatDoesNotLoopStopsAtItsEndAndTellsOnce)
{
    int numStopped = 0;

    YuiDocument document;
    serve (document);
    document.setAction ("stopped", [&numStopped] (const Array<var>&)
    {
        ++numStopped;
    });
    load (document, robotWith ("playing: true; onClipEnded: action (\"stopped\");"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto& counter = robot.view->getScene()->getRoot().attach<FrameCounter>();
    ChangeCounter playingChanges (*robot.model, "playing");

    robot.view->refreshDisplay (0.75);
    EXPECT_EQ (numStopped, 0);

    robot.view->refreshDisplay (0.75);
    EXPECT_EQ (numStopped, 1);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 1.0);
    expectRotation (tipRotation (*robot.model), bendAt (1.0f));

    // The model stays asked to play, for what binds `playing` to keep doing so, with its clip stopped at its end: no
    // frame moves it, nor draws the scene again
    EXPECT_TRUE (robot.model->isPlaying());
    EXPECT_EQ (playingChanges.count, 0);

    const auto* animation = robot.model->getInstance().get<AnimationNode>();
    ASSERT_NE (animation, nullptr);
    EXPECT_FALSE (animation->isPlaying());

    Internals::takeRepaintRequest (*robot.view);
    robot.view->refreshDisplay (0.75);

    EXPECT_EQ (numStopped, 1);
    EXPECT_EQ (counter.numFrames, 2);
    EXPECT_FALSE (Internals::takeRepaintRequest (*robot.view));

    // Asked again, it plays from its start, where the model is posed at once
    robot.model->setPlaying (false);
    robot.model->setPlaying (true);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);
    expectRotation (tipRotation (*robot.model), Quaternion());

    robot.view->refreshDisplay (0.25);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    EXPECT_EQ (numStopped, 1);
    EXPECT_EQ (counter.numFrames, 3);
}

TEST_F (SceneModelComponentTests, AClipThatLoopsCarriesOnFromItsOtherEnd)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("loop: true; playing: true;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    int numEnded = 0;
    robot.model->onClipEnded = [&numEnded]
    {
        ++numEnded;
    };

    robot.view->refreshDisplay (0.75);
    robot.view->refreshDisplay (0.5);

    EXPECT_NEAR (robot.model->getTime(), 0.25, 1.0e-9);
    expectRotation (tipRotation (*robot.model), bendAt (0.25f));
    EXPECT_EQ (numEnded, 0);

    // It plays on
    Internals::takeRepaintRequest (*robot.view);
    robot.view->refreshDisplay (0.25);

    EXPECT_NEAR (robot.model->getTime(), 0.5, 1.0e-9);
    EXPECT_TRUE (Internals::takeRepaintRequest (*robot.view));
}

TEST_F (SceneModelComponentTests, MovingAPausedClipShowsItsPoseWithoutAFrame)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith (""));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto& counter = robot.view->getScene()->getRoot().attach<FrameCounter>();
    Internals::takeRepaintRequest (*robot.view);

    robot.model->setTime (0.5);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
    expectRotation (tipRotation (*robot.model), bendAt (0.5f));

    // The strip follows its joint at once, and the scene is asked to draw it: nothing else updates a scene that plays no clip
    expectVector (vertexOf (*robot.model, "Strip", 2), 0.0f, 1.0f + std::sqrt (0.5f), std::sqrt (0.5f));
    EXPECT_TRUE (Internals::takeRepaintRequest (*robot.view));

    // The time is limited to the clip
    robot.model->setProperty (Identifier ("time"), 5.0);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 1.0);
    expectVector (vertexOf (*robot.model, "Strip", 2), 0.0f, 1.0f, 1.0f);

    robot.model->setTime (-1.0);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);
    expectVector (vertexOf (*robot.model, "Strip", 2), 0.0f, 2.0f, 0.0f);

    // Another clip is posed at its start the same way: the face loses the weights its mesh gave it
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.5f, 0.0f);
    Internals::takeRepaintRequest (*robot.view);

    robot.model->setClip ("Smile");

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);
    EXPECT_DOUBLE_EQ (robot.model->getDuration(), 2.0);
    expectWeights (*robot.model, { 0.0f, 0.0f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.0f, 0.0f);
    EXPECT_TRUE (Internals::takeRepaintRequest (*robot.view));

    EXPECT_EQ (counter.numFrames, 0);
}

TEST_F (SceneModelComponentTests, TheTimeTellsWhatFollowsItOnceForEachFrameWhileTheClipPlays)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("", "Node3D { id: probe; position: [robot.time, 0, 0]; } Slider { id: scrub; maximum: robot.duration; value <=> robot.time; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* probe = document.findById<SceneNodeComponent> ("probe");
    auto* scrub = document.findById<Slider> ("scrub");

    ASSERT_NE (probe, nullptr);
    ASSERT_NE (scrub, nullptr);

    ChangeCounter timeChanges (*robot.model, "time");

    // Paused, a frame tells nothing
    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (timeChanges.count, 0);
    expectVector (positionOf (*probe), 0.0f, 0.0f, 0.0f);

    robot.model->setPlaying (true);
    timeChanges.count = 0;

    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (timeChanges.count, 1);
    expectVector (positionOf (*probe), 0.25f, 0.0f, 0.0f);
    EXPECT_DOUBLE_EQ (scrub->getValue(), 0.25);

    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (timeChanges.count, 2);
    expectVector (positionOf (*probe), 0.5f, 0.0f, 0.0f);
    EXPECT_DOUBLE_EQ (scrub->getValue(), 0.5);

    // The slider moves the clip, which plays on from there
    scrub->setValue (0.75);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.75);
    expectRotation (tipRotation (*robot.model), bendAt (0.75f));
    expectVector (positionOf (*probe), 0.75f, 0.0f, 0.0f);

    robot.view->refreshDisplay (0.125);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.875);
    EXPECT_DOUBLE_EQ (scrub->getValue(), 0.875);
}

TEST_F (SceneModelComponentTests, WhatABindingMovesWhenToldTheTimeIsDeformedByTheSameUpdate)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("playing: true; Node3D { id: tip; node: \"Tip\"; rotation: [0, 0, robot.time * 90deg]; }", "Node3D { id: other; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* other = document.findById<SceneNodeComponent> ("other");
    ASSERT_NE (other, nullptr);

    const auto expectTheBoundTurn = [&robot]
    {
        expectRotation (tipRotation (*robot.model), Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi * 0.5f));
        expectVector (vertexOf (*robot.model, "Strip", 2), -std::sqrt (0.5f), 1.0f + std::sqrt (0.5f), 0.0f);
    };

    robot.view->refreshDisplay (0.25);
    robot.view->refreshDisplay (0.25);

    // The clip turned the joint about x, then the node, told where the clip is, turned it about z: the last one to write
    // in the frame shows, and the strip is deformed after both
    expectTheBoundTurn();

    // What a paint does first: the scene follows what the node changed, with no time going by, so the clip leaves the
    // joint as the node turned it
    robot.view->updateScene();
    expectTheBoundTurn();

    // As it does when something else changes between the frame and the paint
    other->setProperty (Identifier ("position"), numbers ({ 1.0, 0.0, 0.0 }));
    robot.view->updateScene();

    expectVector (positionOf (*other), 1.0f, 0.0f, 0.0f);
    expectTheBoundTurn();
}

TEST_F (SceneModelComponentTests, AClipAndABoundNodeDrivingTheSameJointTakeTurns)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("Node3D { id: tip; node: \"Tip\"; rotation: [0, 0, 90deg]; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* tip = document.findById<SceneNodeComponent> ("tip");
    ASSERT_NE (tip, nullptr);

    // The node wrote last
    expectRotation (tipRotation (*robot.model), Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi));

    // The clip writes each time it poses the model
    robot.model->setTime (0.5);
    expectRotation (tipRotation (*robot.model), bendAt (0.5f));

    // The node writes when its value changes
    tip->setProperty (Identifier ("rotation"), numbers ({ 0.0, 0.0, 0.0 }));
    expectRotation (tipRotation (*robot.model), Quaternion());

    // A clip that starts playing writes at once, where it is, then on every frame in which time goes by
    robot.model->setPlaying (true);
    expectRotation (tipRotation (*robot.model), bendAt (0.5f));

    robot.view->refreshDisplay (0.25);
    expectRotation (tipRotation (*robot.model), bendAt (0.75f));

    // A frame in which none does leaves the joint to the node
    tip->setProperty (Identifier ("rotation"), numbers ({ 0.0, 0.0, MathConstants<double>::halfPi }));
    robot.view->refreshDisplay (0.0);
    expectRotation (tipRotation (*robot.model), Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi));
}

TEST_F (SceneModelComponentTests, AClipIsNamedOrCountedAndOneTheModelDoesNotHaveIsReported)
{
    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Model3D { id: counted; source: \"robot.gltf\"; clip: \"1\"; }\n"
                                                 "    Model3D { id: unknown; source: \"robot.gltf\"; clip: \"Jump\"; playing: true; }\n"
                                                 "    Model3D { id: still; source: \"device.gltf\"; clip: \"Open\"; }\n"
                                                 "    Model3D { id: quiet; source: \"device.gltf\"; playing: true; speed: 2; time: 0.5; }\n"
                                                 "    Model3D { id: missing; source: \"nowhere.gltf\"; clip: \"Open\"; }\n"
                                                 "}");

    finishLoading (document);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // A model that could not be loaded says so itself, and one without clips only when it is asked for one
    expectDiagnostic (document, "Model3D.clip: the model has no clip 'Jump'");
    expectDiagnostic (document, "Model3D.clip: the model has no clip 'Open'");
    expectDiagnostic (document, "Model3D.source: the file 'nowhere.gltf' can't be read");
    EXPECT_EQ (document.getDiagnostics().size(), 3u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* diagnostic = findDiagnostic (document, "'Jump'");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 4);

    auto* view = document.findById<SceneComponent> ("view");
    auto* counted = document.findById<SceneModelComponent> ("counted");
    auto* unknown = document.findById<SceneModelComponent> ("unknown");
    auto* quiet = document.findById<SceneModelComponent> ("quiet");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (counted, nullptr);
    ASSERT_NE (unknown, nullptr);
    ASSERT_NE (quiet, nullptr);
    showScene (*view);

    // A number is the index of a clip when no clip has it as its name
    EXPECT_EQ (counted->getClip(), "1");
    EXPECT_DOUBLE_EQ (counted->getDuration(), 2.0);

    // A model without the clip it is asked for plays nothing
    auto& counter = view->getScene()->getRoot().attach<FrameCounter>();

    EXPECT_TRUE (unknown->isPlaying());
    EXPECT_DOUBLE_EQ (unknown->getDuration(), 0.0);
    EXPECT_EQ (unknown->getClipNames().size(), 2);

    view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 0);
    EXPECT_DOUBLE_EQ (unknown->getTime(), 0.0);

    // A model without clips keeps what it was asked, for the file that has some
    EXPECT_TRUE (quiet->isPlaying());
    EXPECT_FLOAT_EQ (quiet->getSpeed(), 2.0f);
    EXPECT_DOUBLE_EQ (quiet->getTime(), 0.5);
    EXPECT_DOUBLE_EQ (quiet->getDuration(), 0.0);
    EXPECT_TRUE (quiet->getClipNames().isEmpty());

    files["device.gltf"] = robotFile();
    document.notifyAssetChanged ("device.gltf");
    finishLoading (document);

    EXPECT_DOUBLE_EQ (quiet->getDuration(), 1.0);
    EXPECT_DOUBLE_EQ (quiet->getTime(), 0.5);
    expectRotation (tipRotation (*quiet), bendAt (0.5f));

    // A clip the model has plays, as the model was asked all along
    unknown->setClip ("Bend");
    EXPECT_DOUBLE_EQ (unknown->getDuration(), 1.0);

    view->refreshDisplay (0.25);
    EXPECT_EQ (counter.numFrames, 1);
    EXPECT_DOUBLE_EQ (unknown->getTime(), 0.25);
}

TEST_F (SceneModelComponentTests, AModelInstantiatedAgainPlaysOnAsItWas)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("clip: \"Smile\"; speed: 2; loop: true; playing: true;", "Node3D { id: group; position: [0, 3, 0]; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* group = document.findById<SceneNodeComponent> ("group");
    ASSERT_NE (group, nullptr);

    const auto expectPlaying = [&robot] (double seconds)
    {
        const auto* animation = robot.model->getInstance().get<AnimationNode>();
        ASSERT_NE (animation, nullptr);

        EXPECT_EQ (animation->getCurrentClip(), 1);
        EXPECT_FLOAT_EQ (animation->getSpeed(), 2.0f);
        EXPECT_TRUE (animation->isLooping());
        EXPECT_TRUE (animation->isPlaying());
        EXPECT_DOUBLE_EQ (animation->getTime(), seconds);
        EXPECT_DOUBLE_EQ (robot.model->getTime(), seconds);
    };

    robot.view->refreshDisplay (0.25);
    expectPlaying (0.5);

    // The file changed: another instance, playing the same clip from the same place, where it is posed at once
    const auto first = robot.model->getInstance();
    document.notifyAssetChanged ("robot.gltf");
    finishLoading (document);

    EXPECT_FALSE (first.isValid());
    ASSERT_TRUE (robot.model->getInstance().isValid());
    expectPlaying (0.5);
    expectWeights (*robot.model, { 0.25f, 0.125f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.25f, 0.0f);

    robot.view->refreshDisplay (0.25);
    expectPlaying (1.0);

    // Moved to another parent of its scene, the model keeps its entity and the instance under it: its clip plays on
    const auto second = robot.model->getInstance();
    group->addAndMakeVisible (*robot.model);

    EXPECT_TRUE (second.isValid());
    EXPECT_EQ (robot.model->getInstance(), second);
    EXPECT_EQ (robot.model->getEntity().getParent(), group->getEntity());
    expectPlaying (1.0);

    robot.view->refreshDisplay (0.25);
    EXPECT_EQ (robot.model->getInstance(), second);
    expectPlaying (1.5);

    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneModelComponentTests, ModelsPlayingTogetherUpdateTheirSceneOnceForEachFrame)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("playing: true;", "Model3D { id: other; source: \"robot.gltf\"; clip: \"Smile\"; speed: 2; playing: true; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* other = document.findById<SceneModelComponent> ("other");
    ASSERT_NE (other, nullptr);

    auto& counter = robot.view->getScene()->getRoot().attach<FrameCounter>();

    robot.view->refreshDisplay (0.25);

    EXPECT_EQ (counter.numFrames, 1);
    EXPECT_DOUBLE_EQ (counter.seconds, 0.25);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    EXPECT_DOUBLE_EQ (other->getTime(), 0.5);

    // A scene that is updated on every frame anyway is not updated again for its models
    robot.view->setContinuousUpdates (true);
    robot.view->refreshDisplay (0.25);

    EXPECT_EQ (counter.numFrames, 2);
    EXPECT_DOUBLE_EQ (counter.seconds, 0.5);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
    EXPECT_DOUBLE_EQ (other->getTime(), 1.0);
}

TEST_F (SceneModelComponentTests, AClipThatEndsMayLetGoOfTheSceneBeingUpdated)
{
    /** Hears whether the model still tells about its clip once the handler of its end has run. */
    struct Witness final : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            if (numDone > 0 && StringArray ({ "time", "duration", "clips", "playing" }).contains (name.toString()))
                ++numToldAfterwards;
        }

        int numDone = 0;
        int numToldAfterwards = 0;
    };

    // The scene is updated for its clip, or on every frame as well: the handler runs inside that update either way
    for (const auto* continuous : { "false", "true" })
    {
        for (const auto isLoadingAnother : { false, true })
        {
            SCOPED_TRACE (String ("continuous: ") + continuous + (isLoadingAnother ? ", another document is loaded" : ", the document is cleared"));

            // Before the document, whose components it hears go
            Witness witness;

            YuiDocument document;
            serve (document);

            // What a handler may do: nothing is left of the scene that is being updated, nor of the model that tells it
            document.setAction ("done", [&] (const Array<var>&)
            {
                ++witness.numDone;

                if (isLoadingAnother)
                    EXPECT_TRUE (document.loadFromString ("Label { text: \"Done\"; }").wasOk());
                else
                    document.clear();
            });

            load (document, "Scene3D { id: view; continuous: " + String (continuous) + "; Model3D { id: robot; source: \"robot.gltf\"; playing: true; onClipEnded: action (\"done\"); } }");

            auto* view = document.findById<SceneComponent> ("view");
            auto* model = document.findById<SceneModelComponent> ("robot");

            ASSERT_NE (view, nullptr);
            ASSERT_NE (model, nullptr);

            showScene (*view);
            model->addComponentListener (&witness);

            view->refreshDisplay (0.75);
            ASSERT_EQ (witness.numDone, 0);
            ASSERT_EQ (document.findById<SceneComponent> ("view"), view);

            // The frame that ends the clip. Only the component keeps its scene, which has to outlive the update it is in
            view->refreshDisplay (0.75);

            EXPECT_EQ (witness.numDone, 1);
            EXPECT_EQ (witness.numToldAfterwards, 0);
            EXPECT_EQ (document.findById ("view"), nullptr);
            EXPECT_EQ (document.findById ("robot"), nullptr);
            EXPECT_EQ (document.getRoot() != nullptr, isLoadingAnother);
        }
    }
}

TEST_F (SceneModelComponentTests, AClipNamedWhileTheModelPlaysBackwardsStartsAtItsEnd)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("speed: -1; playing: true;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    int numEnded = 0;
    robot.model->onClipEnded = [&numEnded]
    {
        ++numEnded;
    };

    robot.view->refreshDisplay (0.5);
    ASSERT_DOUBLE_EQ (robot.model->getTime(), 0.5);

    // Its start is the end a clip played backwards runs into: it is put at its end, where the model is posed at once
    robot.model->setClip ("Smile");

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 2.0);
    EXPECT_DOUBLE_EQ (robot.model->getDuration(), 2.0);
    expectWeights (*robot.model, { 1.0f, 0.5f });

    const auto* animation = robot.model->getInstance().get<AnimationNode>();
    ASSERT_NE (animation, nullptr);
    EXPECT_TRUE (animation->isPlaying());

    // And plays from there
    robot.view->refreshDisplay (0.5);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 1.5);
    expectWeights (*robot.model, { 0.75f, 0.375f });
    EXPECT_EQ (numEnded, 0);
}

TEST_F (SceneModelComponentTests, ASliderLimitedToTheDurationFollowsTheClipThatIsNamed)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("clip: \"Smile\"; playing: true;", "Slider { id: scrub; maximum: robot.duration; value <=> robot.time; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* scrub = document.findById<Slider> ("scrub");
    ASSERT_NE (scrub, nullptr);

    // Where the clip is and how long it is, for the model and for the slider that follows both
    const auto expectClipAt = [&robot, scrub] (double seconds, double duration)
    {
        EXPECT_DOUBLE_EQ (robot.model->getTime(), seconds);
        EXPECT_DOUBLE_EQ (robot.model->getDuration(), duration);
        EXPECT_DOUBLE_EQ (scrub->getValue(), seconds);
        EXPECT_DOUBLE_EQ (scrub->getRange().end, duration);
    };

    robot.view->refreshDisplay (1.5);
    expectClipAt (1.5, 2.0);

    // A shorter clip. The slider hears where the clip is before how long it is: with its range shrunk first, it would
    // limit the value it still held and give the clip its end as the place to be
    robot.model->setClip ("Bend");
    expectClipAt (0.0, 1.0);

    robot.view->refreshDisplay (0.25);
    expectClipAt (0.25, 1.0);

    // A clip the model does not have: nothing plays, the clip is nowhere and lasts nothing
    robot.model->setClip ("Jump");
    expectClipAt (0.0, 0.0);

    robot.view->refreshDisplay (0.25);
    expectClipAt (0.0, 0.0);

    // A longer clip, which plays as the model was asked all along
    robot.model->setClip ("Smile");
    expectClipAt (0.0, 2.0);

    robot.view->refreshDisplay (0.5);
    expectClipAt (0.5, 2.0);

    // Backwards a clip starts at its end, which a shorter one has inside the range the slider still has
    robot.model->setSpeed (-1.0f);
    robot.model->setClip ("Bend");
    expectClipAt (1.0, 1.0);

    robot.view->refreshDisplay (0.25);
    expectClipAt (0.75, 1.0);

    // A longer one has it past that range. The slider hears how long the clip is before where it is: told where first, it
    // would stay at the end of the range it had
    robot.model->setClip ("Smile");
    expectClipAt (2.0, 2.0);

    robot.view->refreshDisplay (0.25);
    expectClipAt (1.75, 2.0);
}

TEST_F (SceneModelComponentTests, AnotherSourceOrSceneStartsOverAClipThatHadEnded)
{
    files["slow.gltf"] = robotFile (4.0f);

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; Model3D { id: robot; source: \"slow.gltf\"; playing: true; } Slider { id: scrub; maximum: robot.duration; value <=> robot.time; } }");

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* scrub = document.findById<Slider> ("scrub");
    ASSERT_NE (scrub, nullptr);

    int numEnded = 0;
    robot.model->onClipEnded = [&numEnded]
    {
        ++numEnded;
    };

    // The clip of the first file lasts 4 seconds, and ends
    robot.view->refreshDisplay (5.0);

    ASSERT_EQ (numEnded, 1);
    ASSERT_DOUBLE_EQ (robot.model->getTime(), 4.0);
    ASSERT_DOUBLE_EQ (scrub->getValue(), 4.0);

    // Another file is another model: its clip, a second long, is at its start and plays, and the slider is there with it
    robot.model->setSource ("robot.gltf");
    finishLoading (document);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);
    EXPECT_DOUBLE_EQ (robot.model->getDuration(), 1.0);
    EXPECT_DOUBLE_EQ (scrub->getValue(), 0.0);
    EXPECT_DOUBLE_EQ (scrub->getRange().end, 1.0);

    robot.view->refreshDisplay (0.25);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    expectRotation (tipRotation (*robot.model), bendAt (0.25f));
    EXPECT_EQ (numEnded, 1);

    // As is another scene of the file
    robot.view->refreshDisplay (1.0);

    ASSERT_EQ (numEnded, 2);
    ASSERT_DOUBLE_EQ (robot.model->getTime(), 1.0);

    robot.model->setSceneIndex (1);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.0);

    robot.view->refreshDisplay (0.25);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    EXPECT_EQ (numEnded, 2);

    // A file that changes is the same model, which carries on from where it was
    document.notifyAssetChanged ("robot.gltf");
    finishLoading (document);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);

    robot.view->refreshDisplay (0.25);
    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
}

TEST_F (SceneModelComponentTests, WhatIsNotANumberIsNeitherATimeNorASpeed)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("playing: true;"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    int numEnded = 0;
    robot.model->onClipEnded = [&numEnded]
    {
        ++numEnded;
    };

    robot.view->refreshDisplay (0.25);

    ChangeCounter timeChanges (*robot.model, "time");
    ChangeCounter speedChanges (*robot.model, "speed");

    const auto notANumber = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<double>::infinity();

    robot.model->setTime (notANumber);
    robot.model->setProperty (Identifier ("time"), notANumber);
    robot.model->setSpeed (static_cast<float> (notANumber));
    robot.model->setSpeed (static_cast<float> (infinity));
    robot.model->setProperty (Identifier ("speed"), -infinity);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.25);
    EXPECT_FLOAT_EQ (robot.model->getSpeed(), 1.0f);
    EXPECT_EQ (timeChanges.count, 0);
    EXPECT_EQ (speedChanges.count, 0);

    // The clip plays on, tells where it is once for a frame, and ends
    robot.view->refreshDisplay (0.25);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 0.5);
    EXPECT_EQ (timeChanges.count, 1);

    robot.view->refreshDisplay (1.0);

    EXPECT_DOUBLE_EQ (robot.model->getTime(), 1.0);
    EXPECT_EQ (numEnded, 1);
}

//==============================================================================

TEST_F (SceneModelComponentTests, ABoundNodeSetsTheWeightsOfTheMorphTargetsOfItsEntity)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("Node3D { id: face; node: \"Face\"; morphWeights: [1, 0]; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* face = document.findById<SceneNodeComponent> ("face");
    ASSERT_NE (face, nullptr);

    const Identifier morphWeights ("morphWeights");

    // The first target lifts the last vertex by 1, and the mesh follows at once, with no frame updating the scene
    expectWeights (*robot.model, { 1.0f, 0.0f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 2.0f, 0.0f);
    expectVector (vertexOf (*robot.model, "Face", 1), 1.0f, 0.0f, 0.0f);
    expectNumbers (face->getProperty (morphWeights), { 1.0, 0.0 });

    // A target the node gives no weight keeps the one of the model, and the weights of targets the mesh does not have are ignored
    Internals::takeRepaintRequest (*robot.view);
    face->setProperty (morphWeights, numbers ({ 0.75 }));
    expectWeights (*robot.model, { 0.75f, 0.25f });
    EXPECT_TRUE (Internals::takeRepaintRequest (*robot.view));

    face->setProperty (morphWeights, numbers ({ 0.0, 1.0, 5.0 }));
    expectWeights (*robot.model, { 0.0f, 1.0f });
    expectVector (vertexOf (*robot.model, "Face", 1), 3.0f, 0.0f, 0.0f);

    // No longer set, the mesh gets back the weights it had
    face->setProperty (morphWeights, var());
    EXPECT_TRUE (face->getProperty (morphWeights).isVoid());
    expectWeights (*robot.model, { 0.5f, 0.25f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.5f, 0.0f);
    expectVector (vertexOf (*robot.model, "Face", 1), 1.5f, 0.0f, 0.0f);

    // As it does when the node goes
    face->setProperty (morphWeights, numbers ({ 1.0, 1.0 }));
    expectWeights (*robot.model, { 1.0f, 1.0f });

    load (document, robotWith (""));
    ASSERT_EQ (document.findById<SceneModelComponent> ("robot"), robot.model);
    expectWeights (*robot.model, { 0.5f, 0.25f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.5f, 0.0f);
}

TEST_F (SceneModelComponentTests, MorphWeightsOfAnEntityWithoutMorphTargetsAreReported)
{
    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    Model3D {\n"
                                                 "        id: robot; source: \"robot.gltf\";\n"
                                                 "        Node3D { id: strip; node: \"Strip\"; morphWeights: [1, 0]; }\n"
                                                 "        Node3D { id: face; node: \"Face\"; morphWeights: [1, 0]; }\n"
                                                 "    }\n"
                                                 "    Node3D { id: plain; morphWeights: [0.5]; }\n"
                                                 "}");

    finishLoading (document);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // The strip is deformed by a skin alone, and a node of its own draws nothing
    std::vector<int> lines;

    for (const auto& diagnostic : document.getDiagnostics())
    {
        EXPECT_EQ (diagnostic.message, "Node3D.morphWeights: the entity of the node draws no mesh with morph targets");
        EXPECT_TRUE (diagnostic.isError());
        lines.push_back (diagnostic.line);
    }

    std::sort (lines.begin(), lines.end());
    EXPECT_EQ (lines, (std::vector<int> { 4, 7 }));

    // What is not a list of numbers sets no weight, and says so
    auto* robot = document.findById<SceneModelComponent> ("robot");
    auto* face = document.findById<SceneNodeComponent> ("face");

    ASSERT_NE (robot, nullptr);
    ASSERT_NE (face, nullptr);
    expectWeights (*robot, { 1.0f, 0.0f });

    face->setProperty (Identifier ("morphWeights"), var ("wide"));
    expectDiagnostic (document, "Node3D.morphWeights: expected numbers");
    expectWeights (*robot, { 0.5f, 0.25f });
}

TEST_F (SceneModelComponentTests, ATransitionBlendsTheWeightsOfTheMorphTargets)
{
    YuiDocument document;
    serve (document);
    load (document, "style {\n"
                    "    .face { morphWeights: [0, 0]; transition: morphWeights 100ms; }\n"
                    "    .smiling { morphWeights: [1, 0.5]; }\n"
                    "}\n"
                        + robotWith ("Node3D { id: face; class: \"face\"; node: \"Face\"; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* face = document.findById<SceneNodeComponent> ("face");
    ASSERT_NE (face, nullptr);

    auto& context = *document.getRoot()->getStyleContext();
    expectWeights (*robot.model, { 0.0f, 0.0f });

    // Two lists of as many weights are blended, one weight with the other
    face->addStyleClass (Identifier ("smiling"));
    context.refresh (0.0);
    expectWeights (*robot.model, { 0.0f, 0.0f });

    context.refresh (0.05);
    expectWeights (*robot.model, { 0.5f, 0.25f }, 1.0e-3f);
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 1.5f, 0.0f, 1.0e-3f);

    context.refresh (0.06);
    expectWeights (*robot.model, { 1.0f, 0.5f });
    expectVector (vertexOf (*robot.model, "Face", 2), 0.0f, 2.0f, 0.0f);

    // Without the rule the weights go back the same way, to the ones the node had
    face->removeStyleClass (Identifier ("smiling"));
    context.refresh (0.0);
    context.refresh (0.11);
    expectWeights (*robot.model, { 0.0f, 0.0f });
}

TEST_F (SceneModelComponentTests, AClipAnimatingTheWeightsAndANodeSettingThemTakeTurns)
{
    YuiDocument document;
    serve (document);
    load (document, robotWith ("clip: \"Smile\"; Node3D { id: face; node: \"Face\"; morphWeights: [1, 1]; }"));

    const Robot robot (document);
    ASSERT_TRUE (robot.isValid());

    auto* face = document.findById<SceneNodeComponent> ("face");
    ASSERT_NE (face, nullptr);

    // The node wrote last
    expectWeights (*robot.model, { 1.0f, 1.0f });

    // The clip writes each time it poses the model
    robot.model->setTime (1.0);
    expectWeights (*robot.model, { 0.5f, 0.25f });

    // The node writes when its value changes
    face->setProperty (Identifier ("morphWeights"), numbers ({ 0.75, 0.0 }));
    expectWeights (*robot.model, { 0.75f, 0.0f });

    // A clip that starts playing writes at once, where it is, then on every frame in which time goes by
    robot.model->setPlaying (true);
    expectWeights (*robot.model, { 0.5f, 0.25f });

    robot.view->refreshDisplay (0.5);
    expectWeights (*robot.model, { 0.75f, 0.375f });

    // No longer set, the mesh gets back the weights it had when the node first set them, the ones the clip had posed,
    // until the clip poses it again
    face->setProperty (Identifier ("morphWeights"), var());
    expectWeights (*robot.model, { 0.0f, 0.0f });

    robot.view->refreshDisplay (0.25);
    expectWeights (*robot.model, { 0.875f, 0.4375f });
}

// TEMPORARY-PERF-AB begin

TEST_F (SceneModelComponentTests, DISABLED_TimesWhatTheMessageThreadSpendsOnAModelAndOnAMaterialSource)
{
    const auto tell = [] (const String& what, std::vector<double> milliseconds)
    {
        std::sort (milliseconds.begin(), milliseconds.end());

        const auto line = what + " = " + String (milliseconds[milliseconds.size() / 2], 3);

        File ("/tmp/w6b-async.txt").appendText (line + "\n", false, false, "\n");
        std::cout << line << std::endl;
    };

    const auto file = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("examples/graphics/data/gltf/mpc.glb");

    if (! file.loadFileAsData (files["mpc.glb"]))
        GTEST_SKIP() << "mpc.glb is missing";

    std::vector<double> times;

    // The call that names the file of a model, in a scene that has loaded nothing yet
    for (int pass = 0; pass < 31; ++pass)
    {
        YuiDocument document;
        serve (document);
        load (document, "Scene3D { id: view; Model3D { id: model; } }");

        auto* model = document.findById<SceneModelComponent> ("model");
        ASSERT_NE (model, nullptr);

        const auto start = Time::getMillisecondCounterHiRes();
        model->setSource ("mpc.glb");
        times.push_back (Time::getMillisecondCounterHiRes() - start);
    }

    tell ("Model3D.source set to mpc.glb", times);

    // A texture as large as the ones of an application: what decoding it takes, and what the message thread spends on a
    // document naming it, with the frame that follows
    {
        Image big (2048, 2048, PixelFormat::RGBA);

        for (int y = 0; y < big.getHeight(); ++y)
            for (int x = 0; x < big.getWidth(); ++x)
                big.setPixelColor (x, y, Color (0xff000000u | (static_cast<uint32> ((x ^ y) & 0xff) << 16) | (static_cast<uint32> ((x * 7 + y * 3) & 0xff) << 8) | static_cast<uint32> (((x * y) >> 4) & 0xff)));

        {
            PngImageFormatWriter writer (new MemoryOutputStream (files["big.png"], false), big.getPixelFormat());
            ASSERT_TRUE (writer.writeImage (big));
        }

        times.clear();

        for (int pass = 0; pass < 9; ++pass)
        {
            const auto start = Time::getMillisecondCounterHiRes();
            const auto decoded = Image::loadFromData (files["big.png"].asBytes());
            times.push_back (Time::getMillisecondCounterHiRes() - start);

            ASSERT_TRUE (decoded.wasOk());
        }

        tell ("a 2048x2048 PNG of " + String (static_cast<int> (files["big.png"].getSize() / 1024)) + " KB decoded, alone", times);

        times.clear();

        for (int pass = 0; pass < 9; ++pass)
        {
            YuiDocument document;
            serve (document);

            const auto start = Time::getMillisecondCounterHiRes();

            const auto result = document.loadFromString ("Scene3D { id: view; Mesh3D { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { baseColorTexture: \"big.png\"; } } }");

            if (auto* view = showSceneOf (document))
                view->refreshDisplay (0.0);

            times.push_back (Time::getMillisecondCounterHiRes() - start);

            ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
        }

        tell ("document with a Material3D naming that texture loaded, with its frame", times);
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    files["fx/hologram.glsl"] = bytesOf (R"(YUP_MATERIAL_PARAMS
{
    vec4 rim;
    float strength;
} params;

vec4 yupMaterial (YupSurface surface)
{
    return vec4 (surface.baseColor.rgb + params.rim.rgb * params.strength, surface.baseColor.a);
}
)");

    times.clear();

    // The load of a document whose material is compiled from its source, with the frame that follows it
    for (int pass = 0; pass < 15; ++pass)
    {
        YuiDocument document;
        serve (document);

        const auto start = Time::getMillisecondCounterHiRes();

        const auto result = document.loadFromString ("shader Hologram { material: \"fx/hologram.glsl\"; property strength: number = 2; }\n"
                                                     "Scene3D { id: view; Mesh3D { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Hologram; } } }");

        if (auto* view = showSceneOf (document))
            view->refreshDisplay (0.0);

        times.push_back (Time::getMillisecondCounterHiRes() - start);

        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
        ASSERT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
        ASSERT_EQ (reads["fx/hologram.glsl"], pass + 1);
    }

    tell ("document with a Material3D shader source loaded", times);
#endif
}

// TEMPORARY-PERF-AB end
