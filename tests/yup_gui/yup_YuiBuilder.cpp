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

class YuiBuilderTests : public ::testing::Test
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
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    static DataTree parse (const String& text)
    {
        const auto result = YuiParser::parse (text);
        EXPECT_TRUE (result.wasOk()) << (result.wasOk() ? String() : result.diagnostics.front().toString());
        return result.document;
    }

    // Builds a document, returning its diagnostics with the line and column they were written at
    static StringArray buildAndLocate (const String& text, bool shouldBuild)
    {
        const auto parsed = YuiParser::parse (text);
        EXPECT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics);

        YuiBuilder builder;
        EXPECT_EQ (builder.build (parsed.document).wasOk(), shouldBuild) << text;

        StringArray located;

        for (const auto& diagnostic : builder.getDiagnostics())
            located.add (parsed.sourceMap.locate (diagnostic).toString());

        return located;
    }

    struct RecordingProvider : BindingProvider
    {
        std::vector<Property> getProperties() const override { return { { Identifier ("gain"), ComponentProperty::Type::number } }; }

        var getValue (const Identifier&) const override { return gain; }

        void setValue (const Identifier& name, const var& value) override
        {
            gain = value;
            notifyValueChanged (name);
        }

        void beginGesture (const Identifier& name) override { gestures.add ("begin " + name.toString()); }

        void endGesture (const Identifier& name) override { gestures.add ("end " + name.toString()); }

        var gain = 0.5;
        StringArray gestures;
    };

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (YuiBuilderTests, BuildsTheComponentTree)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Component {
            id: panel; width: 200; height: 100;
            Slider { id: knob; class: "knob big"; sliderType: rotaryVerticalDrag;
                     x: parent.width / 4; width: parent.width / 2; height: 50; value: 0.25; }
            Label { id: caption; text: "Volume"; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* root = builder.getRoot();
    ASSERT_NE (root, nullptr);
    EXPECT_EQ (root->getComponentID(), "panel");
    EXPECT_EQ (root->getNumChildComponents(), 2);
    EXPECT_FALSE (root->isOpaque());

    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_EQ (knob->getParentComponent(), root);
    EXPECT_EQ (knob->getSliderType(), Slider::RotaryVerticalDrag);
    EXPECT_TRUE (knob->hasStyleClass (Identifier ("knob")));
    EXPECT_TRUE (knob->hasStyleClass (Identifier ("big")));
    EXPECT_FLOAT_EQ (knob->getX(), 50.0f);
    EXPECT_FLOAT_EQ (knob->getWidth(), 100.0f);
    EXPECT_DOUBLE_EQ (knob->getValue(), 0.25);

    auto* caption = builder.findById<Label> ("caption");
    ASSERT_NE (caption, nullptr);
    EXPECT_EQ (caption->getText(), "Volume");
    EXPECT_EQ (builder.findById ("missing"), nullptr);
}

TEST_F (YuiBuilderTests, FailsForUnknownTypesAndBuildsNothing)
{
    YuiBuilder builder;

    EXPECT_TRUE (builder.build (parse ("Component { Unknown { } }")).failed());
    EXPECT_EQ (builder.getRoot(), nullptr);

    EXPECT_TRUE (builder.build (parse ("style { }")).failed());
}

TEST_F (YuiBuilderTests, ReportsPropertiesThatCantBeSet)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Slider { normalized: 0.5; sliderType: sideways; width: unknownSymbol; })yui"));

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (builder.getDiagnostics().size(), 3u);
}

