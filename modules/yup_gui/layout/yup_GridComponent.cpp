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

namespace yup
{

namespace
{

const detail::EnumNames<Grid::AlignItems> gridAlignments {
    { "start", Grid::AlignItems::flexStart },
    { "end", Grid::AlignItems::flexEnd },
    { "center", Grid::AlignItems::center },
    { "stretch", Grid::AlignItems::stretch },
    { "baseline", Grid::AlignItems::baseline }
};

const detail::EnumNames<GridItem::AlignSelf> gridSelfAlignments {
    { "auto", GridItem::AlignSelf::autoAlign },
    { "start", GridItem::AlignSelf::flexStart },
    { "end", GridItem::AlignSelf::flexEnd },
    { "center", GridItem::AlignSelf::center },
    { "stretch", GridItem::AlignSelf::stretch },
    { "baseline", GridItem::AlignSelf::baseline }
};

constexpr int maxGridTrackRepeatCount = 4096;

std::optional<int> tryParseGridRepeatCount (const String& raw)
{
    const auto trimmed = raw.trim();
    if (trimmed.isEmpty() || ! trimmed.containsOnly ("0123456789"))
        return std::nullopt;

    const auto count = trimmed.getIntValue();
    if (count <= 0 || count > maxGridTrackRepeatCount)
        return std::nullopt;

    return count;
}

std::optional<String> extractGridFunctionArgs (const String& token, StringRef functionName)
{
    const auto prefixLen = static_cast<int> (functionName.length());

    if (token.length() <= prefixLen + 1 || ! token.endsWithChar (')'))
        return std::nullopt;

    if (! token.substring (0, prefixLen).equalsIgnoreCase (String (functionName)))
        return std::nullopt;

    const auto afterName = token.substring (prefixLen).trimStart();
    if (! afterName.startsWithChar ('('))
        return std::nullopt;

    return afterName.substring (1, afterName.length() - 1).trim();
}

std::optional<Grid::TrackInfo> tryParseSimpleGridTrack (const String& rawToken)
{
    const auto token = rawToken.trim();
    if (token.isEmpty())
        return std::nullopt;

    if (token.equalsIgnoreCase ("auto"))
        return Grid::TrackInfo::auto_();

    if (token.endsWithIgnoreCase ("fr"))
    {
        if (const auto value = detail::parseStrictFloat (token.dropLastCharacters (2), false))
            return Grid::TrackInfo::fr (*value);

        return std::nullopt;
    }

    if (token.endsWith ("%"))
    {
        if (const auto value = detail::parseStrictFloat (token.dropLastCharacters (1), false))
            return Grid::TrackInfo::percent (*value);

        return std::nullopt;
    }

    if (const auto value = detail::parseStrictFloat (token, false, true))
        return Grid::TrackInfo::px (*value);

    return std::nullopt;
}

std::optional<Grid::TrackInfo> tryParseSingleGridTrack (const String& rawToken)
{
    const auto token = rawToken.trim();
    if (token.isEmpty())
        return std::nullopt;

    if (const auto argsText = extractGridFunctionArgs (token, "minmax"))
    {
        StringArray args;
        if (! detail::splitTopLevel (*argsText, true, args) || args.size() != 2)
            return std::nullopt;

        const auto minimum = tryParseSimpleGridTrack (args[0]);
        const auto maximum = tryParseSimpleGridTrack (args[1]);

        if (! minimum.has_value() || ! maximum.has_value() || minimum->isFractional())
            return std::nullopt;

        return Grid::TrackInfo::minmax (*minimum, *maximum);
    }

    if (const auto argsText = extractGridFunctionArgs (token, "fit-content"))
    {
        StringArray args;
        if (! detail::splitTopLevel (*argsText, true, args) || args.size() != 1)
            return std::nullopt;

        if (const auto maximumPx = detail::parseStrictFloat (args[0], false, true))
            return Grid::TrackInfo::fitContent (*maximumPx);

        return std::nullopt;
    }

    return tryParseSimpleGridTrack (token);
}

float resolveGridTrackMinimumPixels (const Grid::TrackInfo& track, float availableSize, float defaultAutoSize)
{
    switch (track.minimum.type)
    {
        case Grid::TrackInfo::SizeType::pixels:
            return jmax (0.0f, track.minimum.value);

        case Grid::TrackInfo::SizeType::percent:
            return jmax (0.0f, availableSize * (track.minimum.value * 0.01f));

        case Grid::TrackInfo::SizeType::autoSize:
            return jmax (0.0f, defaultAutoSize);

        case Grid::TrackInfo::SizeType::fraction:
            break;
    }

    return 0.0f;
}

Array<Grid::TrackInfo> parseGridTrackTemplate (const var& value,
                                               float availableSize,
                                               float gap,
                                               float defaultAutoSize)
{
    Array<Grid::TrackInfo> tracks;

    if (value.isVoid() || value.isUndefined() || value.isBool())
        return tracks;

    if (! value.isString())
    {
        const auto numeric = static_cast<double> (value);
        const auto count = static_cast<int> (numeric);

        if (! std::isfinite (numeric)
            || static_cast<double> (count) != numeric
            || count <= 0
            || count > maxGridTrackRepeatCount)
        {
            return tracks;
        }

        for (int i = 0; i < count; ++i)
            tracks.add (Grid::TrackInfo::fr (1.0f));

        return tracks;
    }

    const auto text = value.toString().trim();
    if (text.isEmpty())
        return tracks;

    if (text.containsOnly ("0123456789"))
    {
        const auto count = tryParseGridRepeatCount (text);
        if (! count.has_value())
            return tracks;

        for (int i = 0; i < *count; ++i)
            tracks.add (Grid::TrackInfo::fr (1.0f));

        return tracks;
    }

    StringArray tokens;
    if (! detail::splitTopLevel (text, false, tokens) || tokens.isEmpty())
        return {};

    for (const auto& token : tokens)
    {
        if (const auto repeatArgsText = extractGridFunctionArgs (token, "repeat"))
        {
            StringArray repeatArgs;
            if (! detail::splitTopLevel (*repeatArgsText, true, repeatArgs) || repeatArgs.size() != 2)
                return {};

            StringArray patternTokens;
            if (! detail::splitTopLevel (repeatArgs[1], false, patternTokens) || patternTokens.isEmpty())
                return {};

            Array<Grid::TrackInfo> pattern;
            for (const auto& patternToken : patternTokens)
            {
                const auto parsed = tryParseSingleGridTrack (patternToken);
                if (! parsed.has_value())
                    return {};

                pattern.add (*parsed);
            }

            const auto countSpec = repeatArgs[0].trim();
            if (countSpec.equalsIgnoreCase ("auto-fill") || countSpec.equalsIgnoreCase ("auto-fit"))
            {
                if (pattern.size() == 1)
                {
                    const auto repeated = Grid::repeatToFill (pattern.getReference (0),
                                                              availableSize,
                                                              gap,
                                                              defaultAutoSize);
                    if (tracks.size() + repeated.size() > maxGridTrackRepeatCount)
                        return {};

                    tracks.addArray (repeated);
                }
                else
                {
                    float patternMinimum = 0.0f;
                    for (int i = 0; i < pattern.size(); ++i)
                    {
                        if (i > 0)
                            patternMinimum += gap;

                        patternMinimum += resolveGridTrackMinimumPixels (pattern.getReference (i),
                                                                         availableSize,
                                                                         defaultAutoSize);
                    }

                    int repeatCount = 1;
                    const float step = patternMinimum + gap;
                    if (step > 0.0f && availableSize > 0.0f)
                        repeatCount = jlimit (1, maxGridTrackRepeatCount / pattern.size(), static_cast<int> (std::floor ((availableSize + gap) / step)));

                    if (tracks.size() + repeatCount * pattern.size() > maxGridTrackRepeatCount)
                        return {};

                    for (int r = 0; r < repeatCount; ++r)
                        tracks.addArray (pattern);
                }
            }
            else
            {
                const auto repeatCount = tryParseGridRepeatCount (countSpec);
                if (! repeatCount.has_value() || tracks.size() + (*repeatCount * pattern.size()) > maxGridTrackRepeatCount)
                    return {};

                for (int r = 0; r < *repeatCount; ++r)
                    tracks.addArray (pattern);
            }

            continue;
        }

        const auto parsed = tryParseSingleGridTrack (token);
        if (! parsed.has_value() || tracks.size() >= maxGridTrackRepeatCount)
            return {};

        tracks.add (*parsed);
    }

    return tracks;
}

} // namespace

//==============================================================================

void GridComponent::setColumns (const var& columns)
{
    setProperty (Properties::columnsId, columns);
}

void GridComponent::setRows (const var& rows)
{
    setProperty (Properties::rowsId, rows);
}

//==============================================================================

Array<Grid::TrackInfo> GridComponent::parseTracks (const var& value)
{
    return parseGridTrackTemplate (value, 0.0f, 0.0f, 0.0f);
}

//==============================================================================

const ComponentTypeInfo& GridComponent::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;
    using LayoutEffect = ComponentProperty::LayoutEffect;

