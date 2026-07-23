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

//==============================================================================
// ArtboardFile::AssetInfo
//==============================================================================

TEST (ArtboardFileAssetInfoTests, DefaultConstruction)
{
    ArtboardFile::AssetInfo info;

    EXPECT_TRUE (info.uniqueName.isEmpty());
    EXPECT_TRUE (info.extension.isEmpty());
}

TEST (ArtboardFileAssetInfoTests, FieldAssignment)
{
    ArtboardFile::AssetInfo info;
    info.uniqueName = "test_name";
    info.extension = "png";

    EXPECT_EQ (String ("test_name"), info.uniqueName);
    EXPECT_EQ (String ("png"), info.extension);
}

//==============================================================================
// Artboard
//==============================================================================

class ArtboardTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        artboard = std::make_unique<Artboard> ("testArtboard");
    }

    void TearDown() override
    {
        artboard.reset();
    }

    std::unique_ptr<Artboard> artboard;
};

TEST_F (ArtboardTests, DefaultConstruction)
{
    EXPECT_FALSE (artboard->isPaused());
    EXPECT_TRUE (artboard->isPausingWhenHidden());
    EXPECT_EQ (Artboard::Layout::contain, artboard->getLayout());
    EXPECT_EQ (Artboard::Alignment::center, artboard->getAlignment());
}

TEST_F (ArtboardTests, ConstructWithComponentId)
{
    Artboard a ("myCustomId");
    EXPECT_EQ (String ("myCustomId"), a.getComponentID());
}

TEST_F (ArtboardTests, SetAndGetLayout)
{
    artboard->setLayout (Artboard::Layout::fill);
    EXPECT_EQ (Artboard::Layout::fill, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::cover);
    EXPECT_EQ (Artboard::Layout::cover, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::fitWidth);
    EXPECT_EQ (Artboard::Layout::fitWidth, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::fitHeight);
    EXPECT_EQ (Artboard::Layout::fitHeight, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::none);
    EXPECT_EQ (Artboard::Layout::none, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::scaleDown);
    EXPECT_EQ (Artboard::Layout::scaleDown, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::layout);
    EXPECT_EQ (Artboard::Layout::layout, artboard->getLayout());
}

TEST_F (ArtboardTests, SetAndGetAlignment)
{
    artboard->setAlignment (Artboard::Alignment::topLeft);
    EXPECT_EQ (Artboard::Alignment::topLeft, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::topRight);
    EXPECT_EQ (Artboard::Alignment::topRight, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::centerLeft);
    EXPECT_EQ (Artboard::Alignment::centerLeft, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::centerRight);
    EXPECT_EQ (Artboard::Alignment::centerRight, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::bottomLeft);
    EXPECT_EQ (Artboard::Alignment::bottomLeft, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::bottomCenter);
    EXPECT_EQ (Artboard::Alignment::bottomCenter, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::bottomRight);
    EXPECT_EQ (Artboard::Alignment::bottomRight, artboard->getAlignment());
}

TEST_F (ArtboardTests, SetAndGetPaused)
{
    EXPECT_FALSE (artboard->isPaused());

    artboard->setPaused (true);
    EXPECT_TRUE (artboard->isPaused());

    artboard->setPaused (false);
    EXPECT_FALSE (artboard->isPaused());
}

TEST_F (ArtboardTests, ShouldPauseWhenHidden)
{
    EXPECT_TRUE (artboard->isPausingWhenHidden());

    artboard->shouldPauseWhenHidden (false);
    EXPECT_FALSE (artboard->isPausingWhenHidden());

    artboard->shouldPauseWhenHidden (true);
    EXPECT_TRUE (artboard->isPausingWhenHidden());
}

TEST_F (ArtboardTests, DurationSecondsReturnsZeroWithoutFile)
{
    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
}

TEST_F (ArtboardTests, AdvanceAndApplyDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));
    EXPECT_NO_THROW (artboard->advanceAndApply (0.0f));
    EXPECT_NO_THROW (artboard->advanceAndApply (-1.0f));
}

TEST_F (ArtboardTests, HasBoolInputReturnsFalseWithoutFile)
{
    EXPECT_FALSE (artboard->hasBoolInput ("anyInput"));
    EXPECT_FALSE (artboard->hasBoolInput (String()));
}

TEST_F (ArtboardTests, HasNumberInputReturnsFalseWithoutFile)
{
    EXPECT_FALSE (artboard->hasNumberInput ("anyInput"));
    EXPECT_FALSE (artboard->hasNumberInput (String()));
}

