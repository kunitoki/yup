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

LightingInputs LightingInputs::create (const GpuDevice::Ptr& device, const Scene& scene,
                                       Span<const SceneDrawList::Light> lights, const ShadowRenderer::Result& shadow)
{
    LightingInputs inputs;
    auto& block = inputs.block;

    const auto ambient = scene.getAmbientColor();
    block.ambient[0] = lightingSrgbToLinear (ambient.getRedFloat());
    block.ambient[1] = lightingSrgbToLinear (ambient.getGreenFloat());
    block.ambient[2] = lightingSrgbToLinear (ambient.getBlueFloat());

    std::copy_n (shadow.shadowMatrix.getData(), 16, block.shadowMatrix);
    std::copy_n (shadow.shadowInfo + 1, 3, block.shadowInfo + 1);
    inputs.shadowMap = shadow.map;

    // The shadowed light gets shadow slot 0: draws pick theirs by list index through packedSlots
    std::vector<uint32> order;
    inputs.packedLights = GpuSceneData::packLights (lights, static_cast<int> (shadow.shadowInfo[0]), order);

    inputs.areaLights = hasAreaLights (lights);

    inputs.packedSlots.resize (order.size());
    for (size_t slot = 0; slot < order.size(); ++slot)
        inputs.packedSlots[order[slot]] = static_cast<uint32> (slot);

    const auto& environment = scene.getEnvironment();
    if (environment == nullptr || scene.getEnvironmentIntensity() <= 0.0f)
        return inputs;

    inputs.environment = environment->getGpuTexture (device);
    inputs.environmentSampler = environment->getGpuSampler (device);

    if (inputs.environment == nullptr || inputs.environmentSampler == nullptr)
    {
        inputs.environment = nullptr;
        inputs.environmentSampler = nullptr;
        return inputs;
    }

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
