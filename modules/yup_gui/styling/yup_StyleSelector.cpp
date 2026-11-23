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

constexpr int specificityOfId = 10000;
constexpr int specificityOfClass = 100;
constexpr int specificityOfType = 1;

bool isNameCharacter (yup_wchar character) noexcept
{
    return CharacterFunctions::isLetterOrDigit (character) || character == '_' || character == '-';
}

String readName (String::CharPointerType& text)
{
    const auto start = text;

    while (isNameCharacter (*text))
        ++text;

    return String (start, text);
}

void skipWhitespace (String::CharPointerType& text, bool& sawWhitespace)
{
    while (CharacterFunctions::isWhitespace (*text))
    {
        sawWhitespace = true;
        ++text;
    }
}

bool isStateProperty (const Component& component, const Identifier& name)
{
    const auto* descriptor = component.getTypeInfo().findProperty (name);
    return descriptor != nullptr && descriptor->isState;
}

} // namespace

//==============================================================================

Span<const StyleSelector::PseudoEntry> StyleSelector::getBuiltinPseudos() noexcept
{
    static constexpr PseudoEntry builtinPseudos[] = {
        { "hover", PseudoKind::hovered, "hovered" },
        { "hovered", PseudoKind::hovered, "hovered" },
        { "pressed", PseudoKind::pressed, "pressed" },
        { "active", PseudoKind::pressed, "pressed" },
        { "focus", PseudoKind::focused, "focused" },
        { "focused", PseudoKind::focused, "focused" },
        { "focus-within", PseudoKind::focusWithin, "focus-within" },
        { "disabled", PseudoKind::disabled, "enabled" },
        { "enabled", PseudoKind::enabled, "enabled" }
    };

    return builtinPseudos;
}

StringArray StyleSelector::getPseudoClassNames()
{
    StringArray names { "root" };

    for (const auto& entry : getBuiltinPseudos())
        names.add (entry.name);

    return names;
}

