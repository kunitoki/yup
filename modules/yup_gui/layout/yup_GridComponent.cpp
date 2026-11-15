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

const StringArray gridAlignments { "start", "end", "center", "stretch", "baseline" };
const StringArray gridSelfAlignments { "auto", "start", "end", "center", "stretch", "baseline" };

Grid::AlignItems toAlignItems (const String& name)
{
    const auto index = gridAlignments.indexOf (name);
    return index >= 0 ? static_cast<Grid::AlignItems> (index) : Grid::AlignItems::stretch;
}

GridItem::AlignSelf toGridAlignSelf (const String& name)
{
    const auto index = gridSelfAlignments.indexOf (name);
    return index > 0 ? static_cast<GridItem::AlignSelf> (index) : GridItem::AlignSelf::autoAlign;
}

constexpr int maxGridTrackRepeatCount = 4096;

std::optional<float> tryParseGridNonNegativeFloat (const String& raw)
{
    const auto trimmed = raw.trim();
    if (trimmed.isEmpty())
        return std::nullopt;

    const auto utf8 = trimmed.toStdString();
    char* end = nullptr;
    const float parsed = std::strtof (utf8.c_str(), &end);

    if (end == utf8.c_str() || *end != '\0' || ! std::isfinite (parsed) || parsed < 0.0f)
        return std::nullopt;

    return parsed;
}

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

bool splitGridTopLevel (const String& text, bool commaOnly, StringArray& outTokens)
{
    outTokens.clear();

    int depth = 0;
    int tokenStart = 0;
    const int length = text.length();

    for (int i = 0; i < length; ++i)
    {
        const auto ch = text[i];

        if (ch == '(')
        {
            ++depth;
            continue;
        }

        if (ch == ')')
        {
            --depth;
            if (depth < 0)
                return false;

            continue;
        }

        if (depth == 0)
        {
            const bool isSeparator = commaOnly
                                       ? (ch == ',')
                                       : (ch == ',' || CharacterFunctions::isWhitespace (ch));

            if (isSeparator)
            {
                const auto segment = text.substring (tokenStart, i).trim();

                if (commaOnly || segment.isNotEmpty())
                    outTokens.add (segment);

                tokenStart = i + 1;
            }
        }
    }

    if (depth != 0)
        return false;

    const auto tail = text.substring (tokenStart).trim();
    if (commaOnly || tail.isNotEmpty())
        outTokens.add (tail);

    return true;
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
        if (const auto value = tryParseGridNonNegativeFloat (token.dropLastCharacters (2)))
            return Grid::TrackInfo::fr (*value);

        return std::nullopt;
    }

    if (token.endsWith ("%"))
    {
        if (const auto value = tryParseGridNonNegativeFloat (token.dropLastCharacters (1)))
            return Grid::TrackInfo::percent (*value);

        return std::nullopt;
    }

    if (token.endsWithIgnoreCase ("px"))
    {
        if (const auto value = tryParseGridNonNegativeFloat (token.dropLastCharacters (2)))
            return Grid::TrackInfo::px (*value);

        return std::nullopt;
    }

    if (const auto value = tryParseGridNonNegativeFloat (token))
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
        if (! splitGridTopLevel (*argsText, true, args) || args.size() != 2)
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
        if (! splitGridTopLevel (*argsText, true, args) || args.size() != 1)
            return std::nullopt;

        const auto arg = args[0].trim();
        const auto numberText = arg.endsWithIgnoreCase ("px") ? arg.dropLastCharacters (2) : arg;

        if (const auto maximumPx = tryParseGridNonNegativeFloat (numberText))
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
    if (! splitGridTopLevel (text, false, tokens) || tokens.isEmpty())
        return {};

    for (const auto& token : tokens)
    {
        if (const auto repeatArgsText = extractGridFunctionArgs (token, "repeat"))
        {
            StringArray repeatArgs;
            if (! splitGridTopLevel (*repeatArgsText, true, repeatArgs) || repeatArgs.size() != 2)
                return {};

            StringArray patternTokens;
            if (! splitGridTopLevel (repeatArgs[1], false, patternTokens) || patternTokens.isEmpty())
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
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (ComponentProperty {
            .name = Properties::columnsId,
            .type = ComponentProperty::Type::any,
            .isInherited = false,
            .description = "The column tracks: a count, or a list such as \"1fr 80px auto 20%\"." });

        props.push_back (ComponentProperty {
            .name = Properties::rowsId,
            .type = ComponentProperty::Type::any,
            .isInherited = false,
            .description = "The row tracks: a count, or a list such as \"1fr 80px auto 20%\"." });

        props.push_back (ComponentProperty {
            .name = Properties::areasId,
            .type = ComponentProperty::Type::string,
            .isInherited = false,
            .description = "The named areas, one row per comma separated group, as in \"head head, side main\"." });

        props.push_back (ComponentProperty {
            .name = Properties::justifyId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = gridAlignments[3],
            .isInherited = false,
            .enumValues = gridAlignments,
            .description = "How the children are placed horizontally in their cells." });

        props.push_back (ComponentProperty {
            .name = Properties::alignId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = gridAlignments[3],
            .isInherited = false,
            .enumValues = gridAlignments,
            .description = "How the children are placed vertically in their cells." });

        return ComponentTypeInfo { Identifier ("Grid"), &LayoutComponent::getClassTypeInfo(), std::move (props), {}, []
                                   {
            return std::make_unique<GridComponent>();
                                  } };
    }();

    return info;
}

const ComponentTypeInfo& GridComponent::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

void GridComponent::performLayout (Rectangle<float> area)
{
    Grid grid;

    const auto gap = getGap();
    grid.gap = gap;
    grid.templateColumns = parseGridTrackTemplate (findProperty (Properties::columnsId), area.getWidth(), gap, grid.autoColumns);
    grid.templateRows = parseGridTrackTemplate (findProperty (Properties::rowsId), area.getHeight(), gap, grid.autoRows);
    grid.justifyItems = toAlignItems (findProperty (Properties::justifyId).toString());
    grid.alignItems = toAlignItems (findProperty (Properties::alignId).toString());

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

        item.width = getItemNumber (*child, Properties::preferredWidthId);
        item.height = getItemNumber (*child, Properties::preferredHeightId);
        item.minWidth = getItemNumber (*child, Properties::minWidthId);
        item.maxWidth = getItemNumber (*child, Properties::maxWidthId);
        item.minHeight = getItemNumber (*child, Properties::minHeightId);
        item.maxHeight = getItemNumber (*child, Properties::maxHeightId);
        item.margin = getItemMargin (*child);
        item.justifySelf = toGridAlignSelf (getItemString (*child, Properties::justifySelfId));
        item.alignSelf = toGridAlignSelf (getItemString (*child, Properties::alignSelfId));

        grid.items.add (item);
    }

    grid.performLayout (area);
}

} // namespace yup
