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

/** The `Scene3D` of a `.yui` document: a SceneComponent making its scene, with the features its
    properties set and the entities of the 3D components written in it.
*/
class Scene3DTests : public SceneDocumentTests
{
protected:
    /** A PNG panorama two pixels wide and high: white above the horizon, black below it. */
    static MemoryBlock skyPng()
    {
        Image panorama (2, 2, PixelFormat::RGBA);
        panorama.fillColor (Colors::black);
        panorama.setPixelColor (0, 0, Colors::white);
        panorama.setPixelColor (1, 0, Colors::white);

        MemoryBlock bytes;

        {
            // The writer owns the stream, which trims the block to what was written when it goes
            PngImageFormatWriter writer (new MemoryOutputStream (bytes, false), panorama.getPixelFormat());
            EXPECT_TRUE (writer.writeImage (panorama));
        }

        return bytes;
    }

    /** The environment the root of the scene of a component is given, or nullptr. */
    static EnvironmentMap::Ptr environmentOf (const SceneComponent& view)
    {
        const auto* node = view.getScene() != nullptr ? view.getScene()->getRoot().get<EnvironmentNode>() : nullptr;
        return node != nullptr ? node->map : EnvironmentMap::Ptr();
    }

    /** How much red light of an environment falls on a surface facing a direction: 1 from a white half of the sky. */
    static float lightOn (const EnvironmentMap& environment, const Vector3<float>& normal)
    {
        return SphericalHarmonics::evaluateIrradiance (environment.getIrradianceCoefficients(), normal).getX();
    }
};

TEST_F (Scene3DTests, RegistersTheTypesWithTheirBasesAndFactories)
{
    registerYuiComponentTypes();

    const auto& registry = ComponentTypeRegistry::getInstance();

    const auto* scene = registry.findType (Identifier ("Scene3D"));
    const auto* node = registry.findType (Identifier ("Node3D"));
    const auto* material = registry.findType (Identifier ("Material3D"));

    ASSERT_NE (scene, nullptr);
    ASSERT_NE (node, nullptr);
    ASSERT_NE (material, nullptr);

    EXPECT_EQ (scene, &SceneComponent::getClassTypeInfo());
    EXPECT_EQ (node, &SceneNodeComponent::getClassTypeInfo());
    EXPECT_EQ (material, &SceneMaterialComponent::getClassTypeInfo());

    // The scene, a node and a material are components, the other ones are nodes
    EXPECT_EQ (scene->getBaseTypeInfo(), &Component::getClassTypeInfo());
    EXPECT_EQ (node->getBaseTypeInfo(), &Component::getClassTypeInfo());
    EXPECT_EQ (material->getBaseTypeInfo(), &Component::getClassTypeInfo());

    for (const char* name : { "Camera3D", "Light3D", "Mesh3D", "Model3D" })
    {
        const auto* type = registry.findType (Identifier (name));

        ASSERT_NE (type, nullptr) << name;
        EXPECT_EQ (type->getBaseTypeInfo(), node) << name;
        EXPECT_TRUE (type->hasProperty (Identifier ("position"))) << name;
        EXPECT_TRUE (type->hasProperty (Identifier ("visible"))) << name;
    }

    EXPECT_NE (dynamic_cast<SceneComponent*> (scene->createInstance().get()), nullptr);
    EXPECT_NE (dynamic_cast<SceneNodeComponent*> (node->createInstance().get()), nullptr);
    EXPECT_NE (dynamic_cast<SceneCameraComponent*> (registry.findType (Identifier ("Camera3D"))->createInstance().get()), nullptr);
    EXPECT_NE (dynamic_cast<SceneLightComponent*> (registry.findType (Identifier ("Light3D"))->createInstance().get()), nullptr);
    EXPECT_NE (dynamic_cast<SceneMeshComponent*> (registry.findType (Identifier ("Mesh3D"))->createInstance().get()), nullptr);
    EXPECT_NE (dynamic_cast<SceneModelComponent*> (registry.findType (Identifier ("Model3D"))->createInstance().get()), nullptr);
    EXPECT_NE (dynamic_cast<SceneMaterialComponent*> (material->createInstance().get()), nullptr);
}

