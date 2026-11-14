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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_EQ (builder.getDiagnostics().size(), 3);
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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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

TEST_F (YuiBuilderTests, ReportsLinksToUnknownTargets)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse ("Slider { id: gain; value <=> model.gain; }")).wasOk());

    ASSERT_EQ (builder.getDiagnostics().size(), 1);
    EXPECT_EQ (builder.getDiagnostics()[0], "Slider.value: unknown element 'model'");
}

TEST_F (YuiBuilderTests, RebuildingReplacesTheTree)
{
    YuiBuilder builder;
    ASSERT_TRUE (builder.build (parse ("Component { id: first; }")).wasOk());
    ASSERT_TRUE (builder.build (parse ("Component { id: second; }")).wasOk());

    EXPECT_EQ (builder.getRoot()->getComponentID(), "second");
    EXPECT_EQ (builder.findById ("first"), nullptr);
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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_EQ (builder.getDiagnostics().size(), 3) << builder.getDiagnostics().joinIntoString ("\n");
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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

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
    EXPECT_TRUE (builder.getDiagnostics().isEmpty()) << builder.getDiagnostics().joinIntoString ("\n");

    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.5f);

    ComponentTestHelper<Slider>::triggerMouseDown (*knob, MouseEvent {});
    knob->setValue (0.9);
    ComponentTestHelper<Slider>::triggerMouseUp (*knob, MouseEvent {});

    EXPECT_FLOAT_EQ (static_cast<float> (provider.gain), 0.9f);
    EXPECT_EQ (provider.gestures.joinIntoString (", "), "begin gain, end gain");
}
