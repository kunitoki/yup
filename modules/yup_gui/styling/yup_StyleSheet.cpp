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

std::optional<MouseCursor::Type> findStyleCursor (const String& name)
{
    static const std::pair<const char*, MouseCursor::Type> cursors[] = {
        { "none", MouseCursor::None },
        { "default", MouseCursor::Default },
        { "arrow", MouseCursor::Arrow },
        { "text", MouseCursor::Text },
        { "wait", MouseCursor::Wait },
        { "waitArrow", MouseCursor::WaitArrow },
        { "hand", MouseCursor::Hand },
        { "crosshair", MouseCursor::Crosshair },
        { "crossbones", MouseCursor::Crossbones },
        { "resizeLeftRight", MouseCursor::ResizeLeftRight },
        { "resizeUpDown", MouseCursor::ResizeUpDown },
        { "resizeTopLeftRightBottom", MouseCursor::ResizeTopLeftRightBottom },
        { "resizeBottomLeftRightTop", MouseCursor::ResizeBottomLeftRightTop },
        { "resizeAll", MouseCursor::ResizeAll }
    };

    return detail::findNamed (cursors, name);
}

bool isExpressionInfixOperator (const String& token)
{
    static const char* const operators[] = {
        "+", "-", "*", "/", "%", "?", ":", "==", "!=", "<", "<=", ">", ">=", "&&", "||"
    };

    for (const auto* op : operators)
        if (token == op)
            return true;

    return false;
}

bool looksLikeGlowDeclaration (const String& parsed, const std::vector<StringArray>& entries)
{
    if (parsed.isEmpty() || parsed == "none")
        return true;

    if (entries.size() > 1)
        return true;

    if (entries.empty() || entries.front().isEmpty())
        return false;

    const auto& tokens = entries.front();

    if (tokens[0].startsWithChar ('#'))
        return true;

    int lengths = 0;
    while (lengths < tokens.size() && detail::parseStrictFloat (tokens[lengths]).has_value())
        ++lengths;

    if (lengths == 0 || lengths == tokens.size())
        return false;

    if (lengths >= 3)
        return true;

    return ! isExpressionInfixOperator (tokens[lengths]);
}

