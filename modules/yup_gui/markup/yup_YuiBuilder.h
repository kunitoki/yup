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
    Creates the components of a `.yui` document.

    The builder instantiates the root component of a parsed document (see YuiParser) and its
    children through the ComponentTypeRegistry, and owns them. Each element is a component of
    the registered type named by the element, and its properties are set on it:

    - `id` becomes the component ID, found again with @ref findById;
    - `class` sets the style classes, several of them separated by spaces in a string;
    - every other property is an expression evaluated against the new component, so it can
      read the properties of its parent, like `width: parent.width - 20;`, the properties of
      another element by its ID, like `text: knob.value;`, and written with
      Component::setProperty. Enumerations take their value as a bare name.

    The whole tree is created first, so an expression can reference any element of the
    document whatever the order they are written in, and the values are then applied parents
    before children. An expression that reads something which can change becomes a live
    binding, re-evaluated whenever what it reads changes (see BindingGraph).

    @code
    YuiBuilder builder;
    const auto parsed = YuiParser::parse (text);
    if (parsed.wasOk() && builder.build (parsed.document).wasOk())
        addAndMakeVisible (builder.getRoot());
    @endcode

    @see YuiParser, ComponentTypeRegistry, PropertyExpression, BindingGraph

    @tags{UI}
*/
class YUP_API YuiBuilder
{
public:
    //==============================================================================
    /** Creates a builder, registering the built-in component types if they aren't yet. */
    YuiBuilder();
    ~YuiBuilder();

    //==============================================================================
    /** Builds the root component of a document, replacing what was built before.

        @param document A `yui` document, or the element of a single component.

        @returns A failure when a component can't be created, in which case nothing is
                 built. Properties that can't be set are reported by @ref getDiagnostics.
    */
    Result build (const DataTree& document);

    /** Destroys every component the builder created. */
    void clear();

    //==============================================================================
    /** Returns the root component, or nullptr if nothing is built. */
    Component* getRoot() const noexcept;

    /** Returns the first built component with the given ID, or nullptr. */
    Component* findById (const String& id) const;

    /** Returns the first built component with the given ID and type, or nullptr. */
    template <class T>
    T* findById (const String& id) const
    {
        return dynamic_cast<T*> (findById (id));
    }

    /** Returns the problems met by the last build, one per property that couldn't be set. */
    const StringArray& getDiagnostics() const noexcept { return diagnostics; }

    //==============================================================================
    /** Returns the bindings keeping the built properties up to date. */
    BindingGraph& getBindings() noexcept { return bindings; }

private:
    //==============================================================================
    Result createComponent (const DataTree& element, Component* parent);
    void applyProperties (Component& component, const DataTree& element);
    void applyProperty (Component& component, const Identifier& name, const String& valueText);

    std::vector<std::unique_ptr<Component>> components;
    std::vector<std::pair<Component*, DataTree>> elements;
    BindingGraph bindings;
    StringArray diagnostics;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiBuilder)
};

} // namespace yup
