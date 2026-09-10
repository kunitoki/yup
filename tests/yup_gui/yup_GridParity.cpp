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

/*  Parses one CSS track function into a TrackInfo.

    The corpus writes tracks exactly as CSS spells them, and this drives the
    real Grid API from those strings - so `repeat(3, 1fr)` here really does call
    Grid::repeat, and `repeat(auto-fill, ...)` really does call
    Grid::repeatToFill, rather than testing a pre-expanded list.

    Deliberately absent: CSS `auto`, and `fit-content()` / `auto-fit` which are
    built on it. Those are sized from the contents of the track, which YUP
    cannot measure yet, so they are excluded from the corpus rather than added
    to it as permanently failing cases - Grid::TrackInfo::auto_() is a fixed
    autoRows/autoColumns size, not CSS auto, and is covered by the ordinary unit
    tests instead.
*/
Grid::TrackInfo parseTrack (const String& spec);

/** Splits "a, b" into its two top-level arguments, ignoring nested commas. */
bool splitArguments (const String& text, String& first, String& second)
{
    int depth = 0;

    for (int i = 0; i < text.length(); ++i)
    {
        const auto c = text[i];

        if (c == '(')
            ++depth;
        else if (c == ')')
            --depth;
        else if (c == ',' && depth == 0)
        {
            first = text.substring (0, i).trim();
            second = text.substring (i + 1).trim();
            return true;
        }
    }

    return false;
}

Grid::TrackInfo parseTrack (const String& spec)
{
    const auto text = spec.trim();

    if (text.startsWith ("minmax(") && text.endsWith (")"))
    {
        String lo, hi;

        if (splitArguments (text.substring (7, text.length() - 1), lo, hi))
            return Grid::TrackInfo::minmax (parseTrack (lo), parseTrack (hi));

        jassertfalse;
        return Grid::TrackInfo::px (0.0f);
    }

    if (text.startsWith ("fit-content(") && text.endsWith (")"))
        return Grid::TrackInfo::fitContent (text.substring (12, text.length() - 1).trim().getFloatValue());

    if (text.endsWith ("fr"))
        return Grid::TrackInfo::fr (text.dropLastCharacters (2).getFloatValue());

    if (text.endsWith ("px"))
        return Grid::TrackInfo::px (text.dropLastCharacters (2).getFloatValue());

    if (text.endsWith ("%"))
        return Grid::TrackInfo::percent (text.dropLastCharacters (1).getFloatValue());

    jassertfalse; // an intrinsic track slipped into the corpus
    return Grid::TrackInfo::px (0.0f);
}

/*  Builds one axis of the template from the corpus's CSS strings.

    A `[name]` token names the line at the current track index, which is how the
    corpus round-trips named lines; the 1-based CSS numbering never appears
    because the names are attached to YUP's own 0-based indices here.
*/
void addTracks (Array<Grid::TrackInfo>& tracks,
                const var& specs,
                Grid& grid,
                bool isColumnAxis,
                float availableSize,
                float gap,
                float defaultSize)
{
    auto* array = specs.getArray();

    if (array == nullptr)
        return;

    for (const auto& entry : *array)
    {
        const auto text = entry.toString().trim();

        if (text.startsWith ("[") && text.endsWith ("]"))
        {
            const auto name = text.substring (1, text.length() - 1);

            if (isColumnAxis)
                grid.setColumnLineName (tracks.size(), name);
            else
                grid.setRowLineName (tracks.size(), name);

            continue;
        }

        if (text.startsWith ("repeat(") && text.endsWith (")"))
        {
            String count, inner;

            if (! splitArguments (text.substring (7, text.length() - 1), count, inner))
            {
                jassertfalse;
                continue;
            }

            const auto track = parseTrack (inner);

            if (count == "auto-fill" || count == "auto-fit")
                tracks.addArray (Grid::repeatToFill (track, availableSize, gap, defaultSize));
            else
                tracks.addArray (Grid::repeat (count.getIntValue(), track));

            continue;
        }

        tracks.add (parseTrack (text));
    }
}

Grid::AutoFlow toAutoFlow (const String& value)
{
    if (value == "column")
        return Grid::AutoFlow::column;
    if (value == "row dense")
        return Grid::AutoFlow::rowDense;
    if (value == "column dense")
        return Grid::AutoFlow::columnDense;

    return Grid::AutoFlow::row;
}

Grid::AlignContent toGridAlignContent (const String& value)
{
    if (value == "end" || value == "flex-end")
        return Grid::AlignContent::flexEnd;
    if (value == "center")
        return Grid::AlignContent::center;
    if (value == "space-between")
        return Grid::AlignContent::spaceBetween;
    if (value == "space-around")
        return Grid::AlignContent::spaceAround;
    if (value == "space-evenly")
        return Grid::AlignContent::spaceEvenly;

    return Grid::AlignContent::flexStart;
}

Grid::AlignItems toGridAlignItems (const String& value)
{
    if (value == "start" || value == "flex-start")
        return Grid::AlignItems::flexStart;
    if (value == "end" || value == "flex-end")
        return Grid::AlignItems::flexEnd;
    if (value == "center")
        return Grid::AlignItems::center;
    if (value == "baseline")
        return Grid::AlignItems::baseline;

    return Grid::AlignItems::stretch;
}