void parseRuleDeclarationValue (StyleSheet::Declaration& declaration)
{
    using Kind = StyleSheet::Declaration::Kind;
    static const Identifier paletteId ("palette");
    static const Identifier drawingId ("drawing");
    static const Identifier transitionId ("transition");
    static const Identifier animationId ("animation");
    static const Identifier cursorId ("cursor");
    static const Identifier effectId ("effect");
    static const Identifier filterId ("filter");
    static const Identifier shadowId ("shadow");
    static const Identifier glowId ("glow");

    if (declaration.name == paletteId)
    {
        declaration.kind = Kind::palette;
        declaration.parsedName = declaration.value.unquoted().trim();
    }
    else if (declaration.name == drawingId)
    {
        declaration.kind = Kind::drawing;
        declaration.parsedName = declaration.value.unquoted().trim();
    }
    else if (declaration.name == effectId)
    {
        declaration.kind = Kind::effect;
        declaration.parsedName = declaration.value.unquoted().trim();
    }
    else if (declaration.name == filterId)
    {
        declaration.kind = Kind::filter;
        declaration.parsedName = declaration.value.unquoted().trim();

        if (auto parsed = FilterEffect::parseSteps (declaration.parsedName, true); parsed.wasOk())
            declaration.filters = std::move (parsed.getReference());
        else
            declaration.parseError = "filter: " + parsed.getErrorMessage();
    }
    else if (declaration.name == cursorId)
    {
        declaration.kind = Kind::cursor;
        declaration.parsedName = declaration.value.unquoted().trim();
        declaration.cursor = findStyleCursor (declaration.parsedName);

        if (declaration.parsedName.isNotEmpty() && ! declaration.cursor.has_value())
            declaration.parseError = "Unknown cursor '" + declaration.parsedName + "'";
    }
    else if (declaration.name == transitionId)
    {
        declaration.kind = Kind::transition;
        declaration.parsedName = declaration.value.unquoted();

        if (auto parsed = StyleAnimator::parseTransitions (declaration.parsedName); parsed.wasOk())
            declaration.transitions = std::move (parsed.getReference());
        else
            declaration.parseError = "transition: " + parsed.getErrorMessage();
    }
    else if (declaration.name == animationId)
    {
        declaration.kind = Kind::animation;
        declaration.parsedName = declaration.value.unquoted();

        if (declaration.parsedName.trim().isNotEmpty())
        {
            if (auto parsed = StyleAnimator::parseAnimations (declaration.parsedName); parsed.wasOk())
                declaration.animations = std::move (parsed.getReference());
            else
                declaration.parseError = "animation: " + parsed.getErrorMessage();
        }
    }
    else if (declaration.name == shadowId)
    {
        declaration.kind = Kind::shadow;
        declaration.parsedName = declaration.value.unquoted().trim();

        if (declaration.parsedName.isEmpty() || declaration.parsedName == "none")
            return;

        for (const auto& tokens : detail::tokenizeStyleValue (declaration.parsedName))
        {
            float parsedLengths[4] {};
            int lengths = 0;

            while (lengths < jmin (4, tokens.size() - 1))
            {
                const auto length = detail::parseStrictFloat (tokens[lengths]);
                if (! length.has_value())
                    break;

                parsedLengths[lengths++] = *length;
            }

            if (lengths < 2)
            {
                declaration.shadows.clear();
                declaration.parseError = "shadow: Expected 'x y [blur [spread]] color', got '" + tokens.joinIntoString (" ") + "'";
                return;
            }

            declaration.shadows.push_back ({
                .offset = { parsedLengths[0], parsedLengths[1] },
                .blur = lengths > 2 ? jmax (0.0f, parsedLengths[2]) : 0.0f,
                .spread = lengths > 3 ? parsedLengths[3] : 0.0f,
                .colorExpression = tokens.joinIntoString (" ", lengths)
            });
        }
    }
    else if (declaration.name == glowId)
    {
        const auto parsed = declaration.value.unquoted().trim();
        const auto entries = detail::tokenizeStyleValue (parsed);

        if (! looksLikeGlowDeclaration (parsed, entries))
            return;

        declaration.kind = Kind::glow;
        declaration.parsedName = parsed;

        if (parsed.isEmpty() || parsed == "none")
            return;

        for (const auto& tokens : entries)
        {
            float parsedLengths[2] {};
            int lengths = 0;

            while (lengths < jmin (2, tokens.size() - 1))
            {
                const auto length = detail::parseStrictFloat (tokens[lengths]);
                if (! length.has_value())
                    break;

                parsedLengths[lengths++] = *length;
            }

            if (lengths < 1 || detail::parseStrictFloat (tokens[lengths]).has_value())
            {
                declaration.shadows.clear();
                declaration.parseError = "glow: Expected 'blur [spread] color', got '" + tokens.joinIntoString (" ") + "'";
                return;
            }

            declaration.shadows.push_back ({
                .offset = {},
                .blur = jmax (0.0f, parsedLengths[0]),
                .spread = lengths > 1 ? parsedLengths[1] : 0.0f,
                .colorExpression = tokens.joinIntoString (" ", lengths)
            });
        }
    }
}

