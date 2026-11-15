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

namespace yup
{

/** Gives the binding tests access to the internal mouse entry points. */
template <>
class ComponentTestHelper<Slider>
{
public:
    static void triggerMouseEnter (Component& component, const MouseEvent& event)
    {
        component.internalMouseEnter (event);
    }

    static void triggerMouseDown (Component& component, const MouseEvent& event)
    {
        component.internalMouseDown (event);
    }

    static void triggerMouseUp (Component& component, const MouseEvent& event)
    {
        component.internalMouseUp (event);
    }
};

} // namespace yup

using namespace yup;

class BindingGraphTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        graph.setComponentResolver ([this] (const String& id) -> Component*
                                    {
            const auto it = named.find (id);
            return it != named.end() ? it->second : nullptr;
        });
    }

    void TearDown() override
    {
        graph.clear();
        named.clear();

        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    void nameIt (const String& id, Component& component)
    {
        named[id] = &component;
    }

    static constexpr auto valueId = "value";
    static constexpr auto widthId = "width";

    BindingGraph graph;
    std::map<String, Component*> named;
    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (BindingGraphTests, EvaluatesAConstantWithoutKeepingABinding)
{
    Slider slider { Slider::LinearHorizontal };

    const auto result = graph.bind (slider, Identifier (valueId), "0.25", ComponentProperty::Type::number);

    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (graph.getNumBindings(), 0);
    EXPECT_FLOAT_EQ (static_cast<float> (slider.getValue()), 0.25f);
}

TEST_F (BindingGraphTests, FailsOnAnExpressionThatDoesNotCompile)
{
    Slider slider { Slider::LinearHorizontal };

    const auto result = graph.bind (slider, Identifier (valueId), "1 +", ComponentProperty::Type::number);

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (graph.getNumBindings(), 0);
}

TEST_F (BindingGraphTests, UpdatesTheTargetWhenTheSourceChanges)
{
    Slider source { Slider::LinearHorizontal }, target { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.value * 2", ComponentProperty::Type::number).wasOk());
    EXPECT_EQ (graph.getNumBindings(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 0.0f);

    source.setValue (3.0);

    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 6.0f);
}

TEST_F (BindingGraphTests, PropagatesThroughAChainOfBindings)
{
    Slider first { Slider::LinearHorizontal }, second { Slider::LinearHorizontal }, third { Slider::LinearHorizontal };
    first.setRange (0.0, 10.0);
    second.setRange (0.0, 10.0);
    third.setRange (0.0, 10.0);
    nameIt ("first", first);
    nameIt ("second", second);

    ASSERT_TRUE (graph.bind (second, Identifier (valueId), "first.value + 1", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (graph.bind (third, Identifier (valueId), "second.value + 1", ComponentProperty::Type::number).wasOk());

    first.setValue (2.0);

    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 3.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (third.getValue()), 4.0f);
}

TEST_F (BindingGraphTests, ReadsThePropertiesOfTheTargetItself)
{
    Component parent;
    Slider child { Slider::LinearHorizontal };
    parent.setBounds (0, 0, 200, 100);
    parent.addAndMakeVisible (child);

    ASSERT_TRUE (graph.bind (child, Identifier (widthId), "parent.width / 2", ComponentProperty::Type::number).wasOk());
    EXPECT_FLOAT_EQ (child.getWidth(), 100.0f);

    parent.setBounds (0, 0, 300, 100);

    EXPECT_FLOAT_EQ (child.getWidth(), 150.0f);
}

TEST_F (BindingGraphTests, FollowsTheStyleStateOfAnotherComponent)
{
    Component source;
    source.setVisible (true);

    Slider target { Slider::LinearHorizontal };
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.hovered ? 4 : 1", ComponentProperty::Type::number).wasOk());
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 1.0f);

    ComponentTestHelper<Slider>::triggerMouseEnter (source, MouseEvent {});

    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 4.0f);
}

TEST_F (BindingGraphTests, BreaksTheBindingWhenTheTargetIsSetExplicitly)
{
    Slider source { Slider::LinearHorizontal }, target { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.value", ComponentProperty::Type::number).wasOk());

    target.setValue (7.0);
    EXPECT_EQ (graph.getNumBindings(), 0);

    source.setValue (2.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 7.0f);
}

TEST_F (BindingGraphTests, RemovesTheBindingsOfADeletedComponent)
{
    Slider target { Slider::LinearHorizontal };
    target.setRange (0.0, 10.0);

    auto source = std::make_unique<Slider> (Slider::LinearHorizontal);
    source->setRange (0.0, 10.0);
    nameIt ("source", *source);

    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.value", ComponentProperty::Type::number).wasOk());
    EXPECT_EQ (graph.getNumBindings(), 1);

    named.clear();
    source.reset();

    EXPECT_EQ (graph.getNumBindings(), 0);
}

TEST_F (BindingGraphTests, UnbindsLeavingTheValueInPlace)
{
    Slider source { Slider::LinearHorizontal }, target { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.value", ComponentProperty::Type::number).wasOk());
    EXPECT_TRUE (graph.hasBinding (target, Identifier (valueId)));
    source.setValue (5.0);

    EXPECT_TRUE (graph.unbind (target, Identifier (valueId)));
    EXPECT_FALSE (graph.unbind (target, Identifier (valueId)));
    EXPECT_FALSE (graph.hasBinding (target, Identifier (valueId)));

    source.setValue (1.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 5.0f);
}

TEST_F (BindingGraphTests, ReplacesAnExistingBindingOnTheSameProperty)
{
    Slider source { Slider::LinearHorizontal }, target { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.value", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "source.value + 1", ComponentProperty::Type::number).wasOk());

    EXPECT_EQ (graph.getNumBindings(), 1);

    source.setValue (2.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 3.0f);
}

TEST_F (BindingGraphTests, BreaksACycleAndReportsIt)
{
    Component first, second;
    first.setBounds (0, 0, 10, 10);
    second.setBounds (0, 0, 10, 10);
    nameIt ("first", first);
    nameIt ("second", second);

    ASSERT_TRUE (graph.bind (first, Identifier (widthId), "second.width + 1", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (graph.bind (second, Identifier (widthId), "first.width + 1", ComponentProperty::Type::number).wasOk());

    EXPECT_LT (graph.getNumBindings(), 2);
    EXPECT_FALSE (graph.getDiagnostics().isEmpty());
}

TEST_F (BindingGraphTests, LinkKeepsBothPropertiesEqual)
{
    Slider target { Slider::LinearHorizontal }, source { Slider::LinearHorizontal };
    target.setRange (0.0, 10.0);
    source.setRange (0.0, 10.0);
    source.setValue (3.0);

    graph.link (target, Identifier (valueId), source, Identifier (valueId));

    EXPECT_EQ (graph.getNumLinks(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 3.0f);

    target.setValue (7.0);
    EXPECT_FLOAT_EQ (static_cast<float> (source.getValue()), 7.0f);

    source.setValue (1.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 1.0f);
    EXPECT_EQ (graph.getNumLinks(), 1);
}

TEST_F (BindingGraphTests, LinksAStoreBackedPropertyThroughAChain)
{
    Component holder;
    Slider first { Slider::LinearHorizontal }, second { Slider::LinearHorizontal };
    first.setRange (0.0, 10.0);
    second.setRange (0.0, 10.0);

    graph.link (holder, Identifier (valueId), first, Identifier (valueId));
    graph.link (second, Identifier (valueId), holder, Identifier (valueId));

    first.setValue (4.0);

    EXPECT_FLOAT_EQ (static_cast<float> (holder.getProperty (Identifier (valueId))), 4.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 4.0f);

    second.setValue (2.0);

    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 2.0f);
}

TEST_F (BindingGraphTests, RemovesTheLinksOfADeletedComponent)
{
    Slider target { Slider::LinearHorizontal };
    auto source = std::make_unique<Slider> (Slider::LinearHorizontal);

    graph.link (target, Identifier (valueId), *source, Identifier (valueId));
    source.reset();

    EXPECT_EQ (graph.getNumLinks(), 0);
}

TEST_F (BindingGraphTests, UnlinksLeavingTheValuesInPlace)
{
    Slider target { Slider::LinearHorizontal }, source { Slider::LinearHorizontal };
    target.setRange (0.0, 10.0);
    source.setRange (0.0, 10.0);

    graph.link (target, Identifier (valueId), source, Identifier (valueId));
    source.setValue (5.0);

    EXPECT_TRUE (graph.unlink (source, Identifier (valueId)));
    EXPECT_FALSE (graph.unlink (source, Identifier (valueId)));

    source.setValue (1.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 5.0f);
}

TEST_F (BindingGraphTests, UpdatesPaletteDependentBindingsOnComponentAndThemePaletteChanges)
{
    auto oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::Ptr theme = new ApplicationTheme();
    ApplicationTheme::setGlobalTheme (theme);

    ThemePalette initialPalette;
    initialPalette.setColor (ThemePalette::Role::accent, Colors::red);
    theme->setPalette (initialPalette);

    Component parent ("parent");
    Slider slider (Slider::LinearHorizontal);
    parent.addChildComponent (slider);

    ASSERT_TRUE (graph.bind (slider, Identifier ("thumbColor"), "palette.accent", ComponentProperty::Type::color).wasOk());
    EXPECT_EQ (slider.getColor (Slider::Style::thumbColorId), Colors::red);

    ThemePalette scopedPalette;
    scopedPalette.setColor (ThemePalette::Role::accent, Colors::cyan);
    parent.setPalette (scopedPalette);
    EXPECT_EQ (slider.getColor (Slider::Style::thumbColorId), Colors::cyan);

    parent.setPalette (std::nullopt);
    EXPECT_EQ (slider.getColor (Slider::Style::thumbColorId), Colors::red);

    ThemePalette updatedThemePalette;
    updatedThemePalette.setColor (ThemePalette::Role::accent, Colors::lime);
    theme->setPalette (updatedThemePalette);
    graph.refreshPaletteBindings();
    EXPECT_EQ (slider.getColor (Slider::Style::thumbColorId), Colors::lime);

    graph.clear();
    ApplicationTheme::setGlobalTheme (oldTheme.get());
}