GridItem::AlignSelf toGridAlignSelf (const String& value)
{
    if (value == "start" || value == "flex-start")
        return GridItem::AlignSelf::flexStart;
    if (value == "end" || value == "flex-end")
        return GridItem::AlignSelf::flexEnd;
    if (value == "center")
        return GridItem::AlignSelf::center;
    if (value == "stretch")
        return GridItem::AlignSelf::stretch;
    if (value == "baseline")
        return GridItem::AlignSelf::baseline;

    return GridItem::AlignSelf::autoAlign;
}

} // namespace

//==============================================================================
TEST (GridParityTests, CorpusFileIsPresent)
{
    auto corpus = loadGoldenCorpus ("grid_golden.json");

    ASSERT_FALSE (corpus.isVoid())
        << "tests/data/layout/grid_golden.json is missing; regenerate it by serving "
           "tests/data/layout/ over http and calling window.captureAll() in capture.html";

    auto* cases = corpus.getProperty ("cases", var()).getArray();
    ASSERT_NE (nullptr, cases);
    EXPECT_GT (cases->size(), 0);
}

TEST (GridParityTests, MatchesTheBrowserForEveryCorpusCase)
{
    auto corpus = loadGoldenCorpus ("grid_golden.json");
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

        const float width = readFloat (container, "width", 0.0f);
        const float height = readFloat (container, "height", 0.0f);

        Grid grid;
        grid.columnGap = readFloat (container, "columnGap", 0.0f);
        grid.rowGap = readFloat (container, "rowGap", 0.0f);
        grid.autoColumns = readFloat (container, "autoColumns", 100.0f);
        grid.autoRows = readFloat (container, "autoRows", 40.0f);
        grid.justifyItems = toGridAlignItems (readString (container, "justifyItems", "stretch"));
        grid.alignItems = toGridAlignItems (readString (container, "alignItems", "stretch"));
        grid.justifyContent = toGridAlignContent (readString (container, "justifyContent", "start"));
        grid.alignContent = toGridAlignContent (readString (container, "alignContent", "start"));
        grid.autoFlow = toAutoFlow (readString (container, "autoFlow", "row"));

        addTracks (grid.templateColumns, container.getProperty ("templateColumns", var()),
                   grid, true, width, grid.columnGap, grid.autoColumns);
        addTracks (grid.templateRows, container.getProperty ("templateRows", var()),
                   grid, false, height, grid.rowGap, grid.autoRows);

        if (auto* areaRows = container.getProperty ("areas", var()).getArray())
        {
            if (areaRows->size() > 0)
            {
                StringArray patterns;

                for (const auto& row : *areaRows)
                    patterns.add (row.toString());

                const auto result = grid.setTemplateAreas (patterns);
                ASSERT_TRUE (result.wasOk()) << name << ": " << result.getErrorMessage();
            }
        }

        for (const auto& spec : *itemSpecs)
        {
            auto* component = components.add (new Component());
            component->setBounds (Rectangle<float> (0.0f, 0.0f, 0.0f, 0.0f));

            GridItem item (*component);

            // capture.html writes YUP's 0-based line numbers with -1 meaning
            // auto-place, converting to CSS's 1-based lines on the way in.
            item.column = readInt (spec, "column", GridItem::autoPlace);
            item.row = readInt (spec, "row", GridItem::autoPlace);
            item.columnSpan = readInt (spec, "columnSpan", 1);
            item.rowSpan = readInt (spec, "rowSpan", 1);

            item.width = readFloat (spec, "width", -1.0f);
            item.height = readFloat (spec, "height", -1.0f);
            item.widthPercent = readFloat (spec, "widthPercent", -1.0f);
            item.heightPercent = readFloat (spec, "heightPercent", -1.0f);

            item.minWidth = readFloat (spec, "minWidth", -1.0f);
            item.maxWidth = readFloat (spec, "maxWidth", -1.0f);
            item.minHeight = readFloat (spec, "minHeight", -1.0f);
            item.maxHeight = readFloat (spec, "maxHeight", -1.0f);

            if (hasValue (spec, "justifySelf"))
                item.justifySelf = toGridAlignSelf (readString (spec, "justifySelf", ""));

            if (hasValue (spec, "alignSelf"))
                item.alignSelf = toGridAlignSelf (readString (spec, "alignSelf", ""));

            item.area = readString (spec, "area", {});
            item.columnStartName = readString (spec, "columnStartName", {});
            item.rowStartName = readString (spec, "rowStartName", {});

            if (auto* margin = spec.getProperty ("margin", var()).getArray())
            {
                if (margin->size() == 4)
                {
                    item.marginLeft = static_cast<float> (static_cast<double> (margin->getUnchecked (0)));
                    item.marginRight = static_cast<float> (static_cast<double> (margin->getUnchecked (1)));
                    item.marginTop = static_cast<float> (static_cast<double> (margin->getUnchecked (2)));
                    item.marginBottom = static_cast<float> (static_cast<double> (margin->getUnchecked (3)));
                }
            }

            grid.items.add (item);
        }

        grid.performLayout (Rectangle<float> (0.0f, 0.0f, width, height));

        for (int i = 0; i < components.size(); ++i)
            expectParity (name, i, *components.getUnchecked (i), expected->getUnchecked (i));
    }
}
