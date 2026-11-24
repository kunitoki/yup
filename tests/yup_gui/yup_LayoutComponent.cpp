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
        using LayoutComponent::getLayoutChildren;
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

TEST_F (LayoutComponentTests, LeavesChildrenExcludedFromTheLayoutOut)
{
    Component laidOut;
    Component overlay;

    layout.addAndMakeVisible (laidOut);
    layout.addAndMakeVisible (overlay);
    overlay.setExcludedFromParentLayout (true);

    EXPECT_TRUE (overlay.isExcludedFromParentLayout());
    EXPECT_FALSE (laidOut.isExcludedFromParentLayout());

    const auto children = layout.getLayoutChildren();

    ASSERT_EQ (children.size(), 1u);
    EXPECT_EQ (children.front(), &laidOut);
}

TEST_F (LayoutComponentTests, LaysOutAgainWhenAChildIsExcludedFromTheLayout)
{
    Component child;
    layout.addAndMakeVisible (child);
    layout.setBounds (0, 0, 100, 60);

    const auto before = layout.layoutCount;
    child.setExcludedFromParentLayout (true);

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

    // Custom LayoutComponent subclasses declare their own LayoutEffect::own properties via ComponentTypeBuilder.
    struct CustomTrackLayout : LayoutComponent
    {
        int layoutCount = 0;

        static const ComponentTypeInfo& getClassTypeInfo()
        {
            static const auto info = ComponentTypeBuilder<CustomTrackLayout> (Identifier ("CustomTrackLayout"), LayoutComponent::getClassTypeInfo())
                                         .layoutProperty (Identifier ("customLayoutProperty"), ComponentProperty::Type::integer, 1)
                                         .build();
            return info;
        }

        const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

        void performLayout (Rectangle<float>) override
        {
            ++layoutCount;
        }
    };

    CustomTrackLayout customLayout;
    customLayout.setBounds (0.0f, 0.0f, 100.0f, 50.0f);
    const int customBefore = customLayout.layoutCount;
    customLayout.setProperty (Identifier ("unrelatedToken"), 99);
    EXPECT_EQ (customLayout.layoutCount, customBefore);

    customLayout.setProperty (Identifier ("customLayoutProperty"), 42);
    EXPECT_EQ (customLayout.layoutCount, customBefore + 1);
}

TEST_F (LayoutComponentTests, ABatchLaysOutOnceWhenItEnds)
{
    Component first, second;
    layout.setBounds (0, 0, 100, 60);

    const auto before = layout.layoutCount;

    {
        const LayoutComponent::ScopedLayoutBatch batch;

        layout.addAndMakeVisible (first);
        layout.addAndMakeVisible (second);
        first.setProperty (LayoutComponent::Properties::flexGrowId, 1.0);
        layout.setBounds (0, 0, 200, 60);

        {
            // A batch inside another ends with it
            const LayoutComponent::ScopedLayoutBatch inner;
            layout.setGap (4.0f);
        }

        EXPECT_EQ (layout.layoutCount, before);
    }

    EXPECT_EQ (layout.layoutCount, before + 1);
    EXPECT_EQ (layout.lastArea, Rectangle<float> (0.0f, 0.0f, 200.0f, 60.0f));

    // With no batch a layout is done at once
    layout.setGap (8.0f);
    EXPECT_EQ (layout.layoutCount, before + 2);
}

TEST_F (LayoutComponentTests, ABatchLeavesOutWhatIsGoneOrWasLaidOutByItsParent)
{
    struct CountingRow : RowComponent
    {
        int layoutCount = 0;

        void performLayout (Rectangle<float> area) override
        {
            ++layoutCount;
            RowComponent::performLayout (area);
        }
    };

    ColumnComponent column;
    CountingRow row;
    Component cell;
    auto gone = std::make_unique<TestLayout>();

    column.setBounds (0, 0, 100, 60);

    {
        const LayoutComponent::ScopedLayoutBatch batch;

        column.addAndMakeVisible (row);
        row.setProperty (LayoutComponent::Properties::preferredHeightId, 20.0);
        row.addAndMakeVisible (cell);
        cell.setProperty (LayoutComponent::Properties::flexGrowId, 1.0);

        gone->setBounds (0, 0, 10, 10);
        gone.reset();
    }

    // The column gave the row its bounds, which laid it out: it is not laid out again
    EXPECT_EQ (row.layoutCount, 1);
    EXPECT_EQ (row.getBounds(), Rectangle<float> (0.0f, 0.0f, 100.0f, 20.0f));
    EXPECT_EQ (cell.getBounds(), Rectangle<float> (0.0f, 0.0f, 100.0f, 20.0f));
}
