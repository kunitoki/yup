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

#include "yup_LayoutParity.h"

using namespace yup;
using namespace yup::test;

namespace
{

FlexBox::Direction toDirection (const String& value)
{
    if (value == "row-reverse")
        return FlexBox::Direction::rowReverse;
    if (value == "column")
        return FlexBox::Direction::column;
    if (value == "column-reverse")
        return FlexBox::Direction::columnReverse;

    return FlexBox::Direction::row;
}

FlexBox::Wrap toWrap (const String& value)
{
    if (value == "wrap")
        return FlexBox::Wrap::wrap;
    if (value == "wrap-reverse")
        return FlexBox::Wrap::wrapReverse;

    return FlexBox::Wrap::noWrap;
}

FlexBox::JustifyContent toJustifyContent (const String& value)
{
    if (value == "flex-end")
        return FlexBox::JustifyContent::flexEnd;
    if (value == "center")
        return FlexBox::JustifyContent::center;
    if (value == "space-between")
        return FlexBox::JustifyContent::spaceBetween;
    if (value == "space-around")
        return FlexBox::JustifyContent::spaceAround;
    if (value == "space-evenly")
        return FlexBox::JustifyContent::spaceEvenly;

    return FlexBox::JustifyContent::flexStart;
}

FlexBox::AlignItems toAlignItems (const String& value)
{
    if (value == "flex-start")
        return FlexBox::AlignItems::flexStart;
    if (value == "flex-end")
        return FlexBox::AlignItems::flexEnd;
    if (value == "center")
        return FlexBox::AlignItems::center;
    if (value == "baseline")
        return FlexBox::AlignItems::baseline;

    return FlexBox::AlignItems::stretch;
}

FlexBox::AlignContent toAlignContent (const String& value)
{
    if (value == "flex-start")
        return FlexBox::AlignContent::flexStart;
    if (value == "flex-end")
        return FlexBox::AlignContent::flexEnd;
    if (value == "center")
        return FlexBox::AlignContent::center;
    if (value == "space-between")
        return FlexBox::AlignContent::spaceBetween;
    if (value == "space-around")
        return FlexBox::AlignContent::spaceAround;
    if (value == "space-evenly")
        return FlexBox::AlignContent::spaceEvenly;

    return FlexBox::AlignContent::stretch;
}

FlexItem::AlignSelf toAlignSelf (const String& value)
{
    if (value == "flex-start")
        return FlexItem::AlignSelf::flexStart;
    if (value == "flex-end")
        return FlexItem::AlignSelf::flexEnd;
    if (value == "center")
        return FlexItem::AlignSelf::center;
    if (value == "stretch")
        return FlexItem::AlignSelf::stretch;
    if (value == "baseline")
        return FlexItem::AlignSelf::baseline;

    return FlexItem::AlignSelf::autoAlign;
}

} // namespace

//==============================================================================
TEST (FlexBoxParityTests, CorpusFileIsPresent)
{
    auto corpus = loadGoldenCorpus ("flexbox_golden.json");

    ASSERT_FALSE (corpus.isVoid())
        << "tests/data/layout/flexbox_golden.json is missing; regenerate it by serving "
           "tests/data/layout/ over http and calling window.captureAll() in capture.html";

    auto* cases = corpus.getProperty ("cases", var()).getArray();
    ASSERT_NE (nullptr, cases);
    EXPECT_GT (cases->size(), 0);
}