ResultValue<StyleSelector> StyleSelector::parse (const String& text)
{
    StyleSelector selector;
    selector.source = text.trim();

    auto position = selector.source.getCharPointer();
    bool sawWhitespace = false;

    skipWhitespace (position, sawWhitespace);

    if (position.isEmpty())
        return makeResultValueFail ("A selector cannot be empty");

    while (! position.isEmpty())
    {
        Compound compound;
        compound.combinatorToPrevious = Combinator::descendant;

        if (! selector.compounds.empty())
        {
            if (*position == ',')
                return makeResultValueFail ("Unexpected ',' in selector '" + selector.source + "'");

            if (*position == '>')
            {
                compound.combinatorToPrevious = Combinator::child;
                ++position;
                skipWhitespace (position, sawWhitespace);
            }
            else if (! sawWhitespace)
            {
                return makeResultValueFail ("Expected a combinator in selector '" + selector.source + "'");
            }
        }

        bool compoundIsEmpty = true;

        if (*position == '*')
        {
            compound.isUniversal = true;
            compoundIsEmpty = false;
            ++position;
        }
        else if (CharacterFunctions::isLetter (*position) || *position == '_')
        {
            compound.type = Identifier (readName (position));
            selector.specificity += specificityOfType;
            compoundIsEmpty = false;
        }

        for (;;)
        {
            if (*position == '.')
            {
                ++position;
                const auto name = readName (position);

                if (name.isEmpty())
                    return makeResultValueFail ("Expected a class name in selector '" + selector.source + "'");

                compound.classes.add (Identifier (name));
                selector.specificity += specificityOfClass;
                compoundIsEmpty = false;
            }
            else if (*position == '#')
            {
                ++position;
                const auto name = readName (position);

                if (name.isEmpty())
                    return makeResultValueFail ("Expected an id in selector '" + selector.source + "'");

                compound.id = name;
                selector.specificity += specificityOfId;
                compoundIsEmpty = false;
            }
            else if (*position == ':')
            {
                ++position;

                if (*position == ':')
                    return makeResultValueFail ("Pseudo-elements are not supported in selector '" + selector.source + "'");

                const auto name = readName (position);

                if (name.isEmpty())
                    return makeResultValueFail ("Expected a pseudo-class in selector '" + selector.source + "'");

                if (name == "root")
                {
                    compound.isRoot = true;
                }
                else
                {
                    bool matchedBuiltin = false;

                    for (const auto& entry : getBuiltinPseudos())
                    {
                        if (name == entry.name)
                        {
                            compound.pseudoClasses.push_back ({ entry.kind, {} });
                            selector.referencedProperties.addIfNotAlreadyThere (Identifier (entry.watchedProperty));
                            matchedBuiltin = true;
                            break;
                        }
                    }

                    if (! matchedBuiltin)
                    {
                        const Identifier prop (name);
                        compound.pseudoClasses.push_back ({ PseudoKind::stateProperty, prop });
                        selector.referencedProperties.addIfNotAlreadyThere (prop);
                    }
                }

                selector.specificity += specificityOfClass;
                compoundIsEmpty = false;
            }
            else if (*position == '[')
            {
                ++position;
                bool ignored = false;
                skipWhitespace (position, ignored);

                const auto name = readName (position);

                if (name.isEmpty())
                    return makeResultValueFail ("Expected a property name in selector '" + selector.source + "'");

                Attribute attribute;
                attribute.name = Identifier (name);

                skipWhitespace (position, ignored);

                if (*position == '!' || *position == '=')
                {
                    attribute.op = *position == '!' ? AttributeOperator::notEquals : AttributeOperator::equals;

                    if (*position == '!')
                    {
                        ++position;

                        if (*position != '=')
                            return makeResultValueFail ("Expected '!=' in selector '" + selector.source + "'");
                    }

                    ++position;
                    skipWhitespace (position, ignored);

                    if (*position == '"' || *position == '\'')
                    {
                        const auto quote = *position;
                        const auto valueStart = ++position;

                        while (! position.isEmpty() && *position != quote)
                            ++position;

                        attribute.value = String (valueStart, position);

                        if (*position != quote)
                            return makeResultValueFail ("Unterminated string in selector '" + selector.source + "'");

                        ++position;
                    }
                    else
                    {
                        const auto valueStart = position;

                        while (! position.isEmpty() && *position != ']' && ! CharacterFunctions::isWhitespace (*position))
                            ++position;

                        attribute.value = String (valueStart, position);
                    }
                }

                skipWhitespace (position, ignored);

                if (*position != ']')
                    return makeResultValueFail ("Expected ']' in selector '" + selector.source + "'");

                ++position;

                compound.attributes.push_back (std::move (attribute));
                selector.specificity += specificityOfClass;
                compoundIsEmpty = false;
            }
            else
            {
                break;
            }
        }

        if (compoundIsEmpty)
            return makeResultValueFail ("Unexpected character in selector '" + selector.source + "'");

        if (compound.id.isNotEmpty())
            selector.referencedProperties.addIfNotAlreadyThere (Identifier ("id"));

        for (const auto& attribute : compound.attributes)
            selector.referencedProperties.addIfNotAlreadyThere (attribute.name);

        selector.compounds.push_back (std::move (compound));

        sawWhitespace = false;
        skipWhitespace (position, sawWhitespace);
    }

    return makeResultValueOk (std::move (selector));
}

