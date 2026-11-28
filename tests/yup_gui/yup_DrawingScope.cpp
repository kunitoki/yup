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

#include <memory>

using namespace yup;

class DrawingScopeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        addGraphicsExpressionTypes (context);
        DrawingScope::addExpressionArrayTypes (context);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    var evaluate (DrawingScope& scope, const String& text)
    {
        scope.install();

        auto compiled = CompiledExpression::compile (text, context);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        if (compiled.failed())
            return {};

        const auto& program = compiled.getReference();
        const int outputIndex = program.findOutput ("out");
        EXPECT_GE (outputIndex, 0);

        ExpressionState state (program);
        auto inputs = scope.bindInputs (program);
        scope.updateInputs (inputs, state);
        state.evaluate();

        return state.getVar (outputIndex);
    }

    template <class T>
    T evaluateAs (DrawingScope& scope, const String& text)
    {
        scope.install();

        auto compiled = CompiledExpression::compile (text, context);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        if (compiled.failed())
            return {};

        const auto& program = compiled.getReference();
        const auto output = program.getOutput<T> ("out");
        EXPECT_TRUE (output.wasOk()) << output.getErrorMessage();
        if (output.failed())
            return {};

        ExpressionState state (program);
        auto inputs = scope.bindInputs (program);
        scope.updateInputs (inputs, state);
        state.evaluate();

        return state.get (output.getReference());
    }

    /** A component keeping an array in a block of its own, and another one it gives as a var each time it is asked. */
    class ScopeWidget : public Component
    {
    public:
        const ComponentTypeInfo& getDefaultTypeInfo() const override
        {
            static const ComponentTypeInfo info = ComponentTypeBuilder<ScopeWidget> ("ScopeWidget")
                .series ("levels", &ScopeWidget::getLevels)
                .series ("spots", &ScopeWidget::getSpots)
                .readOnly ("steps", &ScopeWidget::getSteps)
                .build();
            return info;
        }

        Span<const float> getLevels() const noexcept { return levels; }
        Span<const Point<float>> getSpots() const noexcept { return spots; }

        StringArray getSteps() const
        {
            ++numStepsReads;
            return steps;
        }

        std::vector<float> levels { 1.0f, 2.0f, 4.0f };
        std::vector<Point<float>> spots { { 1.0f, 2.0f }, { 3.0f, 4.0f } };
        StringArray steps { "a", "bb" };
        mutable int numStepsReads = 0;
    };

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    ExpressionContext context;
};

TEST_F (DrawingScopeTest, ResolvesNumberPropertyByBareName)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (3.0, dontSendNotification);

    DrawingScope scope (slider, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = value * 2 + 1")), 7.0);
}

TEST_F (DrawingScopeTest, ResolvesNumberPropertyBySelfPrefix)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (4.0, dontSendNotification);

    DrawingScope scope (slider, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = self.value")), 4.0);
}

TEST_F (DrawingScopeTest, ResolvesBooleanProperty)
{
    Slider slider (Slider::LinearHorizontal);

    DrawingScope scope (slider, context);

    // A slider that is not being dragged reports dragging == false.
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = dragging ? 1 : 0")), 0.0);
}

TEST_F (DrawingScopeTest, ResolvesColorPropertyChannels)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setColor (Slider::Style::thumbColorId, Color (128, 64, 32)); // r, g, b (opaque)

    DrawingScope scope (slider, context);

    EXPECT_NEAR (static_cast<double> (evaluate (scope, "out = thumbColor.g")), 64.0 / 255.0, 1.0e-4);
}

