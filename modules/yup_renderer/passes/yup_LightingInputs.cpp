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

namespace
{

float lightingSrgbToLinear (float value) noexcept
{
    return value <= 0.04045f ? value / 12.92f : std::pow ((value + 0.055f) / 1.055f, 2.4f);
}

} // namespace

//==============================================================================
bool LightingInputs::hasAreaLights (Span<const SceneDrawList::Light> lights) noexcept
{
    return std::any_of (lights.begin(), lights.end(), [] (const SceneDrawList::Light& light)
    {
        return light.node->radius > 0.0f;
    });
}

bool LightingInputs::hasMaterialLayers (Span<const SceneDrawList::DrawItem> items) noexcept
{
    return std::any_of (items.begin(), items.end(), [] (const SceneDrawList::DrawItem& item)
    {
        return GpuSceneData::hasLayers (*item.material);
    });
}

GpuTexture::Ptr LightingInputs::prepareEnvironment (ProbeRenderer& probes, const Scene& scene, EnvironmentCube& slot)
{
    const auto& environment = scene.getEnvironment();
    if (environment != slot.environment)
        slot = { environment };

    if (environment == nullptr || scene.getEnvironmentIntensity() <= 0.0f)
        return nullptr;

    // A replaced cube stays alive until its replacement exists, so the two never share an address
    GpuTexture::Ptr replacedCube;

    // One source of truth: a cube in another format than the renderer's radiance format is built again
    if (slot.cube != nullptr && slot.cube->getFormat() != probes.getRadianceFormat())
    {
        replacedCube = slot.cube;
        slot = { environment };
    }

    if (slot.status.isValid())
    {
        if (const auto result = slot.status.poll(); result.has_value())
        {
            if (result->failed())
            {
                replacedCube = std::exchange (slot.cube, nullptr);

                if (++slot.consecutiveFailures >= 2)
                {
                    YUP_DBG ("LightingInputs: the GPU failed to build the environment cube twice, shading without it: " << result->getErrorMessage());
                    slot.givenUp = true;
                }
            }
            else
            {
                slot.consecutiveFailures = 0;
            }

            slot.status = {};
        }
    }

    if (slot.cube == nullptr && ! slot.givenUp)
    {
        auto build = probes.buildEnvironmentCube (*environment);
        slot.cube = std::move (build.cube);
        slot.image = std::move (build.image);
        slot.status = build.status;

        // Nothing recorded fails the same way next time
        if (slot.cube == nullptr && slot.status.isValid())
            slot.givenUp = true;
    }

    return slot.cube;
}

GpuTexture::Ptr LightingInputs::createBlackCube (const GpuDevice::Ptr& device)
{
    GpuTextureDesc desc (1, 1, GpuTextureFormat::rgba8unorm);
    desc.type = GpuTextureType::cube;
    desc.depthOrArrayLayers = 6;
    desc.label = "LightingInputs black cube";

    auto cube = GpuTexture::create (device, desc);
    if (cube == nullptr)
        return nullptr;

    const uint8 black[] = { 0, 0, 0, 255 };

    for (uint32 face = 0; face < 6; ++face)
    {
        GpuTextureDataDesc data;
        data.data = black;
        data.layer = face;

        if (! cube->upload (data))
            return nullptr;
    }

    return cube;
}

LightingInputs LightingInputs::create (const Scene& scene, const GpuTexture::Ptr& environmentCube,
                                       Span<const SceneDrawList::Light> lights, Span<const int> firstShadowRecord)
{
    LightingInputs inputs;
    auto& block = inputs.block;

    const auto ambient = scene.getAmbientColor();
    block.ambient[0] = lightingSrgbToLinear (ambient.getRedFloat());
    block.ambient[1] = lightingSrgbToLinear (ambient.getGreenFloat());
    block.ambient[2] = lightingSrgbToLinear (ambient.getBlueFloat());

    // Draws pick their lights by list index through packedSlots
    std::vector<uint32> order;
    inputs.packedLights = GpuSceneData::packLights (lights, firstShadowRecord, order);

    inputs.areaLights = hasAreaLights (lights);

    inputs.packedSlots.resize (order.size());
    for (size_t slot = 0; slot < order.size(); ++slot)
        inputs.packedSlots[order[slot]] = static_cast<uint32> (slot);

    const auto& environment = scene.getEnvironment();
    if (environment == nullptr || scene.getEnvironmentIntensity() <= 0.0f || environmentCube == nullptr)
        return inputs;

    inputs.environment = environmentCube;

    const auto lastLevel = static_cast<float> (environment->getNumLevels() - 1);
    block.ambient[3] = scene.isEnvironmentVisible() ? 1.0f : 0.0f;
    block.environmentInfo[0] = scene.getEnvironmentIntensity();
    block.environmentInfo[1] = lastLevel;
    block.environmentInfo[2] = scene.getEnvironmentBlur() * lastLevel;

    const auto& coefficients = environment->getIrradianceCoefficients();
    for (size_t i = 0; i < coefficients.size(); ++i)
    {
        block.irradiance[i][0] = coefficients[i].getX();
        block.irradiance[i][1] = coefficients[i].getY();
        block.irradiance[i][2] = coefficients[i].getZ();
    }

    return inputs;
}

} // namespace yup