TEST_F (YuiBuilderTests, ReportsWhereEachProblemIsWritten)
{
    const auto located = buildAndLocate (R"yui(component Slider : Component { }
component Card : Column {
    property spot: font;
    property level: number = unknownSymbol;
    property alias lost <=> missing.value;
    property alias none <=> knob.nothing;
    Slider { id: knob; }
}
tokens { property accent: number = 1; property broken: number = unknownSymbol; }
tokens { property accent: number = 2; }
Column {
    Card { spot: 1; }
    Slider { id: twin; hovered: true; }
    Slider { id: twin; sliderType: sideways; }
    Slider { width: unknownSymbol; }
    Slider { value <=> knob; }
    Slider { value <=> nowhere.value; }
    Slider { value <=> twin.nothing; }
    Slider { hovered <=> twin.value; }
    TextButton { onClick: 3; }
    TextButton { onClick: action (missingName); }
    TextButton { onClick: nowhere.onClick(); }
    TextButton { onClick: twin.onValueChanged(); }
    TextButton { onClick: action ("go", unknownSymbol); }
    let x: 1;
})yui",
                                         true);

    const StringArray expected {
        "1:1: component Slider: the type is already declared",
        "5:14: component Card.lost: unknown element in 'missing.value'",
        "6:14: component Card.none: Slider has no property 'nothing'",
        "25:5: 'let' is not a component type",
        "14:14: The id 'twin' is used more than once",
        "10:19: tokens: 'accent' is declared more than once",
        "4:14: Card.level: ",
        "9:48: Column.broken: ",
        "12:12: Card.spot: properties of this type can't be set from .yui yet",
        "13:24: Slider.hovered: the property is read-only",
        "14:24: Slider.sliderType: expected one of ",
        "15:14: Slider.width: ",
        "16:14: Slider.value: expected element.property after '<=>'",
        "17:14: Slider.value: unknown element 'nowhere'",
        "18:14: Slider.value: Slider has no property 'nothing'",
        "19:14: Slider.hovered: the property is read-only",
        "20:18: TextButton.onClick: expected a call, like action (\"name\") or root.signal()",
        "21:18: TextButton.onClick: action expects the name of the action as a string",
        "22:18: TextButton.onClick: unknown function 'nowhere.onClick'",
        "23:18: TextButton.onClick: 'twin.onValueChanged' is not a signal declared by a component",
        "24:18: TextButton.onClick: "
    };

    EXPECT_EQ (located.size(), expected.size()) << located.joinIntoString ("\n");

    for (const auto& prefix : expected)
    {
        EXPECT_TRUE (std::any_of (located.begin(), located.end(), [&] (const String& line)
        {
            return line.startsWith (prefix);
        })) << "No diagnostic starting with '" << prefix << "' in:\n"
            << located.joinIntoString ("\n");
    }
}

TEST_F (YuiBuilderTests, ReportsWhereTheErrorFailingTheBuildIsWritten)
{
    const std::pair<const char*, const char*> cases[] = {
        { "component A : B { }\ncomponent B : A { }\nA { }", "1:15: The component 'A' is based on itself" },
        { "component Broken : Missing { }\nBroken { }", "1:20: Unknown component type 'Missing'" },
        { "component Loop : Column { Loop { } }\nLoop { }", "1:27: The component 'Loop' contains itself" },
        { "Column { Unknown { } }", "1:10: Unknown component type 'Unknown'" },
        { "Layout { }", "1:1: The component type 'Layout' can't be created from .yui" },
        { "style { }", "The document has no root component" }
    };

    // The layout base type has no factory, applications may register such types
    auto& registry = ComponentTypeRegistry::getInstance();
    registry.registerType (LayoutComponent::getClassTypeInfo());

    for (const auto& [text, expected] : cases)
        EXPECT_EQ (buildAndLocate (text, false).joinIntoString ("\n"), String (expected)) << text;

    registry.unregisterType (LayoutComponent::getClassTypeInfo().getTypeName());
}

TEST_F (YuiBuilderTests, LinksTwoPropertiesBothWays)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Component {
            Slider { id: follower; value <=> knob.value; }
            Slider { id: knob; value: 0.3; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* follower = builder.findById<Slider> ("follower");
    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (follower, nullptr);
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (follower->getValue()), 0.3f);

    follower->setValue (0.8);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.8f);

    knob->setValue (0.1);
    EXPECT_FLOAT_EQ (static_cast<float> (follower->getValue()), 0.1f);
}

TEST_F (YuiBuilderTests, AppliesValuesInTheOrderTheTypeDeclaresThem)
{
    RecordingProvider provider;
    provider.gain = 1000.0;

    YuiBuilder builder;
    builder.setModel ("synth", provider);

    const auto result = builder.build (parse (R"yui(
        Component {
            Slider { id: constant; value: 1000; defaultValue: 500; maximum: 20000; minimum: 20; interval: 1; }
            Slider { id: linked; value <=> synth.gain; maximum: 20000; minimum: 20; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* constant = builder.findById<Slider> ("constant");
    ASSERT_NE (constant, nullptr);
    EXPECT_DOUBLE_EQ (constant->getValue(), 1000.0);
    EXPECT_DOUBLE_EQ (constant->getDefaultValue(), 500.0);
    EXPECT_DOUBLE_EQ (constant->getInterval(), 1.0);

    auto* linked = builder.findById<Slider> ("linked");
    ASSERT_NE (linked, nullptr);
    EXPECT_DOUBLE_EQ (linked->getValue(), 1000.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (provider.gain), 1000.0);
}

TEST_F (YuiBuilderTests, ReportsLinksToUnknownTargets)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse ("Slider { id: gain; value <=> model.gain; }")).wasOk());

    ASSERT_EQ (builder.getDiagnostics().size(), 1u);
    EXPECT_EQ (builder.getDiagnostics()[0].message, "Slider.value: unknown element 'model'");
}

TEST_F (YuiBuilderTests, RebuildingReplacesTheTree)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse ("Component { id: first; }")).wasOk());
    ASSERT_TRUE (builder.build (parse ("Component { id: second; }")).wasOk());

    EXPECT_EQ (builder.getRoot()->getComponentID(), "second");
    EXPECT_EQ (builder.findById ("first"), nullptr);
}

