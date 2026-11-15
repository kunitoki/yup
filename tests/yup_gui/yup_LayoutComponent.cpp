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

#include <clocale>

using namespace yup;

class LayoutComponentTests : public ::testing::Test
{
protected:
    class TestLayout : public LayoutComponent
    {
    public:
        using LayoutComponent::parseSpacing;

        int layoutCount = 0;
        Rectangle<float> lastArea;

    protected:
        void performLayout (Rectangle<float> area) override
        {
            ++layoutCount;
            lastArea = area;
        }
    };

    TestLayout layout;
};

TEST_F (LayoutComponentTests, LaysOutInsideThePadding)
{
    layout.setBounds (0, 0, 100, 60);
    layout.setProperty (LayoutComponent::Properties::paddingId, "5 10 15 20");

    EXPECT_EQ (layout.lastArea, Rectangle<float> (20.0f, 5.0f, 70.0f, 40.0f));
    EXPECT_EQ (layout.getPadding(), EdgeSpacing<float> (20.0f, 5.0f, 10.0f, 15.0f));
}

TEST_F (LayoutComponentTests, LaysOutAgainWhenAnItemPropertyChanges)
{
    Component child;
    layout.addAndMakeVisible (child);
    layout.setBounds (0, 0, 100, 60);

    const auto before = layout.layoutCount;
    child.setProperty (Identifier ("flexGrow"), 1.0);

    EXPECT_GT (layout.layoutCount, before);
}

TEST_F (LayoutComponentTests, LaysOutAgainWhenAChildIsHidden)
{
    Component child;
    layout.addAndMakeVisible (child);
    layout.setBounds (0, 0, 100, 60);

    const auto before = layout.layoutCount;
    child.setVisible (false);

    EXPECT_GT (layout.layoutCount, before);
}

TEST_F (LayoutComponentTests, DoesNotInheritItemPropertiesFromTheParent)
{
    Component child;
    layout.addAndMakeVisible (child);

    layout.setProperty (Identifier ("flexGrow"), 3.0);

    EXPECT_FLOAT_EQ (layout.findPropertyAs<float> (Identifier ("flexGrow")), 3.0f);
    EXPECT_FLOAT_EQ (child.findPropertyAs<float> (Identifier ("flexGrow")), 0.0f);
}

TEST_F (LayoutComponentTests, RejectsSpacingWithExtraOrMalformedTokens)
{
    EXPECT_EQ (TestLayout::parseSpacing ("8px 16px"), EdgeSpacing<float> (16.0f, 8.0f));
    EXPECT_EQ (TestLayout::parseSpacing ("1 2 3 4 5"), EdgeSpacing<float>());
    EXPECT_EQ (TestLayout::parseSpacing ("8 bogus"), EdgeSpacing<float>());
    EXPECT_EQ (TestLayout::parseSpacing ("abc"), EdgeSpacing<float>());
    EXPECT_EQ (TestLayout::parseSpacing ("px"), EdgeSpacing<float>());
    EXPECT_EQ (TestLayout::parseSpacing ("0x10px"), EdgeSpacing<float>());
    EXPECT_EQ (TestLayout::parseSpacing ("nan"), EdgeSpacing<float>());
    EXPECT_EQ (TestLayout::parseSpacing ("inf"), EdgeSpacing<float>());
}

TEST_F (LayoutComponentTests, ParsesDecimalSpacingRegardlessOfCLocale)
{
    const char* current = std::setlocale (LC_NUMERIC, nullptr);
    const String savedLocale = current != nullptr ? String (current) : String ("C");
    const ScopeGuard restoreLocale { [&savedLocale]
    {
        std::setlocale (LC_NUMERIC, savedLocale.toRawUTF8());
    } };

    for (const char* candidate : { "de_DE.UTF-8", "fr_FR.UTF-8", "de_DE", "French_France.1252" })
    {
        if (std::setlocale (LC_NUMERIC, candidate) != nullptr)
            break;
    }

    EXPECT_EQ (TestLayout::parseSpacing ("1.5px 2.25"), EdgeSpacing<float> (2.25f, 1.5f));
}

TEST_F (LayoutComponentTests, StandardContainersSkipRelayoutOnUnrelatedStyleChanges)
{
    struct CountingFlexBox : FlexBoxComponent
    {
        int layoutCount = 0;

        void performLayout (Rectangle<float> area) override
        {
            ++layoutCount;
            FlexBoxComponent::performLayout (area);
        }
    };

    CountingFlexBox flex;
    Component child;
    flex.addAndMakeVisible (child);
    flex.setBounds (0.0f, 0.0f, 200.0f, 80.0f);

    const int afterInitialLayout = flex.layoutCount;
    EXPECT_GT (afterInitialLayout, 0);

    flex.setProperty (Identifier ("customVisualToken"), 0.75);
    flex.setColor (Identifier ("customColor"), Colors::orange);
    EXPECT_EQ (flex.layoutCount, afterInitialLayout);

    flex.setGap (12.0f);
    EXPECT_EQ (flex.layoutCount, afterInitialLayout + 1);

    flex.setPadding ({ 4.0f, 8.0f, 4.0f, 8.0f });
    EXPECT_EQ (flex.layoutCount, afterInitialLayout + 2);

    child.setProperty (LayoutComponent::Properties::flexGrowId, 2.0);
    EXPECT_EQ (flex.layoutCount, afterInitialLayout + 3);

    // Multi-token string padding with leading "0" must still trigger relayout when reset to numeric 0.0.
    flex.setPadding ({ 20.0f, 0.0f, 20.0f, 0.0f });
    EXPECT_EQ (flex.layoutCount, afterInitialLayout + 4);
    EXPECT_FLOAT_EQ (child.getX(), 20.0f);

    flex.removeProperty (LayoutComponent::Properties::paddingId);
    EXPECT_EQ (flex.layoutCount, afterInitialLayout + 5);
    EXPECT_FLOAT_EQ (child.getX(), 0.0f);

    // Custom LayoutComponent subclasses still relayout on styleChanged when they may read custom properties.
    layout.setBounds (0.0f, 0.0f, 100.0f, 50.0f);
    const int customBefore = layout.layoutCount;
    layout.setProperty (Identifier ("customLayoutProperty"), 42);
    EXPECT_GT (layout.layoutCount, customBefore);
}