TEST_F (DrawingScopeTest, ResolvesParentProperty)
{
    Slider parent (Slider::LinearHorizontal);
    parent.setRange (0.0, 20.0, 0.0);
    parent.setValue (8.0, dontSendNotification);

    Slider child (Slider::LinearHorizontal);
    parent.addChildComponent (child);

    DrawingScope scope (child, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = parent.value")), 8.0);
}

TEST_F (DrawingScopeTest, UnknownSymbolFailsToCompile)
{
    Slider slider (Slider::LinearHorizontal);

    DrawingScope scope (slider, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("out = notAProperty", context);
    EXPECT_TRUE (compiled.failed());
}

TEST_F (DrawingScopeTest, ResolvesRolesOfTheEffectivePalette)
{
    ThemePalette themePalette;
    themePalette.setColor (ThemePalette::Role::accent, Color (0xff336699));
    theme->setPalette (themePalette);

    Component parent;
    Slider slider (Slider::LinearHorizontal);
    parent.addChildComponent (slider);

    DrawingScope scope (slider, context);

    EXPECT_NEAR (static_cast<double> (evaluate (scope, "out = palette.accent.b")), 0x99 / 255.0, 1.0e-4);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = palette.dark ? 1 : 0")), 1.0);

    ThemePalette parentPalette (std::vector<Color> { Color (0xff101010), Color (0xffeeeeee) }, ThemePalette::Mode::light);
    parentPalette.setColor (ThemePalette::Role::accent, Color (0xff996633));
    parent.setPalette (parentPalette);

    EXPECT_NEAR (static_cast<double> (evaluate (scope, "out = palette.accent.b")), 0x33 / 255.0, 1.0e-4);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = palette.dark ? 1 : 0")), 0.0);
}

TEST_F (DrawingScopeTest, ResolvesCustomColorsOfTheEffectivePalette)
{
    Component parent;
    Slider slider (Slider::LinearHorizontal);
    parent.addChildComponent (slider);

    ThemePalette parentPalette;
    parentPalette.setCustomColor ("meterHot", Color (0xff336699));
    parent.setPalette (parentPalette);

    DrawingScope scope (slider, context);

    EXPECT_NEAR (static_cast<double> (evaluate (scope, "out = palette.meterHot.b")), 0x99 / 255.0, 1.0e-4);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = palette.missing.a")), 0.0);
}

TEST_F (DrawingScopeTest, InvalidPaletteNameFailsToCompile)
{
    Slider slider (Slider::LinearHorizontal);

    DrawingScope scope (slider, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("out = palette.2tone", context);
    EXPECT_TRUE (compiled.failed());
}

TEST_F (DrawingScopeTest, ResolvesPointAndRectangleProperties)
{
    const ComponentTypeInfo extended (Identifier ("Knob"),
                                      &Slider::getClassTypeInfo(),
                                      { ComponentProperty { .name = Identifier ("origin"), .type = ComponentProperty::Type::point },
                                        ComponentProperty { .name = Identifier ("area"), .type = ComponentProperty::Type::rectangle } });

    Slider slider (Slider::LinearHorizontal);
    slider.setTypeInfo (&extended);
    slider.setProperty (Identifier ("origin"), VariantConverter<Point<float>>::toVar ({ 3.0f, 4.0f }));
    slider.setProperty (Identifier ("area"), VariantConverter<Rectangle<float>>::toVar ({ 1.0f, 2.0f, 10.0f, 20.0f }));

    DrawingScope scope (slider, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = origin.y")), 4.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = area.width + area.x")), 11.0);
}

TEST_F (DrawingScopeTest, ResolvesArrayProperties)
{
    const ComponentTypeInfo extended (Identifier ("Knob"),
                                      &Slider::getClassTypeInfo(),
                                      { ComponentProperty { .name = Identifier ("lanes"), .type = ComponentProperty::Type::array, .elementType = ComponentProperty::Type::number },
                                        ComponentProperty { .name = Identifier ("names"), .type = ComponentProperty::Type::array, .elementType = ComponentProperty::Type::string },
                                        ComponentProperty { .name = Identifier ("spots"), .type = ComponentProperty::Type::array, .elementType = ComponentProperty::Type::point } });

    Slider slider (Slider::LinearHorizontal);
    slider.setTypeInfo (&extended);

    DrawingScope scope (slider, context);

    // An array the component doesn't hold a value for is empty
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = len (lanes)")), 0.0);

    slider.setProperty (Identifier ("lanes"), var (Array<var> { 0.5, 1.5, 2.5 }));
    slider.setProperty (Identifier ("names"), var (Array<var> { "a", "bb" }));
    slider.setProperty (Identifier ("spots"), var (Array<var> { VariantConverter<Point<float>>::toVar ({ 1.0f, 2.0f }),
                                                               VariantConverter<Point<float>>::toVar ({ 3.0f, 4.0f }) }));

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = len (lanes)")), 3.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = lanes[1] + lanes[2]")), 4.0);
    EXPECT_EQ (evaluate (scope, "out = names[1]").toString(), "bb");
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = spots[1].x")), 3.0);

    // The values follow the component, and a value that isn't an array leaves it empty
    slider.setProperty (Identifier ("lanes"), var (Array<var> { 1.0 }));
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = len (lanes)")), 1.0);

    slider.setProperty (Identifier ("lanes"), 7.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = len (lanes)")), 0.0);
}