TEST_F (Scene3DTests, BuildsTheSceneADocumentDescribes)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;

            Camera3D { id: camera; position: [0, 0, 4]; }
            Light3D { id: lamp; type: point; intensity: 3; }
            Mesh3D {
                id: quad;
                vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
                indices: [0, 1, 2,  0, 2, 3];

                Material3D { id: paint; baseColor: #ff0000; roughness: 0.4; }
            }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* camera = document.findById<SceneCameraComponent> ("camera");
    auto* lamp = document.findById<SceneLightComponent> ("lamp");
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    ASSERT_NE (lamp, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (paint, nullptr);

    EXPECT_EQ (view, document.getRoot());

    // The scene is the one the component made, and its entities are those of the nodes in the order they are written
    const auto scene = view->getScene();
    ASSERT_NE (scene, nullptr);

    const auto root = scene->getRoot();
    ASSERT_EQ (root.getNumChildren(), 3);
    EXPECT_EQ (root.getChild (0), camera->getEntity());
    EXPECT_EQ (root.getChild (1), lamp->getEntity());
    EXPECT_EQ (root.getChild (2), quad->getEntity());

    const auto* cameraNode = camera->getEntity().get<CameraNode>();
    ASSERT_NE (cameraNode, nullptr);
    EXPECT_EQ (scene->getActiveCamera(), cameraNode);
    expectVector (positionOf (*camera), 0.0f, 0.0f, 4.0f);

    const auto* light = lamp->getEntity().get<LightNode>();
    ASSERT_NE (light, nullptr);
    EXPECT_EQ (light->type, LightNode::Type::point);
    EXPECT_FLOAT_EQ (light->intensity, 3.0f);
    EXPECT_FLOAT_EQ (light->color[0], 1.0f);
    EXPECT_FLOAT_EQ (light->color[1], 1.0f);
    EXPECT_FLOAT_EQ (light->color[2], 1.0f);

    const auto* meshNode = quad->getEntity().get<MeshNode>();
    ASSERT_NE (meshNode, nullptr);
    ASSERT_NE (meshNode->mesh, nullptr);
    ASSERT_EQ (meshNode->mesh->getNumPrimitives(), 1);
    EXPECT_EQ (meshNode->mesh->getPrimitive (0).indices.size(), 6u);

    // The material is not a node: it has no entity, and overrides the material of the mesh it is in
    EXPECT_EQ (quad->getEntity().getNumChildren(), 0);

    const auto* materialNode = quad->getEntity().get<MaterialNode>();
    ASSERT_NE (materialNode, nullptr);
    EXPECT_EQ (materialNode->material, paint->getMaterial());
    EXPECT_FLOAT_EQ (materialNode->material->baseColorFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (materialNode->material->baseColorFactor[1], 0.0f);
    EXPECT_FLOAT_EQ (materialNode->material->baseColorFactor[2], 0.0f);
    EXPECT_FLOAT_EQ (materialNode->material->baseColorFactor[3], 1.0f);
    EXPECT_FLOAT_EQ (materialNode->material->roughnessFactor, 0.4f);
}

TEST_F (Scene3DTests, MakesNoSceneUntilSomethingNeedsOne)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; }");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    EXPECT_EQ (view->getScene(), nullptr);

    // What the scene resolves to without a node is what reads back
    EXPECT_EQ (view->getBackgroundColor(), BackgroundNode().color);
    EXPECT_FLOAT_EQ (view->getExposure(), 1.0f);
    EXPECT_EQ (view->getScene(), nullptr);

    view->setExposure (2.0f);
    ASSERT_NE (view->getScene(), nullptr);
    EXPECT_FLOAT_EQ (view->getExposure(), 2.0f);
}

