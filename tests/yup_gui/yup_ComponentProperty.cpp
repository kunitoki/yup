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

class ComponentPropertyTest : public ::testing::Test
{
protected:
    struct RecordingListener : public ComponentListener
    {
        void componentPropertyChanged (Component& component, const Identifier& name) override
        {
            ignoreUnused (component);
            ++callCount;
            lastName = name;
        }

        int callCount = 0;
        Identifier lastName;
    };

    static const ComponentProperty* findByName (const std::vector<const ComponentProperty*>& properties, const Identifier& name)
    {
        for (const auto* property : properties)
            if (property->name == name)
                return property;

        return nullptr;
    }

    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

// =============================================================================
// ComponentProperty descriptor helpers
// =============================================================================

TEST_F (ComponentPropertyTest, StoreBackedDescriptorHelpers)
{
    const auto property = makeColorProperty (Identifier ("thumbColor"), Identifier ("sliderThumb"));

    EXPECT_TRUE (property.isStoreBacked());
    EXPECT_FALSE (property.isStateBacked());
    EXPECT_FALSE (property.isReadOnly());
    EXPECT_EQ (property.getStorageKey(), Identifier ("sliderThumb"));
    EXPECT_EQ (property.type, ComponentProperty::Type::color);
}

TEST_F (ComponentPropertyTest, StorageKeyFallsBackToName)
{
    ComponentProperty property {
        .name = Identifier ("custom"),
        .type = ComponentProperty::Type::number
    };

    EXPECT_EQ (property.getStorageKey(), Identifier ("custom"));
}

TEST_F (ComponentPropertyTest, StateBackedReadOnlyHelpers)
{
    ComponentProperty readOnly {
        .name = Identifier ("normalized"),
        .type = ComponentProperty::Type::number,
        .getter = [] (const Component&) { return var (0.0); }
    };

    EXPECT_TRUE (readOnly.isStateBacked());
    EXPECT_FALSE (readOnly.isStoreBacked());
    EXPECT_TRUE (readOnly.isReadOnly());

    ComponentProperty readWrite {
        .name = Identifier ("value"),
        .type = ComponentProperty::Type::number,
        .getter = [] (const Component&) { return var (0.0); },
        .setter = [] (Component&, const var&) {}
    };

    EXPECT_TRUE (readWrite.isStateBacked());
    EXPECT_FALSE (readWrite.isReadOnly());
}

// =============================================================================
// Store-backed custom properties
// =============================================================================

TEST_F (ComponentPropertyTest, StoreBackedCustomRoundTrip)
{
    Component c ("c");

    EXPECT_FALSE (c.hasProperty (Identifier ("foo")));

    c.setProperty (Identifier ("foo"), 3.0);
    EXPECT_TRUE (c.hasProperty (Identifier ("foo")));
    EXPECT_DOUBLE_EQ (static_cast<double> (c.getProperty (Identifier ("foo"))), 3.0);

    c.removeProperty (Identifier ("foo"));
    EXPECT_FALSE (c.hasProperty (Identifier ("foo")));
    EXPECT_TRUE (c.getProperty (Identifier ("foo")).isVoid());
}

TEST_F (ComponentPropertyTest, StoreBackedCascadePrefersNearestAncestor)
{
    Component parent ("parent");
    Component child ("child");
    parent.addAndMakeVisible (child);

    const Identifier id ("spacing");

    parent.setProperty (id, 4.0);

    // getProperty reads locally only, findProperty cascades to the parent.
    EXPECT_TRUE (child.getProperty (id).isVoid());
    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (id)), 4.0);

    // A local override wins over the ancestor.
    child.setProperty (id, 9.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (id)), 9.0);

    // Removing the override falls back to the ancestor again.
    child.removeProperty (id);
    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (id)), 4.0);
}

TEST_F (ComponentPropertyTest, FindPropertyFallsBackToThemeThenDefault)
{
    const Identifier id ("spacing");

    Component c ("c");

    // No store value anywhere: an undescribed property resolves to a void var.
    EXPECT_TRUE (c.findProperty (id).isVoid());

    // The theme default is consulted after the component chain.
    theme->setProperty (id, 7.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (c.findProperty (id)), 7.0);
}

TEST_F (ComponentPropertyTest, DescriptorDefaultResolvesWhenUnset)
{
    Label label;

    // Label declares textHeightProportion with a curated default of 0.75.
    EXPECT_TRUE (label.getProperty (Identifier ("textHeightProportion")).isVoid());
    EXPECT_DOUBLE_EQ (static_cast<double> (label.findProperty (Identifier ("textHeightProportion"))), 0.75);
}

// =============================================================================
// State-backed properties route through accessors
// =============================================================================

TEST_F (ComponentPropertyTest, StateBackedRoutesThroughAccessors)
{
    Component c ("c");

    c.setProperty (Identifier ("visible"), false);
    EXPECT_FALSE (c.isVisible());
    EXPECT_FALSE (static_cast<bool> (c.getProperty (Identifier ("visible"))));

    c.setProperty (Identifier ("visible"), true);
    EXPECT_TRUE (c.isVisible());
    EXPECT_TRUE (static_cast<bool> (c.getProperty (Identifier ("visible"))));
}