TEST_F (YuiBuilderTests, RebuildingAPatchedTreeKeepsTheComponentsAndWhatTheUserChanged)
{
    auto document = parse (R"yui(
        Component {
            Slider { id: knob; value: 0.25; }
            Label { id: caption; text: "a"; }
        }
    )yui");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());

    auto* root = builder.getRoot();
    auto* knob = builder.findById<Slider> ("knob");
    auto* caption = builder.findById<Label> ("caption");
    ASSERT_NE (knob, nullptr);
    knob->setValue (0.75);

    YuiTreeDiff::apply (document, parse (R"yui(
        Component {
            TextButton { id: first; }
            Slider { id: knob; value: 0.25; }
            Label { id: caption; text: "b"; }
        }
    )yui"));

    ASSERT_TRUE (builder.build (document).wasOk());

    EXPECT_EQ (builder.getRoot(), root);
    EXPECT_EQ (builder.findById ("knob"), knob);
    EXPECT_EQ (builder.findById ("caption"), caption);
    EXPECT_DOUBLE_EQ (knob->getValue(), 0.75);
    EXPECT_EQ (caption->getText(), "b");

    ASSERT_EQ (root->getNumChildComponents(), 3);
    EXPECT_EQ (root->getChildComponent (0), builder.findById ("first"));
    EXPECT_EQ (root->getChildComponent (1), knob);
}

TEST_F (YuiBuilderTests, RebuildingResetsWhatTheDocumentNoLongerWrites)
{
    auto document = parse (R"yui(
        Component {
            Label { id: caption; class: "big"; text: "a"; flexGrow: 2; }
            TextButton { id: button; onClick: action ("press"); }
        }
    )yui");

    YuiBuilder builder;
    int presses = 0;
    builder.setAction ("press", [&] (const Array<var>&) { ++presses; });
    ASSERT_TRUE (builder.build (document).wasOk());

    YuiTreeDiff::apply (document, parse ("Component { Label { id: caption; } TextButton { id: button; } }"));
    ASSERT_TRUE (builder.build (document).wasOk());

    auto* caption = builder.findById<Label> ("caption");
    ASSERT_NE (caption, nullptr);
    EXPECT_EQ (caption->getText(), String());
    EXPECT_FALSE (caption->hasProperty ("flexGrow"));
    EXPECT_TRUE (caption->getStyleClasses().isEmpty());

    auto* button = builder.findById<TextButton> ("button");
    ASSERT_NE (button, nullptr);
    button->onClick();
    EXPECT_EQ (presses, 0);
}

TEST_F (YuiBuilderTests, KeepsWhatWasBuiltWhenTheNewDocumentCantBeBuilt)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse ("Component { Slider { id: knob; } }")).wasOk());
    auto* root = builder.getRoot();

    EXPECT_TRUE (builder.build (parse ("Component { Missing { } }")).failed());

    EXPECT_EQ (builder.getRoot(), root);
    EXPECT_NE (builder.findById ("knob"), nullptr);
}

TEST_F (YuiBuilderTests, BuildsALayoutContainerThatPositionsItsChildren)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Row {
            id: bar; width: 120; height: 40; gap: 20;
            Label { id: left; flexGrow: 1; flexBasis: 0; }
            Label { id: right; flexGrow: 1; flexBasis: 0; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* left = builder.findById ("left");
    auto* right = builder.findById ("right");
    ASSERT_NE (left, nullptr);
    ASSERT_NE (right, nullptr);

    EXPECT_EQ (left->getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 40.0f));
    EXPECT_EQ (right->getBounds(), Rectangle<float> (70.0f, 0.0f, 50.0f, 40.0f));
}