TEST_F (Scene3DTests, PropertiesSetTheFeatureNodesOfTheRoot)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;
            background: #102030;
            ambient: #404040;
            exposure: 1.5;
            toneMapping: agx;
            defaultLight: false;
            continuous: true;
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    ASSERT_NE (view->getScene(), nullptr);

    const auto root = view->getScene()->getRoot();

    ASSERT_NE (root.get<BackgroundNode>(), nullptr);
    ASSERT_NE (root.get<AmbientLightNode>(), nullptr);
    ASSERT_NE (root.get<ExposureNode>(), nullptr);
    ASSERT_NE (root.get<ToneMappingNode>(), nullptr);
    ASSERT_NE (root.get<DefaultLightNode>(), nullptr);

    EXPECT_EQ (root.get<BackgroundNode>()->color, Color (0xff102030));
    EXPECT_EQ (root.get<AmbientLightNode>()->color, Color (0xff404040));
    EXPECT_FLOAT_EQ (root.get<ExposureNode>()->exposure, 1.5f);
    EXPECT_EQ (root.get<ToneMappingNode>()->type, ToneMappingOperator::agx);
    EXPECT_FALSE (root.get<DefaultLightNode>()->enabled);
    EXPECT_TRUE (view->isUpdatingContinuously());

    // They read back what the scene resolves to
    EXPECT_EQ (VariantConverter<Color>::fromVar (view->getProperty (Identifier ("background"))), Color (0xff102030));
    EXPECT_EQ (view->getProperty (Identifier ("toneMapping")).toString(), "agx");
    EXPECT_FALSE (static_cast<bool> (view->getProperty (Identifier ("defaultLight"))));

    // A value taken out of the document goes back to the default of the scene
    load (document, "Scene3D { id: view; }");
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);

    const auto settings = SceneSettings::resolve (*view->getScene());
    EXPECT_EQ (settings.backgroundColor, SceneSettings().backgroundColor);
    EXPECT_EQ (settings.ambientColor, SceneSettings().ambientColor);
    EXPECT_FLOAT_EQ (settings.exposure, 1.0f);
    EXPECT_EQ (settings.toneMapping, ToneMappingOperator::reinhard);
    EXPECT_TRUE (settings.useDefaultLight);
    EXPECT_FALSE (view->isUpdatingContinuously());
}

TEST_F (Scene3DTests, PropertiesSetTheSettingsOfTheRenderer)
{
    // Declared with what a renderer starts from: the tier is left to the device
    const auto& type = SceneComponent::getClassTypeInfo();

    const auto* tier = type.findProperty (Identifier ("tier"));
    const auto* samples = type.findProperty (Identifier ("samples"));
    const auto* bloom = type.findProperty (Identifier ("bloom"));

    ASSERT_NE (tier, nullptr);
    ASSERT_NE (samples, nullptr);
    ASSERT_NE (bloom, nullptr);

    EXPECT_EQ (tier->enumValues, StringArray ({ "automatic", "raster", "full" }));
    EXPECT_EQ (tier->defaultValue.toString(), "automatic");
    EXPECT_EQ (static_cast<int> (samples->defaultValue), SceneRenderer().getSampleCount());
    EXPECT_FALSE (static_cast<bool> (bloom->defaultValue));

    YuiDocument document;
    load (document, "Scene3D { id: view; tier: raster; samples: 2; bloom: true; }");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);

    const auto& renderer = view->getRenderer();

    EXPECT_EQ (renderer.getRenderSettings().tier, RenderSettings::TierPreference::raster);
    EXPECT_EQ (renderer.getSampleCount(), 2);
    EXPECT_TRUE (renderer.getRenderSettings().bloom);

    // They read back what the renderer is asked for
    EXPECT_EQ (view->getProperty (Identifier ("tier")).toString(), "raster");
    EXPECT_EQ (static_cast<int> (view->getProperty (Identifier ("samples"))), 2);
    EXPECT_TRUE (static_cast<bool> (view->getProperty (Identifier ("bloom"))));

    // A value taken out of the document goes back to the one a new renderer has
    load (document, "Scene3D { id: view; }");
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);

    EXPECT_EQ (renderer.getRenderSettings().tier, RenderSettings::TierPreference::automatic);
    EXPECT_EQ (renderer.getSampleCount(), SceneRenderer().getSampleCount());
    EXPECT_FALSE (renderer.getRenderSettings().bloom);
}

