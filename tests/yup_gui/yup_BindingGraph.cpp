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

    DrawingScope::ComponentResolver resolver()
    {
        return [this] (const String& id) -> Component*
        {
            const auto it = named.find (id);
            return it != named.end() ? it->second : nullptr;
        };
    }

    Result bind (Component& target, const char* property, const String& text, ComponentProperty::Type type)
    {
        return graph.bind (target, Identifier (property), text, type, resolver());
    }

    /** A model of two numbers, counting how many times each of them is read. */
    class CountingProvider final : public BindingProvider
    {
    public:
        std::vector<Property> getProperties() const override
        {
            return {
                { Identifier ("first"), ComponentProperty::Type::number },
                { Identifier ("second"), ComponentProperty::Type::number }
            };
        }

        var getValue (const Identifier& name) const override
        {
            ++reads[name.toString()];

            if (const auto* value = values.getVarPointer (name))
                return *value;

            return {};
        }

        void setValue (const Identifier& name, const var& value) override
        {
            if (values.set (name, value))
                notifyValueChanged (name);
        }

        NamedValueSet values { { Identifier ("first"), 1.0 }, { Identifier ("second"), 2.0 } };
        mutable std::map<String, int> reads;
    };

    /** Tells the changes of the `value` of the components it listens to, in the order they come. */
    class ValueChangeListener final : public ComponentListener
    {
    public:
        void componentPropertyChanged (Component& component, const Identifier& name) override
        {
            if (name != Identifier (valueId))
                return;

            changed.add (component.getComponentID());

            if (onValueChanged != nullptr)
                onValueChanged();
        }

        StringArray changed;
        std::function<void()> onValueChanged;
    };

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

    ASSERT_TRUE (bind (target, valueId, "source.value * 2", ComponentProperty::Type::number).wasOk());
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

    ASSERT_TRUE (bind (second, valueId, "first.value + 1", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (third, valueId, "second.value + 1", ComponentProperty::Type::number).wasOk());

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

    ASSERT_TRUE (bind (target, valueId, "source.hovered ? 4 : 1", ComponentProperty::Type::number).wasOk());
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

    ASSERT_TRUE (bind (target, valueId, "source.value", ComponentProperty::Type::number).wasOk());

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

    ASSERT_TRUE (bind (target, valueId, "source.value", ComponentProperty::Type::number).wasOk());
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

    ASSERT_TRUE (bind (target, valueId, "source.value", ComponentProperty::Type::number).wasOk());
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

    ASSERT_TRUE (bind (target, valueId, "source.value", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (target, valueId, "source.value + 1", ComponentProperty::Type::number).wasOk());

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

    ASSERT_TRUE (bind (first, widthId, "second.width + 1", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (second, widthId, "first.width + 1", ComponentProperty::Type::number).wasOk());

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

TEST_F (BindingGraphTests, FindsTheModelAPropertyIsLinkedToThroughOtherProperties)
{
    CountingProvider provider;
    BindingModel model (provider);

    Component holder;
    Slider dial { Slider::LinearHorizontal }, alone { Slider::LinearHorizontal };
    dial.setRange (0.0, 10.0);

    // As the alias of a component block: the dial to the holder, the holder to the model
    graph.link (dial, Identifier (valueId), holder, Identifier ("amount"));
    graph.link (holder, Identifier ("amount"), model, Identifier ("second"));

    const auto expectLinked = [&] (const Component& component, const char* property)
    {
        const auto found = graph.findLinkedModel (component, Identifier (property));

        ASSERT_TRUE (found.has_value()) << property;
        EXPECT_EQ (found->model, &model);
        EXPECT_EQ (found->property, Identifier ("second"));
    };

    // Two links away, one, and the property of the model itself
    expectLinked (dial, valueId);
    expectLinked (holder, "amount");
    expectLinked (model, "second");

    // Another property of a linked component is linked to nothing, as a component with no link
    EXPECT_FALSE (graph.findLinkedModel (dial, Identifier ("minimum")).has_value());
    EXPECT_FALSE (graph.findLinkedModel (alone, Identifier (valueId)).has_value());

    graph.unlink (holder, Identifier ("amount"));
    EXPECT_FALSE (graph.findLinkedModel (dial, Identifier (valueId)).has_value());
}

TEST_F (BindingGraphTests, ALinkToAnElementOfAListIsNotALinkToTheModelProperty)
{
    DataTree tree ("seq");
    tree.beginTransaction().setProperty ("steps", var (Array<var> { false, true }));

    DataTreeBindingProvider provider (tree, nullptr);
    BindingModel model (provider);

    ToggleButton step;
    graph.link (step, Identifier ("checked"), model, Identifier ("steps"), 1);

    ASSERT_TRUE (step.getToggleState());
    EXPECT_FALSE (graph.findLinkedModel (step, Identifier ("checked")).has_value());
}

TEST_F (BindingGraphTests, ALinkOnEnabledMovesWhatWasGivenNotTheStateOfAParent)
{
    Component parent, child, model;
    parent.addChildComponent (child);
    model.setProperty ("on", true);

    graph.link (child, Identifier ("enabled"), model, Identifier ("on"));

    parent.setEnabled (false);
    EXPECT_TRUE (static_cast<bool> (model.getProperty ("on")));

    model.setProperty ("on", false);
    parent.setEnabled (true);

    EXPECT_FALSE (child.isEnabled());
    EXPECT_FALSE (static_cast<bool> (model.getProperty ("on")));
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

TEST_F (BindingGraphTests, LinkToAnElementOfAListKeepsItEqual)
{
    Component model;
    ToggleButton second, fourth;

    model.setProperty ("steps", var (Array<var> { false, true, false, false }));

    graph.link (second, Identifier ("checked"), model, Identifier ("steps"), 1);
    graph.link (fourth, Identifier ("checked"), model, Identifier ("steps"), 3);

    // Each takes its element first
    EXPECT_TRUE (second.getToggleState());
    EXPECT_FALSE (fourth.getToggleState());

    // A button writes its element, leaving the others as they are
    fourth.setToggleState (true);
    EXPECT_EQ (JSON::toString (model.getProperty ("steps"), true), String ("[false, true, false, true]"));

    second.setToggleState (false);
    EXPECT_EQ (JSON::toString (model.getProperty ("steps"), true), String ("[false, false, false, true]"));

    // A new list reaches the buttons whose element changed
    model.setProperty ("steps", var (Array<var> { true, true, true, false }));
    EXPECT_TRUE (second.getToggleState());
    EXPECT_FALSE (fourth.getToggleState());
    EXPECT_EQ (JSON::toString (model.getProperty ("steps"), true), String ("[true, true, true, false]"));
}

TEST_F (BindingGraphTests, LinkToAnElementOutsideTheListDoesNothingUntilItIsThere)
{
    Component model;
    ToggleButton button;

    model.setProperty ("steps", var (Array<var> { true }));
    graph.link (button, Identifier ("checked"), model, Identifier ("steps"), 2);
    EXPECT_FALSE (button.getToggleState());

    button.setToggleState (true);
    EXPECT_EQ (JSON::toString (model.getProperty ("steps"), true), String ("[true]"));

    model.setProperty ("steps", var (Array<var> { true, true, false }));
    EXPECT_FALSE (button.getToggleState());
    EXPECT_EQ (JSON::toString (model.getProperty ("steps"), true), String ("[true, true, false]"));
}

TEST_F (BindingGraphTests, AConstantReplacesTheBindingOfItsProperty)
{
    Slider source { Slider::LinearHorizontal }, target { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (bind (target, valueId, "source.value", ComponentProperty::Type::number).wasOk());
    EXPECT_EQ (graph.getNumBindings(), 1);

    bool isLive = true;
    ASSERT_TRUE (graph.bind (target, Identifier (valueId), "4", ComponentProperty::Type::number, resolver(), &isLive).wasOk());

    EXPECT_FALSE (isLive);
    EXPECT_EQ (graph.getNumBindings(), 0);
    EXPECT_FALSE (graph.hasBinding (target, Identifier (valueId)));
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 4.0f);

    source.setValue (2.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 4.0f);
}

TEST_F (BindingGraphTests, UnbindingWhatHasNoBindingLeavesTheOthersInPlace)
{
    Slider source { Slider::LinearHorizontal }, target { Slider::LinearHorizontal }, other { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    target.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (bind (target, valueId, "source.value", ComponentProperty::Type::number).wasOk());

    // Another component, another property of the bound one, and what the binding reads rather than writes
    EXPECT_FALSE (graph.unbind (other, Identifier (valueId)));
    EXPECT_FALSE (graph.unbind (target, Identifier (widthId)));
    EXPECT_FALSE (graph.unbind (source, Identifier (valueId)));

    EXPECT_EQ (graph.getNumBindings(), 1);
    EXPECT_TRUE (graph.hasBinding (target, Identifier (valueId)));
    EXPECT_FALSE (graph.hasBinding (source, Identifier (valueId)));

    source.setValue (3.0);
    EXPECT_FLOAT_EQ (static_cast<float> (target.getValue()), 3.0f);
}

TEST_F (BindingGraphTests, UpdatesTheBindingsOfOneSourceInTheOrderTheyWereMade)
{
    ValueChangeListener listener;

    Slider source { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    nameIt ("source", source);

    std::vector<std::unique_ptr<Slider>> targets;

    for (int index = 0; index < 6; ++index)
    {
        auto& target = *targets.emplace_back (std::make_unique<Slider> (Slider::LinearHorizontal));
        target.setRange (0.0, 10.0);
        target.setComponentID (String (index));
        target.addComponentListener (&listener);
    }

    // Made in an order that is neither the one of the components nor the one of their addresses
    for (const auto index : { 3, 0, 5, 1, 4, 2 })
        ASSERT_TRUE (bind (*targets[static_cast<size_t> (index)], valueId, "source.value", ComponentProperty::Type::number).wasOk());

    source.setValue (2.0);
    EXPECT_EQ (listener.changed.joinIntoString (" "), "3 0 5 1 4 2");

    // A binding made again comes last
    ASSERT_TRUE (bind (*targets[0], valueId, "source.value + 1", ComponentProperty::Type::number).wasOk());
    EXPECT_EQ (graph.getNumBindings(), 6);

    listener.changed.clear();
    source.setValue (4.0);
    EXPECT_EQ (listener.changed.joinIntoString (" "), "3 5 1 4 2 0");
    EXPECT_FLOAT_EQ (static_cast<float> (targets[0]->getValue()), 5.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (targets[2]->getValue()), 4.0f);
}

TEST_F (BindingGraphTests, AChangeOfOnePropertyLeavesTheBindingsReadingAnotherAlone)
{
    CountingProvider provider;
    BindingModel model (provider);
    nameIt ("model", model);

    Slider first { Slider::LinearHorizontal }, second { Slider::LinearHorizontal };
    first.setRange (0.0, 10.0);
    second.setRange (0.0, 10.0);

    ASSERT_TRUE (bind (first, valueId, "model.first", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (second, valueId, "model.second", ComponentProperty::Type::number).wasOk());
    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 1.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 2.0f);

    provider.reads.clear();
    provider.setValue (Identifier ("first"), 5.0);

    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 5.0f);
    EXPECT_GT (provider.reads["first"], 0);
    EXPECT_EQ (provider.reads["second"], 0);

    provider.reads.clear();
    provider.setValue (Identifier ("second"), 7.0);

    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 7.0f);
    EXPECT_EQ (provider.reads["first"], 0);
    EXPECT_GT (provider.reads["second"], 0);
}

TEST_F (BindingGraphTests, RemovesOnlyTheBindingsOfADeletedTarget)
{
    Slider source { Slider::LinearHorizontal }, kept { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    kept.setRange (0.0, 10.0);
    nameIt ("source", source);

    auto target = std::make_unique<Slider> (Slider::LinearHorizontal);
    target->setRange (0.0, 10.0);

    ASSERT_TRUE (bind (*target, valueId, "source.value", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (kept, valueId, "source.value + 1", ComponentProperty::Type::number).wasOk());
    EXPECT_EQ (graph.getNumBindings(), 2);

    target.reset();

    EXPECT_EQ (graph.getNumBindings(), 1);
    EXPECT_TRUE (graph.hasBinding (kept, Identifier (valueId)));

    source.setValue (3.0);
    EXPECT_FLOAT_EQ (static_cast<float> (kept.getValue()), 4.0f);
}

TEST_F (BindingGraphTests, RemovesTheBindingsAndTheLinksOfADeletedModel)
{
    CountingProvider provider;
    auto model = std::make_unique<BindingModel> (provider);
    nameIt ("model", *model);

    Slider bound { Slider::LinearHorizontal }, linked { Slider::LinearHorizontal };
    bound.setRange (0.0, 10.0);
    linked.setRange (0.0, 10.0);

    ASSERT_TRUE (bind (bound, valueId, "model.first * 2", ComponentProperty::Type::number).wasOk());
    graph.link (linked, Identifier (valueId), *model, Identifier ("second"));

    EXPECT_EQ (graph.getNumBindings(), 1);
    EXPECT_EQ (graph.getNumLinks(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (bound.getValue()), 2.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (linked.getValue()), 2.0f);

    named.clear();
    model.reset();

    EXPECT_EQ (graph.getNumBindings(), 0);
    EXPECT_EQ (graph.getNumLinks(), 0);
    EXPECT_FALSE (graph.hasBinding (bound, Identifier (valueId)));

    // Nothing is left that follows the model, or writes to it
    provider.setValue (Identifier ("first"), 4.0);
    linked.setValue (7.0);

    EXPECT_FLOAT_EQ (static_cast<float> (bound.getValue()), 2.0f);
    EXPECT_DOUBLE_EQ (static_cast<double> (provider.getValue (Identifier ("second"))), 2.0);

    // The components that are left can be bound again
    Slider source { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (bind (bound, valueId, "source.value + 1", ComponentProperty::Type::number).wasOk());
    source.setValue (5.0);
    EXPECT_FLOAT_EQ (static_cast<float> (bound.getValue()), 6.0f);
}

TEST_F (BindingGraphTests, ABindingWhoseUpdateUnbindsAnotherLeavesItAsItWas)
{
    ValueChangeListener listener;

    Slider source { Slider::LinearHorizontal }, first { Slider::LinearHorizontal }, second { Slider::LinearHorizontal }, third { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    first.setRange (0.0, 10.0);
    second.setRange (0.0, 10.0);
    third.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (bind (first, valueId, "source.value", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (second, valueId, "source.value", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (third, valueId, "source.value", ComponentProperty::Type::number).wasOk());

    listener.onValueChanged = [&]
    {
        graph.unbind (second, Identifier (valueId));
    };

    first.addComponentListener (&listener);
    source.setValue (3.0);

    // The second one was waiting for its turn when the first one removed it
    EXPECT_EQ (graph.getNumBindings(), 2);
    EXPECT_FALSE (graph.hasBinding (second, Identifier (valueId)));
    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 3.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 0.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (third.getValue()), 3.0f);
}

TEST_F (BindingGraphTests, ABindingWhoseUpdateBindsAnotherAgainEvaluatesTheNewOne)
{
    ValueChangeListener listener;

    Slider source { Slider::LinearHorizontal }, first { Slider::LinearHorizontal }, second { Slider::LinearHorizontal };
    source.setRange (0.0, 10.0);
    first.setRange (0.0, 10.0);
    second.setRange (0.0, 10.0);
    nameIt ("source", source);

    ASSERT_TRUE (bind (first, valueId, "source.value", ComponentProperty::Type::number).wasOk());
    ASSERT_TRUE (bind (second, valueId, "source.value", ComponentProperty::Type::number).wasOk());

    listener.onValueChanged = [&]
    {
        EXPECT_TRUE (bind (second, valueId, "source.value + 1", ComponentProperty::Type::number).wasOk());
    };

    first.addComponentListener (&listener);
    source.setValue (3.0);

    EXPECT_EQ (graph.getNumBindings(), 2);
    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 3.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 4.0f);

    listener.onValueChanged = nullptr;
    source.setValue (5.0);

    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 5.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 6.0f);
}

TEST_F (BindingGraphTests, UnlinkingAPropertyLeavesTheLinksOfTheOthers)
{
    Component model;
    Slider first { Slider::LinearHorizontal }, second { Slider::LinearHorizontal }, third { Slider::LinearHorizontal };
    first.setRange (0.0, 10.0);
    second.setRange (0.0, 10.0);
    third.setRange (0.0, 10.0);

    model.setProperty ("cutoff", 1.0);
    model.setProperty ("gain", 2.0);

    graph.link (first, Identifier (valueId), model, Identifier ("cutoff"));
    graph.link (second, Identifier (valueId), model, Identifier ("gain"));
    graph.link (third, Identifier (valueId), model, Identifier ("cutoff"));
    EXPECT_EQ (graph.getNumLinks(), 3);

    // Both links of the property go, whichever side it is on
    EXPECT_TRUE (graph.unlink (model, Identifier ("cutoff")));
    EXPECT_EQ (graph.getNumLinks(), 1);

    model.setProperty ("cutoff", 5.0);
    model.setProperty ("gain", 6.0);

    EXPECT_FLOAT_EQ (static_cast<float> (first.getValue()), 1.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (second.getValue()), 6.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (third.getValue()), 1.0f);

    second.setValue (8.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (model.getProperty ("gain")), 8.0);
}

TEST_F (BindingGraphTests, ALiteralStoresWhatAnExpressionOfTheSameValueStores)
{
    Component literal, computed;

    const auto expectSame = [&] (const String& literalText, const String& expressionText)
    {
        bool isLive = true;
        const auto bound = graph.bind (literal, Identifier ("custom"), literalText, std::nullopt, nullptr, &isLive);

        EXPECT_TRUE (bound.wasOk()) << literalText << ": " << bound.getErrorMessage();
        EXPECT_FALSE (isLive) << literalText;
        EXPECT_TRUE (graph.bind (computed, Identifier ("custom"), expressionText, std::nullopt).wasOk()) << expressionText;

        EXPECT_FALSE (literal.getProperty ("custom").isVoid()) << literalText;
        EXPECT_TRUE (literal.getProperty ("custom").equalsWithSameType (computed.getProperty ("custom")))
            << literalText << " gives " << literal.getProperty ("custom").toString()
            << ", " << expressionText << " gives " << computed.getProperty ("custom").toString();
    };

    expectSame ("200", "100 + 100");
    expectSame ("-7", "3 - 10");
    expectSame ("0.5", "1 / 2.0");
    expectSame ("true", "! false");
    expectSame ("false", "! true");
    expectSame ("\"Reset\"", "\"Re\" + \"set\"");

    EXPECT_EQ (graph.getNumBindings(), 0);
    EXPECT_FALSE (graph.hasBinding (literal, Identifier ("custom")));
}
