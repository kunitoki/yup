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

#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

#include "yup_YuiTestHelpers.h"

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
        EXPECT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

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

    // A column counting its layouts, which documents name once it is registered
    struct CountingColumn : ColumnComponent
    {
        static const ComponentTypeInfo& getClassTypeInfo()
        {
            static const auto info = ComponentTypeBuilder<CountingColumn> (Identifier ("CountingColumn"), ColumnComponent::getClassTypeInfo()).build();
            return info;
        }

        const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

        void performLayout (Rectangle<float> area) override
        {
            ++numLayouts;
            ColumnComponent::performLayout (area);
        }

        static inline int numLayouts = 0;
    };

    // A label counting the instances made of it, which documents name once it is registered
    struct CountingLabel : Label
    {
        CountingLabel()
        {
            ++numCreated;
        }

        static const ComponentTypeInfo& getClassTypeInfo()
        {
            static const auto info = ComponentTypeBuilder<CountingLabel> (Identifier ("CountingLabel"), Label::getClassTypeInfo()).build();
            return info;
        }

        const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

        static inline int numCreated = 0;
    };

    // The children of a component, in their order
    static std::vector<Component*> childrenOf (const Component* parent)
    {
        std::vector<Component*> children;

        for (int index = 0; index < parent->getNumChildComponents(); ++index)
            children.push_back (parent->getChildComponent (index));

        return children;
    }

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
        "12:12: Card.spot: ",
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

    registerBuiltInComponentTypes();

    for (const auto& [text, expected] : cases)
        EXPECT_EQ (buildAndLocate (text, false).joinIntoString ("\n"), String (expected)) << text;
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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

TEST_F (YuiBuilderTests, ReportsTheImportsAParseLeftUnresolved)
{
    // Parsing with no import resolver reads no file: what the import was for is missing, which is told where it is written
    EXPECT_EQ (buildAndLocate ("style { }\nimport \"parts/knob.yui\";\nComponent { }", true),
               StringArray ({ "2:1: Unresolved import 'parts/knob.yui'" }));
}

TEST_F (YuiBuilderTests, ReportsAnIdOrAClassThatIsNotWrittenAsNames)
{
    const auto parsed = YuiParser::parse (R"yui(Component {
    Label { class: on ? "a" : "b"; }
    Label { id: first + second; }
    Label { id: caption; class: "big, bold title"; }
    Label { id: "named"; class: plain; }
})yui");

    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parsed.document).wasOk());

    StringArray located;

    for (const auto& diagnostic : builder.getDiagnostics())
        located.add (parsed.sourceMap.locate (diagnostic).toString());

    EXPECT_EQ (located, StringArray ({ "2:13: Label.class: expected a name or names in a string, such as \"knob large\"", "3:13: Label.id: expected a name, such as knob" }));

    // A value that is not names gives the component none
    auto* root = builder.getRoot();
    ASSERT_EQ (root->getNumChildComponents(), 4);
    EXPECT_TRUE (root->getChildComponent (0)->getStyleClasses().isEmpty());
    EXPECT_EQ (root->getChildComponent (1)->getComponentID(), String());

    auto* caption = builder.findById ("caption");
    ASSERT_NE (caption, nullptr);
    EXPECT_EQ (caption->getStyleClasses().size(), 3);

    auto* named = builder.findById ("named");
    ASSERT_NE (named, nullptr);
    EXPECT_EQ (named->getStyleClasses().size(), 1);
}

TEST_F (YuiBuilderTests, ReportsDrawingsAndShadersDeclaredTwiceAndPropertiesDeclaredWithAnotherType)
{
    // The same property with the same type, as the two shaders declare it, is no problem
    EXPECT_EQ (buildAndLocate (R"yui(drawing Dot { property size: number = 4; circle { radius: size; } }
drawing Dot { property size: color = red; }
shader Crt { fragment: "crt.frag"; property amount: number = 1; }
shader Crt { fragment: "other.frag"; property amount: number = 2; }
Component { })yui",
                               true),
               StringArray ({ "2:1: The drawing 'Dot' is already declared",
                              "2:24: The property 'size' is already declared with another type",
                              "4:1: The shader 'Crt' is already declared" }));
}

TEST_F (YuiBuilderTests, TellsOnceAboutTheChangesOfTheTreeWhenTheyAreApplied)
{
    YuiBuilder builder;
    auto document = parse ("Component { id: panel; Label { id: caption; } }");
    ASSERT_TRUE (builder.build (document).wasOk());

    int numMutations = 0;
    builder.onTreeMutated = [&numMutations]
    {
        ++numMutations;
    };

    {
        auto transaction = document.beginTransaction();
        transaction.setProperty ("first", 1);
        transaction.setProperty ("second", 2);
        transaction.addChild (DataTree ("extra"));
    }

    {
        DataTree::BatchTransaction batch;
        batch.edit (document).setProperty ("first", 3);
        batch.edit (document.getChild (0)).setProperty ("third", 4);
    }

    // The changes wait for the message loop, which a test does not have
    EXPECT_EQ (0, numMutations);

    builder.applyPendingChanges();
    EXPECT_EQ (1, numMutations);

    // With nothing changed there is nothing to tell
    builder.applyPendingChanges();
    EXPECT_EQ (1, numMutations);

    builder.setIgnoreTreeChanges (true);

    {
        auto transaction = document.beginTransaction();
        transaction.setProperty ("first", 5);
    }

    builder.applyPendingChanges();
    EXPECT_EQ (1, numMutations);
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
            Label { id: caption; class: "big"; text: "a"; flexGrow: 2; visible: false; }
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
    EXPECT_TRUE (caption->isVisible());

    auto* button = builder.findById<TextButton> ("button");
    ASSERT_NE (button, nullptr);
    EXPECT_TRUE (button->onClick == nullptr);
    EXPECT_EQ (presses, 0);
}

