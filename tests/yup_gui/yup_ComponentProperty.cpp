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
            names.add (name);
        }

        void reset()
        {
            callCount = 0;
            lastName = {};
            names.clear();
        }

        int callCount = 0;
        Identifier lastName;
        Array<Identifier> names;
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
    const auto info = ComponentTypeBuilder<Component> (Identifier ("TestStoreBacked"))
                          .color ("thumbColor", Identifier ("sliderThumb"))
                          .build();
    const auto* property = info.findProperty (Identifier ("thumbColor"));
    ASSERT_NE (property, nullptr);

    EXPECT_TRUE (property->isStoreBacked());
    EXPECT_FALSE (property->isStateBacked());
    EXPECT_FALSE (property->isReadOnly());
    EXPECT_EQ (property->getStorageKey(), Identifier ("sliderThumb"));
    EXPECT_EQ (property->type, ComponentProperty::Type::color);
}

TEST_F (ComponentPropertyTest, TypeNamesIncludeArrays)
{
    using Type = ComponentProperty::Type;

    EXPECT_EQ (Type::number, ComponentProperty::getTypeFromName ("number"));
    EXPECT_EQ (Type::drawing, ComponentProperty::getTypeFromName ("drawing"));
    EXPECT_EQ (Type::any, ComponentProperty::getTypeFromName ("nonsense"));
    EXPECT_EQ (Type::any, ComponentProperty::getElementTypeFromName ("number"));

    EXPECT_EQ (Type::array, ComponentProperty::getTypeFromName ("string[]"));
    EXPECT_EQ (Type::string, ComponentProperty::getElementTypeFromName ("string[]"));
    EXPECT_EQ (Type::array, ComponentProperty::getTypeFromName ("color[]"));
    EXPECT_EQ (Type::color, ComponentProperty::getElementTypeFromName ("color[]"));

    EXPECT_EQ (Type::array, ComponentProperty::getTypeFromName ("point[]"));
    EXPECT_EQ (Type::point, ComponentProperty::getElementTypeFromName ("point[]"));

    // Only the scalar types an expression can read make an array.
    EXPECT_EQ (Type::any, ComponentProperty::getTypeFromName ("drawing[]"));
    EXPECT_EQ (Type::any, ComponentProperty::getElementTypeFromName ("drawing[]"));
    EXPECT_EQ (Type::any, ComponentProperty::getTypeFromName ("nonsense[]"));
    EXPECT_EQ (Type::any, ComponentProperty::getTypeFromName ("number[][]"));

    EXPECT_TRUE (ComponentProperty::isExpressionType (Type::integer));
    EXPECT_TRUE (ComponentProperty::isExpressionType (Type::rectangle));
    EXPECT_TRUE (ComponentProperty::isExpressionType (Type::transform));
    EXPECT_TRUE (ComponentProperty::isExpressionType (Type::font));
    EXPECT_FALSE (ComponentProperty::isExpressionType (Type::array));
    EXPECT_FALSE (ComponentProperty::isExpressionType (Type::drawing));
}

TEST_F (ComponentPropertyTest, StorageKeyFallsBackToName){
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
    ListBox listBox;

    // ListBox declares refreshIndicatorSize with a curated default of 56.
    EXPECT_TRUE (listBox.getProperty (Identifier ("refreshIndicatorSize")).isVoid());
    EXPECT_DOUBLE_EQ (static_cast<double> (listBox.findProperty (Identifier ("refreshIndicatorSize"))), 56.0);
}