TEST_F (Scene3DTests, ATranslucentBackgroundMakesTheComponentNotOpaque)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; }");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    EXPECT_TRUE (view->isOpaque());

    load (document, "Scene3D { id: view; background: colors.transparentBlack; }");
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);
    EXPECT_EQ (view->getBackgroundColor(), Colors::transparentBlack);
    EXPECT_FALSE (view->isOpaque());

    load (document, "Scene3D { id: view; background: #000000; }");
    EXPECT_TRUE (view->isOpaque());

    view->setBackgroundColor (Colors::black.withAlpha (0.5f));
    EXPECT_FALSE (view->isOpaque());
}

TEST_F (Scene3DTests, AStyleRuleSetsTheFeaturesOfTheScene)
{
    YuiDocument document;
    load (document, R"yui(
        style {
            .night { background: #000010; exposure: 0.5; }
        }

        Scene3D { id: view; })yui");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);

    view->addStyleClass (Identifier ("night"));
    view->getStyleContext()->restyleNow();

    EXPECT_EQ (view->getBackgroundColor(), Color (0xff000010));
    EXPECT_FLOAT_EQ (view->getExposure(), 0.5f);

    view->removeStyleClass (Identifier ("night"));
    view->getStyleContext()->restyleNow();

    EXPECT_EQ (view->getBackgroundColor(), SceneSettings().backgroundColor);
    EXPECT_FLOAT_EQ (view->getExposure(), 1.0f);
}

TEST_F (Scene3DTests, AComponentThatIsNotANodeIsAnOrdinaryChildWithoutAnEntity)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;

            Node3D { id: group; }
            Label { id: caption; text: "Device"; }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* caption = document.findById<Label> ("caption");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (group, nullptr);
    ASSERT_NE (caption, nullptr);

    // Painted over the scene, which is how a heads-up display is written
    EXPECT_EQ (caption->getParentComponent(), view);
    EXPECT_EQ (view->getIndexOfChildComponent (caption), 1);
    EXPECT_EQ (caption->getText(), "Device");

    const auto root = view->getScene()->getRoot();
    ASSERT_EQ (root.getNumChildren(), 1);
    EXPECT_EQ (root.getChild (0), group->getEntity());
}

TEST_F (Scene3DTests, ACompositeOfA3DTypeInstantiates)
{
    YuiDocument document;
    load (document, R"yui(
        component Lamp : Light3D { intensity: 5; }

        Scene3D {
            id: view;

            Lamp { id: lamp; type: spot; position: [0, 2, 0]; }
        })yui");

    auto* lamp = document.findById<SceneLightComponent> ("lamp");
    ASSERT_NE (lamp, nullptr);

    const auto* light = lamp->getEntity().get<LightNode>();
    ASSERT_NE (light, nullptr);
    EXPECT_EQ (light->type, LightNode::Type::spot);
    EXPECT_FLOAT_EQ (light->intensity, 5.0f);
    expectVector (positionOf (*lamp), 0.0f, 2.0f, 0.0f);
}