TEST_F (DrawingScopeTest, ReadsSeriesWhereTheComponentKeepsThem)
{
    ScopeWidget widget;
    DrawingScope scope (widget, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("count = len (levels); last = levels[2]; peak = max (levels, 0, 3); x = spots[1].x", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    ExpressionState state (program);
    auto inputs = scope.bindInputs (program);

    scope.updateInputs (inputs, state);
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("count"))), 3);
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("last"))), 4.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("peak"))), 4.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("x"))), 3.0f);

    // The state views the memory of the component: what changes there is read without another update
    widget.levels[2] = 9.0f;
    state.evaluate();
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("last"))), 9.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("peak"))), 9.0f);

    // A series that moved or changed its length is followed by the next update
    widget.levels = { 5.0f };
    scope.updateInputs (inputs, state);
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("count"))), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("last"))), 5.0f);

    widget.levels.clear();
    scope.updateInputs (inputs, state);
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("count"))), 0);
    EXPECT_FLOAT_EQ (static_cast<float> (state.getVar (program.findOutput ("last"))), 0.0f);
}

TEST_F (DrawingScopeTest, ConvertsAnArrayGivenAsAVarOnceForARoundOfUpdates)
{
    ScopeWidget widget;
    DrawingScope scope (widget, context);
    scope.install();

    auto first = CompiledExpression::compile ("out = len (steps)", context);
    auto second = CompiledExpression::compile ("out = steps[1]", context);
    ASSERT_TRUE (first.wasOk()) << first.getErrorMessage();
    ASSERT_TRUE (second.wasOk()) << second.getErrorMessage();

    ExpressionState firstState (first.getReference());
    ExpressionState secondState (second.getReference());
    auto firstInputs = scope.bindInputs (first.getReference());
    auto secondInputs = scope.bindInputs (second.getReference());

    const auto update = [&]
    {
        scope.updateInputs (firstInputs, firstState);
        scope.updateInputs (secondInputs, secondState);
        firstState.evaluate();
        secondState.evaluate();
    };

    // Without rounds the component is asked by every update
    update();
    EXPECT_EQ (widget.numStepsReads, 2);
    EXPECT_EQ (static_cast<int> (firstState.getVar (0)), 2);
    EXPECT_EQ (secondState.getVar (0).toString(), String ("bb"));

    // In a round it is asked once, whichever programs read the array and however often
    widget.numStepsReads = 0;
    scope.beginUpdates();
    update();
    update();
    EXPECT_EQ (widget.numStepsReads, 1);
    EXPECT_EQ (static_cast<int> (firstState.getVar (0)), 2);
    EXPECT_EQ (secondState.getVar (0).toString(), String ("bb"));

    // The next round reads what the component holds by then
    widget.steps = { "c", "dd", "eee" };
    scope.beginUpdates();
    update();
    EXPECT_EQ (widget.numStepsReads, 2);
    EXPECT_EQ (static_cast<int> (firstState.getVar (0)), 3);
    EXPECT_EQ (secondState.getVar (0).toString(), String ("dd"));
}

TEST_F (DrawingScopeTest, ASeriesOfAnotherElementTypeLeavesNothingToRead)
{
    const auto steps = context.addArray<float> ("steps");

    auto compiled = CompiledExpression::compile ("out = len (steps)", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    ExpressionState state (program);

    const float numbers[] = { 1.0f, 2.0f, 3.0f };
    DrawingScope::bindArrayView (state, steps.index, ComponentProperty::Type::number, { numbers, 3, 0 });
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("out"))), 3);

    // Points can't feed numbers, and the numbers that were viewed are let go
    const Point<float> points[] = { { 1.0f, 2.0f } };
    DrawingScope::bindArrayView (state, steps.index, ComponentProperty::Type::point, { points, 1, 0 });
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("out"))), 0);

    // A view of nothing is an empty array
    DrawingScope::bindArrayView (state, steps.index, ComponentProperty::Type::number, {});
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("out"))), 0);
}

