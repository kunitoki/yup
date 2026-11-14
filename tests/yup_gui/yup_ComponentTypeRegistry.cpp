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

namespace
{

const ComponentTypeInfo& customTypeInfo()
{
    static const ComponentTypeInfo info {
        Identifier ("TestCustomType"),
        &Component::getClassTypeInfo(),
        {},
        {},
        [] { return std::make_unique<Component>(); }
    };

    return info;
}

} // namespace

class ComponentTypeRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto& registry = ComponentTypeRegistry::getInstance();
        registry.clear();
        registerBuiltInComponentTypes();
    }

    void TearDown() override
    {
        ComponentTypeRegistry::getInstance().clear();
    }
};

TEST_F (ComponentTypeRegistryTest, RegistersAllBuiltInTypes)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    for (const char* name : { "Component", "Label", "Slider", "TextButton", "ToggleButton", "SwitchButton", "ComboBox", "TextEditor", "ProgressBar", "ScrollBar", "ListBox" })
        EXPECT_TRUE (registry.isRegistered (Identifier (name))) << name;
}

TEST_F (ComponentTypeRegistryTest, FindTypeReturnsMatchingInfo)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    const auto* slider = registry.findType (Identifier ("Slider"));
    ASSERT_NE (slider, nullptr);
    EXPECT_EQ (slider->getTypeName(), Identifier ("Slider"));

    EXPECT_EQ (registry.findType (Identifier ("DoesNotExist")), nullptr);
}

TEST_F (ComponentTypeRegistryTest, RegisteredTypeNamesAreSorted)
{
    const auto names = ComponentTypeRegistry::getInstance().getRegisteredTypeNames();

    EXPECT_TRUE (names.contains ("Component"));
    EXPECT_TRUE (names.contains ("Slider"));

    for (int i = 1; i < names.size(); ++i)
        EXPECT_LE (names[i - 1].compare (names[i]), 0);
}

TEST_F (ComponentTypeRegistryTest, CreateInstanceUsesFactory)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    auto label = registry.createInstance (Identifier ("Label"));
    ASSERT_NE (label, nullptr);
    EXPECT_EQ (label->getTypeInfo().getTypeName(), Identifier ("Label"));

    auto slider = registry.createInstance (Identifier ("Slider"));
    ASSERT_NE (slider, nullptr);
    EXPECT_EQ (slider->getProperty (Identifier ("sliderType")), var ("linearHorizontal"));

    // Unknown types also yield nullptr.
    EXPECT_EQ (registry.createInstance (Identifier ("DoesNotExist")), nullptr);
}

TEST_F (ComponentTypeRegistryTest, RegisterAndUnregisterCustomType)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    EXPECT_FALSE (registry.isRegistered (Identifier ("TestCustomType")));

    registry.registerType (customTypeInfo());
    EXPECT_TRUE (registry.isRegistered (Identifier ("TestCustomType")));

    auto instance = registry.createInstance (Identifier ("TestCustomType"));
    EXPECT_NE (instance, nullptr);

    registry.unregisterType (Identifier ("TestCustomType"));
    EXPECT_FALSE (registry.isRegistered (Identifier ("TestCustomType")));
}

TEST_F (ComponentTypeRegistryTest, ClearRemovesAllTypes)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    registry.clear();

    EXPECT_FALSE (registry.isRegistered (Identifier ("Component")));
    EXPECT_TRUE (registry.getRegisteredTypeNames().isEmpty());
}