TEST_F (Scene3DTests, SettingASceneMovesTheNodesIntoIt)
{
    SceneComponent view;
    SceneNodeComponent node;
    SceneCameraComponent camera;

    view.addAndMakeVisible (node);
    view.addAndMakeVisible (camera);

    const auto made = view.getScene();
    ASSERT_NE (made, nullptr);
    ASSERT_TRUE (node.getEntity().isValid());
    EXPECT_EQ (node.getEntity().getScene(), made.get());

    const auto own = Scene::Ptr (new Scene());
    view.setScene (own);

    // The entities are made again in the scene the component now shows
    EXPECT_EQ (made->getRoot().getNumChildren(), 0);
    ASSERT_EQ (own->getRoot().getNumChildren(), 2);
    EXPECT_EQ (own->getRoot().getChild (0), node.getEntity());
    EXPECT_EQ (own->getRoot().getChild (1), camera.getEntity());
    EXPECT_EQ (own->getActiveCamera(), camera.getEntity().get<CameraNode>());

    // Without a scene the nodes get one made for them
    view.setScene (nullptr);
    ASSERT_NE (view.getScene(), nullptr);
    EXPECT_NE (view.getScene(), own);
    EXPECT_EQ (own->getRoot().getNumChildren(), 0);
    EXPECT_EQ (view.getScene()->getRoot().getNumChildren(), 2);
}

TEST_F (Scene3DTests, SettingASceneTellsTheFeaturesThatReadAnotherValue)
{
    struct Recorder final : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            changed.add (name.toString());
        }

        StringArray changed;
    };

    SceneComponent view;
    view.setBackgroundColor (Color (0xff102030));
    view.setExposure (2.0f);

    Recorder recorder;
    view.addComponentListener (&recorder);

    // The features are those of the scene shown: another one gives them its own, and the ones that read another
    // value are told, in the order of the properties
    const auto other = Scene::Ptr (new Scene());
    other->getRoot().attach<ExposureNode> (2.0f);
    other->getRoot().attach<ToneMappingNode> (ToneMappingOperator::agx);

    view.setScene (other);

    EXPECT_EQ (view.getBackgroundColor(), SceneSettings().backgroundColor);
    EXPECT_FLOAT_EQ (view.getExposure(), 2.0f);
    EXPECT_EQ (view.getToneMapping(), ToneMappingOperator::agx);
    EXPECT_EQ (recorder.changed, StringArray ({ "background", "toneMapping" }));

    // The scene it already shows changes nothing
    recorder.changed.clear();
    view.setScene (other);
    EXPECT_TRUE (recorder.changed.isEmpty());

    // And no scene reads as the defaults
    view.setScene (nullptr);
    EXPECT_EQ (recorder.changed, StringArray ({ "exposure", "toneMapping" }));

    view.removeComponentListener (&recorder);
}

TEST_F (Scene3DTests, ASceneWithoutNodesIsFilledWithTheBackgroundItReads)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Label { text: \"Empty\"; } }");

    auto* view = showSceneOf (document);
    ASSERT_NE (view, nullptr);
    ASSERT_EQ (view->getScene(), nullptr);

    // Nothing asked for a scene yet: the component fills itself as the scene it will make would
    const auto fills = paintWithoutGpu (*view);

    ASSERT_EQ (fills.size(), 1u);
    EXPECT_EQ (fills.front(), view->getBackgroundColor());
    EXPECT_EQ (fills.front(), BackgroundNode().color);
}

TEST_F (Scene3DTests, ASceneThatCanNotBeDrawnIsFilledWithItsBackgroundWhenNoThemeGivesAFont)
{
    SceneComponent view;
    view.setScene (Scene::Ptr (new Scene()));
    view.setBackgroundColor (Color (0xff102030));
    showScene (view);

    // What prevents drawing the scene is written with the font of the theme: without one the background is all there is
    ApplicationTheme::setGlobalTheme (nullptr);

    const auto fills = paintWithoutGpu (view);

    ASSERT_EQ (fills.size(), 1u);
    EXPECT_EQ (fills.front(), Color (0xff102030));
}

