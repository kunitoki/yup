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

#include <yup_renderer/yup_renderer.h>

#if __has_include(<YuiTestResources.h>)
#include <YuiTestResources.h>

using namespace yup;

// What yup_add_yui_resources embeds for a material shader, see YuiTestResources in tests/CMakeLists.txt
TEST (YuiResourcesTests, FindsTheTwoVariantsAMaterialShaderIsCompiledInto)
{
    std::size_t rasterSize = 0;
    std::size_t fullSize = 0;
    const auto* raster = YuiTestResources_find ("fx/rim.glsl.raster.ysl", rasterSize);
    const auto* full = YuiTestResources_find ("fx/rim.glsl.full.ysl", fullSize);

    ASSERT_NE (raster, nullptr);
    ASSERT_NE (full, nullptr);

    // Creating the shader checks that each variant is the template of its tier
    const auto created = MaterialShader::createFromData (raster, rasterSize, full, fullSize);
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    const auto shader = created.getValue();
    EXPECT_NE (shader->getRasterBundle(), nullptr);
    EXPECT_NE (shader->getFullBundle(), nullptr);

    // Compiled around the material that was given: the stock one has no uniform block
    const auto uniforms = shader->createUniforms();
    EXPECT_EQ (uniforms.getNumComponents ("rim"), 4);
    EXPECT_EQ (uniforms.getNumComponents ("strength"), 1);

    std::size_t sourceSize = 0;
    EXPECT_EQ (YuiTestResources_find ("fx/rim.glsl", sourceSize), nullptr);
}

#endif
