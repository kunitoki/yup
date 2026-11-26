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

class ComponentTypeInfoTests : public ::testing::Test
{
protected:
    enum class DialMode
    {
        linear = 0,
        rotary = 1
    };

    class SampleWidget : public Component
    {
    public:
        const ComponentTypeInfo& getDefaultTypeInfo() const override
        {
            static const ComponentTypeInfo info = ComponentTypeBuilder<SampleWidget> ("SampleWidget")
                .property ("label", &SampleWidget::getLabel, &SampleWidget::setLabel, String ("Init"))
                .styleableProperty ("origin", &SampleWidget::getOrigin, &SampleWidget::setOrigin, Point<float> { 2.0f, 4.0f })
                .state ("armed", &SampleWidget::isArmed, [] (SampleWidget& w, bool v) { w.setArmed (v); })
                .readOnly ("doubledCount", &SampleWidget::getDoubledCount, 0)
                .enumeration ("mode", { "linear", "rotary" }, &SampleWidget::getMode, &SampleWidget::setMode)
                .color ("ringColor", Identifier ("sample_ring"))
                .signal ("onTriggered", &SampleWidget::onTriggered, "Triggered", { { Identifier ("count"), ComponentProperty::Type::integer } })
                .build();
            return info;
        }

        String getLabel() const { return labelText; }
        void setLabel (const String& text) { labelText = text; }

        Point<float> getOrigin() const noexcept { return originPoint; }
        void setOrigin (Point<float> pt) noexcept { originPoint = pt; }

        bool isArmed() const noexcept { return armedState; }
        void setArmed (bool armed) noexcept { armedState = armed; }

        int getDoubledCount() const noexcept { return triggerCount * 2; }

        DialMode getMode() const noexcept { return dialMode; }
        void setMode (DialMode m) noexcept { dialMode = m; }

        int triggerCount = 3;
        std::function<void (int)> onTriggered;

    private:
        String labelText = "Init";
        Point<float> originPoint { 2.0f, 4.0f };
        bool armedState = false;
        DialMode dialMode = DialMode::linear;
    };

    class MeterWidget : public Component
    {
    public:
        const ComponentTypeInfo& getDefaultTypeInfo() const override { return getMeterTypeInfo(); }

        static const ComponentTypeInfo& getMeterTypeInfo()
        {
            static const ComponentTypeInfo info = ComponentTypeBuilder<MeterWidget> ("MeterWidget")
                .series ("levels", &MeterWidget::getLevels, &MeterWidget::getRevision, "The level of each band.")
                .series ("names", &MeterWidget::getNames)
                .part ("icon", &MeterWidget::getIconBounds, "The icon.")
                .part ("glow")
                .fixedPart ("text", &MeterWidget::getTextBounds, "The text.")
                .build();
            return info;
        }

        Span<const float> getLevels() const noexcept { return levels; }
        Span<const String> getNames() const noexcept { return names; }
        uint32 getRevision() const noexcept { return revision; }
        Rectangle<float> getIconBounds() const noexcept { return { 2.0f, 2.0f, 8.0f, 8.0f }; }
        Rectangle<float> getTextBounds() const noexcept { return { 12.0f, 2.0f, 30.0f, 8.0f }; }

        std::vector<float> levels { 0.25f, 0.5f, 1.0f };
        std::vector<String> names { "low", "high" };
        uint32 revision = 1;
    };

    class LabeledMeterWidget : public MeterWidget
    {
    public:
        const ComponentTypeInfo& getDefaultTypeInfo() const override
        {
            static const ComponentTypeInfo info = ComponentTypeBuilder<LabeledMeterWidget> ("LabeledMeterWidget", getMeterTypeInfo())
                .part ("glow", "The glow of a labeled meter.")
                .build();
            return info;
        }
    };

    /** Makes a theme of its own the global one for as long as it lives: parts are painted by the theme. */
    struct ScopedTheme
    {
        ScopedTheme() { ApplicationTheme::setGlobalTheme (theme); }

        ~ScopedTheme() { ApplicationTheme::setGlobalTheme (oldTheme.get()); }

        ApplicationTheme::Ptr oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::Ptr theme = new ApplicationTheme();
    };
};