TEST_F (ComponentPropertyTest, ColorPropertiesFallBackToThemeColors)
{
    Slider slider (Slider::RotaryVerticalDrag);
    const Identifier thumbColor ("thumbColor");

    theme->setPaletteColor (Slider::Style::thumbColorId, Color (0xff102030));
    EXPECT_EQ (slider.findPropertyAs<Color> (thumbColor).getARGB(), 0xff102030u);

    theme->setColor (Slider::Style::thumbColorId, Color (0xff405060));
    EXPECT_EQ (slider.findPropertyAs<Color> (thumbColor).getARGB(), 0xff405060u);

    slider.setColor (Slider::Style::thumbColorId, Color (0xff708090));
    EXPECT_EQ (slider.findPropertyAs<Color> (thumbColor).getARGB(), 0xff708090u);
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

TEST_F (ComponentPropertyTest, SetColorOnDescriptorMapsStorageKeyAndNotifiesPropertyName)
{
    RecordingListener listener;
    Slider slider (Slider::LinearHorizontal);
    slider.addComponentListener (&listener);

    slider.setColor (Slider::Style::thumbColorId, Colors::magenta);

    EXPECT_EQ (listener.callCount, 1);
    EXPECT_EQ (listener.lastName, Identifier ("thumbColor"));
    EXPECT_EQ (slider.getColor (Slider::Style::thumbColorId), Colors::magenta);
    EXPECT_EQ (slider.getColor (Identifier ("thumbColor")), Colors::magenta);
    EXPECT_EQ (slider.findColor (Identifier ("thumbColor")), Colors::magenta);
    EXPECT_EQ (VariantConverter<Color>::fromVar (slider.getProperty (Identifier ("thumbColor"))), Colors::magenta);

    slider.setColor (Identifier ("thumbColor"), std::nullopt);

    EXPECT_EQ (listener.callCount, 2);
    EXPECT_EQ (listener.lastName, Identifier ("thumbColor"));
    EXPECT_FALSE (slider.getColor (Slider::Style::thumbColorId).has_value());

    slider.removeComponentListener (&listener);
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

TEST_F (ComponentPropertyTest, ConnectedSignalReceivesItsArguments)
{
    Slider slider (Slider::LinearHorizontal);
    Array<var> received;

    const auto* signal = slider.getTypeInfo().findSignal (Identifier ("onValueChanged"));
    ASSERT_NE (signal, nullptr);

    signal->connect (slider, [&] (const Array<var>& arguments) { received = arguments; });
    slider.setValue (0.25);

    ASSERT_EQ (received.size(), 1);
    EXPECT_DOUBLE_EQ (static_cast<double> (received[0]), 0.25);
}

TEST_F (ComponentPropertyTest, InstanceTypeInfoExtendsTheClassTypeInfo)
{
    const ComponentTypeInfo extended (Identifier ("Knob"),
                                      &Slider::getClassTypeInfo(),
                                      { ComponentProperty { .name = Identifier ("label"), .type = ComponentProperty::Type::string } });

    Slider slider (Slider::LinearHorizontal);
    slider.setTypeInfo (&extended);

    EXPECT_EQ (slider.getTypeInfo().getTypeName(), Identifier ("Knob"));
    EXPECT_TRUE (slider.getTypeInfo().isSubtypeOf (Identifier ("Slider")));
    EXPECT_NE (slider.getTypeInfo().findProperty (Identifier ("label")), nullptr);
    EXPECT_NE (slider.getTypeInfo().findProperty (Identifier ("value")), nullptr);
    EXPECT_EQ (&slider.getDefaultTypeInfo(), &Slider::getClassTypeInfo());

    slider.setTypeInfo (nullptr);

    EXPECT_EQ (&slider.getTypeInfo(), &Slider::getClassTypeInfo());
}

TEST_F (ComponentPropertyTest, TypeInfoFactory)
{
    // Label is default-constructible, so it has a factory.
    EXPECT_TRUE (Label::getClassTypeInfo().hasFactory());
    auto label = Label::getClassTypeInfo().createInstance();
    EXPECT_NE (label, nullptr);

    // Slider defaults to a horizontal linear slider, its type is a property.
    EXPECT_TRUE (Slider::getClassTypeInfo().hasFactory());
    EXPECT_NE (Slider::getClassTypeInfo().createInstance(), nullptr);
}

TEST_F (ComponentPropertyTest, OverriddenTypeInfoReportsDerivedType)
{
    Label label;

    EXPECT_EQ (label.getTypeInfo().getTypeName(), Identifier ("Label"));
    EXPECT_TRUE (label.getTypeInfo().isSubtypeOf (Identifier ("Component")));
}

TEST_F (ComponentPropertyTest, TypeInfoFlattenedLookupOverridesBaseAndSurvivesMove)
{
    ComponentTypeInfo derived (Identifier ("CustomSlider"),
                               &Slider::getClassTypeInfo(),
                               { ComponentProperty { .name = Identifier ("opacity"),
                                                     .type = ComponentProperty::Type::number,
                                                     .defaultValue = 0.5 },
                                 ComponentProperty { .name = Identifier ("accent"),
                                                     .storageId = Identifier ("customAccentKey"),
                                                     .type = ComponentProperty::Type::color } });

    const auto* overridden = derived.findProperty (Identifier ("opacity"));
    ASSERT_NE (overridden, nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (overridden->defaultValue), 0.5);

    const auto* byStorage = derived.findPropertyByStorageKey (Identifier ("customAccentKey"));
    ASSERT_NE (byStorage, nullptr);
    EXPECT_EQ (byStorage->name, Identifier ("accent"));
    EXPECT_EQ (byStorage, &derived.getOwnProperties()[1]);
    EXPECT_NE (derived.findPropertyByStorageKey (Slider::Style::thumbColorId), nullptr);
}

TEST_F (ComponentPropertyTest, ButtonSubtypesDeriveFromButtonTypeInfo)
{
    const auto& buttonType = Button::getClassTypeInfo();
    EXPECT_FALSE (buttonType.hasFactory());
    EXPECT_NE (buttonType.findSignal (Identifier ("onClick")), nullptr);

    for (const auto* typeInfo : { &TextButton::getClassTypeInfo(), &ToggleButton::getClassTypeInfo(), &SwitchButton::getClassTypeInfo() })
    {
        EXPECT_TRUE (typeInfo->isSubtypeOf (buttonType)) << typeInfo->getTypeName().toString();
        EXPECT_NE (typeInfo->findSignal (Identifier ("onClick")), nullptr) << typeInfo->getTypeName().toString();
    }
}

TEST_F (ComponentPropertyTest, AllWritableStateBackedPropertiesNotifyListeners)
{
    const ComponentTypeInfo* const types[] = {
        &Component::getClassTypeInfo(),
        &TextButton::getClassTypeInfo(),
        &ToggleButton::getClassTypeInfo(),
        &SwitchButton::getClassTypeInfo(),
        &Label::getClassTypeInfo(),
        &Slider::getClassTypeInfo(),
        &ComboBox::getClassTypeInfo(),
        &TextEditor::getClassTypeInfo(),
        &ProgressBar::getClassTypeInfo(),
        &ImageComponent::getClassTypeInfo(),
        &ScrollBar::getClassTypeInfo(),
        &ListBoxItem::getClassTypeInfo(),
        &ListBox::getClassTypeInfo(),
        &TreeViewRow::getClassTypeInfo(),
        &TreeView::getClassTypeInfo(),
        &Viewport::getClassTypeInfo(),
        &TabBar::getClassTypeInfo(),
        &TabComponent::getClassTypeInfo(),
        &ResizableCorner::getClassTypeInfo(),
        &CodeEditor::getClassTypeInfo(),
        &FlexBoxComponent::getClassTypeInfo(),
        &GridComponent::getClassTypeInfo(),
        &StackComponent::getClassTypeInfo(),
        &WindowComponent::getClassTypeInfo()
    };

    for (const auto* typeInfo : types)
    {
        for (const auto& property : typeInfo->getOwnProperties())
        {
            if (! property.isStateBacked() || property.isReadOnly())
                continue;

            auto instance = typeInfo->createInstance();
            ASSERT_NE (instance, nullptr) << typeInfo->getTypeName().toString();

            Component pageOne;
            Component pageTwo;
            if (auto* tabs = dynamic_cast<TabComponent*> (instance.get()))
            {
                tabs->addTab (Identifier ("one"), "One", pageOne);
                tabs->addTab (Identifier ("two"), "Two", pageTwo);
            }
            else if (auto* bar = dynamic_cast<TabBar*> (instance.get()))
            {
                bar->addTab (Identifier ("one"), "One");
                bar->addTab (Identifier ("two"), "Two");
            }
            else if (auto* combo = dynamic_cast<ComboBox*> (instance.get()))
            {
                combo->addItem ("One", 1);
                combo->addItem ("Two", 2);
            }
            else if (auto* list = dynamic_cast<ListBox*> (instance.get()))
            {
                list->setItems ({ "One", "Two" });
            }

            const auto current = property.getter (*instance);
            var nextValue;

            switch (property.type)
            {
                case ComponentProperty::Type::boolean:
                    nextValue = ! static_cast<bool> (current);
                    break;

                case ComponentProperty::Type::number:
                    nextValue = approximatelyEqual (static_cast<double> (current), 0.5) ? var (0.75) : var (0.5);
                    break;

                case ComponentProperty::Type::integer:
                    nextValue = static_cast<int> (current) == 1 ? var (0) : var (1);
                    break;

                case ComponentProperty::Type::string:
                    nextValue = property.name == Identifier ("selectedTab") ? var ("two") : var ("updated");
                    break;

                case ComponentProperty::Type::enumeration:
                    ASSERT_GE (property.enumValues.size(), 2) << property.name.toString();
                    nextValue = property.enumValues[current.toString() == property.enumValues[0] ? 1 : 0];
                    break;

                case ComponentProperty::Type::point:
                    nextValue = VariantConverter<Point<float>>::toVar ({ 12.0f, 24.0f });
                    break;

                case ComponentProperty::Type::rectangle:
                    nextValue = VariantConverter<Rectangle<float>>::toVar ({ 5.0f, 10.0f, 80.0f, 40.0f });
                    break;

                case ComponentProperty::Type::transform:
                    nextValue = VariantConverter<AffineTransform>::toVar (AffineTransform::translation (8.0f, 16.0f));
                    break;

                case ComponentProperty::Type::color:
                    nextValue = VariantConverter<Color>::toVar (Color (0xff123456));
                    break;

                case ComponentProperty::Type::array:
                {
                    Array<var> elements;
                    elements.add ("Alpha");
                    elements.add ("Beta");
                    nextValue = elements;
                    break;
                }

                default:
                    FAIL() << "Unhandled writable state-backed property type on " << typeInfo->getTypeName().toString() << "." << property.name.toString();
            }

            RecordingListener listener;
            instance->addComponentListener (&listener);
            instance->setProperty (property.name, nextValue);

            EXPECT_TRUE (listener.names.contains (property.name))
                << typeInfo->getTypeName().toString() << "." << property.name.toString() << " did not notify on change";

            instance->removeComponentListener (&listener);
        }
    }
}

TEST_F (ComponentPropertyTest, StateBackedMutatorsNotifyEvenWithDontSendNotification)
{
    RecordingListener listener;

    {
        Component component ("initial");
        component.setBounds (1.0f, 2.0f, 30.0f, 40.0f);
        component.addComponentListener (&listener);

        // Changing only x must notify x and bounds, without spurious y/width/height notifications.
        component.setBounds (10.0f, 2.0f, 30.0f, 40.0f);
        ASSERT_EQ (listener.names.size(), 2);
        EXPECT_TRUE (listener.names.contains (Identifier ("x")));
        EXPECT_TRUE (listener.names.contains (Identifier ("bounds")));

        listener.reset();
        component.setTransform (AffineTransform::translation (4.0f, 6.0f));
        EXPECT_TRUE (listener.names.contains (Identifier ("transform")));

        listener.reset();
        component.setComponentID ("renamed");
        EXPECT_TRUE (listener.names.contains (Identifier ("id")));

        component.removeComponentListener (&listener);
    }

    {
        Slider slider (Slider::TwoValueHorizontal);
        slider.setRange (0.0, 10.0, 0.0);
        slider.addComponentListener (&listener);

        listener.reset();
        slider.setValue (5.0, dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("value")));
        EXPECT_TRUE (listener.names.contains (Identifier ("normalized")));

        // Changing the range while value stays inside it still shifts normalized.
        listener.reset();
        slider.setRange (0.0, 20.0, 0.0);
        EXPECT_TRUE (listener.names.contains (Identifier ("maximum")));
        EXPECT_TRUE (listener.names.contains (Identifier ("normalized")));

        listener.reset();
        slider.setMinValue (2.0, dontSendNotification, false);
        EXPECT_TRUE (listener.names.contains (Identifier ("minValue")));

        listener.reset();
        slider.setMaxValue (8.0, dontSendNotification, false);
        EXPECT_TRUE (listener.names.contains (Identifier ("maxValue")));

        slider.removeComponentListener (&listener);
    }

    {
        Label label;
        label.addComponentListener (&listener);
        listener.reset();
        label.setText ("quiet", dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("text")));
        label.removeComponentListener (&listener);
    }

    {
        TextEditor editor;
        editor.addComponentListener (&listener);
        listener.reset();
        editor.setText ("quiet", dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("text")));
        editor.removeComponentListener (&listener);
    }

    {
        ComboBox combo;
        combo.setItems ({ "One", "Two" }, dontSendNotification);
        combo.addComponentListener (&listener);

        listener.reset();
        combo.setSelectedId (2, dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("selectedId")));
        EXPECT_TRUE (listener.names.contains (Identifier ("selectedIndex")));
        EXPECT_TRUE (listener.names.contains (Identifier ("text")));

        combo.removeComponentListener (&listener);
    }

    {
        ListBox listBox;
        listBox.setItems ({ "One", "Two" });
        listBox.addComponentListener (&listener);

        listener.reset();
        listBox.selectRow (1, false, dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("selectedRow")));

        listener.reset();
        listBox.deselectAllRows (dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("selectedRow")));

        listBox.removeComponentListener (&listener);
    }

    {
        ProgressBar bar;
        bar.addComponentListener (&listener);

        listener.reset();
        bar.setProgress (0.4, dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("progress")));

        listener.reset();
        bar.setProgress (-1.0, dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("progress")));
        EXPECT_TRUE (listener.names.contains (Identifier ("indeterminate")));

        bar.removeComponentListener (&listener);
    }

    {
        TabComponent tabs;
        Component firstPage;
        Component secondPage;
        tabs.addTab (Identifier ("first"), "First", firstPage);
        tabs.addTab (Identifier ("second"), "Second", secondPage);
        tabs.addComponentListener (&listener);

        listener.reset();
        tabs.getTabBar().setSelectedTab (Identifier ("second"), dontSendNotification);
        EXPECT_TRUE (listener.names.contains (Identifier ("selectedTab")));

        tabs.removeComponentListener (&listener);
    }
}