ResultValue<std::vector<StyleSelector>> StyleSelector::parseList (const String& text)
{
    const auto trimmed = text.trim();

    if (trimmed.isEmpty())
        return makeResultValueFail ("A selector cannot be empty");

    std::vector<StyleSelector> selectors;
    auto current = trimmed.getCharPointer();
    auto segmentStart = current;
    yup_wchar quote = 0;
    int bracketDepth = 0;

    auto addSegment = [&] (String::CharPointerType segmentEnd) -> Result
    {
        const auto segment = String (segmentStart, segmentEnd).trim();

        if (segment.isEmpty())
            return Result::fail ("Expected a selector in selector list '" + trimmed + "'");

        auto parsed = parse (segment);

        if (parsed.failed())
            return Result::fail (parsed.getErrorMessage());

        selectors.push_back (std::move (parsed.getReference()));
        return Result::ok();
    };

    while (! current.isEmpty())
    {
        const auto ch = *current;

        if (quote != 0)
        {
            if (ch == quote)
                quote = 0;

            ++current;
            continue;
        }

        if (ch == '"' || ch == '\'')
        {
            quote = ch;
        }
        else if (ch == '[')
        {
            ++bracketDepth;
        }
        else if (ch == ']' && bracketDepth > 0)
        {
            --bracketDepth;
        }
        else if (ch == ',' && bracketDepth == 0)
        {
            if (const auto result = addSegment (current); result.failed())
                return makeResultValueFail (result.getErrorMessage());

            ++current;
            segmentStart = current;
            continue;
        }

        ++current;
    }

    if (const auto result = addSegment (current); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    return makeResultValueOk (std::move (selectors));
}

//==============================================================================

bool StyleSelector::Compound::matches (const Component& component, const Component* styleRoot) const
{
    if (isRoot && &component != styleRoot)
        return false;

    if (type.isValid() && ! component.getTypeInfo().isSubtypeOf (type))
        return false;

    if (id.isNotEmpty() && component.getComponentID() != id)
        return false;

    for (const auto& styleClass : classes)
        if (! component.hasStyleClass (styleClass))
            return false;

    for (const auto& pseudoClass : pseudoClasses)
    {
        bool state = false;

        switch (pseudoClass.kind)
        {
            case PseudoKind::hovered:       state = component.isHovered(); break;
            case PseudoKind::pressed:       state = component.isPressed(); break;
            case PseudoKind::focused:       state = component.isFocused(); break;
            case PseudoKind::focusWithin:   state = component.hasFocusWithin(); break;
            case PseudoKind::disabled:      state = ! component.isEnabled(); break;
            case PseudoKind::enabled:       state = component.isEnabled(); break;
            case PseudoKind::stateProperty:
                state = isStateProperty (component, pseudoClass.property)
                     && static_cast<bool> (component.getProperty (pseudoClass.property));
                break;
        }

        if (! state)
            return false;
    }

    for (const auto& attribute : attributes)
    {
        if (attribute.op == AttributeOperator::exists)
        {
            if (! component.hasProperty (attribute.name))
                return false;

            continue;
        }

        const auto value = component.findProperty (attribute.name);
        const bool equal = value.isVoid() ? false : value.toString() == attribute.value;

        if (equal != (attribute.op == AttributeOperator::equals))
            return false;
    }

    return true;
}

//==============================================================================

bool StyleSelector::matches (const Component& component,
                             const Component* styleRoot,
                             const Component* parentOverride) const
{
    if (compounds.empty())
        return false;

    return matchesFrom (component, static_cast<int> (compounds.size()) - 1, styleRoot, parentOverride);
}

bool StyleSelector::matchesFrom (const Component& component,
                                 int compoundIndex,
                                 const Component* styleRoot,
                                 const Component* parentOverride) const
{
    const auto& compound = compounds[static_cast<size_t> (compoundIndex)];

    if (! compound.matches (component, styleRoot))
        return false;

    if (compoundIndex == 0)
        return true;

    if (&component == styleRoot)
        return false;

    const auto& next = compounds[static_cast<size_t> (compoundIndex)];
    auto* parent = parentOverride != nullptr ? parentOverride : component.getParentComponent();

    if (next.combinatorToPrevious == Combinator::child)
        return parent != nullptr && matchesFrom (*parent, compoundIndex - 1, styleRoot, nullptr);

    for (; parent != nullptr; parent = parent->getParentComponent())
    {
        if (matchesFrom (*parent, compoundIndex - 1, styleRoot, nullptr))
            return true;

        if (parent == styleRoot)
            break;
    }

    return false;
}

} // namespace yup