TEST_F (DrawingScopeTest, DestroyedComponentFeedsNothing)
{
    auto slider = std::make_unique<Slider> (Slider::LinearHorizontal);
    slider->setRange (0.0, 10.0, 0.0);
    slider->setValue (5.0, dontSendNotification);

    DrawingScope scope (*slider, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("out = value + 1", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    const int outputIndex = program.findOutput ("out");

    ExpressionState state (program);
    auto inputs = scope.bindInputs (program);

    slider.reset();

    scope.updateInputs (inputs, state); // must not touch the destroyed component
    state.evaluate();

    // value defaults to 0 when the component is gone, so the result is just the constant.
    EXPECT_DOUBLE_EQ (static_cast<double> (state.getVar (outputIndex)), 1.0);
}

TEST_F (DrawingScopeTest, ADestroyedComponentLeavesNoArrayToRead)
{
    auto widget = std::make_unique<ScopeWidget>();
    DrawingScope scope (*widget, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("count = len (levels); numSteps = len (steps)", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    ExpressionState state (program);
    auto inputs = scope.bindInputs (program);

    scope.beginUpdates();
    scope.updateInputs (inputs, state);
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("count"))), 3);
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("numSteps"))), 2);

    // The series was in the memory of the component and the other array in what the round kept: both are let go
    widget.reset();
    scope.beginUpdates();
    scope.updateInputs (inputs, state);
    state.evaluate();
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("count"))), 0);
    EXPECT_EQ (static_cast<int> (state.getVar (program.findOutput ("numSteps"))), 0);
}

TEST_F (DrawingScopeTest, ResolvesBindingProviderModelsDirectly)
{
    DataTree params ("params");
    {
        auto tx = params.beginTransaction();
        tx.setProperty ("cutoff", 0.25);
        tx.setProperty ("bypass", true);
        tx.setProperty ("label", "Warm");
        tx.setProperty ("curve", var (Array<var> { 2.0, 6.0 }));
    }

    DataTreeBindingProvider provider (params);
    BindingModel model (provider);
    Slider slider (Slider::LinearHorizontal);

    DrawingScope scope (slider, context);
    scope.setComponentResolver ([&] (const String& name) -> Component*
    {
        return name == "params" ? &model : nullptr;
    });

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = params.cutoff * 4")), 1.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = params.bypass ? params.curve[1] : 0")), 6.0);
    EXPECT_EQ (evaluate (scope, "out = params.label").toString(), "Warm");

    // Properties added to a dynamic DataTree provider after the model was created refresh automatically
    params.beginTransaction().setProperty ("resonance", 0.75);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = params.resonance * 2")), 1.5);

    auto compiled = CompiledExpression::compile ("out = params.cutoff + params.curve[0]", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
    const auto deps = scope.getDependencies (compiled.getReference());
    ASSERT_EQ (deps.size(), 2u);
    EXPECT_EQ (deps[0].component, &model);
    EXPECT_EQ (deps[1].component, &model);
}

TEST_F (DrawingScopeTest, SkipsContextDeclaredInputsOverSameNamedProperties)
{
    ExpressionContext drawingContext;
    addGraphicsExpressionTypes (drawingContext);
    const auto contextBounds = drawingContext.addInput<Rectangle<float>> ("bounds");

    const ComponentTypeInfo withCustomBounds (Identifier ("WithBounds"),
                                              &Slider::getClassTypeInfo(),
                                              { ComponentProperty { .name = Identifier ("bounds"),
                                                                    .type = ComponentProperty::Type::rectangle } });

    Slider slider (Slider::LinearHorizontal);
    slider.setTypeInfo (&withCustomBounds);
    slider.setProperty (Identifier ("bounds"), VariantConverter<Rectangle<float>>::toVar ({ 0.0f, 0.0f, 999.0f, 999.0f }));

    DrawingScope scope (slider, drawingContext);
    scope.install();

    auto compiled = CompiledExpression::compile ("out = bounds.width", drawingContext);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    EXPECT_TRUE (scope.getDependencies (program).empty());

    ExpressionState state (program);
    auto inputs = scope.bindInputs (program);
    EXPECT_TRUE (inputs.isEmpty());

    state.set (contextBounds, Rectangle<float> (0.0f, 0.0f, 42.0f, 10.0f));
    scope.updateInputs (inputs, state);
    state.evaluate();

    EXPECT_DOUBLE_EQ (static_cast<double> (state.getVar (program.findOutput ("out"))), 42.0);
}

