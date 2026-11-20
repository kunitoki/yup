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
