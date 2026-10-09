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

class SceneRendererTests : public ::testing::Test
{
};

TEST_F (SceneRendererTests, UsesFourSamplesByDefault)
{
    const SceneRenderer renderer;

    EXPECT_EQ (renderer.getSampleCount(), 4);
    EXPECT_TRUE (renderer.getLastError().isEmpty());
}

TEST_F (SceneRendererTests, SampleCountIsAtLeastOne)
{
    SceneRenderer renderer;

    renderer.setSampleCount (8);
    EXPECT_EQ (renderer.getSampleCount(), 8);

    renderer.setSampleCount (0);
    EXPECT_EQ (renderer.getSampleCount(), 1);
}

TEST_F (SceneRendererTests, RenderWithoutDeviceFails)
{
    SceneRenderer renderer;
    Scene scene;

    EXPECT_EQ (renderer.render (nullptr, scene, 64, 64), nullptr);
    EXPECT_FALSE (renderer.getLastError().isEmpty());
}

#if YUP_APPLE
class SceneRendererMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
    }

    /** A plate facing the default camera, with a smaller one floating over it to cast a shadow. */
    static EntityNode::Ptr makePlates()
    {
        const auto quad = [] (float halfSize, float z)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { -halfSize, -halfSize, z };
            vertices[1].position = { halfSize, -halfSize, z };
            vertices[2].position = { halfSize, halfSize, z };
            vertices[3].position = { -halfSize, halfSize, z };

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            return vertices;
        };

        auto mesh = Mesh::Ptr (new Mesh ("plates"));
        mesh->addPrimitive (quad (1.0f, 0.0f), { 0, 1, 2, 0, 2, 3 });
        mesh->addPrimitive (quad (0.3f, 0.5f), { 0, 1, 2, 0, 2, 3 });

        auto entity = EntityNode::Ptr (new EntityNode ("plates"));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** Renders and reads the pixels back through a canvas. */
    Image renderToImage (SceneRenderer& renderer, Scene& scene, int size)
    {
        auto texture = renderer.render (device, scene, size, size);
        if (texture == nullptr)
            return {};

        auto canvas = GpuCanvas::create (*context, size, size);
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw();
        g.drawTexture (texture, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
        return canvas->asImage();
    }

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
};

TEST_F (SceneRendererMetalTests, RendersEnvironmentShadowsAndBackground)
{
    Scene scene;
    scene.getRoot()->addChild (makePlates());

    // Shining down -Z, tilted so the shadow of the small plate falls beside it
    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    sun->attach<LightNode>().castsShadows = true;
    sun->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
    scene.getRoot()->addChild (sun);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.setEnvironmentVisible (true);
    scene.setToneMapping (Scene::ToneMapping::aces);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
    ASSERT_TRUE (image.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    const auto corner = image.getPixelColor (1, 1);
    EXPECT_NEAR (corner.getRed(), 206, 3);
    EXPECT_NEAR (corner.getGreen(), 206, 3);
    EXPECT_NEAR (corner.getBlue(), 206, 3);
}
#endif