TEST_F (Scene3DTests, AFeatureIsPaintedBeforeItsChangeIsTold)
{
    /** Lets go of the scene it listens to when its background changes, as a handler loading its document again does. */
    struct Owner final : ComponentListener
    {
        void componentPropertyChanged (Component& component, const Identifier& name) override
        {
            if (name != Identifier ("background"))
                return;

            wasRepaintRequested = ComponentTestHelper<SceneDocumentTests>::takeRepaintRequest (component);
            view.reset();
        }

        std::unique_ptr<SceneComponent> view = std::make_unique<SceneComponent>();
        bool wasRepaintRequested = false;
    };

    Owner owner;
    showScene (*owner.view);
    owner.view->addComponentListener (&owner);
    Internals::takeRepaintRequest (*owner.view);

    // The repaint is asked for first: once the change is told there may be no component left to ask it of
    owner.view->setBackgroundColor (Color (0xff102030));

    EXPECT_TRUE (owner.view == nullptr);
    EXPECT_TRUE (owner.wasRepaintRequested);
}

TEST_F (Scene3DTests, ASettingOfTheRendererThatChangesIsPaintedAndTold)
{
    struct Recorder final : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            changed.add (name.toString());
        }

        StringArray changed;
    };

    SceneComponent view;
    showScene (view);

    Recorder recorder;
    view.addComponentListener (&recorder);
    Internals::takeRepaintRequest (view);

    // The renderer finds the settings it is given at the next frame it draws: the scene is painted again for each
    view.setProperty (Identifier ("tier"), "raster");
    EXPECT_EQ (view.getRenderer().getRenderSettings().tier, RenderSettings::TierPreference::raster);
    EXPECT_TRUE (Internals::takeRepaintRequest (view));

    view.setProperty (Identifier ("samples"), 8);
    EXPECT_EQ (view.getRenderer().getSampleCount(), 8);
    EXPECT_TRUE (Internals::takeRepaintRequest (view));

    view.setProperty (Identifier ("bloom"), true);
    EXPECT_TRUE (view.getRenderer().getRenderSettings().bloom);
    EXPECT_TRUE (Internals::takeRepaintRequest (view));

    EXPECT_EQ (recorder.changed, StringArray ({ "tier", "samples", "bloom" }));

    // The value a setting already has changes nothing, and neither does a name that is no tier
    recorder.changed.clear();

    view.setProperty (Identifier ("tier"), "raster");
    view.setProperty (Identifier ("tier"), "fastest");
    view.setProperty (Identifier ("samples"), 8);
    view.setProperty (Identifier ("bloom"), true);

    EXPECT_EQ (view.getRenderer().getRenderSettings().tier, RenderSettings::TierPreference::raster);
    EXPECT_FALSE (Internals::takeRepaintRequest (view));
    EXPECT_TRUE (recorder.changed.isEmpty());

    view.removeComponentListener (&recorder);
}

TEST_F (Scene3DTests, AFileThatChangesIsToldToAllThatUseItWhateverTheFirstOnesDo)
{
    files["wood.png"] = redPng();

    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            id: view;

            Node3D { id: first;  Material3D { baseColorTexture: "wood.png"; } }
            Node3D { id: second;  Material3D { id: paint; baseColorTexture: "wood.png"; } }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* first = document.findById<SceneNodeComponent> ("first");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (paint, nullptr);
    ASSERT_NE (paint->getMaterial()->baseColorTexture, nullptr);

    // The file is not an image anymore. The first material to be told says so, and who hears of it takes
    // the node of that material out of the scene
    files["wood.png"] = bytesOf ("this is not an image");

    int told = 0;

    const DiagnosticListener listener (document, [&] (const DataTreeDiagnostic&)
    {
        ++told;
        view->removeChildComponent (first);
    });

    document.notifyAssetChanged ("wood.png");
    finishLoading (document);

    // The material written after it was told all the same: it has no texture left
    EXPECT_GE (told, 1);
    EXPECT_EQ (first->getParentComponent(), nullptr);
    EXPECT_EQ (paint->getMaterial()->baseColorTexture, nullptr);
}