TEST_F (ComponentTypeInfoTests, BuilderInfersTypesDefaultsStatelessBoolSettersSignalsAndInheritance)
{
    SampleWidget widget;
    const auto& info = widget.getTypeInfo();

    EXPECT_EQ (info.getTypeName(), Identifier ("SampleWidget"));
    EXPECT_TRUE (info.isSubtypeOf (Identifier ("Component")));
    EXPECT_TRUE (info.hasFactory());

    auto created = info.createInstance();
    ASSERT_NE (created, nullptr);
    EXPECT_NE (dynamic_cast<SampleWidget*> (created.get()), nullptr);

    // Base properties (like opacity) are inherited alongside own properties
    EXPECT_NE (info.findProperty (Identifier ("opacity")), nullptr);

    const auto* originProp = info.findProperty (Identifier ("origin"));
    ASSERT_NE (originProp, nullptr);
    EXPECT_EQ (originProp->type, ComponentProperty::Type::point);
    EXPECT_TRUE (originProp->isStyleable);
    EXPECT_EQ (VariantConverter<Point<float>>::fromVar (originProp->defaultValue), Point<float> (2.0f, 4.0f));

    const auto* armedProp = info.findProperty (Identifier ("armed"));
    ASSERT_NE (armedProp, nullptr);
    EXPECT_EQ (armedProp->type, ComponentProperty::Type::boolean);
    EXPECT_TRUE (armedProp->isState);
    EXPECT_FALSE (armedProp->isStyleable);
    widget.setProperty (Identifier ("armed"), true);
    EXPECT_TRUE (widget.isArmed());

    const auto* readOnlyProp = info.findProperty (Identifier ("doubledCount"));
    ASSERT_NE (readOnlyProp, nullptr);
    EXPECT_TRUE (readOnlyProp->isReadOnly());
    EXPECT_EQ (static_cast<int> (widget.getProperty (Identifier ("doubledCount"))), 6);

    widget.setProperty (Identifier ("mode"), "rotary");
    EXPECT_EQ (widget.getMode(), DialMode::rotary);
    EXPECT_EQ (widget.getProperty (Identifier ("mode")).toString(), "rotary");

    EXPECT_EQ (info.findPropertyByStorageKey (Identifier ("sample_ring")), info.findProperty (Identifier ("ringColor")));

    const auto* trigSignal = info.findSignal (Identifier ("onTriggered"));
    ASSERT_NE (trigSignal, nullptr);
    int received = 0;
    trigSignal->connect (widget, [&] (const Array<var>& args)
    {
        ASSERT_EQ (args.size(), 1);
        received = static_cast<int> (args[0]);
    });
    widget.onTriggered (42);
    EXPECT_EQ (received, 42);
}

TEST_F (ComponentTypeInfoTests, ConnectingANullHandlerDisconnectsASignal)
{
    struct DefaultTheme
    {
        DefaultTheme() { ApplicationTheme::setGlobalTheme (createThemeVersion1()); }

        ~DefaultTheme() { ApplicationTheme::setGlobalTheme (oldTheme.get()); }

        ApplicationTheme::Ptr oldTheme = ApplicationTheme::getGlobalTheme();
    };

    const DefaultTheme theme;
    const auto handler = [] (const Array<var>&) {};

    // A signal the builder makes of a callback of the widget
    SampleWidget widget;
    const auto* triggered = widget.getTypeInfo().findSignal (Identifier ("onTriggered"));
    ASSERT_NE (triggered, nullptr);

    triggered->connect (widget, handler);
    EXPECT_TRUE (widget.onTriggered != nullptr);

    triggered->connect (widget, nullptr);
    EXPECT_TRUE (widget.onTriggered == nullptr);

    // And those the widgets connect themselves, which leave no callback to call a handler that is not there
    TabBar bar;
    const auto* selectionChanged = bar.getTypeInfo().findSignal (Identifier ("onSelectionChanged"));
    ASSERT_NE (selectionChanged, nullptr);

    selectionChanged->connect (bar, handler);
    EXPECT_TRUE (bar.onSelectionChanged != nullptr);

    selectionChanged->connect (bar, nullptr);
    EXPECT_TRUE (bar.onSelectionChanged == nullptr);

    TabComponent tabs;
    const auto* tabChanged = tabs.getTypeInfo().findSignal (Identifier ("onTabChanged"));
    ASSERT_NE (tabChanged, nullptr);

    tabChanged->connect (tabs, handler);
    EXPECT_TRUE (tabs.getTabBar().onSelectionChanged != nullptr);

    tabChanged->connect (tabs, nullptr);
    EXPECT_TRUE (tabs.getTabBar().onSelectionChanged == nullptr);
}

