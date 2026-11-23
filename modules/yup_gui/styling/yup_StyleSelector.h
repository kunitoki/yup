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

#pragma once

namespace yup
{

//==============================================================================
/**
    A compiled stylesheet selector, matched against live components.

    Specificity follows CSS too: ids count 10000, classes, attributes and pseudo-classes 100,
    and types 1, so more specific rules win inside a StyleSheet.

    @code
    const auto selector = StyleSelector::parse (".panel > Slider:hover");
    if (selector.wasOk() && selector.getReference().matches (slider, &root))
        ...

    const auto list = StyleSelector::parseList ("TextButton, ComboBox:open");
    @endcode

    @see StyleSheet, StyleContext

    @tags{UI}
*/
class YUP_API StyleSelector
{
public:
    //==============================================================================
    /** Creates an empty selector that matches nothing. */
    StyleSelector() = default;

    //==============================================================================
    /** Parses a single selector.

        @param text The selector text.

        @returns The compiled selector, or a failure describing the syntax error.
    */
    static ResultValue<StyleSelector> parse (const String& text);

    /** Parses a comma-separated list of one or more selectors.

        Commas inside attribute brackets (`[text="a, b"]`) and quoted strings are preserved,
        and each selector in the list is compiled independently with its own specificity.

        @param text The selector or comma-separated selector list text.

        @returns The compiled selectors in source order, or a failure describing the syntax error.
    */
    static ResultValue<std::vector<StyleSelector>> parseList (const String& text);

    /** Returns the pseudo-classes every component has, such as `hover` and `disabled`, with `root`,
        for a tool to list them. A selector also takes the state properties of a type as
        pseudo-classes, such as the `checked` of a ToggleButton.
    */
    static StringArray getPseudoClassNames();

    //==============================================================================
    /** Returns true when this selector matches a component.

        @param component      The component to match.
        @param styleRoot      The component the stylesheet is attached to, matched by `:root`,
                              and the point where the ancestor walk stops. May be nullptr.
        @param parentOverride Optional logical parent used for combinator matching on @p component
                              instead of @ref Component::getParentComponent (for transient popups).
    */
    bool matches (const Component& component,
                  const Component* styleRoot = nullptr,
                  const Component* parentOverride = nullptr) const;

    //==============================================================================
    /** Returns the CSS specificity of this selector. */
    int getSpecificity() const noexcept { return specificity; }

    /** Returns true when the selector matches nothing. */
    bool isEmpty() const noexcept { return compounds.empty(); }

    /** Returns the property names the selector reads, so a change can trigger a restyle.

        These are the attribute selector names and the state pseudo-classes that are not
        tracked by the core (`:checked`, `:dragging`, ...).
    */
    const Array<Identifier>& getReferencedProperties() const noexcept { return referencedProperties; }

    /** Returns the selector text it was parsed from. */
    const String& toString() const noexcept { return source; }

private:
    //==============================================================================
    enum class AttributeOperator
    {
        exists,
        equals,
        notEquals
    };

    struct Attribute
    {
        Identifier name;
        String value;
        AttributeOperator op = AttributeOperator::exists;
    };

    enum class Combinator
    {
        descendant,
        child
    };

    enum class PseudoKind
    {
        hovered,
        pressed,
        focused,
        focusWithin,
        disabled,
        enabled,
        stateProperty
    };

    struct PseudoClass
    {
        PseudoKind kind = PseudoKind::stateProperty;
        Identifier property;
    };

    struct PseudoEntry
    {
        const char* name;
        PseudoKind kind;
        const char* watchedProperty;
    };

    static Span<const PseudoEntry> getBuiltinPseudos() noexcept;

    struct Compound
    {
        Identifier type;
        String id;
        Array<Identifier> classes;
        std::vector<PseudoClass> pseudoClasses;
        std::vector<Attribute> attributes;
        Combinator combinatorToPrevious = Combinator::descendant;
        bool isRoot = false;
        bool isUniversal = false;

        bool matches (const Component& component, const Component* styleRoot) const;
    };

    bool matchesFrom (const Component& component,
                      int compoundIndex,
                      const Component* styleRoot,
                      const Component* parentOverride) const;

    std::vector<Compound> compounds;
    Array<Identifier> referencedProperties;
    String source;
    int specificity = 0;
};

} // namespace yup
