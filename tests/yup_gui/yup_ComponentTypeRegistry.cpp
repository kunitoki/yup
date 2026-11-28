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

    for (const char* name : { "Component", "Button", "Layout", "Label", "Slider", "TextButton", "ToggleButton", "SwitchButton", "ComboBox", "TextEditor", "ProgressBar", "ScrollBar", "ResizableDivider", "JoystickComponent", "ListBox", "Viewport", "TabComponent", "FlexBox", "Row", "Column", "Grid", "Stack" })
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
    const auto custom = ComponentTypeBuilder<Component> (Identifier ("TestCustomType")).build();

    registry.registerType (custom);

    const auto* registered = registry.findType (Identifier ("TestCustomType"));
    registry.unregisterType (custom);

    ASSERT_NE (registered, nullptr);
    EXPECT_NE (registered->createInstance(), nullptr);
}

TEST_F (ComponentTypeRegistryTest, ADefaultTypeLeavesTheOneRegisteredUnderItsName)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    // The registry points at them for as long as the process runs
    static const auto first = ComponentTypeBuilder<Component> (Identifier ("TestDefaultType")).build();
    static const auto second = ComponentTypeBuilder<Component> (Identifier ("TestDefaultType")).build();

    registry.registerDefaultType (first);
    EXPECT_EQ (registry.findType (Identifier ("TestDefaultType")), &first);

    // What a module comes with does not take the place of what was registered before, as the built-in types are
    registry.registerDefaultType (second);
    EXPECT_EQ (registry.findType (Identifier ("TestDefaultType")), &first);

    registry.registerType (second);
    EXPECT_EQ (registry.findType (Identifier ("TestDefaultType")), &second);
}

TEST_F (ComponentTypeRegistryTest, DefaultTypesAreRegisteredTogetherAndLeaveTheOnesRegisteredUnderTheirNames)
{
    auto& registry = ComponentTypeRegistry::getInstance();

    // The registry points at them for as long as the process runs
    static const auto taken = ComponentTypeBuilder<Component> (Identifier ("TestDefaultTypesTaken")).build();
    static const auto first = ComponentTypeBuilder<Component> (Identifier ("TestDefaultTypesFirst")).build();
    static const auto second = ComponentTypeBuilder<Component> (Identifier ("TestDefaultTypesSecond")).build();
    static const auto late = ComponentTypeBuilder<Component> (Identifier ("TestDefaultTypesTaken")).build();

    registry.registerType (taken);
    registry.registerDefaultTypes ({ first, late, second });

    EXPECT_EQ (registry.findType (Identifier ("TestDefaultTypesFirst")), &first);
    EXPECT_EQ (registry.findType (Identifier ("TestDefaultTypesSecond")), &second);
    EXPECT_EQ (registry.findType (Identifier ("TestDefaultTypesTaken")), &taken);
}

TEST_F (ComponentTypeRegistryTest, UnregisteringATypeRemovesIt)
{
    auto& registry = ComponentTypeRegistry::getInstance();
    const auto info = ComponentTypeBuilder<Component> (Identifier ("TestUnregisteredType")).build();
    const auto numTypes = registry.getRegisteredTypes().size();

    registry.registerType (info);
    EXPECT_EQ (registry.findType (Identifier ("TestUnregisteredType")), &info);
    EXPECT_EQ (registry.getRegisteredTypes().size(), numTypes + 1);

    registry.unregisterType (info);
    EXPECT_EQ (registry.findType (Identifier ("TestUnregisteredType")), nullptr);
    EXPECT_EQ (registry.getRegisteredTypes().size(), numTypes);

    // One that is not registered is left alone
    registry.unregisterType (info);
    EXPECT_EQ (registry.getRegisteredTypes().size(), numTypes);
}

TEST_F (ComponentTypeRegistryTest, UnregisteringATypeLeavesAnotherOneRegisteredUnderItsName)
{
    auto& registry = ComponentTypeRegistry::getInstance();
    const auto first = ComponentTypeBuilder<Component> (Identifier ("TestReplacedType")).build();
    const auto second = ComponentTypeBuilder<Component> (Identifier ("TestReplacedType")).build();

    registry.registerType (first);
    registry.registerType (second);

    // The first one is not the one registered under the name anymore
    registry.unregisterType (first);
    EXPECT_EQ (registry.findType (Identifier ("TestReplacedType")), &second);

    registry.unregisterType (second);
    EXPECT_EQ (registry.findType (Identifier ("TestReplacedType")), nullptr);
}
