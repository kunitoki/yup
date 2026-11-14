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

Result readDeclarations (const DataTree& parent, std::vector<StyleSheet::Declaration>& target)
{
    for (const auto& element : parent)
    {
        // A property declared in a rule (`property glow: number = 0;`) also sets its default
        const auto type = element.getType().toString();
        if (type != "declaration" && ! (type == "property" && element.hasProperty ("value")))
            continue;

        const auto name = element.getProperty ("name").toString().trim();

        if (name.isEmpty())
            return Result::fail ("A declaration needs a name");

        StyleSheet::Declaration declaration;

        if (const auto dot = name.indexOfChar ('.'); dot > 0)
        {
            declaration.qualifier = Identifier (name.substring (0, dot));
            declaration.name = Identifier (name.substring (dot + 1));
        }
        else
        {
            declaration.name = Identifier (name);
        }

        declaration.value = element.getProperty ("value").toString().trim();

        if (declaration.value.isEmpty())
            return Result::fail ("The declaration '" + name + "' needs a value");

        target.push_back (std::move (declaration));
    }

    return Result::ok();
}

ResultValue<ThemePalette> readPalette (const DataTree& element)
{
    const auto name = element.getProperty ("name").toString().trim();
    const auto colors = element.getProperty ("colors").toString().trim();

    if (colors.isEmpty())
        return makeResultValueFail ("The palette '" + name + "' needs colors");

    const auto mode = element.getProperty ("mode").toString().trim() == "light"
                        ? ThemePalette::Mode::light
                        : ThemePalette::Mode::dark;

    auto palette = ThemePalette::fromString (colors, mode);

    if (palette.failed())
        return makeResultValueFail ("The palette '" + name + "' is invalid: " + palette.getErrorMessage());

    auto result = palette.getReference();

    for (int i = 0; i < element.getNumProperties(); ++i)
    {
        const auto propertyName = element.getPropertyName (i);
        const auto propertyText = propertyName.toString();

        if (propertyText == "name" || propertyText == "colors" || propertyText == "mode")
            continue;

        const auto role = ThemePalette::getRoleFromName (propertyText);

        if (! role.has_value())
            return makeResultValueFail ("The palette '" + name + "' sets the unknown role '" + propertyText + "'");

        const auto color = Color::fromString (element.getProperty (propertyName).toString());
        result.setColor (*role, color);
    }

    return makeResultValueOk (std::move (result));
}

} // namespace

//==============================================================================

ResultValue<StyleSheet::Ptr> StyleSheet::fromTree (const DataTree& tree)
{
    const auto rootType = tree.getType().toString();

    if (! tree.isValid() || (rootType != "yui" && rootType != "style"))
        return makeResultValueFail ("Expected a 'yui' document or a 'style' element");

    Ptr sheet (new StyleSheet());

    if (const auto result = sheet->addElements (tree); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    return makeResultValueOk (std::move (sheet));
}

Result StyleSheet::addElements (const DataTree& parent)
{
    for (const auto& element : parent)
    {
        const auto type = element.getType().toString();

        if (type == "style")
        {
            if (const auto result = addElements (element); result.failed())
                return result;
        }
        else if (type == "rule")
        {
            auto selector = StyleSelector::parse (element.getProperty ("selector").toString());

            if (selector.failed())
                return Result::fail (selector.getErrorMessage());

            Rule rule;
            rule.selector = selector.getReference();
            rule.order = static_cast<int> (rules.size());

            if (const auto result = readDeclarations (element, rule.declarations); result.failed())
                return result;

            for (const auto& property : rule.selector.getReferencedProperties())
                watchedProperties.addIfNotAlreadyThere (property);

            rules.push_back (std::move (rule));
        }
        else if (type == "palette")
        {
            auto palette = readPalette (element);

            if (palette.failed())
                return Result::fail (palette.getErrorMessage());

            palettes.insert_or_assign (element.getProperty ("name").toString().trim(), palette.getReference());
        }
        else if (type == "drawing")
        {
            const auto name = element.getProperty ("name").toString().trim();

            if (name.isNotEmpty())
                drawings.insert_or_assign (name, element);
        }
        else if (type == "keyframes")
        {
            KeyframeSequence sequence;
            sequence.name = Identifier (element.getProperty ("name").toString().trim());

            if (! sequence.name.isValid())
                return Result::fail ("A keyframes block needs a name");

            for (const auto& frameElement : element)
            {
                if (frameElement.getType().toString() != "keyframe")
                    continue;

                Keyframe frame;
                frame.position = jlimit (0.0, 1.0, static_cast<double> (frameElement.getProperty ("at")));

                if (const auto result = readDeclarations (frameElement, frame.declarations); result.failed())
                    return result;

                sequence.frames.push_back (std::move (frame));
            }

            std::stable_sort (sequence.frames.begin(), sequence.frames.end(), [] (const auto& a, const auto& b)
            {
                return a.position < b.position;
            });

            keyframes.push_back (std::move (sequence));
        }
    }

    return Result::ok();
}

ResultValue<StyleSheet::Ptr> StyleSheet::parse (const String& text)
{
    const auto parsed = YuiParser::parse (text);

    if (! parsed.wasOk())
        return makeResultValueFail (parsed.diagnostics.front().toString());

    return fromTree (parsed.document);
}

//==============================================================================

const StyleSheet::KeyframeSequence* StyleSheet::findKeyframes (const Identifier& name) const
{
    for (const auto& sequence : keyframes)
        if (sequence.name == name)
            return &sequence;

    return nullptr;
}

const ThemePalette* StyleSheet::findPalette (const String& name) const
{
    const auto it = palettes.find (name);
    return it != palettes.end() ? &it->second : nullptr;
}

DataTree StyleSheet::findDrawing (const String& name) const
{
    const auto it = drawings.find (name);
    return it != drawings.end() ? it->second : DataTree();
}

StringArray StyleSheet::getPaletteNames() const
{
    StringArray names;

    for (const auto& [name, palette] : palettes)
        names.add (name);

    return names;
}

//==============================================================================

bool StyleSheet::watchesProperty (const Identifier& name) const
{
    return watchedProperties.contains (name);
}

} // namespace yup