    static const auto info = ComponentTypeBuilder<GridComponent> (Identifier ("Grid"), LayoutComponent::getClassTypeInfo())
                                 .layoutProperty (Properties::columnsId, Type::any, {}, LayoutEffect::own, "The column tracks: a count, or a list such as \"1fr 80px auto 20%\".")
                                 .layoutProperty (Properties::rowsId, Type::any, {}, LayoutEffect::own, "The row tracks: a count, or a list such as \"1fr 80px auto 20%\".")
                                 .layoutProperty (Properties::areasId, Type::string, {}, LayoutEffect::own, "The named areas, one row per comma separated group, as in \"head head, side main\".")
                                 .layoutProperty (Properties::justifyId, Type::enumeration, gridAlignments.names[3], LayoutEffect::own, "How the children are placed horizontally in their cells.", gridAlignments.names)
                                 .layoutProperty (Properties::alignId, Type::enumeration, gridAlignments.names[3], LayoutEffect::own, "How the children are placed vertically in their cells.", gridAlignments.names)
                                 .build();

    return info;
}

//==============================================================================

void GridComponent::performLayout (Rectangle<float> area)
{
    Grid grid;

    const auto gap = getGap();
    grid.gap = gap;
    grid.templateColumns = parseGridTrackTemplate (findProperty (Properties::columnsId), area.getWidth(), gap, grid.autoColumns);
    grid.templateRows = parseGridTrackTemplate (findProperty (Properties::rowsId), area.getHeight(), gap, grid.autoRows);
    grid.justifyItems = gridAlignments.toEnum (findProperty (Properties::justifyId).toString(), Grid::AlignItems::stretch);
    grid.alignItems = gridAlignments.toEnum (findProperty (Properties::alignId).toString(), Grid::AlignItems::stretch);

    const auto areas = findProperty (Properties::areasId).toString().trim();
    bool hasAreas = false;

    if (areas.isNotEmpty())
    {
        StringArray rowPatterns;

        for (const auto& row : StringArray::fromTokens (areas, ",", ""))
        {
            if (const auto trimmed = row.trim(); trimmed.isNotEmpty())
                rowPatterns.add (trimmed);
        }

        hasAreas = grid.setTemplateAreas (rowPatterns).wasOk();
    }

    for (auto* child : getLayoutChildren())
    {
        GridItem item (*child);

        if (hasAreas)
            item.area = getItemString (*child, Properties::gridAreaId);

        if (item.area.isEmpty())
        {
            item.column = getItemInteger (*child, Properties::gridColumnId);
            item.row = getItemInteger (*child, Properties::gridRowId);
            item.columnSpan = jmax (1, getItemInteger (*child, Properties::gridColumnSpanId));
            item.rowSpan = jmax (1, getItemInteger (*child, Properties::gridRowSpanId));
        }

        readItemSizing (item, *child);
        item.justifySelf = gridSelfAlignments.toEnum (getItemString (*child, Properties::justifySelfId), GridItem::AlignSelf::autoAlign);
        item.alignSelf = gridSelfAlignments.toEnum (getItemString (*child, Properties::alignSelfId), GridItem::AlignSelf::autoAlign);

        grid.items.add (item);
    }

    grid.performLayout (area);
}

} // namespace yup
