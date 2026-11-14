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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class ComponentEffectRegistryTests : public ::testing::Test
{
protected:
    class RegistryTestEffect : public ComponentEffect
    {
    public:
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}
    };

    void TearDown() override
    {
        ComponentEffectRegistry::getInstance().unregisterEffect (effectName);
    }

    static ComponentEffect::Ptr createTestEffect()
    {
        return ComponentEffect::Ptr (new RegistryTestEffect());
    }

    const Identifier effectName { "registryTestEffect" };
};

TEST_F (ComponentEffectRegistryTests, CreatesAFreshEffectPerCall)
{
    auto& registry = ComponentEffectRegistry::getInstance();
    registry.registerEffect (effectName, createTestEffect);

    EXPECT_TRUE (registry.isRegistered (effectName));

    const auto first = registry.createEffect (effectName);
    const auto second = registry.createEffect (effectName);

    ASSERT_NE (first, nullptr);
    EXPECT_NE (first, second);
    EXPECT_NE (dynamic_cast<RegistryTestEffect*> (first.get()), nullptr);
}

TEST_F (ComponentEffectRegistryTests, ReturnsNullForUnknownNames)
{
    auto& registry = ComponentEffectRegistry::getInstance();

    EXPECT_FALSE (registry.isRegistered (effectName));
    EXPECT_EQ (registry.createEffect (effectName), nullptr);

    registry.registerEffect (effectName, createTestEffect);
    registry.unregisterEffect (effectName);

    EXPECT_FALSE (registry.isRegistered (effectName));
    EXPECT_EQ (registry.createEffect (effectName), nullptr);
}
