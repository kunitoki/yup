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

std::optional<YuiDiagnostic> readDeclarations (const DataTree& parent, std::vector<StyleSheet::Declaration>& target)
{
    for (const auto& element : parent)
    {
        // A property declared in a rule (`property glow: number = 0;`) also sets its default
        const auto type = element.getType().toString();
        if (type != "declaration" && ! (type == "property" && element.hasProperty ("value")))
            continue;

        const auto name = element.getProperty ("name").toString().trim();

        if (name.isEmpty())
            return YuiDiagnostic { .message = "A declaration needs a name", .element = element };

        StyleSheet::Declaration declaration;
        declaration.element = element;

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
            return YuiDiagnostic { .message = "The declaration '" + name + "' needs a value", .element = element };

        target.push_back (std::move (declaration));
    }

    return std::nullopt;
}

std::optional<YuiDiagnostic> readPalette (const DataTree& element, std::map<String, ThemePalette>& palettes)
{
    const auto name = element.getProperty ("name").toString().trim();
    const auto colors = element.getProperty ("colors").toString().trim();

    if (colors.isEmpty())
        return YuiDiagnostic { .message = "The palette '" + name + "' needs colors", .element = element };

    const auto mode = element.getProperty ("mode").toString().trim() == "light"
                        ? ThemePalette::Mode::light
                        : ThemePalette::Mode::dark;

    auto palette = ThemePalette::fromString (colors, mode);

    if (palette.failed())
        return YuiDiagnostic { .message = "The palette '" + name + "' is invalid: " + palette.getErrorMessage(), .element = element, .property = "colors" };

    auto result = palette.getReference();

    for (int i = 0; i < element.getNumProperties(); ++i)
    {
        const auto propertyName = element.getPropertyName (i);
        const auto propertyText = propertyName.toString();

        if (propertyText == "name" || propertyText == "colors" || propertyText == "mode")
            continue;

        const auto role = ThemePalette::getRoleFromName (propertyText);

        if (! role.has_value())
            return YuiDiagnostic { .message = "The palette '" + name + "' sets the unknown role '" + propertyText + "'", .element = element, .property = propertyName };

        const auto color = Color::fromString (element.getProperty (propertyName).toString());
        result.setColor (*role, color);
    }

    palettes.insert_or_assign (name, std::move (result));
    return std::nullopt;
}

} // namespace

//==============================================================================

ResultValue<StyleSheet::Ptr> StyleSheet::fromTree (const DataTree& tree, YuiDiagnostic* failure)
{
    const auto rootType = tree.getType().toString();

    Ptr sheet (new StyleSheet());
    sheet->tree = tree;

    auto problem = tree.isValid() && (rootType == "yui" || rootType == "style")
                     ? sheet->addElements (tree)
                     : YuiDiagnostic { .message = "Expected a 'yui' document or a 'style' element" };

    if (! problem.has_value())
        return makeResultValueOk (std::move (sheet));

    if (failure != nullptr)
        *failure = *problem;

    return makeResultValueFail (problem->message);
}

std::optional<YuiDiagnostic> StyleSheet::addElements (const DataTree& parent)
{
    for (const auto& element : parent)
    {
        const auto type = element.getType().toString();

        if (type == "style")
        {
            if (auto problem = addElements (element))
                return problem;
        }
        else if (type == "rule")
        {
            auto selector = StyleSelector::parse (element.getProperty ("selector").toString());

            if (selector.failed())
                return YuiDiagnostic { .message = selector.getErrorMessage(), .element = element, .property = "selector" };

            Rule rule;
            rule.selector = selector.getReference();
            rule.order = static_cast<int> (rules.size());

            if (auto problem = readDeclarations (element, rule.declarations))
                return problem;

            for (const auto& property : rule.selector.getReferencedProperties())
                watchedProperties.addIfNotAlreadyThere (property);

            rules.push_back (std::move (rule));
        }
        else if (type == "palette")
        {
            if (auto problem = readPalette (element, palettes))
                return problem;
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
                return YuiDiagnostic { .message = "A keyframes block needs a name", .element = element };

            for (const auto& frameElement : element)
            {
                if (frameElement.getType().toString() != "keyframe")
                    continue;

                Keyframe frame;
                frame.position = jlimit (0.0, 1.0, static_cast<double> (frameElement.getProperty ("at")));

                if (auto problem = readDeclarations (frameElement, frame.declarations))
                    return problem;

                sequence.frames.push_back (std::move (frame));
            }

            std::stable_sort (sequence.frames.begin(), sequence.frames.end(), [] (const auto& a, const auto& b)
            {
                return a.position < b.position;
            });

            keyframes.push_back (std::move (sequence));
        }
    }

    return std::nullopt;
}

ResultValue<StyleSheet::Ptr> StyleSheet::parse (const String& text)
{
    const auto parsed = YuiParser::parse (text);

    if (! parsed.wasOk())
        return makeResultValueFail (parsed.diagnostics.front().toString());

    return fromTree (parsed.document);
}

std::vector<StringArray> StyleSheet::tokenizeValue (const String& value)
{
    std::vector<StringArray> entries (1);
    String token;
    int depth = 0;

    const auto flush = [&]
    {
        if (token.isEmpty())
            return;

        auto& tokens = entries.back();

        if (token.startsWithChar ('(') && ! tokens.isEmpty())
            tokens.set (tokens.size() - 1, tokens[tokens.size() - 1] + token);
        else
            tokens.add (token);

        token.clear();
    };

    for (auto p = value.getCharPointer(); ! p.isEmpty(); ++p)
    {
        const auto c = *p;

        if (depth == 0 && (c == ',' || CharacterFunctions::isWhitespace (c)))
        {
            flush();

            if (c == ',')
                entries.emplace_back();

            continue;
        }

        if (c == '(')
            ++depth;
        else if (c == ')')
            depth = jmax (0, depth - 1);

        token += c;
    }

    flush();
    return entries;
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