TEST_F (DrawingScopeTest, CachedProgramFollowsReparentingTypeChangesAndReplacedTargets)
{
    ComponentTypeInfo rootTokens (Identifier ("RootTokens"),
                                        &Slider::getClassTypeInfo(),
                                        { ComponentProperty { .name = Identifier ("token"),
                                                              .type = ComponentProperty::Type::number,
                                                              .defaultValue = 0.0,
                                                              .isInherited = true } });

    Slider parentA (Slider::LinearHorizontal);
    parentA.setTypeInfo (&rootTokens);
    parentA.setRange (0.0, 100.0, 0.0);
    parentA.setValue (10.0, dontSendNotification);
    parentA.setProperty (Identifier ("token"), 5.0);

    Slider parentB (Slider::LinearHorizontal);
    parentB.setTypeInfo (&rootTokens);
    parentB.setRange (0.0, 100.0, 0.0);
    parentB.setValue (20.0, dontSendNotification);
    parentB.setProperty (Identifier ("token"), 15.0);

    auto external = std::make_unique<Slider> (Slider::LinearHorizontal);
    external->setRange (0.0, 100.0, 0.0);
    external->setValue (3.0, dontSendNotification);

    Slider child (Slider::LinearHorizontal);
    child.setRange (0.0, 100.0, 0.0);
    child.setValue (2.0, dontSendNotification);
    parentA.addChildComponent (child);

    DrawingScope scope (child, context);
    scope.setComponentResolver ([&] (const String& id) -> Component*
    {
        return id == "knob" ? external.get() : nullptr;
    });
    scope.install();

    auto compiled = CompiledExpression::compile ("out = value + self.value + parent.value + token + knob.value", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    const int outputIndex = program.findOutput ("out");
    ExpressionState state (program);
    auto inputs = scope.bindInputs (program);

    const auto evalOnce = [&]
    {
        scope.updateInputs (inputs, state);
        state.evaluate();
        return static_cast<double> (state.getVar (outputIndex));
    };

    EXPECT_DOUBLE_EQ (evalOnce(), 2.0 + 2.0 + 10.0 + 5.0 + 3.0);

    // Reparenting updates both parent.value and inherited bare token lookups.
    parentB.addChildComponent (child);
    child.setValue (4.0, dontSendNotification);
    EXPECT_DOUBLE_EQ (evalOnce(), 4.0 + 4.0 + 20.0 + 15.0 + 3.0);

    // Recreating the resolved target refreshes the weak reference automatically.
    external = std::make_unique<Slider> (Slider::LinearHorizontal);
    external->setRange (0.0, 100.0, 0.0);
    external->setValue (9.0, dontSendNotification);
    EXPECT_DOUBLE_EQ (evalOnce(), 4.0 + 4.0 + 20.0 + 15.0 + 9.0);

    // Updating the component resolver invalidates cached bindings immediately.
    Slider replacement (Slider::LinearHorizontal);
    replacement.setRange (0.0, 100.0, 0.0);
    replacement.setValue (30.0, dontSendNotification);
    scope.setComponentResolver ([&] (const String& id) -> Component*
    {
        return id == "knob" ? &replacement : nullptr;
    });
    EXPECT_DOUBLE_EQ (evalOnce(), 4.0 + 4.0 + 20.0 + 15.0 + 30.0);

    // Replacing a ComponentTypeInfo refreshes descriptors without dangling pointers.
    const ComponentTypeInfo rebuiltTokens (Identifier ("RootTokensRebuilt"),
                                           &Slider::getClassTypeInfo(),
                                           { ComponentProperty { .name = Identifier ("token"),
                                                                 .type = ComponentProperty::Type::number,
                                                                 .defaultValue = 99.0,
                                                                 .isInherited = true } });
    parentB.setTypeInfo (&rebuiltTokens);
    parentB.removeProperty (Identifier ("token"));
    EXPECT_DOUBLE_EQ (evalOnce(), 4.0 + 4.0 + 20.0 + 99.0 + 30.0);
}

TEST_F (DrawingScopeTest, BridgesFontAndFontArrayPropertiesAndThemeScope)
{
    auto oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::Ptr customTheme = new ApplicationTheme();
    customTheme->setDefaultFont (Font().withHeight (15.0f));
    customTheme->setDefaultIconFont (Font().withHeight (19.0f));
    customTheme->setDefaultMonospaceFont (Font().withHeight (13.0f));

    ThemePalette darkPalette;
    darkPalette.setColor (ThemePalette::Role::accent, Color (0xff336699));
    customTheme->setPalette (darkPalette);
    customTheme->setColor (Identifier ("brandHighlight"), Color (0xffff8844));
    customTheme->setProperty (Identifier ("gridSnap"), 8.0);
    ApplicationTheme::setGlobalTheme (customTheme);

    const ComponentTypeInfo withFonts (Identifier ("WithFonts"),
                                       &Component::getClassTypeInfo(),
                                       { ComponentProperty { .name = Identifier ("headlineFont"),
                                                             .type = ComponentProperty::Type::font,
                                                             .defaultValue = VariantConverter<Font>::toVar (Font().withHeight (22.0f)) },
                                         ComponentProperty { .name = Identifier ("fontStack"),
                                                             .type = ComponentProperty::Type::array,
                                                             .elementType = ComponentProperty::Type::font } });

    Component panel ("panel");
    panel.setTypeInfo (&withFonts);
    panel.setProperty (Identifier ("fontStack"), var (Array<var> {
                                                     VariantConverter<Font>::toVar (Font().withHeight (10.0f)),
                                                     VariantConverter<Font>::toVar (Font().withHeight (28.0f)) }));

    DrawingScope scope (panel, context);

    // Font properties and font[] arrays bridge cleanly through VariantConverter<Font>.
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = headlineFont.height")), 22.0f);
    const auto scaledFont = evaluateAs<Font> (scope, "out = headlineFont.withHeight (30)");
    EXPECT_FLOAT_EQ (scaledFont.getHeight(), 30.0f);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = len (fontStack)")), 2.0);
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = fontStack[1].height")), 28.0f);

    // theme.* resolves default fonts, theme palette mode/roles, and registered theme colors/properties.
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = theme.font.height")), 15.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = theme.defaultFont.height")), 15.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = theme.iconFont.height")), 19.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = theme.monospaceFont.height")), 13.0f);
    EXPECT_TRUE (static_cast<bool> (evaluate (scope, "out = theme.dark")));
    EXPECT_EQ (evaluateAs<Color> (scope, "out = theme.accent"), Color (0xff336699));
    EXPECT_EQ (evaluateAs<Color> (scope, "out = theme.brandHighlight"), Color (0xffff8844));
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = theme.gridSnap * 2")), 16.0);

    // Expressions reading theme.* mark BoundInputs as palette/theme-dependent so StyleContext refreshes on theme changes.
    scope.install();
    auto compiled = CompiledExpression::compile ("out = theme.font.height", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
    EXPECT_TRUE (scope.bindInputs (compiled.getReference()).readsPalette());

    customTheme->setDefaultFont (Font().withHeight (21.0f));
    EXPECT_FLOAT_EQ (static_cast<float> (evaluate (scope, "out = theme.font.height")), 21.0f);

    ApplicationTheme::setGlobalTheme (oldTheme);
}

TEST_F (DrawingScopeTest, AnArrayOfAnotherElementTypeLeavesNothingOfTheOldOneToRead)
{
    const auto steps = context.addArray<float> ("steps");

    auto compiled = CompiledExpression::compile ("out = len (steps)", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    ExpressionState state (program);
    DrawingScope::ArrayInput input;

    input.bind (state, steps.index, ComponentProperty::Type::number, var (Array<var> { 1.0, 2.0 }));
    state.evaluate();
    EXPECT_DOUBLE_EQ (static_cast<double> (state.getVar (program.findOutput ("out"))), 2.0);

    // Strings can't feed numbers, and the numbers there were are gone
    input.bind (state, steps.index, ComponentProperty::Type::string, var (Array<var> { "a" }));
    state.evaluate();
    EXPECT_DOUBLE_EQ (static_cast<double> (state.getVar (program.findOutput ("out"))), 0.0);
}

