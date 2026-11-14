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

Grid::TrackInfo parseGridTrack (const String& token)
{
    if (token.equalsIgnoreCase ("auto"))
        return Grid::TrackInfo::auto_();

    if (token.endsWithIgnoreCase ("fr"))
        return Grid::TrackInfo::fr (token.dropLastCharacters (2).getFloatValue());

    if (token.endsWith ("%"))
        return Grid::TrackInfo::percent (token.dropLastCharacters (1).getFloatValue());

    if (token.endsWithIgnoreCase ("px"))
        return Grid::TrackInfo::px (token.dropLastCharacters (2).getFloatValue());

    return Grid::TrackInfo::px (token.getFloatValue());
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
    Array<Grid::TrackInfo> tracks;

    if (value.isVoid() || value.isUndefined())
        return tracks;

    if (! value.isString())
    {
        const auto count = static_cast<int> (value);

        for (int i = 0; i < count; ++i)
            tracks.add (Grid::TrackInfo::fr (1.0f));

        return tracks;
    }

    const auto text = value.toString().trim();

    if (text.isEmpty())
        return tracks;

    if (text.containsOnly ("0123456789"))
    {
        const auto count = text.getIntValue();

        for (int i = 0; i < count; ++i)
            tracks.add (Grid::TrackInfo::fr (1.0f));

        return tracks;
    }

    for (const auto& token : splitTokens (text))
        tracks.add (parseGridTrack (token));

    return tracks;
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

    grid.templateColumns = parseTracks (findProperty (Properties::columnsId));
    grid.templateRows = parseTracks (findProperty (Properties::rowsId));
    grid.gap = getGap();
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
