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
    One property kept up to date by an expression.

    A PropertyBinding owns a compiled PropertyExpression and the property it writes. It
    knows every component property the expression reads, so whoever owns it can re-evaluate
    it when one of them changes. BindingGraph does exactly that, and is how bindings are
    normally created; use this class directly only to drive the updates yourself.

    @see BindingGraph, PropertyExpression, DrawingScope

    @tags{UI}
*/
class YUP_API PropertyBinding
{
public:
    //==============================================================================
    /** Creates a binding writing a property of a component.

        @param target      The component whose property the expression writes.
        @param property    The name of the property that is written.
        @param expression  The compiled expression, never null.
    */
    PropertyBinding (Component& target, const Identifier& property, std::unique_ptr<PropertyExpression> expression);

    /** Destructor. */
    ~PropertyBinding();

    //==============================================================================
    /** Returns the component the binding writes to, or nullptr once it has been deleted. */
    Component* getTarget() const noexcept { return target.get(); }

    /** Returns the name of the property the binding writes. */
    const Identifier& getProperty() const noexcept { return property; }

    /** Returns the text the binding was compiled from. */
    const String& getSource() const noexcept { return expression->getSource(); }

    //==============================================================================
    /** Returns the component properties the expression reads. */
    const std::vector<DrawingScope::Dependency>& getDependencies() const noexcept { return dependencies; }

    /** Returns true if the expression reads the given property of the given component. */
    bool dependsOn (const Component& component, const Identifier& name) const;

    /** Returns true if the expression reads the given property of the given binding provider. */
    bool dependsOn (const BindingProvider& provider, const Identifier& name) const;

    /** Returns true if the expression reads a role of the palette. */
    bool readsPalette() const noexcept { return expression->readsPalette(); }

    //==============================================================================
    /** Evaluates the expression and writes the result into the target property.

        @returns True if the target is alive and the value it holds changed.
    */
    bool evaluate();

private:
    WeakReference<Component> target;
    Identifier property;
    std::unique_ptr<PropertyExpression> expression;
    std::vector<DrawingScope::Dependency> dependencies;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyBinding)
};

} // namespace yup