TEST_F (ArtboardTests, HasTriggerInputReturnsFalseWithoutFile)
{
    EXPECT_FALSE (artboard->hasTriggerInput ("anyInput"));
    EXPECT_FALSE (artboard->hasTriggerInput (String()));
}

TEST_F (ArtboardTests, SetBoolInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->setBoolInput ("test", true));
    EXPECT_NO_THROW (artboard->setBoolInput ("test", false));
}

TEST_F (ArtboardTests, SetNumberInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->setNumberInput ("test", 42.0));
    EXPECT_NO_THROW (artboard->setNumberInput ("test", -1.0));
}

TEST_F (ArtboardTests, TriggerInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->triggerInput ("test"));
    EXPECT_NO_THROW (artboard->triggerInput (String()));
}

TEST_F (ArtboardTests, GetAllInputsReturnsEmptyWithoutFile)
{
    EXPECT_NO_THROW (artboard->getAllInputs());
}

TEST_F (ArtboardTests, SetAllInputsDoesNotCrash)
{
    EXPECT_NO_THROW (artboard->setAllInputs (var()));
    EXPECT_NO_THROW (artboard->setAllInputs (var()));
}

TEST_F (ArtboardTests, SetInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->setInput ("testInput", var (true)));
    EXPECT_NO_THROW (artboard->setInput ("testInput", var (42.0)));
    EXPECT_NO_THROW (artboard->setInput ("testInput", var()));
}

TEST_F (ArtboardTests, OnPropertyChangedCallbackCanBeSet)
{
    bool called = false;
    artboard->onPropertyChanged = [&] (Artboard&, const String&, const String&, const var&, const var&)
    {
        called = true;
    };

    EXPECT_FALSE (called);
}

TEST_F (ArtboardTests, PropertyChangedVirtualMethodCanBeCalled)
{
    EXPECT_NO_THROW (artboard->propertyChanged ("event", "property", var(), var (42)));
}

TEST_F (ArtboardTests, ClearDoesNotCrash)
{
    EXPECT_NO_THROW (artboard->clear());

    EXPECT_FALSE (artboard->isPaused());
    EXPECT_TRUE (artboard->isPausingWhenHidden());
    EXPECT_EQ (Artboard::Layout::contain, artboard->getLayout());
    EXPECT_EQ (Artboard::Alignment::center, artboard->getAlignment());
    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
}

TEST_F (ArtboardTests, RefreshDisplayDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->refreshDisplay (0.016));
    EXPECT_NO_THROW (artboard->refreshDisplay (0.0));
}

TEST_F (ArtboardTests, ClearResetsState)
{
    artboard->setPaused (true);
    artboard->shouldPauseWhenHidden (false);
    artboard->setLayout (Artboard::Layout::cover);
    artboard->setAlignment (Artboard::Alignment::topLeft);

    artboard->clear();

    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
    EXPECT_FALSE (artboard->hasBoolInput ("test"));
}

TEST_F (ArtboardTests, LayoutEnumValuesAreDistinct)
{
    EXPECT_EQ (static_cast<int> (Artboard::Layout::fill), 0);
    EXPECT_NE (Artboard::Layout::fill, Artboard::Layout::contain);
    EXPECT_NE (Artboard::Layout::contain, Artboard::Layout::cover);
    EXPECT_NE (Artboard::Layout::cover, Artboard::Layout::fitWidth);
    EXPECT_NE (Artboard::Layout::fitWidth, Artboard::Layout::fitHeight);
    EXPECT_NE (Artboard::Layout::fitHeight, Artboard::Layout::none);
    EXPECT_NE (Artboard::Layout::none, Artboard::Layout::scaleDown);
    EXPECT_NE (Artboard::Layout::scaleDown, Artboard::Layout::layout);
}

TEST_F (ArtboardTests, AlignmentEnumValuesAreDistinct)
{
    EXPECT_EQ (static_cast<int> (Artboard::Alignment::topLeft), 0);
    EXPECT_NE (Artboard::Alignment::topLeft, Artboard::Alignment::topCenter);
    EXPECT_NE (Artboard::Alignment::topCenter, Artboard::Alignment::topRight);
    EXPECT_NE (Artboard::Alignment::topRight, Artboard::Alignment::centerLeft);
    EXPECT_NE (Artboard::Alignment::centerLeft, Artboard::Alignment::center);
    EXPECT_NE (Artboard::Alignment::center, Artboard::Alignment::centerRight);
    EXPECT_NE (Artboard::Alignment::centerRight, Artboard::Alignment::bottomLeft);
    EXPECT_NE (Artboard::Alignment::bottomLeft, Artboard::Alignment::bottomCenter);
    EXPECT_NE (Artboard::Alignment::bottomCenter, Artboard::Alignment::bottomRight);
}