TEST (FlexBoxParityTests, MatchesTheBrowserForEveryCorpusCase)
{
    auto corpus = loadGoldenCorpus ("flexbox_golden.json");
    ASSERT_FALSE (corpus.isVoid());

    auto* cases = corpus.getProperty ("cases", var()).getArray();
    ASSERT_NE (nullptr, cases);

    for (const auto& testCase : *cases)
    {
        const auto name = readString (testCase, "name", "<unnamed>");
        const auto container = testCase.getProperty ("container", var());

        auto* itemSpecs = testCase.getProperty ("items", var()).getArray();
        auto* expected = testCase.getProperty ("expected", var()).getArray();

        ASSERT_NE (nullptr, itemSpecs) << name;
        ASSERT_NE (nullptr, expected) << name;
        ASSERT_EQ (itemSpecs->size(), expected->size()) << name;

        OwnedArray<Component> components;

        FlexBox box;
        box.flexDirection = toDirection (readString (container, "flexDirection", "row"));
        box.flexWrap = toWrap (readString (container, "flexWrap", "nowrap"));
        box.justifyContent = toJustifyContent (readString (container, "justifyContent", "flex-start"));
        box.alignItems = toAlignItems (readString (container, "alignItems", "stretch"));
        box.alignContent = toAlignContent (readString (container, "alignContent", "stretch"));
        box.gap = readFloat (container, "gap", 0.0f);
        box.rowGap = readFloat (container, "rowGap", -1.0f);
        box.columnGap = readFloat (container, "columnGap", -1.0f);

        if (auto* padding = container.getProperty ("padding", var()).getArray())
        {
            if (padding->size() == 4)
            {
                box.paddingLeft = static_cast<float> (static_cast<double> (padding->getUnchecked (0)));
                box.paddingRight = static_cast<float> (static_cast<double> (padding->getUnchecked (1)));
                box.paddingTop = static_cast<float> (static_cast<double> (padding->getUnchecked (2)));
                box.paddingBottom = static_cast<float> (static_cast<double> (padding->getUnchecked (3)));
            }
        }

        for (const auto& spec : *itemSpecs)
        {
            // Every corpus item is an empty div, whose content size is 0 on
            // both axes. Starting the component at 0x0 is what makes YUP's
            // stubbed "auto means the component's current bounds" agree with
            // the browser, and it also keeps the replay independent of any
            // previous case.
            auto* component = components.add (new Component());
            component->setBounds (Rectangle<float> (0.0f, 0.0f, 0.0f, 0.0f));

            FlexItem item (*component);

            // A missing or null property means auto, which is -1 in FlexItem.
            item.width = readFloat (spec, "width", -1.0f);
            item.height = readFloat (spec, "height", -1.0f);
            item.widthPercent = readFloat (spec, "widthPercent", -1.0f);
            item.heightPercent = readFloat (spec, "heightPercent", -1.0f);
            item.flexBasis = readFloat (spec, "flexBasis", -1.0f);
            item.flexBasisPercent = readFloat (spec, "flexBasisPercent", -1.0f);

            item.flexGrow = readFloat (spec, "flexGrow", 0.0f);
            item.flexShrink = readFloat (spec, "flexShrink", 1.0f);

            item.minWidth = readFloat (spec, "minWidth", -1.0f);
            item.maxWidth = readFloat (spec, "maxWidth", -1.0f);
            item.minHeight = readFloat (spec, "minHeight", -1.0f);
            item.maxHeight = readFloat (spec, "maxHeight", -1.0f);

            item.order = readInt (spec, "order", 0);

            if (hasValue (spec, "alignSelf"))
                item.alignSelf = toAlignSelf (readString (spec, "alignSelf", ""));

            if (auto* margin = spec.getProperty ("margin", var()).getArray())
            {
                if (margin->size() == 4)
                {
                    // The corpus writes the string "auto" for a CSS auto margin
                    // and a number for everything else.
                    float values[4] = {};
                    bool autoFlags[4] = {};

                    for (int m = 0; m < 4; ++m)
                    {
                        const auto& entry = margin->getUnchecked (m);

                        if (entry.isString())
                            autoFlags[m] = (entry.toString() == "auto");
                        else
                            values[m] = static_cast<float> (static_cast<double> (entry));
                    }

                    item.marginLeft = values[0];
                    item.marginRight = values[1];
                    item.marginTop = values[2];
                    item.marginBottom = values[3];

                    item.marginLeftAuto = autoFlags[0];
                    item.marginRightAuto = autoFlags[1];
                    item.marginTopAuto = autoFlags[2];
                    item.marginBottomAuto = autoFlags[3];
                }
            }

            box.items.add (item);
        }

        box.performLayout (Rectangle<float> (0.0f,
                                             0.0f,
                                             readFloat (container, "width", 0.0f),
                                             readFloat (container, "height", 0.0f)));

        for (int i = 0; i < components.size(); ++i)
            expectParity (name, i, *components.getUnchecked (i), expected->getUnchecked (i));
    }
}
