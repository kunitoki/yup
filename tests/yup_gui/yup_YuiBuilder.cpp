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