TEST_F (ComponentPropertyTest, StateBackedWidgetValueRoundTrip)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);

    slider.setProperty (Identifier ("value"), 4.0);
    EXPECT_DOUBLE_EQ (slider.getValue(), 4.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (slider.getProperty (Identifier ("value"))), 4.0);

    // A state-backed property is never kept in the store and cannot be removed.
    slider.removeProperty (Identifier ("value"));
    EXPECT_DOUBLE_EQ (slider.getValue(), 4.0);
}

TEST_F (ComponentPropertyTest, ReadOnlyStatePropertyIsFlaggedByDescriptor)
{
    Slider slider (Slider::LinearHorizontal);

    const auto* descriptor = slider.getTypeInfo().findProperty (Identifier ("normalized"));
    ASSERT_NE (descriptor, nullptr);
    EXPECT_TRUE (descriptor->isReadOnly());
}

// =============================================================================
// Change notifications
// =============================================================================

TEST_F (ComponentPropertyTest, NotifiesListenerOnChange)
{
    RecordingListener listener;
    Component c ("c");
    c.addComponentListener (&listener);

    c.setProperty (Identifier ("foo"), 1.0);
    EXPECT_EQ (listener.callCount, 1);
    EXPECT_EQ (listener.lastName, Identifier ("foo"));

    // Setting the same value again does not fire a change.
    c.setProperty (Identifier ("foo"), 1.0);
    EXPECT_EQ (listener.callCount, 1);

    // A real change fires again.
    c.setProperty (Identifier ("foo"), 2.0);
    EXPECT_EQ (listener.callCount, 2);

    c.removeProperty (Identifier ("foo"));
    EXPECT_EQ (listener.callCount, 3);

    c.removeComponentListener (&listener);
}

TEST_F (ComponentPropertyTest, SetColorNotifiesListener)
{
    RecordingListener listener;
    Component c ("c");
    c.addComponentListener (&listener);

    const Identifier colorId ("myColor");
    c.setColor (colorId, Colors::red);

    EXPECT_EQ (listener.callCount, 1);
    EXPECT_EQ (listener.lastName, colorId);

    c.removeComponentListener (&listener);
}

// =============================================================================
// ComponentTypeInfo
// =============================================================================

TEST_F (ComponentPropertyTest, TypeInfoFindsOwnAndInheritedProperties)
{
    const auto& info = Slider::getClassTypeInfo();

    EXPECT_EQ (info.getTypeName(), Identifier ("Slider"));
    EXPECT_NE (info.findProperty (Identifier ("value")), nullptr);
    EXPECT_NE (info.findProperty (Identifier ("visible")), nullptr); // inherited from Component
    EXPECT_EQ (info.findProperty (Identifier ("doesNotExist")), nullptr);
}

TEST_F (ComponentPropertyTest, TypeInfoSubtypeRelations)
{
    const auto& info = Slider::getClassTypeInfo();

    EXPECT_TRUE (info.isSubtypeOf (Identifier ("Slider")));
    EXPECT_TRUE (info.isSubtypeOf (Identifier ("Component")));
    EXPECT_FALSE (info.isSubtypeOf (Identifier ("Label")));
    EXPECT_TRUE (info.isSubtypeOf (Component::getClassTypeInfo()));
}

TEST_F (ComponentPropertyTest, TypeInfoGetAllPropertiesIncludesInherited)
{
    const auto all = Slider::getClassTypeInfo().getAllProperties();

    EXPECT_NE (findByName (all, Identifier ("value")), nullptr);
    EXPECT_NE (findByName (all, Identifier ("thumbColor")), nullptr);
    EXPECT_NE (findByName (all, Identifier ("visible")), nullptr);
}

TEST_F (ComponentPropertyTest, TypeInfoSignals)
{
    EXPECT_TRUE (Slider::getClassTypeInfo().hasSignal (Identifier ("onValueChanged")));
    EXPECT_FALSE (Slider::getClassTypeInfo().hasSignal (Identifier ("onNothing")));
}

TEST_F (ComponentPropertyTest, TypeInfoFactory)
{
    // Label is default-constructible, so it has a factory.
    EXPECT_TRUE (Label::getClassTypeInfo().hasFactory());
    auto label = Label::getClassTypeInfo().createInstance();
    EXPECT_NE (label, nullptr);

    // Slider requires a SliderType, so it intentionally has no factory.
    EXPECT_FALSE (Slider::getClassTypeInfo().hasFactory());
    EXPECT_EQ (Slider::getClassTypeInfo().createInstance(), nullptr);
}

TEST_F (ComponentPropertyTest, OverriddenTypeInfoReportsDerivedType)
{
    Label label;

    EXPECT_EQ (label.getTypeInfo().getTypeName(), Identifier ("Label"));
    EXPECT_TRUE (label.getTypeInfo().isSubtypeOf (Identifier ("Component")));
}
