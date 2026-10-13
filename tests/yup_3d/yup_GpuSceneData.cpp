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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>

using namespace yup;

class GpuSceneDataTests : public ::testing::Test
{
protected:
    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    /** Rounds like an rgba16f store: 11 significant bits. */
    static float toHalfPrecision (float value)
    {
        int exponent = 0;
        const auto mantissa = std::frexp (value, &exponent);
        return std::ldexp (std::round (mantissa * 2048.0f) / 2048.0f, exponent);
    }
};

TEST_F (GpuSceneDataTests, OctahedralRoundTripsTheAxes)
{
    for (auto axis : { Vector3<float> (1, 0, 0), Vector3<float> (-1, 0, 0), Vector3<float> (0, 1, 0),
                       Vector3<float> (0, -1, 0), Vector3<float> (0, 0, 1), Vector3<float> (0, 0, -1) })
    {
        expectNear (GpuSceneData::decodeOctahedral (GpuSceneData::encodeOctahedral (axis)), axis, 1.0e-6f);
    }
}

TEST_F (GpuSceneDataTests, OctahedralEncodesKnownDirectionsExactly)
{
    const auto expectEncodes = [] (const Vector3<float>& n, float x, float y)
    {
        const auto e = GpuSceneData::encodeOctahedral (n.normalized());
        EXPECT_NEAR (e.getX(), x, 1.0e-6f);
        EXPECT_NEAR (e.getY(), y, 1.0e-6f);
    };

    expectEncodes ({ 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f);
    expectEncodes ({ 0.0f, 0.0f, -1.0f }, 1.0f, 1.0f);

    // Lower hemisphere, asymmetric so swapped or sign-lost fold components show: p = (-0.25, 0.5)
    expectEncodes ({ -1.0f, 2.0f, -1.0f }, -0.5f, 0.75f);

    // The z = 0 seam is not folded
    expectEncodes ({ 1.0f, 1.0f, 0.0f }, 0.5f, 0.5f);

    expectNear (GpuSceneData::decodeOctahedral ({ 0.0f, 0.0f }), { 0.0f, 0.0f, 1.0f }, 1.0e-6f);
}

TEST_F (GpuSceneDataTests, OctahedralRoundTripsTheSphereAtHalfPrecision)
{
    Random random (42);

    for (int i = 0; i < 10000; ++i)
    {
        const auto n = Vector3<float> (random.nextFloat() * 2.0f - 1.0f,
                                       random.nextFloat() * 2.0f - 1.0f,
                                       random.nextFloat() * 2.0f - 1.0f)
                           .normalized();
        if (! std::isfinite (n.getX()))
            continue;

        auto e = GpuSceneData::encodeOctahedral (n);
        EXPECT_LE (std::abs (e.getX()), 1.0f);
        EXPECT_LE (std::abs (e.getY()), 1.0f);

        e = { toHalfPrecision (e.getX()), toHalfPrecision (e.getY()) };
        const auto decoded = GpuSceneData::decodeOctahedral (e);

        EXPECT_NEAR (decoded.length(), 1.0f, 1.0e-5f);
        EXPECT_GT (decoded.dotProduct (n), std::cos (0.002f)); // under 0.12 degrees after an rgba16f store
    }
}

TEST_F (GpuSceneDataTests, GpuLightLayoutIsStd140)
{
    EXPECT_EQ (sizeof (GpuLight), 64u);
    EXPECT_EQ (offsetof (GpuLight, directionRange), 16u);
    EXPECT_EQ (offsetof (GpuLight, colorIntensity), 32u);
    EXPECT_EQ (offsetof (GpuLight, spotRadiusShadow), 48u);
}

TEST_F (GpuSceneDataTests, PacksASpotLight)
{
    LightNode node (LightNode::Type::spot);
    node.color = { 1.0f, 0.5f, 0.25f };
    node.intensity = 10.0f;
    node.range = 4.0f;
    node.radius = 0.1f;
    node.innerConeAngle = 0.2f;
    node.outerConeAngle = 0.4f;

    const SceneDrawList::Light light { &node, { 1.0f, 2.0f, 3.0f }, { 0.0f, -1.0f, 0.0f } };
    const auto packed = GpuSceneData::packLight (light, -1);

    EXPECT_FLOAT_EQ (packed.positionType[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.positionType[2], 3.0f);
    EXPECT_FLOAT_EQ (packed.positionType[3], 2.0f);
    EXPECT_FLOAT_EQ (packed.directionRange[1], -1.0f);
    EXPECT_FLOAT_EQ (packed.directionRange[3], 4.0f);
    EXPECT_FLOAT_EQ (packed.colorIntensity[1], 0.5f);
    EXPECT_FLOAT_EQ (packed.colorIntensity[3], 10.0f);
    EXPECT_FLOAT_EQ (packed.spotRadiusShadow[2], 0.1f);
    EXPECT_FLOAT_EQ (packed.spotRadiusShadow[3], -1.0f);

    const auto scale = 1.0f / (std::cos (0.2f) - std::cos (0.4f));
    EXPECT_NEAR (packed.spotRadiusShadow[0], scale, 1.0e-4f);
    EXPECT_NEAR (packed.spotRadiusShadow[1], -std::cos (0.4f) * scale, 1.0e-4f);
}

TEST_F (GpuSceneDataTests, EffectiveRangeCoversCutoff)
{
    LightNode point (LightNode::Type::point);
    point.intensity = 100.0f;

    const auto range = GpuSceneData::getEffectiveRange (point);
    EXPECT_NEAR (point.intensity / (range * range), GpuSceneData::lightCutoff, 1.0e-6f);

    // A colored light is culled where its brightest channel drops under the cutoff
    point.color = { 0.25f, 0.5f, 0.125f };
    const auto coloredRange = GpuSceneData::getEffectiveRange (point);
    EXPECT_NEAR (point.intensity * 0.5f / (coloredRange * coloredRange), GpuSceneData::lightCutoff, 1.0e-6f);

    point.range = 2.0f; // an explicit range always wins
    EXPECT_FLOAT_EQ (GpuSceneData::getEffectiveRange (point), 2.0f);

    LightNode sun;
    sun.range = 2.0f;
    EXPECT_FLOAT_EQ (GpuSceneData::getEffectiveRange (sun), 0.0f);
}

TEST_F (GpuSceneDataTests, EffectiveOnlyRangeIsNegative)
{
    LightNode point (LightNode::Type::point), ranged (LightNode::Type::point), sun;
    point.intensity = 100.0f;
    ranged.range = 3.0f;
    sun.range = 3.0f;

    const auto pack = [] (const LightNode& node)
    {
        return GpuSceneData::packLight ({ &node, {}, { 0.0f, 0.0f, -1.0f } }, -1).directionRange[3];
    };

    // Negative: culled at abs (w), but not windowed, like a light without range
    EXPECT_FLOAT_EQ (pack (point), -GpuSceneData::getEffectiveRange (point));
    EXPECT_LT (pack (point), 0.0f);
    EXPECT_FLOAT_EQ (pack (ranged), 3.0f);
    EXPECT_FLOAT_EQ (pack (sun), 0.0f);
}

TEST_F (GpuSceneDataTests, DirectionalLightsArePackedFirst)
{
    LightNode point (LightNode::Type::point), sun, spot (LightNode::Type::spot);
    const SceneDrawList::Light lights[] = { { &point, {}, { 0, 0, -1 } }, { &sun, {}, { 0, -1, 0 } }, { &spot, {}, { 0, 0, -1 } } };

    // Each light carries its own first shadow record, whatever slot it is packed into
    const int firstShadowRecord[] = { 6, 0, 12 };

    std::vector<uint32> order;
    const auto packed = GpuSceneData::packLights (lights, firstShadowRecord, order);

    ASSERT_EQ (packed.size(), 3u);
    EXPECT_EQ (order, (std::vector<uint32> { 1, 0, 2 }));
    EXPECT_FLOAT_EQ (packed[0].positionType[3], 0.0f);
    EXPECT_FLOAT_EQ (packed[1].positionType[3], 1.0f);
    EXPECT_FLOAT_EQ (packed[2].positionType[3], 2.0f);
    EXPECT_FLOAT_EQ (packed[0].spotRadiusShadow[3], 0.0f);  // the sun, list index 1
    EXPECT_FLOAT_EQ (packed[1].spotRadiusShadow[3], 6.0f);  // the point light, list index 0
    EXPECT_FLOAT_EQ (packed[2].spotRadiusShadow[3], 12.0f); // the spot light, list index 2
}

TEST_F (GpuSceneDataTests, DarkPointLightHasZeroRange)
{
    LightNode unlit (LightNode::Type::point), black (LightNode::Type::point), negative (LightNode::Type::point);
    unlit.intensity = 0.0f;
    black.intensity = 100.0f;
    black.color = { 0.0f, 0.0f, 0.0f };
    negative.intensity = -1.0f;

    // A node range does not make a dark light reach anything
    LightNode rangedUnlit (LightNode::Type::point), rangedBlack (LightNode::Type::spot);
    rangedUnlit.range = 3.0f;
    rangedUnlit.intensity = 0.0f;
    rangedBlack.range = 3.0f;
    rangedBlack.color = { 0.0f, 0.0f, 0.0f };

    // Contributes nothing anywhere: the range is zero, whatever its sign, so culling can drop it
    for (const auto* node : { &unlit, &black, &negative, &rangedUnlit, &rangedBlack })
    {
        EXPECT_EQ (GpuSceneData::getEffectiveRange (*node), 0.0f);

        const auto packed = GpuSceneData::packLight ({ node, {}, { 0.0f, 0.0f, -1.0f } }, -1);
        EXPECT_EQ (std::abs (packed.directionRange[3]), 0.0f);
    }
}

TEST_F (GpuSceneDataTests, PackLightsKeepsListOrderWithinEachGroup)
{
    LightNode point (LightNode::Type::point), firstSun, spot (LightNode::Type::spot), secondSun;
    const SceneDrawList::Light lights[] = {
        { &point, {}, { 0, 0, -1 } },
        { &firstSun, {}, { 1, 0, 0 } },
        { &spot, {}, { 0, 0, -1 } },
        { &secondSun, {}, { 0, 1, 0 } },
    };

    std::vector<uint32> order { 7, 7 }; // stale content is replaced
    const auto packed = GpuSceneData::packLights (lights, {}, order);

    ASSERT_EQ (packed.size(), 4u);
    EXPECT_EQ (order, (std::vector<uint32> { 1, 3, 0, 2 }));
    EXPECT_FLOAT_EQ (packed[0].directionRange[0], 1.0f);
    EXPECT_FLOAT_EQ (packed[1].directionRange[1], 1.0f);
    EXPECT_FLOAT_EQ (packed[2].positionType[3], 1.0f);
    EXPECT_FLOAT_EQ (packed[3].positionType[3], 2.0f);

    // No shadow records: every slot reads -1
    for (const auto& light : packed)
        EXPECT_FLOAT_EQ (light.spotRadiusShadow[3], -1.0f);

    // The second directional light shadowed: its packed slot, not its list index, holds record 0
    const int firstShadowRecord[] = { -1, -1, -1, 0 };
    const auto shadowed = GpuSceneData::packLights (lights, firstShadowRecord, order);
    EXPECT_FLOAT_EQ (shadowed[1].spotRadiusShadow[3], 0.0f);
    EXPECT_FLOAT_EQ (shadowed[0].spotRadiusShadow[3], -1.0f);
    EXPECT_FLOAT_EQ (shadowed[3].spotRadiusShadow[3], -1.0f);
}

TEST_F (GpuSceneDataTests, GpuMaterialLayoutIsStd430)
{
    EXPECT_EQ (sizeof (GpuMaterial), 32u);
    EXPECT_EQ (offsetof (GpuMaterial, extended), 16u);
}

TEST_F (GpuSceneDataTests, ShadowBlocksAreStd140)
{
    EXPECT_EQ (sizeof (GpuShadowRecord), 112u);
    EXPECT_EQ (offsetof (GpuShadowRecord, atlasRect), 64u);
    EXPECT_EQ (offsetof (GpuShadowRecord, params), 80u);
    EXPECT_EQ (offsetof (GpuShadowRecord, contact), 96u);

    EXPECT_EQ (offsetof (GpuShadowData, cascadeTexel), 16u);
    EXPECT_EQ (offsetof (GpuShadowData, sunInfo), 32u);
    EXPECT_EQ (offsetof (GpuShadowData, filterInfo), 48u);
    EXPECT_EQ (offsetof (GpuShadowData, records), 64u);
    EXPECT_EQ (sizeof (GpuShadowData), 7232u);
    EXPECT_EQ (sizeof (GpuShadowData) % 16, 0u);
    EXPECT_LE (sizeof (GpuShadowData), 16384u); // the WebGL2 minimum uniform block size
}

TEST_F (GpuSceneDataTests, PacksDefaultMaterial)
{
    const Material material;
    const auto packed = GpuSceneData::packMaterial (material);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (packed.specular[i], 1.0f) << i;

    EXPECT_FLOAT_EQ (packed.extended[0], 1.5f);
    EXPECT_FLOAT_EQ (packed.extended[1], 1.0f);
    EXPECT_FLOAT_EQ (packed.extended[2], 0.0f);
    EXPECT_FLOAT_EQ (packed.extended[3], 0.0f);
}

TEST_F (GpuSceneDataTests, PacksSpecularIorAndEmissiveStrength)
{
    Material material;
    material.specularColorFactor = { 1.0f, 0.5f, 0.25f };
    material.specularFactor = 0.75f;
    material.ior = 1.33f;
    material.emissiveStrength = 5.0f;

    const auto packed = GpuSceneData::packMaterial (material);
    EXPECT_FLOAT_EQ (packed.specular[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.specular[1], 0.5f);
    EXPECT_FLOAT_EQ (packed.specular[2], 0.25f);
    EXPECT_FLOAT_EQ (packed.specular[3], 0.75f);
    EXPECT_FLOAT_EQ (packed.extended[0], 1.33f);
    EXPECT_FLOAT_EQ (packed.extended[1], 5.0f);
    EXPECT_FLOAT_EQ (packed.extended[2], 0.0f);
    EXPECT_FLOAT_EQ (packed.extended[3], 0.0f);
}

TEST_F (GpuSceneDataTests, GpuReflectionProbeLayoutIsStd430)
{
    EXPECT_EQ (sizeof (GpuReflectionProbe), 128u);
    EXPECT_EQ (offsetof (GpuReflectionProbe, positionShape), 64u);
    EXPECT_EQ (offsetof (GpuReflectionProbe, influence), 80u);
    EXPECT_EQ (offsetof (GpuReflectionProbe, atlasRect), 96u);
    EXPECT_EQ (offsetof (GpuReflectionProbe, params), 112u);
}

TEST_F (GpuSceneDataTests, PacksASphereProbe)
{
    auto root = EntityNode::Ptr (new EntityNode ("root"));
    auto entity = EntityNode::Ptr (new EntityNode ("probe"));
    entity->setPosition ({ 1.0f, 2.0f, 3.0f });
    entity->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.7f));
    entity->setScale ({ 2.0f, 2.0f, 2.0f });
    auto& node = entity->attach<ReflectionProbeNode>();
    node.influenceRadius = 3.0f;
    node.falloff = 0.25f;
    node.intensity = 2.0f;
    node.parallaxCorrection = false;
    root->addChild (entity);

    SceneDrawList drawList;
    drawList.build (*root, {});
    ASSERT_EQ (drawList.getReflectionProbes().size(), 1u);

    const auto& probe = drawList.getReflectionProbes()[0];
    const auto packed = GpuSceneData::packReflectionProbe (probe, { 0.25f, 0.5f, 0.125f, 0.0625f }, 6);

    EXPECT_FLOAT_EQ (packed.positionShape[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.positionShape[1], 2.0f);
    EXPECT_FLOAT_EQ (packed.positionShape[2], 3.0f);
    EXPECT_FLOAT_EQ (packed.positionShape[3], 0.0f);

    EXPECT_FLOAT_EQ (packed.influence[0], 3.0f);
    EXPECT_FLOAT_EQ (packed.influence[1], 0.0f);
    EXPECT_FLOAT_EQ (packed.influence[2], 0.0f);
    EXPECT_FLOAT_EQ (packed.influence[3], 0.25f);

    EXPECT_FLOAT_EQ (packed.atlasRect[0], 0.25f);
    EXPECT_FLOAT_EQ (packed.atlasRect[1], 0.5f);
    EXPECT_FLOAT_EQ (packed.atlasRect[2], 0.125f);
    EXPECT_FLOAT_EQ (packed.atlasRect[3], 0.0625f);

    EXPECT_FLOAT_EQ (packed.params[0], 2.0f);
    EXPECT_FLOAT_EQ (packed.params[1], 0.0f);
    EXPECT_FLOAT_EQ (packed.params[2], 6.0f);
    EXPECT_FLOAT_EQ (packed.params[3], 0.0f);

    // The world to local matrix brings the capture position to the origin and undoes the scale and rotation
    std::array<float, 16> values;
    std::copy (std::begin (packed.worldToLocal), std::end (packed.worldToLocal), values.begin());
    const Matrix4 worldToLocal (values);

    expectNear (worldToLocal.transformPoint (probe.position), {}, 1.0e-5f);
    expectNear (worldToLocal.transformPoint (probe.world.transformPoint ({ 1.0f, -2.0f, 0.5f })), { 1.0f, -2.0f, 0.5f }, 1.0e-5f);
}

TEST_F (GpuSceneDataTests, PacksABoxProbe)
{
    auto root = EntityNode::Ptr (new EntityNode ("root"));
    auto entity = EntityNode::Ptr (new EntityNode ("probe"));
    entity->setPosition ({ -4.0f, 0.0f, 1.0f });
    auto& node = entity->attach<ReflectionProbeNode>();
    node.shape = ReflectionProbeNode::Shape::box;
    node.influenceExtents = { 1.0f, 2.0f, 4.0f };
    node.falloff = 0.5f;
    root->addChild (entity);

    SceneDrawList drawList;
    drawList.build (*root, {});
    ASSERT_EQ (drawList.getReflectionProbes().size(), 1u);

    const auto& probe = drawList.getReflectionProbes()[0];
    const auto packed = GpuSceneData::packReflectionProbe (probe, { 0.0f, 0.0f, 0.5f, 0.5f }, 4);

    EXPECT_FLOAT_EQ (packed.positionShape[0], -4.0f);
    EXPECT_FLOAT_EQ (packed.positionShape[2], 1.0f);
    EXPECT_FLOAT_EQ (packed.positionShape[3], 1.0f);

    EXPECT_FLOAT_EQ (packed.influence[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.influence[1], 2.0f);
    EXPECT_FLOAT_EQ (packed.influence[2], 4.0f);
    EXPECT_FLOAT_EQ (packed.influence[3], 0.5f);

    EXPECT_FLOAT_EQ (packed.params[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.params[1], 1.0f);
    EXPECT_FLOAT_EQ (packed.params[2], 4.0f);

    EXPECT_FLOAT_EQ (packed.worldToLocal[12], 4.0f);
    EXPECT_FLOAT_EQ (packed.worldToLocal[13], 0.0f);
    EXPECT_FLOAT_EQ (packed.worldToLocal[14], -1.0f);
}

TEST_F (GpuSceneDataTests, PacksANegativeFalloffAsZero)
{
    ReflectionProbeNode node;
    node.falloff = -1.0f;

    SceneDrawList::ReflectionProbe probe;
    probe.node = &node;

    const auto packed = GpuSceneData::packReflectionProbe (probe, {}, 1);
    EXPECT_EQ (packed.influence[3], 0.0f);
}

TEST_F (GpuSceneDataTests, GpuIrradianceVolumeLayoutIsStd430)
{
    EXPECT_EQ (sizeof (GpuIrradianceVolume), 112u);
    EXPECT_EQ (offsetof (GpuIrradianceVolume, resolution), 64u);
    EXPECT_EQ (offsetof (GpuIrradianceVolume, shBlock), 80u);
    EXPECT_EQ (offsetof (GpuIrradianceVolume, visibilityBlock), 96u);
}

TEST_F (GpuSceneDataTests, PacksAVolume)
{
    auto root = EntityNode::Ptr (new EntityNode ("root"));
    auto entity = EntityNode::Ptr (new EntityNode ("volume"));
    entity->setPosition ({ -4.0f, 1.0f, 2.0f });
    entity->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));
    auto& node = entity->attach<IrradianceVolumeNode>();
    node.extents = { 1.0f, 2.0f, 4.0f };
    node.resolutionX = 3;
    node.resolutionY = 2;
    node.resolutionZ = 5;
    node.falloff = 0.25f;
    node.intensity = 1.5f;
    node.normalBias = 0.1f;
    root->addChild (entity);

    SceneDrawList drawList;
    drawList.build (*root, {});
    ASSERT_EQ (drawList.getIrradianceVolumes().size(), 1u);

    const auto& volume = drawList.getIrradianceVolumes()[0];
    const auto packed = GpuSceneData::packIrradianceVolume (volume, { 12, 40 }, { 36, 72 }, 16);

    // The box corners, in entity space (-extents and +extents), map to 0 and 1
    std::array<float, 16> values {};
    std::copy_n (packed.worldToUnit, 16, values.begin());
    const Matrix4 worldToUnit (values);
    expectNear (worldToUnit.transformPoint (volume.world.transformPoint ({ -1.0f, -2.0f, -4.0f })), { 0.0f, 0.0f, 0.0f }, 1.0e-5f);
    expectNear (worldToUnit.transformPoint (volume.world.transformPoint ({ 1.0f, 2.0f, 4.0f })), { 1.0f, 1.0f, 1.0f }, 1.0e-5f);
    expectNear (worldToUnit.transformPoint (volume.world.transformPoint ({ 1.0f, -2.0f, 0.0f })), { 1.0f, 0.0f, 0.5f }, 1.0e-5f);
    expectNear (worldToUnit.transformPoint ({ -4.0f, 1.0f, 2.0f }), { 0.5f, 0.5f, 0.5f }, 1.0e-5f);

    EXPECT_FLOAT_EQ (packed.resolution[0], 3.0f);
    EXPECT_FLOAT_EQ (packed.resolution[1], 2.0f);
    EXPECT_FLOAT_EQ (packed.resolution[2], 5.0f);
    EXPECT_FLOAT_EQ (packed.resolution[3], 1.5f);

    EXPECT_FLOAT_EQ (packed.shBlock[0], 12.0f);
    EXPECT_FLOAT_EQ (packed.shBlock[1], 40.0f);
    EXPECT_FLOAT_EQ (packed.shBlock[2], 0.25f);

    EXPECT_FLOAT_EQ (packed.visibilityBlock[0], 36.0f);
    EXPECT_FLOAT_EQ (packed.visibilityBlock[1], 72.0f);
    EXPECT_FLOAT_EQ (packed.visibilityBlock[2], 16.0f);
    EXPECT_FLOAT_EQ (packed.visibilityBlock[3], 0.1f);
}

TEST_F (GpuSceneDataTests, PacksAVolumeWithoutVisibilityAndANegativeFalloffAsZero)
{
    auto root = EntityNode::Ptr (new EntityNode ("root"));
    root->attach<IrradianceVolumeNode>().falloff = -1.0f;

    SceneDrawList drawList;
    drawList.build (*root, {});
    ASSERT_EQ (drawList.getIrradianceVolumes().size(), 1u);

    const auto packed = GpuSceneData::packIrradianceVolume (drawList.getIrradianceVolumes()[0], { 1, 2 }, {}, 0);
    EXPECT_FLOAT_EQ (packed.resolution[0], 4.0f);
    EXPECT_FLOAT_EQ (packed.shBlock[2], 0.0f);
    EXPECT_FLOAT_EQ (packed.visibilityBlock[2], 0.0f);
}
