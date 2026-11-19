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
    A single value expression evaluated against a component.

    A PropertyExpression compiles one value text, such as `palette.accent.brighter (0.2)`
    or `parent.width * 0.5`, so that it can be evaluated again and again against the live
    properties and palette of a component (see DrawingScope for the symbols it resolves).
    It is how stylesheet declarations and `.yui` instance properties become values.

    The value type comes from the property it is meant for. Without a type, as for custom
    properties, the expression is compiled once and its type is inferred from its output.

    A text such as `[1, gain.value]` is an array literal: each element is an expression of its
    own, of any type, and the value is a var holding them all.

    @code
    auto expression = PropertyExpression::compile (slider, "palette.accent.withAlpha (0.5)", ComponentProperty::Type::color);
    if (expression.wasOk())
        slider.setProperty ("thumbColor", expression.getReference()->evaluate());
    @endcode

    @see DrawingScope, ComponentProperty, StyleContext, YuiBuilder

    @tags{UI}
*/
class YUP_API PropertyExpression
{
public:
    //==============================================================================
    /** Compiles a value text against a component.

        @param component  The component the expression reads its symbols from.
        @param text       The expression text.
        @param type       The type of the value, or nothing to probe it from the text.
        @param resolver   The resolver for references to other named components or models, or nothing.
        @param locals     Values read by their bare name before the properties of the component, or
                          nullptr. Not owned, it must outlive the expression (see DrawingScope::setLocals).
        @param baseDirectory  The folder relative file paths resolve against, usually the one of the
                              `.yui` file, or File() for the working directory. It is where
                              `font ("fonts/Inter.ttf")` and `font ("fonts/Inter.ttf", 14)` load
                              their font file from; a file that can't be loaded gives the default
                              font of the theme.

        @returns The compiled expression, or the error of the first type tried.
    */
    static ResultValue<std::unique_ptr<PropertyExpression>> compile (Component& component,
                                                                     const String& text,
                                                                     std::optional<ComponentProperty::Type> type,
                                                                     DrawingScope::ComponentResolver resolver = nullptr,
                                                                     Component* locals = nullptr,
                                                                     const File& baseDirectory = {});

    /** Splits an array literal such as `[1, gain.value, [2, 3]]` into the texts of its elements.

        Each element is an expression of its own, and a nested literal is one element. The elements
        are added to @p elements, which is not cleared.

        @param text       The text the literal is in, which may hold more than the literal.
        @param elements   Receives the text of each element.
        @param offset     In: the UTF-8 byte offset of the `[`, possibly preceded by whitespace.
                          Out: the offset just after the `]`, or of the problem when it fails.

        @returns Whether the literal is well formed, the message of a failure carrying no position.
    */
    static Result splitArrayLiteral (StringRef text, StringArray& elements, int& offset);

    ~PropertyExpression();

    //==============================================================================
    /** Evaluates the expression with the current state of the component.

        Colors, points and rectangles are returned in their var form (see VariantConverter),
        numbers as doubles, and arrays as a var holding the values of their elements.
    */
    var evaluate();

    /** Returns the text the expression was compiled from. */
    const String& getSource() const noexcept { return source; }

    /** Returns the type of the value the expression produces. */
    ComponentProperty::Type getType() const noexcept { return type; }

    /** Returns true if the expression reads a role of the palette, so its value follows palette switches. */
    bool readsPalette() const noexcept { return paletteDependent; }

    /** Returns true if the expression reads any symbol, so its value can change over time. */
    bool hasInputs() const noexcept;

    /** Returns the component properties the expression reads, so changes to them can be tracked. */
    std::vector<DrawingScope::Dependency> getDependencies() const;

private:
    explicit PropertyExpression (ExpressionContext contextToUse = {});

    static ResultValue<std::unique_ptr<PropertyExpression>> compileArrayLiteral (Component& component,
                                                                                 const String& text,
                                                                                 std::optional<ComponentProperty::Type> type,
                                                                                 DrawingScope::ComponentResolver resolver,
                                                                                 Component* locals,
                                                                                 const File& baseDirectory);

    String source;
    ComponentProperty::Type type = ComponentProperty::Type::number;
    ComponentProperty::Type elementType = ComponentProperty::Type::any;
    std::vector<std::unique_ptr<PropertyExpression>> elements;
    ExpressionContext context;
    std::optional<DrawingScope> scope;
    CompiledExpression program;
    std::optional<ExpressionState> state;
    DrawingScope::BoundInputs inputs;
    bool paletteDependent = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyExpression)
};

} // namespace yup