TEST_F (Scene3DTests, TheProblemsOfTheComponentsThatAreGoneAreForgotten)
{
    SceneComponent view;

    // What a repeat whose range keeps changing does: nodes whose values can't be used come and go, with no load of
    // a document in between
    for (int pass = 0; pass < 4; ++pass)
    {
        SceneNodeComponent node;
        view.addAndMakeVisible (node);
        node.setProperty (Identifier ("position"), 1.0);

        EXPECT_EQ (Internals::getNumProblems (view), 1u);
    }
}

TEST_F (Scene3DTests, AnExposureThatExposesNothingIsReportedAndNotKept)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    exposure: -1;\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().message, "Scene3D.exposure: expected a multiplier that is not negative");
    EXPECT_EQ (document.getDiagnostics().front().line, 3);

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    EXPECT_FLOAT_EQ (view->getExposure(), 1.0f);

    // A value that is not a number never equals the one the scene has: it is not kept either, and nothing is painted again
    Internals::takeRepaintRequest (*view);
    view->setExposure (std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ (view->getExposure(), 1.0f);
    EXPECT_FALSE (Internals::takeRepaintRequest (*view));
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);

    // A multiplier it can have withdraws what was reported
    view->setExposure (2.0f);

    EXPECT_FLOAT_EQ (view->getExposure(), 2.0f);
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);
}

TEST_F (Scene3DTests, ATierTheRendererDoesNotHaveIsReportedAndNotKept)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    tier: fastest;\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    expectDiagnostic (document, "Scene3D.tier: expected one of automatic, raster, full");

    // Where the value is written
    const auto* diagnostic = findDiagnostic (document, "Scene3D.tier");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 3);

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    EXPECT_EQ (view->getRenderer().getRenderSettings().tier, RenderSettings::TierPreference::automatic);
}

//==============================================================================

TEST_F (Scene3DTests, AnEnvironmentLightsTheSceneFromTheNodeOfItsRoot)
{
    files["sky.png"] = skyPng();

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; environment: \"sky.png\"; environmentIntensity: 2; environmentVisible: true; }");

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    ASSERT_NE (view->getScene(), nullptr);

    const auto* node = view->getScene()->getRoot().get<EnvironmentNode>();
    ASSERT_NE (node, nullptr);
    ASSERT_NE (node->map, nullptr);
    EXPECT_FLOAT_EQ (node->intensity, 2.0f);
    EXPECT_TRUE (node->visible);
    EXPECT_EQ (reads["sky.png"], 1);

    // The top of the image is above the scene: what faces up is lit, what faces down is not
    EXPECT_GT (lightOn (*node->map, { 0.0f, 1.0f, 0.0f }), 0.8f);
    EXPECT_LT (lightOn (*node->map, { 0.0f, -1.0f, 0.0f }), 0.2f);

    // The renderer is given what the node of the root has
    const auto settings = SceneSettings::resolve (*view->getScene());
    EXPECT_EQ (settings.environment, node->map);
    EXPECT_FLOAT_EQ (settings.environmentIntensity, 2.0f);
    EXPECT_TRUE (settings.environmentVisible);

    // They read back what they were given, and what the scene resolves to
    EXPECT_EQ (view->getEnvironment(), "sky.png");
    EXPECT_EQ (view->getProperty (Identifier ("environment")).toString(), "sky.png");
    EXPECT_FLOAT_EQ (static_cast<float> (view->getProperty (Identifier ("environmentIntensity"))), 2.0f);
    EXPECT_TRUE (static_cast<bool> (view->getProperty (Identifier ("environmentVisible"))));

    // A value taken out of the document goes back to its default: the scene has no environment left
    load (document, "Scene3D { id: view; }");
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);

    const auto without = SceneSettings::resolve (*view->getScene());
    EXPECT_EQ (without.environment, nullptr);
    EXPECT_FLOAT_EQ (without.environmentIntensity, 1.0f);
    EXPECT_FALSE (without.environmentVisible);
    EXPECT_TRUE (view->getEnvironment().isEmpty());

    // And its image, which nothing names anymore, is read again when it is asked for again
    load (document, "Scene3D { id: view; environment: \"sky.png\"; }");

    EXPECT_NE (environmentOf (*view), nullptr);
    EXPECT_EQ (reads["sky.png"], 2);
}