TEST_F (ComponentPropertyTest, SliderCuratedRotaryGeometryDefaultsAndOverrides)
{
    Slider slider (Slider::RotaryVerticalDrag);

    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryInset")), 0.1f);
    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryDialInset")), 0.105f);
    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryDialOutlineWidth")), 0.0175f);
    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryPointerInset")), 0.175f);
    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryPointerWidth")), 0.03f);
    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryArcWidth")), 0.075f);

    slider.setProperty (Identifier ("rotaryArcWidth"), 0.12);
    EXPECT_FLOAT_EQ (slider.findPropertyAs<float> (Identifier ("rotaryArcWidth")), 0.12f);
}

TEST_F (ComponentPropertyTest, NonDefaultConstructibleGuiTypesExposeDescriptorsWithoutSpawningDesktopWindows)
{
    EXPECT_FALSE (DocumentWindow::getClassTypeInfo().hasFactory());
    EXPECT_NE (DocumentWindow::getClassTypeInfo().findProperty (Identifier ("backgroundColor")), nullptr);
    EXPECT_NE (DocumentWindow::getClassTypeInfo().findProperty (Identifier ("resizable")), nullptr);

    EXPECT_FALSE (TabButton::getClassTypeInfo().hasFactory());
    EXPECT_TRUE (TabButton::getClassTypeInfo().isSubtypeOf (Button::getClassTypeInfo()));
    EXPECT_NE (TabButton::getClassTypeInfo().findProperty (Identifier ("cornerRadius")), nullptr);

    TabBar bar;
    auto& tab = bar.addTab (Identifier ("main"), "Main");
    RecordingListener listener;
    tab.addComponentListener (&listener);

    tab.setText ("Updated");
    tab.setClosable (true);
    EXPECT_TRUE (listener.names.contains (Identifier ("text")));
    EXPECT_TRUE (listener.names.contains (Identifier ("closable")));

    tab.removeComponentListener (&listener);
}