TEST_F (YuiBuilderTests, RebuildingResetsWhatAComponentIsGivenByWhereItIs)
{
    auto document = parse (R"yui(
        Component {
            width: 406; height: 100;
            ResizableDivider { id: split; position: 120; }
            TabComponent { id: tabs; selectedTab: "second"; Column { id: first; } Column { id: second; } }
            Row { width: 120; height: 40; Label { id: item; flexGrow: 1; flexBasis: 0; width: 10; } }
        }
    )yui");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());

    auto* split = builder.findById<ResizableDivider> ("split");
    auto* tabs = builder.findById<TabComponent> ("tabs");
    auto* item = builder.findById ("item");
    ASSERT_NE (split, nullptr);
    ASSERT_NE (tabs, nullptr);
    ASSERT_NE (item, nullptr);
    EXPECT_FLOAT_EQ (split->getPosition(), 120.0f);
    EXPECT_EQ (tabs->getTabBar().getSelectedTabId().toString(), "second");

    YuiTreeDiff::apply (document, parse (R"yui(
        Component {
            width: 406; height: 100;
            ResizableDivider { id: split; }
            TabComponent { id: tabs; Column { id: first; } Column { id: second; } }
            Row { width: 120; height: 40; Label { id: item; flexGrow: 1; flexBasis: 0; } }
        }
    )yui"));
    ASSERT_TRUE (builder.build (document).wasOk());

    // A new instance, in no parent, tells none of them: the middle of the extent, the first page, the bounds of the row
    EXPECT_FLOAT_EQ (split->getPosition(), 200.0f);
    EXPECT_EQ (tabs->getTabBar().getSelectedTabId().toString(), "first");
    EXPECT_EQ (item->getBounds(), Rectangle<float> (0.0f, 0.0f, 120.0f, 40.0f));
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

TEST_F (YuiBuilderTests, RebuildingKeepsEachComponentOfARepeatAndOfADeclaredComponentWhereItsElementIs)
{
    auto document = parse (R"yui(
        component Pair : Row { Label { } Label { } }

        Column {
            id: list;
            Label { id: first; }
            repeat r in 0 .. 2 { repeat c in 0 .. 3 { Slider { } } }
            Pair { id: left; }
            Pair { id: right; }
            Label { id: middle; }
            TextButton { id: last; }
        })yui");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* list = builder.findById ("list");
    ASSERT_NE (list, nullptr);

    // The first label, the six sliders of the repeats, the two pairs, the label in the middle and the button
    const auto made = childrenOf (list);
    ASSERT_EQ (made.size(), 11u);
    EXPECT_EQ (made[7], builder.findById ("left"));
    EXPECT_EQ (made[10], builder.findById ("last"));

    const auto inLeft = childrenOf (made[7]);
    const auto inRight = childrenOf (made[8]);
    ASSERT_EQ (inLeft.size(), 2u);
    ASSERT_EQ (inRight.size(), 2u);

    // Built again as it is, every component is the one it was, where it was: the one of each turn of the repeats, and the
    // ones each pair holds, which are made from the same elements
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_EQ (childrenOf (list), made);
    EXPECT_EQ (childrenOf (made[7]), inLeft);
    EXPECT_EQ (childrenOf (made[8]), inRight);

    // The button comes before the repeats, the second pair before the first, and the label in the middle goes
    YuiTreeDiff::apply (document, parse (R"yui(
        component Pair : Row { Label { } Label { } }

        Column {
            id: list;
            Label { id: first; }
            TextButton { id: last; }
            repeat r in 0 .. 2 { repeat c in 0 .. 3 { Slider { } } }
            Pair { id: right; }
            Pair { id: left; }
        })yui"));

    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    const std::vector<Component*> moved { made[0], made[10], made[1], made[2], made[3], made[4], made[5], made[6], made[8], made[7] };
    EXPECT_EQ (childrenOf (list), moved);
    EXPECT_EQ (childrenOf (made[7]), inLeft);
    EXPECT_EQ (childrenOf (made[8]), inRight);
    EXPECT_EQ (builder.findById ("middle"), nullptr);
}

TEST_F (YuiBuilderTests, AppliesTheValuesOfADeclaredComponentInTheOrderItsTypeDeclaresThem)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        component Dial : Slider { property caption: string; }

        Component {
            Dial { id: dial; caption: "Cutoff"; value: 1000; maximum: 20000; minimum: 20; }
            Slider { id: plain; value: 500; maximum: 20000; minimum: 20; }
            Dial { id: other; value: 250; maximum: 300; minimum: 200; caption: "Gain"; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    // The range of the base comes before its value for the instances too, whose type declares more after them
    EXPECT_DOUBLE_EQ (builder.findById<Slider> ("dial")->getValue(), 1000.0);
    EXPECT_DOUBLE_EQ (builder.findById<Slider> ("plain")->getValue(), 500.0);
    EXPECT_DOUBLE_EQ (builder.findById<Slider> ("other")->getValue(), 250.0);
    EXPECT_EQ (builder.findById ("dial")->getProperty (Identifier ("caption")).toString(), "Cutoff");
    EXPECT_EQ (builder.findById ("other")->getProperty (Identifier ("caption")).toString(), "Gain");
}

TEST_F (YuiBuilderTests, RebuildingResetsTheValuesNoLongerWrittenInTheOrderTheTypeDeclaresThem)
{
    auto document = parse ("Component { Slider { id: knob; value: 1000; maximum: 20000; minimum: 20; } }");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());

    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_DOUBLE_EQ (knob->getValue(), 1000.0);

    YuiTreeDiff::apply (document, parse ("Component { Slider { id: knob; } }"));
    ASSERT_TRUE (builder.build (document).wasOk());
    ASSERT_EQ (builder.findById<Slider> ("knob"), knob);

    // The range goes back before the value, as it is applied before it: the value of a new slider then fits in it
    const Slider fresh (Slider::LinearHorizontal);
    EXPECT_DOUBLE_EQ (knob->getRange().start, fresh.getRange().start);
    EXPECT_DOUBLE_EQ (knob->getRange().end, fresh.getRange().end);
    EXPECT_DOUBLE_EQ (knob->getValue(), fresh.getValue());
}

TEST_F (YuiBuilderTests, RebuildingReadsWhatNewComponentsHaveFromOneInstanceOfTheirType)
{
    ComponentTypeRegistry::getInstance().registerType (CountingLabel::getClassTypeInfo());

    auto document = parse ("Column { id: list; repeat i in 0 .. 4 { CountingLabel { text: \"step\"; strokeWidth: 2; } } }");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    const auto labels = childrenOf (builder.findById ("list"));
    ASSERT_EQ (labels.size(), 4u);
    EXPECT_EQ (dynamic_cast<Label*> (labels[3])->getText(), "step");

    YuiTreeDiff::apply (document, parse ("Column { id: list; repeat i in 0 .. 4 { CountingLabel { } } }"));

    const auto numCreatedBefore = CountingLabel::numCreated;
    ASSERT_TRUE (builder.build (document).wasOk());

    // The four labels are kept and lose two values each: what a new label has is read from one label, not from eight
    EXPECT_EQ (childrenOf (builder.findById ("list")), labels);
    EXPECT_EQ (CountingLabel::numCreated - numCreatedBefore, 1);

    const Label fresh;

    for (auto* component : labels)
    {
        auto* label = dynamic_cast<Label*> (component);
        ASSERT_NE (label, nullptr);
        EXPECT_EQ (label->getText(), fresh.getText());
        EXPECT_FLOAT_EQ (label->getStrokeWidth(), fresh.getStrokeWidth());
    }
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* volume = builder.findById<Slider> ("volume");
    auto* readout = builder.findById<Label> ("readout");
    ASSERT_NE (volume, nullptr);
    ASSERT_NE (readout, nullptr);
    EXPECT_NEAR (readout->getOpacity(), 0.5f, 0.01f);

    ComponentTestHelper<YuiBuilder>::triggerMouseDown (*volume, MouseEvent {});
    EXPECT_NEAR (readout->getOpacity(), 1.0f, 0.01f);

    ComponentTestHelper<YuiBuilder>::triggerMouseUp (*volume, MouseEvent {});
    EXPECT_NEAR (readout->getOpacity(), 0.5f, 0.01f);
}

TEST_F (YuiBuilderTests, RepeatsComponentsWithTheirIndex)
{
    YuiBuilder builder;

    const auto result = builder.build (parse (R"yui(
        Row {
            id: row;
            Label { id: first; text: "start"; }

            repeat i in 0 .. 4 {
                Label { text: format ("Step %d", i + 1); opacity: i / 4.0; }
            }

            Label { id: last; text: "end"; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* row = builder.findById ("row");
    ASSERT_NE (row, nullptr);
    ASSERT_EQ (row->getNumChildComponents(), 6);

    // The components of a repeat stand where it is written, in the order of their index
    EXPECT_EQ (row->getChildComponent (0), builder.findById ("first"));
    EXPECT_EQ (row->getChildComponent (5), builder.findById ("last"));

    for (int index = 0; index < 4; ++index)
    {
        auto* label = dynamic_cast<Label*> (row->getChildComponent (index + 1));
        ASSERT_NE (label, nullptr);
        EXPECT_EQ (label->getText(), "Step " + String (index + 1));
        EXPECT_NEAR (label->getOpacity(), static_cast<float> (index) / 4.0f, 0.01f);
    }
}

TEST_F (YuiBuilderTests, ARepeatInsideARepeatReadsTheIndexAroundIt)
{
    YuiBuilder builder;

    const auto result = builder.build (parse (R"yui(
        Column {
            id: grid;

            repeat r in 0 .. 3 {
                repeat c in 0 .. r + 1 {
                    Label { text: format ("%d:%d", r, c); }
                }
            }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* grid = builder.findById ("grid");
    StringArray texts;

    for (int index = 0; index < grid->getNumChildComponents(); ++index)
        texts.add (dynamic_cast<Label*> (grid->getChildComponent (index))->getText());

    EXPECT_EQ (texts, StringArray ({ "0:0", "1:0", "1:1", "2:0", "2:1", "2:2" }));
}

TEST_F (YuiBuilderTests, AnIdInsideARepeatIsLocalToEachOfItsTurns)
{
    YuiBuilder builder;
    Array<var> received;

    builder.setAction ("pick", [&] (const Array<var>& arguments)
    {
        received = arguments;
    });

    const auto result = builder.build (parse (R"yui(
        Column {
            id: list;
            Label { id: title; text: "Gains"; }

            repeat i in 0 .. 2 {
                Row {
                    Slider { id: knob; value: 0.25 + i * 0.5; }
                    Label { text: title.text + format (" %.2f", knob.value); }
                    TextButton { onClick: action ("pick", i, knob.value); }
                }
            }

            Label { id: outside; text: format ("%.2f", knob.value); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // Inside a turn the id names the component of that turn, and the ids around the repeat are read too
    auto* list = builder.findById ("list");
    ASSERT_EQ (list->getNumChildComponents(), 4);

    for (int index = 0; index < 2; ++index)
    {
        auto* row = list->getChildComponent (index + 1);
        ASSERT_EQ (row->getNumChildComponents(), 3);
        EXPECT_EQ (dynamic_cast<Label*> (row->getChildComponent (1))->getText(), index == 0 ? String ("Gains 0.25") : String ("Gains 0.75"));
    }

    // A handler reads the index and the ids of its turn
    dynamic_cast<TextButton*> (list->getChildComponent (2)->getChildComponent (2))->onClick();
    ASSERT_EQ (received.size(), 2);
    EXPECT_EQ (static_cast<int> (received[0]), 1);
    EXPECT_DOUBLE_EQ (static_cast<double> (received[1]), 0.75);

    // Outside the repeat the id is unknown
    EXPECT_EQ (builder.findById ("knob"), nullptr);
    ASSERT_EQ (builder.getDiagnostics().size(), 1u);
    EXPECT_TRUE (builder.getDiagnostics().front().message.contains ("knob")) << builder.getDiagnostics().front().message;
}

TEST_F (YuiBuilderTests, ARepeatFollowsTheModelItsRangeReads)
{
    DataTree tree ("sequencer");
    tree.beginTransaction().setProperty ("length", 2);
    tree.beginTransaction().setProperty ("steps", var (Array<var> { false, true, false, false }));

    DataTreeBindingProvider sequencer (tree, nullptr);

    YuiBuilder builder;
    builder.setModel ("seq", sequencer);

    const auto result = builder.build (parse (R"yui(
        Row {
            id: row;

            repeat i in 0 .. seq.length {
                ToggleButton { text: format ("%d", i + 1); checked <=> seq.steps[i]; }
            }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* row = builder.findById ("row");
    ASSERT_EQ (row->getNumChildComponents(), 2);

    const auto button = [&] (int index)
    {
        return dynamic_cast<ToggleButton*> (builder.findById ("row")->getChildComponent (index));
    };

    // Each button is linked to its element of the list
    EXPECT_FALSE (button (0)->getToggleState());
    EXPECT_TRUE (button (1)->getToggleState());

    button (0)->setToggleState (true);
    EXPECT_EQ (JSON::toString (tree.getProperty ("steps"), true), String ("[true, true, false, false]"));

    // A change of the range is applied once the change is over, and the components that stay are kept
    auto* kept = button (1);
    tree.beginTransaction().setProperty ("length", 4);
    EXPECT_EQ (builder.findById ("row")->getNumChildComponents(), 2);

    builder.applyPendingChanges();
    ASSERT_EQ (builder.findById ("row")->getNumChildComponents(), 4);
    EXPECT_EQ (button (1), kept);
    EXPECT_TRUE (button (0)->getToggleState());
    EXPECT_EQ (button (3)->getButtonText(), String ("4"));

    button (3)->setToggleState (true);
    EXPECT_EQ (JSON::toString (tree.getProperty ("steps"), true), String ("[true, true, false, true]"));

    tree.beginTransaction().setProperty ("length", 1);
    builder.applyPendingChanges();
    EXPECT_EQ (builder.findById ("row")->getNumChildComponents(), 1);
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    // With nothing changed there is nothing to apply
    builder.applyPendingChanges();
    EXPECT_EQ (builder.findById ("row")->getNumChildComponents(), 1);
}

TEST_F (YuiBuilderTests, ARepeatIsHeldToWhatItCanHold)
{
    YuiBuilder builder;

    // More components than a repeat makes are a problem, and only that many are made
    const auto result = builder.build (parse (R"yui(
        Column {
            id: list;
            repeat i in 0 .. 5000 { Component { } }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (builder.findById ("list")->getNumChildComponents(), 1024);
    ASSERT_FALSE (builder.getDiagnostics().empty());
    EXPECT_TRUE (builder.getDiagnostics().front().message.contains ("1024")) << builder.getDiagnostics().front().message;

    // What is not a component has no place in it, and an unknown type is told as anywhere else
    EXPECT_TRUE (builder.build (parse ("Column { repeat i in 0 .. 2 { Unknown { } } }")).failed());

    ASSERT_TRUE (builder.build (parse ("Column { id: list; repeat i in 2 .. 0 { Label { } } }")).wasOk());
    EXPECT_EQ (builder.findById ("list")->getNumChildComponents(), 0);
}

TEST_F (YuiBuilderTests, LaysAContainerOutOnceItsChildrenAreMade)
{
    ComponentTypeRegistry::getInstance().registerType (CountingColumn::getClassTypeInfo());
    CountingColumn::numLayouts = 0;

    YuiBuilder builder;

    const auto result = builder.build (parse (R"yui(
        CountingColumn {
            id: list; width: 100; height: 400;
            repeat i in 0 .. 100 { Component { preferredHeight: 4; } }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // Not at each child added and each value set
    EXPECT_LE (CountingColumn::numLayouts, 4);

    auto* list = builder.findById ("list");
    ASSERT_EQ (list->getNumChildComponents(), 100);
    EXPECT_EQ (list->getChildComponent (99)->getBounds(), Rectangle<float> (0.0f, 396.0f, 100.0f, 4.0f));

    // Nor at each child going away with the interface
    CountingColumn::numLayouts = 0;
    builder.clear();
    EXPECT_EQ (CountingColumn::numLayouts, 0);
}

TEST_F (YuiBuilderTests, ARepeatWorksInTheBodyOfADeclaredComponent)
{
    YuiBuilder builder;

    const auto result = builder.build (parse (R"yui(
        component Dots : Row {
            property count: integer = 3;

            repeat i in 0 .. root.count {
                Label { text: format ("%d", i); }
            }
        }

        Column {
            Dots { id: three; }
            Dots { id: five; count: 5; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());
    EXPECT_EQ (builder.findById ("three")->getNumChildComponents(), 3);
    EXPECT_EQ (builder.findById ("five")->getNumChildComponents(), 5);
}

TEST_F (YuiBuilderTests, AnIntegerPropertyTakesTheNumberOfAModel)
{
    DataTree tree ("mode");
    tree.beginTransaction().setProperty ("group", 3);
    tree.beginTransaction().setProperty ("steps", var (Array<var> { false, false, true }));

    DataTreeBindingProvider mode (tree, nullptr);

    YuiBuilder builder;
    builder.setModel ("mode", mode);

    const auto result = builder.build (parse (R"yui(
        Row {
            ToggleButton { id: button; radioGroup: mode.group; checked <=> mode.steps[mode.group - 1]; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    // The numbers of a model are never integers by type
    auto* button = builder.findById<ToggleButton> ("button");
    EXPECT_EQ (button->getRadioGroupId(), 3);
    EXPECT_TRUE (button->getToggleState());

    tree.beginTransaction().setProperty ("group", 5.9);
    EXPECT_EQ (button->getRadioGroupId(), 5);
}

TEST_F (YuiBuilderTests, ButtonsOfARadioGroupAreExclusive)
{
    YuiBuilder builder;

    const auto result = builder.build (parse (R"yui(
        Row {
            ToggleButton { id: lfo; radioGroup: 1; checked: true; }
            ToggleButton { id: step; radioGroup: 1; }
            ToggleButton { id: free; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* lfo = builder.findById<ToggleButton> ("lfo");
    auto* step = builder.findById<ToggleButton> ("step");
    ASSERT_NE (lfo, nullptr);
    ASSERT_NE (step, nullptr);
    EXPECT_TRUE (lfo->getToggleState());

    step->setToggleState (true);
    EXPECT_FALSE (lfo->getToggleState());
    EXPECT_EQ (builder.findById<ToggleButton> ("free")->getRadioGroupId(), 0);
}

TEST_F (YuiBuilderTests, SetsThePaintMarginOfAComponent)
{
    YuiBuilder builder;

    const auto result = builder.build (parse (R"yui(
        Component {
            Component { id: glow; paintMargin: 12; }
            Component { id: sides; paintMargin: "0 6"; }
            Component { id: plain; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    EXPECT_EQ (builder.findById ("glow")->getPaintMargin(), EdgeSpacing<float> (12.0f));
    EXPECT_EQ (builder.findById ("sides")->getPaintMargin(), EdgeSpacing<float> (6.0f, 0.0f, 6.0f, 0.0f));
    EXPECT_EQ (builder.findById ("plain")->getPaintMargin(), EdgeSpacing<float>());
    EXPECT_EQ (builder.findById ("glow")->getPaintBounds(), builder.findById ("glow")->getLocalBounds().enlarged (12.0f));
}

TEST_F (YuiBuilderTests, RunsTheFallbackForAnActionNobodyRegistered)
{
    YuiBuilder builder;
    StringArray called;
    int numSaved = 0;

    builder.setAction ("save", [&] (const Array<var>&)
    {
        ++numSaved;
    });

    builder.setFallbackAction ([&] (const String& name, const Array<var>& arguments)
    {
        called.add (name + "/" + String (arguments.size()));
    });

    const auto result = builder.build (parse (R"yui(
        Component {
            TextButton { id: save; onClick: action ("save"); }
            TextButton { id: load; onClick: action ("load", "preset", 2); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    builder.findById<TextButton> ("save")->onClick();
    builder.findById<TextButton> ("load")->onClick();

    EXPECT_EQ (numSaved, 1);
    EXPECT_EQ (called, StringArray ({ "load/2" }));

    // Without a fallback, an action nobody registered does nothing
    builder.setFallbackAction (nullptr);
    builder.findById<TextButton> ("load")->onClick();
    EXPECT_EQ (called.size(), 1);
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_EQ (builder.getDiagnostics().size(), 3u) << DataTreeDiagnostic::join (builder.getDiagnostics());
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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

TEST_F (YuiBuilderTests, AnArgumentOfAHandlerHidesTheIndexOfARepeatWithItsName)
{
    YuiBuilder builder;
    Array<var> received;
    builder.setAction ("set", [&] (const Array<var>& arguments) { received = arguments; });

    const auto result = builder.build (parse (R"yui(
        Column {
            id: list;

            repeat i in 0 .. 4 {
                Slider { onValueChanged: (i) => action ("set", i); }
                Slider { onValueChanged: (v) => action ("set", v, i); }
            }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* list = builder.findById ("list");
    ASSERT_EQ (list->getNumChildComponents(), 8);

    // The sliders of the last turn: the name is the value of the first, not the index 3
    dynamic_cast<Slider*> (list->getChildComponent (6))->setValue (0.25);
    ASSERT_EQ (received.size(), 1);
    EXPECT_FLOAT_EQ (static_cast<float> (received[0]), 0.25f);

    // The index is still read where no argument has its name
    dynamic_cast<Slider*> (list->getChildComponent (7))->setValue (0.5);
    ASSERT_EQ (received.size(), 2);
    EXPECT_FLOAT_EQ (static_cast<float> (received[0]), 0.5f);
    EXPECT_EQ (static_cast<int> (received[1]), 3);
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    dynamic_cast<TextButton*> (builder.findById ("panel")->getChildComponent (0))->onClick();
    builder.findById<TextButton> ("idle")->onClick();

    // A call without arguments forwards the signal's, typed as the signal declares them
    EXPECT_EQ (log.joinIntoString ("\n"), "first\ndone\n2\nlast");
}

TEST_F (YuiBuilderTests, AChangeOfTheTreeMadeByAHandlerIsAppliedOnceTheHandlerIsOver)
{
    auto document = parse (R"yui(
        Column {
            TextButton { id: remove; onClick: { action ("remove"); action ("after", caption.text); } }
            Label { id: caption; text: "One"; }
        })yui");

    YuiBuilder builder;
    StringArray log;

    // The handler running is the one of the element this removes
    builder.setAction ("remove", [&] (const Array<var>&)
    {
        document.getChild (0).beginTransaction().removeChild (0);
        log.add (builder.findById ("remove") != nullptr ? "kept" : "destroyed");
    });

    builder.setAction ("after", [&] (const Array<var>& arguments)
    {
        log.add ("after " + arguments[0].toString());
    });

    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    builder.findById<TextButton> ("remove")->onClick();

    // The components are as they were for as long as the handler runs, so it runs the calls it has left
    EXPECT_EQ (log, StringArray ({ "kept", "after One" }));
    EXPECT_NE (builder.findById ("remove"), nullptr);

    builder.applyPendingChanges();
    EXPECT_EQ (builder.findById ("remove"), nullptr);
    EXPECT_EQ (builder.getRoot()->getNumChildComponents(), 1);
}

TEST_F (YuiBuilderTests, AHandlerRunsNoCallAfterTheOneBuildingTheDocumentAgain)
{
    auto document = parse (R"yui(
        Column {
            TextButton { id: reload; onClick: { action ("reload"); action ("after", caption.text); } }
            Label { id: caption; text: "One"; }
        })yui");

    YuiBuilder builder;
    StringArray log;

    // Not what an action should do: the build gives the button another handler while this one runs
    builder.setAction ("reload", [&] (const Array<var>&)
    {
        log.add ("reload");
        EXPECT_TRUE (builder.build (document).wasOk());
    });

    builder.setAction ("after", [&] (const Array<var>&)
    {
        log.add ("after");
    });

    ASSERT_TRUE (builder.build (document).wasOk());

    auto* reload = builder.findById<TextButton> ("reload");
    ASSERT_NE (reload, nullptr);
    reload->onClick();

    // What the handler was made with is gone, so are the calls it had left
    EXPECT_EQ (log, StringArray ({ "reload" }));
    EXPECT_EQ (builder.findById ("reload"), reload);

    reload->onClick();
    EXPECT_EQ (log, StringArray ({ "reload", "reload" }));
}

TEST_F (YuiBuilderTests, AHandlerWhereAComponentIsUsedReplacesTheOneOfItsBlock)
{
    YuiBuilder builder;
    StringArray log;
    builder.setAction ("log", [&] (const Array<var>& arguments) { log.add (arguments[0].toString()); });

    const auto result = builder.build (parse (R"yui(
        component Picker : Column {
            signal onPicked (index: integer);
            onPicked: action ("log", "block");
            TextButton { onClick: root.onPicked (1); }
        }

        Column {
            Picker { id: plain; }
            Picker { id: used; onPicked: action ("log", "use"); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    dynamic_cast<TextButton*> (builder.findById ("plain")->getChildComponent (0))->onClick();
    EXPECT_EQ (log, StringArray ({ "block" }));

    // As for the signal of a widget, which has one handler, and as the values written there win over the ones of the block
    dynamic_cast<TextButton*> (builder.findById ("used")->getChildComponent (0))->onClick();
    EXPECT_EQ (log, StringArray ({ "block", "use" }));
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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

TEST_F (YuiBuilderTests, DeclaresTransformProperties)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        tokens {
            property panelShift: transform = transform.translation (6, 12);
        }

        component Badge : Column {
            property iconTransform: transform = transform.scaling (2);
            property steps: transform[] = [transform.translation (1, 2), transform.translation (3, 4)];
            transform: root.iconTransform;
        }

        Row {
            transform: panelShift;
            Badge { id: defaultBadge; }
            Badge { id: customBadge; iconTransform: transform.translation (15, 25); }
            Badge { id: resetBadge; iconTransform: none; }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    ASSERT_NE (builder.getRoot(), nullptr);
    EXPECT_EQ (builder.getRoot()->getTransform(), AffineTransform::translation (6.0f, 12.0f));

    auto* defaultBadge = builder.findById ("defaultBadge");
    auto* customBadge = builder.findById ("customBadge");
    auto* resetBadge = builder.findById ("resetBadge");
    ASSERT_NE (defaultBadge, nullptr);
    ASSERT_NE (customBadge, nullptr);
    ASSERT_NE (resetBadge, nullptr);

    EXPECT_EQ (defaultBadge->getTransform(), AffineTransform::scaling (2.0f));
    EXPECT_EQ (customBadge->getTransform(), AffineTransform::translation (15.0f, 25.0f));
    EXPECT_TRUE (resetBadge->getTransform().isIdentity());

    const auto* steps = defaultBadge->getProperty (Identifier ("steps")).getArray();
    ASSERT_NE (steps, nullptr);
    ASSERT_EQ (steps->size(), 2);
    EXPECT_EQ (VariantConverter<AffineTransform>::fromVar ((*steps)[1]), AffineTransform::translation (3.0f, 4.0f));
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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.5f);

    ComponentTestHelper<YuiBuilder>::triggerMouseDown (*knob, MouseEvent {});
    knob->setValue (0.9);
    ComponentTestHelper<YuiBuilder>::triggerMouseUp (*knob, MouseEvent {});

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
    ComponentTestHelper<YuiBuilder>::triggerMouseDown (*knob, MouseEvent {});
    other->setValue (0.3);
    ComponentTestHelper<YuiBuilder>::triggerMouseUp (*knob, MouseEvent {});

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

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

TEST_F (YuiBuilderTests, BuildsWindowImageAndAdditionalGuiWidgets)
{
    YuiBuilder builder;
    const auto result = builder.build (parse (R"yui(
        Window {
            id: win;
            title: "Preview";
            width: 320;
            height: 200;
            backgroundColor: #20242a;

            Image {
                id: backdrop;
                fitting: scaleToFill;
                justification: topLeft;
                imageOpacity: 0.75;
            }

            CodeEditor {
                id: editor;
                syntax: cpp;
                scheme: oneDark;
                text: "int x = 1;";
                minimapVisible: true;
            }

            ResizableCorner {
                id: corner;
                cornerSize: 20;
            }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* window = builder.findById<WindowComponent> ("win");
    ASSERT_NE (window, nullptr);
    EXPECT_EQ (window->getTitle(), "Preview");
    EXPECT_EQ (window->findColor (WindowComponent::Style::backgroundColorId), Color (0xff20242a));

    auto* backdrop = builder.findById<ImageComponent> ("backdrop");
    ASSERT_NE (backdrop, nullptr);
    EXPECT_EQ (backdrop->getFitting(), Fitting::scaleToFill);
    EXPECT_EQ (backdrop->getJustification(), Justification::topLeft);
    EXPECT_FLOAT_EQ (backdrop->getImageOpacity(), 0.75f);
    EXPECT_EQ (backdrop->getBounds(), Rectangle<float> (0.0f, 0.0f, 320.0f, 200.0f));

    auto* editor = builder.findById<CodeEditor> ("editor");
    ASSERT_NE (editor, nullptr);
    EXPECT_EQ (editor->getSyntaxDefinition().getName(), "C++");
    EXPECT_EQ (editor->getScheme().getName(), "One Dark");
    EXPECT_EQ (editor->getText(), "int x = 1;");
    EXPECT_TRUE (editor->isMinimapVisible());

    auto* corner = builder.findById<ResizableCorner> ("corner");
    ASSERT_NE (corner, nullptr);
    EXPECT_FLOAT_EQ (corner->getCornerSize(), 20.0f);
}

TEST_F (YuiBuilderTests, SupportsFontAndDrawingPropertiesPlusInstancePaletteAndDrawingAttributes)
{
    theme->setDefaultFont (Font().withHeight (16.0f));

    YuiBuilder builder;
    auto document = parse (R"yui(
        palette Ember {
            colors: "#18181b, #ff7a18";
            mode: dark;
            accent: #ff9933;
        }

        drawing RingDial {
            property ringSize: number = 4;
            circle { center: bounds.center; radius: ringSize; stroke: palette.accent; }
        }

        drawing DotDial {
            circle { center: bounds.center; radius: 6; fill: palette.accent; }
        }

        component Badge : Label {
            property headingFont: font = theme.font.withHeight (24);
            property badgeDrawing: drawing = RingDial;
            text: format ("%.0f", root.headingFont.height);
        }

        Column {
            id: panel;
            palette: Ember;

            Badge { id: defaultBadge; }
            Badge { id: customBadge; headingFont: theme.font.withHeight (32); badgeDrawing: DotDial; }
            Slider { id: dial; drawing: RingDial; }
            Slider { id: plainDial; drawing: none; palette: none; }
        }
    )yui");

    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* panel = builder.findById ("panel");
    ASSERT_NE (panel, nullptr);
    ASSERT_NE (panel->findPalette(), nullptr);
    EXPECT_EQ (panel->findPalette()->getColor (ThemePalette::Role::accent), Color (0xffff9933));

    auto* defaultBadge = builder.findById<Label> ("defaultBadge");
    auto* customBadge = builder.findById<Label> ("customBadge");
    ASSERT_NE (defaultBadge, nullptr);
    ASSERT_NE (customBadge, nullptr);
    EXPECT_EQ (defaultBadge->getText(), "24");
    EXPECT_EQ (customBadge->getText(), "32");
    EXPECT_EQ (defaultBadge->getProperty (Identifier ("badgeDrawing")).toString(), "RingDial");
    EXPECT_EQ (customBadge->getProperty (Identifier ("badgeDrawing")).toString(), "DotDial");

    auto* dial = builder.findById<Slider> ("dial");
    auto* plainDial = builder.findById<Slider> ("plainDial");
    ASSERT_NE (dial, nullptr);
    ASSERT_NE (plainDial, nullptr);
    EXPECT_NE (dynamic_cast<const DrawingStyle*> (dial->getStyle().get()), nullptr);
    EXPECT_EQ (plainDial->getStyle(), nullptr);
}

TEST_F (YuiBuilderTests, EmitsWarningForUndeclaredInstancePropertiesAndErrorsForUnknownInstancePaletteOrDrawing)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse (R"yui(
        Column {
            Slider { id: a; mysterySetting: 123; }
            Slider { id: b; palette: MissingPalette; drawing: MissingDrawing; }
        }
    )yui"))
                     .wasOk());

    const auto& diagnostics = builder.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 3u) << DataTreeDiagnostic::join (diagnostics);

    int warnings = 0;
    int errors = 0;

    for (const auto& diagnostic : diagnostics)
    {
        if (diagnostic.isWarning())
        {
            ++warnings;
            EXPECT_TRUE (diagnostic.message.contains ("mysterySetting")) << diagnostic.message;
        }
        else if (diagnostic.isError())
        {
            ++errors;
        }
    }

    EXPECT_EQ (warnings, 1);
    EXPECT_EQ (errors, 2);
}

TEST_F (YuiBuilderTests, SetsThePropertiesAShaderDeclaresOnAComponentWithTheirType)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse (R"yui(
        shader Crt {
            fragment: "fx/crt.frag";
            property intensity: number = 0.5;
            property steps: integer = 4;
        }

        Column {
            Component { id: screen; intensity: 0.25 + 0.5; steps: 2.7; sharpness: 3; }
        }
    )yui"))
                     .wasOk());

    auto* screen = builder.findById ("screen");
    ASSERT_NE (screen, nullptr);

    // A declared property has its type: the number given to an integer is cut to a whole one
    EXPECT_DOUBLE_EQ (static_cast<double> (screen->findProperty (Identifier ("intensity"))), 0.75);
    EXPECT_DOUBLE_EQ (static_cast<double> (screen->findProperty (Identifier ("steps"))), 2.0);

    // Only the property nothing declares is reported
    const auto& diagnostics = builder.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics.front().isWarning());
    EXPECT_TRUE (diagnostics.front().message.contains ("sharpness")) << diagnostics.front().message;
}

TEST_F (YuiBuilderTests, APropertyNamingADrawingTakesTheNameOfAShaderToo)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse (R"yui(
        shader Hologram { material: "fx/hologram.glsl"; }
        shader Crt { fragment: "fx/crt.frag"; }
        drawing Dial { circle { center: bounds.center; radius: 4; fill: #ffffff; } }

        component Tile : Component {
            property which: string = "nothing";
            property look: drawing = Crt;
        }

        Column {
            Tile { id: byDefault; }
            Tile { id: bare; look: Hologram; }
            Tile { id: drawn; look: Dial; }
            Tile { id: quoted; look: "Hologram"; }
            Tile { id: computed; which: "Dial"; look: which; }
            Tile { id: cleared; look: none; }
            Tile { id: absent; look: Missing; }
        }
    )yui"))
                     .wasOk());

    const auto lookOf = [&builder] (const char* id)
    {
        return builder.findById (id)->getProperty (Identifier ("look")).toString();
    };

    // The name of a shader is written bare as the name of a drawing is, whatever the shader is for
    EXPECT_EQ (lookOf ("byDefault"), String ("Crt"));
    EXPECT_EQ (lookOf ("bare"), String ("Hologram"));
    EXPECT_EQ (lookOf ("drawn"), String ("Dial"));

    // A text and an expression still give one, and `none` still gives none
    EXPECT_EQ (lookOf ("quoted"), String ("Hologram"));
    EXPECT_EQ (lookOf ("computed"), String ("Dial"));
    EXPECT_EQ (lookOf ("cleared"), String());

    // A bare name the document declares neither a drawing nor a shader of is reported
    const auto& diagnostics = builder.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics.front().isError());
    EXPECT_EQ (diagnostics.front().message, "Tile.look: unknown drawing or shader 'Missing'");
}

TEST_F (YuiBuilderTests, ADrawingSelectedOnAnElementKnowsTheShadersItUsesByTheirBlocks)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse (R"yui(
        shader Plasma { fragment: "fx/plasma.frag"; property speed: number = 1; }
        shader Hologram { material: "fx/hologram.glsl"; }

        drawing Screen { rect { bounds: bounds; fill: #202020; } use Plasma { speed: 2; } }
        drawing Lit { use Hologram { } }
        drawing Wrong { use Plasma { gain: 2; } }
        drawing Lost { use Glow { } }

        Column {
            Component { id: screen; drawing: Screen; }
            Component { id: lit; drawing: Lit; }
            Component { id: wrong; drawing: Wrong; }
            Component { id: lost; drawing: Lost; }
        }
    )yui"))
                     .wasOk());

    // A builder loads no shader. The drawing is made knowing the block of the one it uses, and runs nothing there until
    // the stylesheet styling its component gives it the shader
    const auto installed = builder.findById ("screen")->getStyle();
    const auto* style = dynamic_cast<const DrawingStyle*> (installed.get());
    ASSERT_NE (style, nullptr);

    const auto shaders = style->getShaders();
    ASSERT_EQ (shaders.size(), 1u);
    EXPECT_EQ (shaders.front().block.getProperty ("name").toString(), String ("Plasma"));
    EXPECT_EQ (shaders.front().shader, nullptr);

    // What a block tells is checked: a material, a property it does not declare. A name that is no shader is no drawing either
    StringArray messages;

    for (const auto& diagnostic : builder.getDiagnostics())
        messages.add (diagnostic.message);

    EXPECT_EQ (messages.size(), 3) << messages.joinIntoString ("\n");
    EXPECT_TRUE (messages.contains ("Component.drawing: Lit/use[0]: the shader 'Hologram' is a material, a drawing takes a shader with a 'fragment'"));
    EXPECT_TRUE (messages.contains ("Component.drawing: Wrong/use[0]: unknown property 'gain'"));
    EXPECT_TRUE (messages.contains ("Component.drawing: Lost/use[0]: unknown drawing 'Glow'"));

    EXPECT_EQ (builder.findById ("lit")->getStyle(), nullptr);
    EXPECT_EQ (builder.findById ("wrong")->getStyle(), nullptr);
}

TEST_F (YuiBuilderTests, LiveDataTreeMutationsPatchBuiltComponentsWithoutManualRebuild)
{
    auto document = parse (R"yui(
        Column {
            id: rootCol;
            Slider { id: knob; value: 0.25; }
            Label { id: first; text: "One"; }
            Label { id: second; text: "Two"; }
        }
    )yui");

    YuiBuilder builder;
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* rootCol = builder.findById ("rootCol");
    auto* knob = builder.findById<Slider> ("knob");
    auto* first = builder.findById<Label> ("first");
    auto* second = builder.findById<Label> ("second");
    ASSERT_NE (rootCol, nullptr);
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    // User state on knob survives unrelated live DataTree mutations.
    knob->setValue (0.75);

    auto rootTree = document.getChild (0);
    auto firstTree = rootTree.getChild (1);

    // 1. Property mutation updates Label in place without recreating Slider or losing user value.
    firstTree.beginTransaction().setProperty ("text", "\"Updated\"");

    // The components follow once the change is over, from the message loop, which a test stands for
    EXPECT_EQ (first->getText(), "One");
    builder.applyPendingChanges();

    EXPECT_EQ (builder.findById<Slider> ("knob"), knob);
    EXPECT_DOUBLE_EQ (knob->getValue(), 0.75);
    EXPECT_EQ (builder.findById<Label> ("first"), first);
    EXPECT_EQ (first->getText(), "Updated");

    // 2. Adding a child DataTree instantiates and attaches the new child automatically.
    DataTree addedTree ("Label");
    {
        auto tx = addedTree.beginTransaction();
        tx.setProperty ("id", "third");
        tx.setProperty ("text", "\"Three\"");
    }
    rootTree.beginTransaction().addChild (addedTree);
    builder.applyPendingChanges();

    auto* third = builder.findById<Label> ("third");
    ASSERT_NE (third, nullptr);
    EXPECT_EQ (third->getText(), "Three");
    EXPECT_EQ (rootCol->getNumChildComponents(), 4);

    // Mutating a property on the newly added child is also observed automatically.
    addedTree.beginTransaction().setProperty ("text", "\"Three!\"");
    builder.applyPendingChanges();
    EXPECT_EQ (third->getText(), "Three!");

    // 3. Moving a child DataTree reorders the child Component in its parent.
    rootTree.beginTransaction().moveChild (3, 1);
    builder.applyPendingChanges();
    EXPECT_EQ (rootCol->getChildComponent (1), third);

    // 4. Removing a child DataTree destroys only that child Component.
    rootTree.beginTransaction().removeChild (1);
    builder.applyPendingChanges();
    EXPECT_EQ (builder.findById ("third"), nullptr);
    EXPECT_EQ (rootCol->getNumChildComponents(), 3);
    EXPECT_EQ (builder.findById<Slider> ("knob"), knob);
}

TEST_F (YuiBuilderTests, LoadsFontFilesRelativeToTheBaseDirectory)
{
    YuiBuilder builder;
    builder.setBaseDirectory (File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts"));

    const auto result = builder.build (parse (R"yui(
        component Titled : Component {
            property title: font = font ("YupColrTest.ttf", 14);
        }

        Row {
            Titled { id: plain; }
            Titled { id: large; title: font ("YupColrTest.ttf", 20); }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    const auto titleOf = [&] (const String& id) { return VariantConverter<Font>::fromVar (builder.findById (id)->findProperty (Identifier ("title"))); };

    const auto plain = titleOf ("plain");
    EXPECT_FALSE (plain.isEmpty());
    EXPECT_FLOAT_EQ (plain.getHeight(), 14.0f);

    const auto large = titleOf ("large");
    EXPECT_EQ (large.getFont().get(), plain.getFont().get());
    EXPECT_FLOAT_EQ (large.getHeight(), 20.0f);
}

TEST_F (YuiBuilderTests, AnInitializerSetsUpTheNewComponentsOfATypeBeforeTheirValues)
{
    YuiBuilder builder;
    StringArray initialized;

    builder.setInitializer<Label> ([&] (Label& label)
    {
        initialized.add (label.getComponentID());
        label.setText ("from the code");
    });

    auto document = parse (R"yui(
        Component {
            Label { id: plain; }
            Label { id: written; text: "from the document"; }
            Slider { id: knob; }
        })yui");

    ASSERT_TRUE (builder.build (document).wasOk());

    // The components of the type are set up once they have their id, and the document has the last word
    EXPECT_EQ (initialized.joinIntoString (", "), "plain, written");
    EXPECT_EQ (builder.findById<Label> ("plain")->getText(), "from the code");
    EXPECT_EQ (builder.findById<Label> ("written")->getText(), "from the document");

    builder.findById<Label> ("plain")->setText ("from the user");

    YuiTreeDiff::apply (document, parse (R"yui(
        Component {
            Label { id: plain; }
            Label { id: renamed; text: "from the document"; }
            Slider { id: knob; }
            Label { id: added; }
        })yui"));

    ASSERT_TRUE (builder.build (document).wasOk());

    // A kept component isn't set up again, unless its id changed, and a new one is
    EXPECT_EQ (initialized.size(), 4);
    EXPECT_TRUE (initialized.contains ("renamed"));
    EXPECT_TRUE (initialized.contains ("added"));
    EXPECT_EQ (builder.findById<Label> ("plain")->getText(), "from the user");
    EXPECT_EQ (builder.findById<Label> ("renamed")->getText(), "from the document");
    EXPECT_EQ (builder.findById<Label> ("added")->getText(), "from the code");
}

TEST_F (YuiBuilderTests, AnInitializerOfAnIdRunsAfterTheOnesOfItsType)
{
    YuiBuilder builder;
    int numStrips = 0;

    // Set first, it still runs last
    builder.setInitializer<Label> ("name", [] (Label& label) { label.setText (label.getText() + " named"); });
    builder.setInitializer<Label> ([] (Label& label) { label.setText ("any"); });
    builder.setInitializer<RowComponent> ("strip", [&] (RowComponent&) { ++numStrips; });

    const auto result = builder.build (parse (R"yui(
        component Strip : Row {
            Label { id: name; }
        }

        Column {
            id: list;
            Strip { id: strip; }
            Label { id: other; }

            repeat i in 0 .. 2 {
                Label { id: name; }
            }
        })yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* list = builder.findById ("list");
    ASSERT_EQ (list->getNumChildComponents(), 4);

    const auto textOf = [] (Component* label) { return dynamic_cast<Label*> (label)->getText(); };

    // An id reaches the components inside a declared component and a repeat, and a type the ones declared from it
    EXPECT_EQ (numStrips, 1);
    EXPECT_EQ (textOf (list->getChildComponent (0)->getChildComponent (0)), "any named");
    EXPECT_EQ (textOf (list->getChildComponent (1)), "any");
    EXPECT_EQ (textOf (list->getChildComponent (2)), "any named");
    EXPECT_EQ (textOf (list->getChildComponent (3)), "any named");
}

TEST_F (YuiBuilderTests, AnInitializerSetAfterABuildSetsUpTheComponentsAlreadyBuilt)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse ("Component { Label { id: title; } Label { id: other; } }")).wasOk());

    auto* title = builder.findById<Label> ("title");
    auto* other = builder.findById<Label> ("other");

    builder.setInitializer<Label> ("title", [] (Label& label) { label.setText ("the title"); });
    EXPECT_EQ (title->getText(), "the title");
    EXPECT_EQ (other->getText(), String());

    // The one of the id keeps the last word over the one of the type set after it
    builder.setInitializer<Label> ([] (Label& label) { label.setText ("any"); });
    EXPECT_EQ (title->getText(), "the title");
    EXPECT_EQ (other->getText(), "any");
}

TEST_F (YuiBuilderTests, AnInitializerIsReplacedRemovedAndKeptByClear)
{
    YuiBuilder builder;
    StringArray calls;

    builder.setInitializer<Label> ([&] (Label&) { calls.add ("first"); });
    builder.setInitializer<Label> ([&] (Label&) { calls.add ("second"); });
    builder.setInitializer<Slider> ([&] (Slider&) { calls.add ("slider"); });
    builder.setInitializer<Slider> (nullptr);

    const auto document = parse ("Component { Label { } Slider { } }");
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_EQ (calls.joinIntoString (", "), "second");

    builder.clear();
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_EQ (calls.joinIntoString (", "), "second, second");

    builder.setInitializer<Label> (nullptr);
    builder.clear();
    ASSERT_TRUE (builder.build (document).wasOk());
    EXPECT_EQ (calls.size(), 2);
}