TEST_F (YuiBuilderTests, KeepsPropertiesUpToDateWithBindings)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Component {
            id: panel; width: 200; height: 100;
            Slider { id: knob; value: 0.25; }
            Slider { id: follower; value: knob.value; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* knob = builder.findById<Slider> ("knob");
    auto* follower = builder.findById<Slider> ("follower");
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (follower, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (follower->getValue()), 0.25f);

    knob->setValue (0.75);

    EXPECT_FLOAT_EQ (static_cast<float> (follower->getValue()), 0.75f);
}

TEST_F (YuiBuilderTests, ResolvesAnElementDeclaredLater)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Component {
            width: 200; height: 100;
            Slider { id: follower; value: knob.value; }
            Slider { id: knob; value: 0.5; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* follower = builder.findById<Slider> ("follower");
    ASSERT_NE (follower, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (follower->getValue()), 0.5f);
}

TEST_F (YuiBuilderTests, FollowsTheInteractionStatesOfAnotherElement)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Column {
            width: 200; height: 100;
            Slider { id: volume; sliderType: rotaryVerticalDrag; flexGrow: 1; }
            Label { id: readout; preferredHeight: 24; opacity: volume.pressed ? 1 : 0.5; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* volume = builder.findById<Slider> ("volume");
    auto* readout = builder.findById<Label> ("readout");
    ASSERT_NE (volume, nullptr);
    ASSERT_NE (readout, nullptr);
    EXPECT_NEAR (readout->getOpacity(), 0.5f, 0.01f);

    ComponentTestHelper<Slider>::triggerMouseDown (*volume, MouseEvent {});
    EXPECT_NEAR (readout->getOpacity(), 1.0f, 0.01f);

    ComponentTestHelper<Slider>::triggerMouseUp (*volume, MouseEvent {});
    EXPECT_NEAR (readout->getOpacity(), 0.5f, 0.01f);
}

TEST_F (YuiBuilderTests, RunsAnActionWhenASignalIsEmitted)
{
    YuiBuilder builder;
    Array<var> received;
    int calls = 0;

    builder.setAction ("reset", [&] (const Array<var>& arguments)
    {
        received = arguments;
        ++calls;
    });

    const auto result = builder.build (parse (R"yui(
        Component {
            TextButton { id: go; onClick: action ("reset", knob.value * 2, "now"); }
            Slider { id: knob; value: 0.25; onValueChanged: action ("reset"); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* go = builder.findById<TextButton> ("go");
    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (go, nullptr);
    ASSERT_NE (knob, nullptr);
    calls = 0;

    ASSERT_TRUE (go->onClick != nullptr);
    go->onClick();

    EXPECT_EQ (calls, 1);
    ASSERT_EQ (received.size(), 2);
    EXPECT_FLOAT_EQ (static_cast<float> (received[0]), 0.5f);
    EXPECT_EQ (received[1].toString(), "now");

    knob->setValue (0.75);

    EXPECT_EQ (calls, 2);
    ASSERT_EQ (received.size(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (received[0]), 0.75f);
}

TEST_F (YuiBuilderTests, ReportsHandlersThatArentCalls)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Component {
            TextButton { onClick: 3; }
            TextButton { onClick: action (missingName); }
            TextButton { onClick: nowhere.onClick(); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (builder.getDiagnostics().size(), 3u) << YuiDiagnostic::join (builder.getDiagnostics());
}

TEST_F (YuiBuilderTests, NamesTheArgumentsOfASignalInItsHandler)
{
    YuiBuilder builder;
    Array<var> received;
    builder.setAction ("record", [&] (const Array<var>& arguments) { received = arguments; });

    const auto result = builder.build (parse (R"yui(
        component Picker : Column {
            signal onPicked (index: integer, name: string);
            TextButton { onClick: root.onPicked (2, "two"); }
        }

        Column {
            Slider { id: knob; onValueChanged: (width) => action ("record", width * 2); }
            Picker { id: picker; onPicked: (i, label) => action ("record", label, i + 1); }
            Picker { id: forwarded; onPicked: (i) => action ("record"); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    // A named argument hides the property of the same name
    builder.findById<Slider> ("knob")->setValue (0.25);
    ASSERT_EQ (received.size(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (received[0]), 0.5f);

    auto* picker = builder.findById ("picker");
    ASSERT_NE (picker, nullptr);
    dynamic_cast<TextButton*> (picker->getChildComponent (0))->onClick();
    ASSERT_EQ (received.size(), 2);
    EXPECT_EQ (received[0].toString(), "two");
    EXPECT_EQ (static_cast<int> (received[1]), 3);

    // A call without arguments still receives the ones of the signal
    dynamic_cast<TextButton*> (builder.findById ("forwarded")->getChildComponent (0))->onClick();
    ASSERT_EQ (received.size(), 2);
    EXPECT_EQ (static_cast<int> (received[0]), 2);
    EXPECT_EQ (received[1].toString(), "two");
}

TEST_F (YuiBuilderTests, RunsTheCallsOfAHandlerBlockInOrder)
{
    YuiBuilder builder;
    StringArray log;
    builder.setAction ("log", [&] (const Array<var>& arguments)
    {
        StringArray texts;

        for (const auto& argument : arguments)
            texts.add (argument.toString());

        log.add (texts.joinIntoString (" "));
    });

    const auto result = builder.build (parse (R"yui(
        component Panel : Column {
            signal onDone (count: integer);
            TextButton { onClick: { action ("log", "first"); root.onDone (2); action ("log", "last"); } }
        }

        Column {
            Panel { id: panel; onDone: (count) => { action ("log", "done"); action ("log"); } }
            TextButton { id: idle; onClick: { }; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    dynamic_cast<TextButton*> (builder.findById ("panel")->getChildComponent (0))->onClick();
    builder.findById<TextButton> ("idle")->onClick();

    // A call without arguments forwards the signal's, typed as the signal declares them
    EXPECT_EQ (log.joinIntoString ("\n"), "first\ndone\n2\nlast");
}

TEST_F (YuiBuilderTests, RunsChangedHandlersWhenTheirPropertyChanges)
{
    auto document = parse (R"yui(
        Component {
            Slider { id: knob; changed value: (v) => action ("value", v * 2); }
            Label { id: caption; changed text: action ("text"); }
        })yui");

    YuiBuilder builder;
    Array<var> values, texts;
    builder.setAction ("value", [&] (const Array<var>& arguments) { values.addArray (arguments); });
    builder.setAction ("text", [&] (const Array<var>& arguments) { texts.addArray (arguments); });

    // Building again replaces the handlers instead of adding to them
    ASSERT_TRUE (builder.build (document).wasOk());
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* knob = builder.findById<Slider> ("knob");
    auto* caption = builder.findById<Label> ("caption");
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (caption, nullptr);

    knob->setValue (0.25);
    caption->setText ("hello", sendNotificationSync);

    ASSERT_EQ (values.size(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (values[0]), 0.5f);
    ASSERT_EQ (texts.size(), 1);
    EXPECT_EQ (texts[0].toString(), "hello");

    YuiTreeDiff::apply (document, parse ("Component { Slider { id: knob; } Label { id: caption; } }"));
    ASSERT_TRUE (builder.build (document).wasOk());
    ASSERT_EQ (builder.findById<Slider> ("knob"), knob);

    knob->setValue (0.75);
    EXPECT_EQ (values.size(), 1);
}

TEST_F (YuiBuilderTests, ReportsWhereHandlerProblemsAreWritten)
{
    const auto located = buildAndLocate (R"yui(
        Component {
            TextButton { onClick: (a) => action ("x"); }
            TextButton { onClick: { action ("x"); 3; } }
            TextButton { onClick: (a, a) => action ("x"); }
            Slider { changed nothing: action ("x"); }
            Slider { changed value: (v, w) => action ("x"); }
        })yui",
                                         true);

    EXPECT_EQ (located.joinIntoString ("\n"), StringArray ({
                                                  "3:26: TextButton.onClick: expected at most 0 argument names",
                                                  "4:26: TextButton.onClick: expected a call, like action (\"name\") or root.signal()",
                                                  "5:26: TextButton.onClick: the argument 'a' is named twice",
                                                  "6:22: Slider.changed nothing: unknown property 'nothing'",
                                                  "7:22: Slider.changed value: expected at most 1 argument names",
                                              })
                                                  .joinIntoString ("\n"));
}

TEST_F (YuiBuilderTests, UnescapesActionNamesThroughExpressionAst)
{
    YuiBuilder builder;
    bool invoked = false;
    builder.setAction ("say \"hi\"\nnow", [&] (const Array<var>&)
    {
        invoked = true;
    });

    const auto result = builder.build (parse (R"yui(
        Component {
            TextButton { id: button; onClick: action ("say \"hi\"\nnow"); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* button = builder.findById<TextButton> ("button");
    ASSERT_NE (button, nullptr);
    button->onClick();
    EXPECT_TRUE (invoked);
}

TEST_F (YuiBuilderTests, InstantiatesADeclaredComponent)
{
    YuiBuilder builder;
    Array<var> changes;

    builder.setAction ("cutoffChanged", [&] (const Array<var>& arguments)
    {
        changes.add (arguments[0]);
    });

    const auto result = builder.build (parse (R"yui(
        component LabeledKnob : Column {
            property label: string = "Gain";
            property alias value <=> knob.value;
            signal onChanged (value: number);

            class: "labeled";
            Label { id: caption; text: root.label; }
            Slider { id: knob; onValueChanged: root.onChanged (value); }
        }

        Row {
            LabeledKnob { id: cutoff; class: "wide"; label: "Cutoff"; value: 0.25; onChanged: action ("cutoffChanged");
                          Label { id: extra; text: caption.text; } }
            LabeledKnob { id: resonance; }
            Label { id: caption; text: "outer"; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* cutoff = builder.findById ("cutoff");
    ASSERT_NE (dynamic_cast<ColumnComponent*> (cutoff), nullptr);
    EXPECT_EQ (cutoff->getTypeInfo().getTypeName(), Identifier ("LabeledKnob"));
    EXPECT_TRUE (cutoff->getTypeInfo().isSubtypeOf (Identifier ("Column")));
    EXPECT_TRUE (cutoff->hasStyleClass (Identifier ("labeled")));
    EXPECT_TRUE (cutoff->hasStyleClass (Identifier ("wide")));
    ASSERT_EQ (cutoff->getNumChildComponents(), 3);

    auto* caption = dynamic_cast<Label*> (cutoff->getChildComponent (0));
    auto* knob = dynamic_cast<Slider*> (cutoff->getChildComponent (1));
    ASSERT_NE (caption, nullptr);
    ASSERT_NE (knob, nullptr);
    EXPECT_EQ (caption->getText(), "Cutoff");
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.25f);

    // The ids of a component body are private, the use site children see the outer ones
    EXPECT_EQ (builder.findById ("knob"), nullptr);
    EXPECT_EQ (builder.findById<Label> ("extra")->getText(), "outer");

    auto* resonance = builder.findById ("resonance");
    ASSERT_NE (resonance, nullptr);
    EXPECT_EQ (dynamic_cast<Label*> (resonance->getChildComponent (0))->getText(), "Gain");
    EXPECT_NE (resonance->getChildComponent (1), knob);

    changes.clear();
    knob->setValue (0.5);

    EXPECT_FLOAT_EQ (static_cast<float> (cutoff->getProperty (Identifier ("value"))), 0.5f);
    ASSERT_EQ (changes.size(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (changes[0]), 0.5f);

    cutoff->setProperty (Identifier ("value"), 0.125);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.125f);

    cutoff->setProperty (Identifier ("label"), "Frequency");
    EXPECT_EQ (caption->getText(), "Frequency");

    const auto* typeInfo = builder.findTypeInfo (Identifier ("LabeledKnob"));
    ASSERT_NE (typeInfo, nullptr);
    EXPECT_EQ (typeInfo->findProperty (Identifier ("value"))->type, ComponentProperty::Type::number);
    EXPECT_NE (typeInfo->findSignal (Identifier ("onChanged")), nullptr);
}

TEST_F (YuiBuilderTests, DeclaresArrayAndPointProperties)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        component Lanes : Column {
            property lanes: number[] = [0.25, 0.5];
            property names: string[];
            property origin: point = point (1, 2);

            Slider { id: knob; value: len (root.lanes) / 4.0; }
            Label { id: caption; text: root.names[0]; }
        }

        Row {
            Lanes { id: plain; }
            Lanes { id: filled; lanes: [1, 2, 3]; names: ["x"]; origin: point (5, 6); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* plain = builder.findById ("plain");
    auto* filled = builder.findById ("filled");
    ASSERT_NE (plain, nullptr);
    ASSERT_NE (filled, nullptr);

    const auto sizeOf = [] (Component& component, const char* name)
    {
        const auto* elements = component.findProperty (Identifier (name)).getArray();
        return elements != nullptr ? elements->size() : -1;
    };

    EXPECT_EQ (sizeOf (*plain, "lanes"), 2);
    EXPECT_EQ (sizeOf (*plain, "names"), 0);
    EXPECT_EQ (sizeOf (*filled, "lanes"), 3);
    EXPECT_EQ (VariantConverter<Point<float>>::fromVar (plain->getProperty (Identifier ("origin"))), Point<float> (1.0f, 2.0f));
    EXPECT_EQ (VariantConverter<Point<float>>::fromVar (filled->getProperty (Identifier ("origin"))), Point<float> (5.0f, 6.0f));

    EXPECT_FLOAT_EQ (static_cast<float> (dynamic_cast<Slider*> (plain->getChildComponent (0))->getValue()), 0.5f);
    EXPECT_FLOAT_EQ (static_cast<float> (dynamic_cast<Slider*> (filled->getChildComponent (0))->getValue()), 0.75f);
    EXPECT_EQ (dynamic_cast<Label*> (filled->getChildComponent (1))->getText(), "x");

    // The values of an array are read again when it changes
    plain->setProperty (Identifier ("lanes"), var (Array<var> { 1.0, 2.0, 3.0, 4.0 }));
    EXPECT_FLOAT_EQ (static_cast<float> (dynamic_cast<Slider*> (plain->getChildComponent (0))->getValue()), 1.0f);
}

TEST_F (YuiBuilderTests, NestsDeclaredComponents)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        component Caption : Label {
            property prefix: string = "#";
            text: root.prefix + " caption";
        }

        component Card : Column {
            property title: string;
            Caption { id: heading; prefix: root.title; }
        }

        Card { id: card; title: "Filter"; })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* card = builder.getRoot();
    ASSERT_NE (card, nullptr);
    auto* heading = dynamic_cast<Label*> (card->getChildComponent (0));
    ASSERT_NE (heading, nullptr);
    EXPECT_EQ (heading->getTypeInfo().getTypeName(), Identifier ("Caption"));
    EXPECT_EQ (heading->getText(), "Filter caption");
}

TEST_F (YuiBuilderTests, FailsForRecursiveComponents)
{
    YuiBuilder builder;

    EXPECT_TRUE (builder.build (parse ("component A : B { } component B : A { } A { }")).failed());
    EXPECT_EQ (builder.getRoot(), nullptr);

    EXPECT_TRUE (builder.build (parse ("component A : Column { A { } } A { }")).failed());
    EXPECT_EQ (builder.getRoot(), nullptr);
}

TEST_F (YuiBuilderTests, ResolvesTokensFromAnyDescendant)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        tokens {
            property spacing: number = 8;
            property accent: color = #ff0000;
        }

        Component {
            width: spacing * 20;
            Label { id: caption; width: spacing * 10; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* root = builder.getRoot();
    auto* caption = builder.findById ("caption");
    ASSERT_NE (root, nullptr);
    ASSERT_NE (caption, nullptr);
    EXPECT_EQ (root->getTypeInfo().getTypeName(), Identifier ("Component"));
    EXPECT_EQ (root->getTypeInfo().findProperty (Identifier ("accent"))->type, ComponentProperty::Type::color);
    EXPECT_FLOAT_EQ (root->getWidth(), 160.0f);
    EXPECT_FLOAT_EQ (caption->getWidth(), 80.0f);

    root->setProperty (Identifier ("spacing"), 4.0);
    EXPECT_FLOAT_EQ (caption->getWidth(), 40.0f);
}

TEST_F (YuiBuilderTests, LinksPropertiesToAModel)
{
    DataTree tree ("params");
    tree.beginTransaction().setProperty ("cutoff", 0.4);

    UndoManager undoManager;
    DataTreeBindingProvider params (tree, &undoManager);

    YuiBuilder builder;
    builder.setModel ("params", params);

    const auto result = builder.build (parse (R"yui(
        Component {
            Slider { id: knob; value <=> params.cutoff; }
            Label { id: readout; text: format ("cutoff %.1f", params.cutoff); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* knob = builder.findById<Slider> ("knob");
    auto* readout = builder.findById<Label> ("readout");
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (readout, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.4f);

    knob->setValue (0.7);
    EXPECT_FLOAT_EQ (static_cast<float> (tree.getProperty ("cutoff")), 0.7f);
    EXPECT_TRUE (readout->getText().startsWith ("cutoff 0.7"));

    tree.beginTransaction().setProperty ("cutoff", 0.2);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.2f);
}

TEST_F (YuiBuilderTests, ReadsTheArrayPropertiesOfAModel)
{
    DataTree tree ("params");
    tree.beginTransaction().setProperty ("names", var (Array<var> { "low", "high" }));
    tree.beginTransaction().setProperty ("gains", var (Array<var> { 0.25, 0.75 }));

    DataTreeBindingProvider params (tree, nullptr);

    YuiBuilder builder;
    builder.setModel ("params", params);

    const auto result = builder.build (parse (R"yui(
        Component {
            Label { id: caption; text: params.names[1]; }
            Slider { id: knob; value: params.gains[len (params.gains) - 1]; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* caption = builder.findById<Label> ("caption");
    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (caption, nullptr);
    ASSERT_NE (knob, nullptr);
    EXPECT_EQ (caption->getText(), "high");
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.75f);

    tree.beginTransaction().setProperty ("gains", var (Array<var> { 0.5, 0.125 }));
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.125f);
}

TEST_F (YuiBuilderTests, FollowsTheGesturesOfTheComponentsLinkedToAModel)
{
    RecordingProvider provider;

    YuiBuilder builder;
    builder.setModel ("synth", provider);

    const auto result = builder.build (parse (R"yui(
        Component {
            Slider { id: knob; value <=> synth.gain; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.5f);

    ComponentTestHelper<Slider>::triggerMouseDown (*knob, MouseEvent {});
    knob->setValue (0.9);
    ComponentTestHelper<Slider>::triggerMouseUp (*knob, MouseEvent {});

    EXPECT_FLOAT_EQ (static_cast<float> (provider.gain), 0.9f);
    EXPECT_EQ (provider.gestures.joinIntoString (", "), "begin gain, end gain");
}

TEST_F (YuiBuilderTests, MakesEachChangeOutsideAGestureAGestureOfItsOwn)
{
    RecordingProvider provider;

    YuiBuilder builder;
    builder.setModel ("synth", provider);

    const auto result = builder.build (parse (R"yui(
        Component {
            Slider { id: knob; value <=> synth.gain; }
            Slider { id: other; value <=> synth.gain; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* knob = builder.findById<Slider> ("knob");
    auto* other = builder.findById<Slider> ("other");
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (other, nullptr);

    knob->setValue (0.8);
    EXPECT_FLOAT_EQ (static_cast<float> (provider.gain), 0.8f);
    EXPECT_FLOAT_EQ (static_cast<float> (other->getValue()), 0.8f);
    EXPECT_EQ (provider.gestures.joinIntoString (", "), "begin gain, end gain");

    provider.gestures.clear();
    ComponentTestHelper<Slider>::triggerMouseDown (*knob, MouseEvent {});
    other->setValue (0.3);
    ComponentTestHelper<Slider>::triggerMouseUp (*knob, MouseEvent {});

    EXPECT_FLOAT_EQ (static_cast<float> (provider.gain), 0.3f);
    EXPECT_EQ (provider.gestures.joinIntoString (", "), "begin gain, end gain");
}

TEST_F (YuiBuilderTests, PlacesTheChildOfAViewportAsItsContent)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Viewport {
            id: scroller;
            scrollStep: 12;

            Column { id: content; height: 900; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* viewport = builder.findById<Viewport> ("scroller");
    ASSERT_NE (viewport, nullptr);
    EXPECT_FLOAT_EQ (viewport->getScrollStep(), 12.0f);
    EXPECT_EQ (viewport->getViewedComponent(), builder.findById ("content"));
}

TEST_F (YuiBuilderTests, TurnsTheChildrenOfATabComponentIntoTabs)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        TabComponent {
            id: tabs;
            placement: bottom;
            selectedTab: "second";

            Column { id: first; title: "First"; }
            Column { id: second; title: "Second"; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << YuiDiagnostic::join (builder.getDiagnostics());

    auto* tabs = builder.findById<TabComponent> ("tabs");
    ASSERT_NE (tabs, nullptr);
    EXPECT_EQ (tabs->getTabBarPlacement(), TabComponent::Placement::bottom);
    EXPECT_EQ (tabs->getTabContent (Identifier ("first")), builder.findById ("first"));
    EXPECT_EQ (tabs->getTabContent (Identifier ("second")), builder.findById ("second"));
    EXPECT_EQ (tabs->getTabBar().getSelectedTabId().toString(), "second");

    // The title of a page is the text of its tab, before and after it is built
    const auto* button = tabs->getTabBar().getTabButton (Identifier ("first"));
    ASSERT_NE (button, nullptr);
    EXPECT_EQ (button->getText(), "First");

    builder.findById ("first")->setTitle ("Renamed");
    EXPECT_EQ (button->getText(), "Renamed");
}

TEST_F (YuiBuilderTests, NumbersTheTabsOfThePagesWithoutAnId)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        TabComponent {
            id: tabs;

            Column { title: "First"; }
            Column { title: "Second"; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* tabs = builder.findById<TabComponent> ("tabs");
    ASSERT_NE (tabs, nullptr);
    ASSERT_NE (tabs->getTabContent (Identifier ("tab1")), nullptr);
    ASSERT_NE (tabs->getTabContent (Identifier ("tab2")), nullptr);
    EXPECT_EQ (tabs->getTabContent (Identifier ("tab2"))->getTitle(), "Second");
}

TEST_F (YuiBuilderTests, RebuildingLeavesTheChildrenOfAContainerWhereItPutThem)
{
    auto document = parse (R"yui(
        Viewport {
            id: scroller;
            Column { id: content; }
        })yui");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());

    auto* viewport = builder.findById<Viewport> ("scroller");
    auto* content = builder.findById ("content");
    ASSERT_NE (viewport, nullptr);
    ASSERT_EQ (viewport->getViewedComponent(), content);

    YuiTreeDiff::apply (document, parse (R"yui(
        Viewport {
            id: scroller;
            scrollStep: 8;
            Column { id: content; }
        })yui"));

    ASSERT_TRUE (builder.build (document).wasOk());

    EXPECT_EQ (builder.findById ("content"), content);
    EXPECT_EQ (viewport->getViewedComponent(), content);
    EXPECT_FLOAT_EQ (viewport->getScrollStep(), 8.0f);
}