TEST_F (ComponentTypeInfoTests, IsASubtypeOfTheTypesItDerivesFromAndNotOfTheirNamesakes)
{
    const auto& base = MeterWidget::getMeterTypeInfo();

    LabeledMeterWidget labeled;
    const auto& derived = labeled.getTypeInfo();

    EXPECT_TRUE (derived.isSubtypeOf (derived));
    EXPECT_TRUE (derived.isSubtypeOf (base));
    EXPECT_TRUE (derived.isSubtypeOf (Component::getClassTypeInfo()));
    EXPECT_FALSE (base.isSubtypeOf (derived));

    // Another type info of the same name is another type: its accessors may be those of another class
    const ComponentTypeInfo namesake (base.getTypeName(), nullptr, {});
    EXPECT_FALSE (derived.isSubtypeOf (namesake));
    EXPECT_FALSE (namesake.isSubtypeOf (base));

    // By name they are told as the same
    EXPECT_TRUE (derived.isSubtypeOf (base.getTypeName()));
    EXPECT_TRUE (namesake.isSubtypeOf (base.getTypeName()));
}

TEST_F (ComponentTypeInfoTests, SeriesAreReadWhereTheComponentKeepsThem)
{
    MeterWidget widget;
    const auto& info = widget.getTypeInfo();

    const auto* levels = info.findProperty (Identifier ("levels"));
    ASSERT_NE (levels, nullptr);
    EXPECT_EQ (levels->type, ComponentProperty::Type::array);
    EXPECT_EQ (levels->elementType, ComponentProperty::Type::number);
    EXPECT_TRUE (levels->isSeries());
    EXPECT_TRUE (levels->isReadOnly());
    EXPECT_FALSE (levels->isStyleable);
    EXPECT_EQ (levels->description, String ("The level of each band."));

    // The view is the memory of the component, with the revision it tells
    const auto view = levels->arrayGetter (widget);
    EXPECT_EQ (view.data, widget.levels.data());
    EXPECT_EQ (view.size, 3);
    EXPECT_EQ (view.revision, 1u);
    EXPECT_FLOAT_EQ (view.as<float>()[2], 1.0f);

    widget.levels[2] = 0.75f;
    widget.revision = 2;
    EXPECT_FLOAT_EQ (levels->arrayGetter (widget).as<float>()[2], 0.75f);
    EXPECT_EQ (levels->arrayGetter (widget).revision, 2u);

    // Whatever reads a var gets the same values
    const auto values = widget.getProperty (Identifier ("levels"));
    ASSERT_TRUE (values.isArray());
    ASSERT_EQ (values.size(), 3);
    EXPECT_DOUBLE_EQ (static_cast<double> (values[0]), 0.25);
    EXPECT_DOUBLE_EQ (static_cast<double> (values[2]), 0.75);

    // A series is of the type of its elements, and its revision is not tracked unless it is given
    const auto* names = info.findProperty (Identifier ("names"));
    ASSERT_NE (names, nullptr);
    EXPECT_EQ (names->elementType, ComponentProperty::Type::string);
    EXPECT_EQ (names->arrayGetter (widget).size, 2);
    EXPECT_EQ (names->arrayGetter (widget).revision, 0u);
    EXPECT_EQ (names->arrayGetter (widget).as<String>()[1], String ("high"));
    EXPECT_EQ (widget.getProperty (Identifier ("names"))[0].toString(), String ("low"));

    // An array kept as a var is no series
    EXPECT_FALSE (ComboBox::getClassTypeInfo().findProperty (Identifier ("items"))->isSeries());
}

