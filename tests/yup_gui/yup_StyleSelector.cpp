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

TEST_F (StyleSelectorTests, TellsWhatItsLastCompoundRequires)
{
    const auto selector = parse (".panel > Slider#gain.knob.wide:hover");

    EXPECT_EQ (selector.getSubjectId(), "gain");
    EXPECT_EQ (selector.getSubjectType(), Identifier ("Slider"));
    ASSERT_EQ (selector.getSubjectClasses().size(), 2);
    EXPECT_EQ (selector.getSubjectClasses()[0], Identifier ("knob"));
    EXPECT_EQ (selector.getSubjectClasses()[1], Identifier ("wide"));

    // What a compound before a combinator requires is asked of another component
    for (const auto* text : { "#panel *", ".panel :hover", "Slider > [title=\"a\"]", ":root" })
    {
        const auto other = parse (text);

        EXPECT_TRUE (other.getSubjectId().isEmpty()) << text;
        EXPECT_TRUE (other.getSubjectClasses().isEmpty()) << text;
        EXPECT_FALSE (other.getSubjectType().isValid()) << text;
    }

    const StyleSelector empty {};
    EXPECT_TRUE (empty.getSubjectId().isEmpty());
    EXPECT_TRUE (empty.getSubjectClasses().isEmpty());
    EXPECT_FALSE (empty.getSubjectType().isValid());
}

TEST_F (StyleSelectorTests, TellsThePropertiesSelectingTheDescendantsOfAComponent)
{
    // Read on the component the selector matches
    EXPECT_FALSE (parse ("Slider:hover").selectsDescendantsBy (Identifier ("hovered")));
    EXPECT_FALSE (parse (".panel Slider:hover:disabled").selectsDescendantsBy (Identifier ("hovered")));
    EXPECT_FALSE (parse (".panel Slider:hover:disabled").selectsDescendantsBy (Identifier ("enabled")));
    EXPECT_FALSE (parse ("ToggleButton:checked").selectsDescendantsBy (Identifier ("checked")));
    EXPECT_FALSE (parse ("#gain").selectsDescendantsBy (Identifier ("id")));
    EXPECT_FALSE (parse (".row:focus-within .icon:hover").selectsDescendantsBy (Identifier ("hovered")));

    // Read on a component the matched one is inside of
    EXPECT_TRUE (parse (".row:hover .icon").selectsDescendantsBy (Identifier ("hovered")));
    EXPECT_TRUE (parse (".row:active > .icon").selectsDescendantsBy (Identifier ("pressed")));
    EXPECT_TRUE (parse (".row:focus-within .icon:hover").selectsDescendantsBy (Identifier ("focus-within")));
    EXPECT_TRUE (parse (".row:hover .icon:hover").selectsDescendantsBy (Identifier ("hovered")));
    EXPECT_TRUE (parse ("ToggleButton:checked Label").selectsDescendantsBy (Identifier ("checked")));
    EXPECT_TRUE (parse ("#gain Label").selectsDescendantsBy (Identifier ("id")));

    // A component with no value of its own for an attribute takes the one of a parent
    EXPECT_TRUE (parse ("Slider[title=\"a\"]").selectsDescendantsBy (Identifier ("title")));
    EXPECT_TRUE (parse ("[title] Slider").selectsDescendantsBy (Identifier ("title")));

    EXPECT_FALSE (parse (".row:hover .icon").selectsDescendantsBy (Identifier ("pressed")));
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
    EXPECT_TRUE (StyleSelector::parse ("TextButton, ComboBox").failed());
}

TEST_F (StyleSelectorTests, ParsesCommaSeparatedSelectorLists)
{
    const auto list = StyleSelector::parseList ("TextButton, ComboBox:hover:enabled,\n    #cutoff");
    ASSERT_TRUE (list.wasOk()) << list.getErrorMessage();

    const auto& selectors = list.getReference();
    ASSERT_EQ (selectors.size(), 3u);
    EXPECT_EQ (selectors[0].toString(), "TextButton");
    EXPECT_EQ (selectors[0].getSpecificity(), 1);
    EXPECT_EQ (selectors[1].toString(), "ComboBox:hover:enabled");
    EXPECT_EQ (selectors[1].getSpecificity(), 201);
    EXPECT_EQ (selectors[2].toString(), "#cutoff");
    EXPECT_EQ (selectors[2].getSpecificity(), 10000);

    const auto withQuotedComma = StyleSelector::parseList ("Label[text=\"Hello, world\"], Slider[title='a, b']");
    ASSERT_TRUE (withQuotedComma.wasOk()) << withQuotedComma.getErrorMessage();
    ASSERT_EQ (withQuotedComma.getReference().size(), 2u);

    Label label;
    label.setText ("Hello, world", dontSendNotification);
    EXPECT_TRUE (withQuotedComma.getReference()[0].matches (label));

    EXPECT_TRUE (StyleSelector::parseList ("").failed());
    EXPECT_TRUE (StyleSelector::parseList (", Slider").failed());
    EXPECT_TRUE (StyleSelector::parseList ("Slider, ").failed());
    EXPECT_TRUE (StyleSelector::parseList ("Slider, , Label").failed());
}

TEST_F (StyleSelectorTests, ListsThePseudoClassesOfEveryComponent)
{
    const auto names = StyleSelector::getPseudoClassNames();

    for (const auto* name : { "root", "hover", "pressed", "focus", "focus-within", "disabled", "enabled" })
        EXPECT_TRUE (names.contains (name)) << name;

    for (const auto& name : names)
        EXPECT_TRUE (StyleSelector::parse ("Slider:" + name).wasOk()) << name.toStdString();
}
