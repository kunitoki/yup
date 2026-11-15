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

class StyleSelectorTests : public ::testing::Test
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

    static StyleSelector parse (const String& text)
    {
        auto parsed = StyleSelector::parse (text);
        EXPECT_FALSE (parsed.failed()) << parsed.getErrorMessage();
        return parsed.wasOk() ? parsed.getReference() : StyleSelector {};
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (StyleSelectorTests, MatchesTypeAndBaseTypes)
{
    Slider slider (Slider::LinearHorizontal, "slider");

    EXPECT_TRUE (parse ("Slider").matches (slider));
    EXPECT_TRUE (parse ("Component").matches (slider));
    EXPECT_TRUE (parse ("*").matches (slider));
    EXPECT_FALSE (parse ("TextButton").matches (slider));
}

TEST_F (StyleSelectorTests, MatchesIdentifierAndClasses)
{
    Component component ("panel");
    component.addStyleClass (Identifier ("dark"));

    EXPECT_TRUE (parse ("#panel").matches (component));
    EXPECT_TRUE (parse (".dark").matches (component));
    EXPECT_TRUE (parse ("Component#panel.dark").matches (component));
    EXPECT_FALSE (parse (".light").matches (component));

    component.removeStyleClass (Identifier ("dark"));
    EXPECT_FALSE (parse (".dark").matches (component));
}

TEST_F (StyleSelectorTests, MatchesPseudoClasses)
{
    Component component ("panel");

    EXPECT_TRUE (parse ("Component:enabled").matches (component));
    EXPECT_FALSE (parse ("Component:disabled").matches (component));

    component.setEnabled (false);

    EXPECT_TRUE (parse ("Component:disabled").matches (component));
    EXPECT_FALSE (parse ("Component:enabled").matches (component));
}

TEST_F (StyleSelectorTests, MatchesAttributeSelectors)
{
    Component component ("panel");
    component.setProperty (Identifier ("title"), "hello");

    EXPECT_TRUE (parse ("[title]").matches (component));
    EXPECT_TRUE (parse ("[title=\"hello\"]").matches (component));
    EXPECT_TRUE (parse ("[title!=\"other\"]").matches (component));
    EXPECT_FALSE (parse ("[title=\"other\"]").matches (component));
}

TEST_F (StyleSelectorTests, MatchesDescendantAndChildCombinators)
{
    Component root ("root");
    Component middle ("middle");
    Component leaf ("leaf");

    root.addStyleClass (Identifier ("panel"));
    root.addChildComponent (middle);
    middle.addChildComponent (leaf);

    EXPECT_TRUE (parse (".panel Component").matches (leaf));
    EXPECT_TRUE (parse (".panel #leaf").matches (leaf));
    EXPECT_FALSE (parse (".panel > #leaf").matches (leaf));
    EXPECT_TRUE (parse (".panel > #middle").matches (middle));
}

TEST_F (StyleSelectorTests, DescendantWalkStopsAtStyleRoot)
{
    Component outer ("outer");
    Component inner ("inner");
    Component leaf ("leaf");

    outer.addStyleClass (Identifier ("panel"));
    outer.addChildComponent (inner);
    inner.addChildComponent (leaf);

    EXPECT_TRUE (parse (".panel #leaf").matches (leaf, &outer));
    EXPECT_FALSE (parse (".panel #leaf").matches (leaf, &inner));
}

TEST_F (StyleSelectorTests, RootMatchesOnlyTheStyleRoot)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    EXPECT_TRUE (parse (":root").matches (root, &root));
    EXPECT_FALSE (parse (":root").matches (child, &root));
}

TEST_F (StyleSelectorTests, ComputesCssSpecificity)
{
    EXPECT_EQ (parse ("Slider").getSpecificity(), 1);
    EXPECT_EQ (parse (".panel").getSpecificity(), 100);
    EXPECT_EQ (parse ("#panel").getSpecificity(), 10000);
    EXPECT_EQ (parse ("Slider.panel:hover").getSpecificity(), 201);
    EXPECT_GT (parse ("#panel Slider").getSpecificity(), parse (".panel Slider").getSpecificity());
}

TEST_F (StyleSelectorTests, ReportsReferencedProperties)
{
    const auto selector = parse ("Slider[title=\"a\"]:hover:focused:disabled");

    EXPECT_TRUE (selector.getReferencedProperties().contains (Identifier ("title")));
    EXPECT_TRUE (selector.getReferencedProperties().contains (Identifier ("hovered")));
    EXPECT_TRUE (selector.getReferencedProperties().contains (Identifier ("focused")));
    EXPECT_TRUE (selector.getReferencedProperties().contains (Identifier ("enabled")));
}

TEST_F (StyleSelectorTests, AcceptsFocusedAndHoveredAliases)
{
    EXPECT_TRUE (StyleSelector::parse ("Slider:focused").wasOk());
    EXPECT_TRUE (StyleSelector::parse ("Slider:hovered").wasOk());
    EXPECT_TRUE (StyleSelector::parse ("Slider:active").wasOk());
    EXPECT_TRUE (StyleSelector::parse ("Component:focus-within").wasOk());

    EXPECT_TRUE (parse ("Slider:active").getReferencedProperties().contains (Identifier ("pressed")));
    EXPECT_TRUE (parse ("Component:focus-within").getReferencedProperties().contains (Identifier ("focus-within")));
}

TEST_F (StyleSelectorTests, CombinatorsDoNotEscapeTheStyleRoot)
{
    Component outer ("outer");
    outer.addStyleClass (Identifier ("card"));

    Component styleRoot ("panel");
    outer.addChildComponent (styleRoot);

    Slider knob (Slider::LinearHorizontal, "knob");
    styleRoot.addChildComponent (knob);

    EXPECT_FALSE (parse (".card > #panel").matches (styleRoot, &styleRoot));
    EXPECT_FALSE (parse ("#outer > #panel").matches (styleRoot, &styleRoot));
    EXPECT_FALSE (parse ("#outer #knob").matches (knob, &styleRoot));
    EXPECT_FALSE (parse (".card #knob").matches (knob, &styleRoot));
    EXPECT_TRUE (parse ("#panel > #knob").matches (knob, &styleRoot));
    EXPECT_TRUE (parse ("#panel #knob").matches (knob, &styleRoot));
}

TEST_F (StyleSelectorTests, FailsForInvalidText)
{
    EXPECT_TRUE (StyleSelector::parse ("").failed());
    EXPECT_TRUE (StyleSelector::parse (".").failed());
    EXPECT_TRUE (StyleSelector::parse ("Slider >").failed());
}
