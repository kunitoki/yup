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
    static const auto info = ComponentTypeBuilder<Component> (Identifier ("TestCustomType")).build();
    return info;
}

} // namespace

class ComponentTypeRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        registerBuiltInComponentTypes();
    }
};

TEST_F (ComponentTypeRegistryTest, RegistersAllBuiltInTypes)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    for (const char* name : { "Component", "Button", "Layout", "Label", "Slider", "TextButton", "ToggleButton", "SwitchButton", "ComboBox", "TextEditor", "ProgressBar", "ScrollBar", "ListBox", "Viewport", "TabComponent", "FlexBox", "Row", "Column", "Grid", "Stack" })
        EXPECT_NE (registry.findType (Identifier (name)), nullptr) << name;
}

TEST_F (ComponentTypeRegistryTest, FindTypeReturnsMatchingInfo)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    const auto* slider = registry.findType (Identifier ("Slider"));
    ASSERT_NE (slider, nullptr);
    EXPECT_EQ (slider->getTypeName(), Identifier ("Slider"));

    EXPECT_EQ (registry.findType (Identifier ("DoesNotExist")), nullptr);
}

TEST_F (ComponentTypeRegistryTest, TypeInfosCreateExpectedInstances)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    const auto* labelInfo = registry.findType (Identifier ("Label"));
    ASSERT_NE (labelInfo, nullptr);
    auto label = labelInfo->createInstance();
    ASSERT_NE (label, nullptr);
    EXPECT_EQ (label->getTypeInfo().getTypeName(), Identifier ("Label"));

    const auto* sliderInfo = registry.findType (Identifier ("Slider"));
    ASSERT_NE (sliderInfo, nullptr);
    auto slider = sliderInfo->createInstance();
    ASSERT_NE (slider, nullptr);
    EXPECT_EQ (slider->getProperty (Identifier ("sliderType")), var ("linearHorizontal"));

    const auto* buttonInfo = registry.findType (Identifier ("Button"));
    ASSERT_NE (buttonInfo, nullptr);
    EXPECT_EQ (buttonInfo->createInstance(), nullptr);
}

TEST_F (ComponentTypeRegistryTest, RegisterCustomType)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    registry.registerType (customTypeInfo());

    const auto* registered = registry.findType (Identifier ("TestCustomType"));
    ASSERT_NE (registered, nullptr);
    EXPECT_NE (registered->createInstance(), nullptr);
}