std::optional<YuiDiagnostic> readDeclarations (const DataTree& parent,
                                               std::vector<StyleSheet::Declaration>& target,
                                               std::map<Identifier, ComponentProperty>& declaredProperties,
                                               bool isKeyframe)
{
    for (const auto& element : parent)
    {
        // A property declared in a rule (`property glow: number [= 0];`) types custom properties and optionally sets a default
        const auto type = element.getType().toString();
        if (type != "declaration" && type != "property")
            continue;

        const auto name = element.getProperty ("name").toString().trim();

        if (name.isEmpty())
            return YuiDiagnostic { .message = "A declaration needs a name", .element = element };

        if (type == "property")
        {
            const auto dot = name.indexOfChar ('.');
            const Identifier unqualified (dot >= 0 ? name.substring (dot + 1) : name);
            const auto propTypeName = element.getProperty ("type").toString().trim();

            if (unqualified.isValid())
            {
                declaredProperties.insert_or_assign (unqualified, ComponentProperty {
                    .name = unqualified,
                    .type = ComponentProperty::getTypeFromName (propTypeName),
                    .elementType = ComponentProperty::getElementTypeFromName (propTypeName),
                    .isInherited = true
                });
            }

            if (! element.hasProperty ("value"))
                continue;
        }

        StyleSheet::Declaration declaration;
        declaration.element = element;

        if (const auto dot = name.indexOfChar ('.'); dot >= 0)
        {
            if (dot == 0 || dot + 1 >= name.length())
                return YuiDiagnostic { .message = "A declaration needs a name", .element = element };

            declaration.qualifier = Identifier (name.substring (0, dot));
            declaration.name = Identifier (name.substring (dot + 1));
            declaration.cascadeKey = Identifier (name);
        }
        else
        {
            declaration.name = Identifier (name);
            declaration.cascadeKey = declaration.name;
        }

        declaration.value = element.getProperty ("value").toString().trim();

        if (declaration.value.isEmpty())
            return YuiDiagnostic { .message = "The declaration '" + name + "' needs a value", .element = element };

        if ((! isKeyframe || declaration.name == Identifier ("filter")) && type == "declaration" && ! declaration.qualifier.isValid())
            parseRuleDeclarationValue (declaration);

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

        const auto color = Color::fromString (element.getProperty (propertyName).toString());

        if (const auto role = ThemePalette::getRoleFromName (propertyText))
            result.setColor (*role, color);
        else if (ThemePalette::isValidCustomColorName (propertyText))
            result.setCustomColor (propertyText, color);
        else
            return YuiDiagnostic { .message = "The palette '" + name + "' can't name a color '" + propertyText + "'", .element = element, .property = propertyName };
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
    const auto collectDeclaredProperties = [this] (const DataTree& container)
    {
        for (const auto& child : container)
        {
            if (child.getType().toString() != "property")
                continue;

            const auto rawName = child.getProperty ("name").toString().trim();
            const auto dot = rawName.indexOfChar ('.');
            const Identifier unqualified (dot >= 0 ? rawName.substring (dot + 1) : rawName);
            const auto propTypeName = child.getProperty ("type").toString().trim();

            if (unqualified.isValid() && propTypeName.isNotEmpty())
            {
                declaredProperties.try_emplace (unqualified, ComponentProperty {
                    .name = unqualified,
                    .type = ComponentProperty::getTypeFromName (propTypeName),
                    .elementType = ComponentProperty::getElementTypeFromName (propTypeName),
                    .isInherited = true
                });
            }
        }
    };

    for (const auto& element : parent)
    {
        const auto type = element.getType().toString();

        if (type == "style")
        {
            collectDeclaredProperties (element);

            if (auto problem = addElements (element))
                return problem;
        }
        else if (type == "rule")
        {
            auto selectors = StyleSelector::parseList (element.getProperty ("selector").toString());

            if (selectors.failed())
                return YuiDiagnostic { .message = selectors.getErrorMessage(), .element = element, .property = "selector" };

            std::vector<Declaration> declarations;

            if (auto problem = readDeclarations (element, declarations, declaredProperties, false))
                return problem;

            for (auto& selector : selectors.getReference())
            {
                for (const auto& property : selector.getReferencedProperties())
                    watchedProperties.addIfNotAlreadyThere (property);

                Rule rule;
                rule.selector = std::move (selector);
                rule.declarations = declarations;
                rule.order = static_cast<int> (rules.size());
                rules.push_back (std::move (rule));
            }
        }
        else if (type == "palette")
        {
            if (auto problem = readPaletteElement (element, palettes))
                return problem;
        }
        else if (type == "drawing")
        {
            const auto name = element.getProperty ("name").toString().trim();

            if (name.isNotEmpty())
                drawings.insert_or_assign (name, element);

            collectDeclaredProperties (element);
        }
        else if (type == "tokens" || type == "component")
        {
            collectDeclaredProperties (element);
        }
        else if (type == "keyframes")
        {
            const auto sequenceName = element.getProperty ("name").toString().trim();

            if (sequenceName.isEmpty())
                return YuiDiagnostic { .message = "A keyframes block needs a name", .element = element };

            KeyframeSequence sequence;
            sequence.name = Identifier (sequenceName);

            for (const auto& frameElement : element)
            {
                if (frameElement.getType().toString() != "keyframe")
                    continue;

                Keyframe frame;
                frame.position = jlimit (0.0, 1.0, static_cast<double> (frameElement.getProperty ("at")));

                if (auto problem = readDeclarations (frameElement, frame.declarations, declaredProperties, true))
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

bool StyleSheet::isStyleElement (const DataTree& element) noexcept
{
    static const Identifier style ("style");
    static const Identifier rule ("rule");
    static const Identifier palette ("palette");
    static const Identifier drawing ("drawing");
    static const Identifier tokens ("tokens");
    static const Identifier component ("component");
    static const Identifier keyframes ("keyframes");

    const auto type = element.getType();
    return type == style || type == rule || type == palette || type == drawing
        || type == tokens || type == component || type == keyframes;
}

std::optional<YuiDiagnostic> StyleSheet::readPaletteElement (const DataTree& element, std::map<String, ThemePalette>& palettes)
{
    return readPalette (element, palettes);
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

const ComponentProperty* StyleSheet::findDeclaredProperty (const Identifier& name) const
{
    const auto it = declaredProperties.find (name);
    return it != declaredProperties.end() ? &it->second : nullptr;
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
