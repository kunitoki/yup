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

#include <yup_shading/yup_shading.h>

using namespace yup;

TEST (ShaderTypesTests, EveryLanguageHasItsName)
{
    EXPECT_STREQ (toString (ShaderLanguage::glsl), "glsl");
    EXPECT_STREQ (toString (ShaderLanguage::essl), "essl");
    EXPECT_STREQ (toString (ShaderLanguage::hlsl), "hlsl");
    EXPECT_STREQ (toString (ShaderLanguage::msl), "msl");
    EXPECT_STREQ (toString (ShaderLanguage::spirv), "spirv");
    EXPECT_STREQ (toString (ShaderLanguage::wgsl), "wgsl");
    EXPECT_STREQ (toString (static_cast<ShaderLanguage> (99)), "unknown");
}

TEST (ShaderTypesTests, EveryStageHasItsName)
{
    EXPECT_STREQ (toString (ShaderStage::vertex), "vertex");
    EXPECT_STREQ (toString (ShaderStage::fragment), "fragment");
    EXPECT_STREQ (toString (ShaderStage::compute), "compute");
    EXPECT_STREQ (toString (ShaderStage::geometry), "geometry");
    EXPECT_STREQ (toString (ShaderStage::tessControl), "tessControl");
    EXPECT_STREQ (toString (ShaderStage::tessEval), "tessEval");
    EXPECT_STREQ (toString (static_cast<ShaderStage> (99)), "unknown");
}