TEST_F (ComponentTypeInfoTests, PartsAreDeclaredByTheTypeAndPaintedByTheThemeWhereTheComponentPutsThem)
{
    MeterWidget widget;
    widget.setBounds (0.0f, 0.0f, 48.0f, 12.0f);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (48, 12);
    Graphics g (*context, *renderer);

    const auto& info = widget.getTypeInfo();
    ASSERT_EQ (info.getOwnParts().size(), 3u);
    EXPECT_FALSE (info.hasPart (Identifier ("caret")));

    const auto* icon = info.findPart (Identifier ("icon"));
    ASSERT_NE (icon, nullptr);
    EXPECT_TRUE (icon->isPlaceable);
    EXPECT_EQ (icon->description, String ("The icon."));
    EXPECT_EQ (icon->getArea (widget), Rectangle<float> (2.0f, 2.0f, 8.0f, 8.0f));

    const auto* text = info.findPart (Identifier ("text"));
    ASSERT_NE (text, nullptr);
    EXPECT_FALSE (text->isPlaceable);

    // A theme that sets no painter for a part paints nothing of it
    ScopedTheme scoped;
    EXPECT_FALSE (widget.paintPart (g, Identifier ("icon")));

    std::map<String, Rectangle<float>> painted;

    for (const auto* name : { "icon", "glow", "text" })
    {
        scoped.theme->setComponentPart (info, Identifier (name), [&painted, name] (Graphics&, const ApplicationTheme&, const Component&, const Rectangle<float>& area)
        {
            painted[name] = area;
        });
    }

    EXPECT_FALSE (widget.paintPart (g, Identifier ("caret")));

    // A part goes where the component puts it, or where it is told
    EXPECT_TRUE (widget.paintPart (g, Identifier ("icon")));
    EXPECT_EQ (painted["icon"], Rectangle<float> (2.0f, 2.0f, 8.0f, 8.0f));

    EXPECT_TRUE (widget.paintPart (g, Identifier ("icon"), { 20.0f, 0.0f, 12.0f, 12.0f }));
    EXPECT_EQ (painted["icon"], Rectangle<float> (20.0f, 0.0f, 12.0f, 12.0f));

    // One that has no place of its own takes the bounds of the component
    EXPECT_TRUE (widget.paintPart (g, Identifier ("glow")));
    EXPECT_EQ (painted["glow"], Rectangle<float> (0.0f, 0.0f, 48.0f, 12.0f));

    // One the component lays out itself stays where it is
    EXPECT_TRUE (widget.paintPart (g, Identifier ("text"), { 0.0f, 0.0f, 4.0f, 4.0f }));
    EXPECT_EQ (painted["text"], Rectangle<float> (12.0f, 2.0f, 30.0f, 8.0f));
}

TEST_F (ComponentTypeInfoTests, PartsAreInheritedAndOverridden)
{
    LabeledMeterWidget widget;
    widget.setBounds (0.0f, 0.0f, 48.0f, 12.0f);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (48, 12);
    Graphics g (*context, *renderer);

    const auto& info = widget.getTypeInfo();
    const auto& baseInfo = MeterWidget::getMeterTypeInfo();

    EXPECT_EQ (info.getOwnParts().size(), 1u);
    EXPECT_EQ (info.getAllParts().size(), 3u);
    EXPECT_TRUE (info.hasPart (Identifier ("icon")));
    EXPECT_TRUE (info.findProperty (Identifier ("levels"))->isSeries());

    // The part a derived type declares again is the one found
    EXPECT_EQ (info.findPart (Identifier ("glow"))->description, String ("The glow of a labeled meter."));
    EXPECT_EQ (info.findPart (Identifier ("icon")), baseInfo.findPart (Identifier ("icon")));

    ScopedTheme scoped;
    int numBaseGlows = 0, numOwnGlows = 0, numIcons = 0;

    const auto count = [] (int& counter)
    {
        return [&counter] (Graphics&, const ApplicationTheme&, const Component&, const Rectangle<float>&)
        {
            ++counter;
        };
    };

    // The painter a theme sets for a type paints the part of the types deriving from it
    scoped.theme->setComponentPart (baseInfo, Identifier ("glow"), count (numBaseGlows));
    scoped.theme->setComponentPart (baseInfo, Identifier ("icon"), count (numIcons));

    EXPECT_TRUE (widget.paintPart (g, Identifier ("glow")));
    EXPECT_TRUE (widget.paintPart (g, Identifier ("icon")));
    EXPECT_EQ (numBaseGlows, 1);
    EXPECT_EQ (numIcons, 1);

    // Until it sets one for the derived type, which leaves the base type as it was
    scoped.theme->setComponentPart (info, Identifier ("glow"), count (numOwnGlows));

    EXPECT_TRUE (widget.paintPart (g, Identifier ("glow")));
    EXPECT_EQ (numOwnGlows, 1);
    EXPECT_EQ (numBaseGlows, 1);

    MeterWidget base;
    EXPECT_TRUE (base.paintPart (g, Identifier ("glow")));
    EXPECT_EQ (numOwnGlows, 1);
    EXPECT_EQ (numBaseGlows, 2);
}
