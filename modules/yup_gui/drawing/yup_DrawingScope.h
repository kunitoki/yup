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
    Resolves a component's reflected properties as expression symbols.

    A DrawingScope bridges the reflected component model and the expression compiler:
    it lets an expression reference a component's properties by name, so text such as
    `value * width` resolves against the live component. A bare name or the `self.`
    prefix refers to the bound component, while `parent.` refers to its parent.

    Use it in two steps. Call @ref install before compiling to register the symbol
    resolver on an ExpressionContext, so property names resolve to their types. Then,
    on each evaluation, call @ref updateInputs to push the component's current property
    values into an ExpressionState.

    Only the property types an expression input can carry are bridged: number, integer,
    boolean, string and color. Colors require the graphics expression types to be
    registered on the context (see addGraphicsExpressionTypes); other property types are
    left unresolved, so referencing them is a compile error.

    @see ExpressionContext, CompiledExpression, ExpressionState, ComponentProperty

    @tags{UI}
*/
class YUP_API DrawingScope
{
public:
    //==============================================================================
    /** Creates a scope over a component, resolved against a context.

        @param component   The component whose reflected properties become symbols.
        @param context     The context the symbols are resolved against. Register the
                           graphics expression types on it first for colors to resolve.
    */
    DrawingScope (Component& component, ExpressionContext& context);

    //==============================================================================
    /** Registers the symbol resolver on the context, replacing any previous one.

        Call this before compiling an expression against the context.
    */
    void install();

    /** Writes the component's live property values into a state before evaluation.

        For every input of the program that names a reflected property of the bound
        component (directly, or through `self.` or `parent.`), the current value is
        written to the state. Inputs that match no property are left untouched, and a
        destroyed component is a no-op.

        @param program   A program compiled against this scope's context.
        @param state     A state created from the same program.
    */
    void updateInputs (const CompiledExpression& program, ExpressionState& state) const;

private:
    //==============================================================================
    Component* resolveTarget (const String& symbol, String& propertyName) const;
    const ExpressionType* typeForProperty (ComponentProperty::Type type) const;

    WeakReference<Component> component;
    ExpressionContext& context;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingScope)
};

} // namespace yup