TEST_F (Scene3DTests, AnEnvironmentIsLoadedAgainWhenItsFileChanges)
{
    files["sky.png"] = skyPng();

    YuiDocument document;
    serve (document);
    load (document, "Scene3D { id: view; environment: \"sky.png\"; }");

    auto* view = showSceneOf (document);
    ASSERT_NE (view, nullptr);

    const auto sky = environmentOf (*view);
    ASSERT_NE (sky, nullptr);

    // A file that is not the one of the environment changes nothing
    document.notifyAssetChanged ("other.png");

    EXPECT_FALSE (view->isLoading());
    EXPECT_EQ (environmentOf (*view), sky);
    EXPECT_EQ (reads["sky.png"], 1);

    // Its own is read again at once, and the scene keeps the light it has until the image is decoded
    files["sky.png"] = redPng();
    document.notifyAssetChanged ("sky.png");

    EXPECT_EQ (reads["sky.png"], 2);
    EXPECT_TRUE (view->isLoading());
    EXPECT_EQ (environmentOf (*view), sky);

    Internals::takeRepaintRequest (*view);
    view->finishLoading();

    const auto red = environmentOf (*view);
    ASSERT_NE (red, nullptr);
    EXPECT_NE (red, sky);
    EXPECT_FALSE (view->isLoading());
    EXPECT_TRUE (Internals::takeRepaintRequest (*view));

    // Red all around: from below too, where the sky had no light
    EXPECT_GT (lightOn (*red, { 0.0f, -1.0f, 0.0f }), 0.8f);
    EXPECT_LT (lightOn (*sky, { 0.0f, -1.0f, 0.0f }), 0.2f);

    // The image is a value of the component: it lights another scene it is given, and is not read again for it
    view->setScene (Scene::Ptr (new Scene()));

    EXPECT_EQ (environmentOf (*view), red);
    EXPECT_EQ (reads["sky.png"], 2);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (Scene3DTests, AnEnvironmentThatCanNotBeLoadedIsReported)
{
    files["notes.png"] = bytesOf ("this is not an image");
    files["sky.png"] = skyPng();

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    environment: \"nowhere.png\";\n"
                                                 "}");

    // A file that can't be read is told with the load
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().message, "Scene3D.environment: the file 'nowhere.png' can't be read");
    EXPECT_EQ (document.getDiagnostics().front().line, 3);
    EXPECT_TRUE (document.getDiagnostics().front().isError());

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    EXPECT_FALSE (view->isLoading());
    EXPECT_EQ (environmentOf (*view), nullptr);

    // One that is no image is told once it is decoded, where the environment is written
    view->setEnvironment ("notes.png");

    EXPECT_TRUE (view->isLoading());
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);

    view->finishLoading();

    EXPECT_EQ (Internals::getNumProblems (*view), 1u);
    ASSERT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics().back().message.startsWith ("Scene3D.environment: ")) << document.getDiagnostics().back().message;
    EXPECT_EQ (document.getDiagnostics().back().line, 3);
    EXPECT_EQ (environmentOf (*view), nullptr);

    // An image withdraws what the scene had to tell
    view->setEnvironment ("sky.png");
    view->finishLoading();

    EXPECT_NE (environmentOf (*view), nullptr);
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);
}
