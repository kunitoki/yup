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
    Keeps properties up to date as the properties they are computed from change.

    A binding is an expression that gives a property its value. The graph records every
    property the expression reads while compiling it, and re-evaluates the binding whenever
    one of them changes, so a value written once in markup stays correct for the life of the
    UI:

    @code
    BindingGraph graph;
    graph.setComponentResolver ([&] (const String& id) { return document.findById (id); });
    graph.bind (label, "text", "\"gain \" + knob.value");
    @endcode

    Updates are synchronous: changing a property re-evaluates everything that depends on it
    before the call returns, in dependency order, iteratively rather than recursively. A
    binding that keeps retriggering itself is broken after a few passes and reported through
    @ref getDiagnostics, so a cycle cannot hang the UI.

    A binding is dropped as soon as something else writes its property, whether from code or
    from the user interacting with the widget: an explicit value always wins over a binding.
    Bindings are also dropped when the component they write to, or any component they read
    from, is deleted.

    Expressions must be free of side effects, since the graph decides when and how often they
    run. Symbols resolve through DrawingScope: bare names and `self.` on the target, `parent.`
    on its parent, `palette.` on the palette, and any other prefix through the resolver.

    @see PropertyBinding, PropertyExpression, DrawingScope, YuiBuilder

    @tags{UI}
*/
class YUP_API BindingGraph : private ComponentListener
{
public:
    //==============================================================================
    /** Creates an empty graph. */
    BindingGraph();

    /** Destructor, removing every binding. */
    ~BindingGraph() override;

    //==============================================================================
    /** Sets the resolver turning a name into a component, for expressions referencing other elements.

        It applies to the bindings created after it is set.
    */
    void setComponentResolver (DrawingScope::ComponentResolver resolver);

    //==============================================================================
    /** Gives a property of a component its value from an expression.

        The expression is evaluated immediately. If it reads anything that can change, it is
        kept as a live binding, replacing any binding already on that property.

        @param target    The component whose property is written.
        @param property  The name of the property that is written.
        @param text      The expression text.
        @param type      The type of the value, or nothing to probe it from the text.

        @returns A failure if the expression does not compile, otherwise ok.
    */
    Result bind (Component& target,
                 const Identifier& property,
                 const String& text,
                 std::optional<ComponentProperty::Type> type = std::nullopt);

    /** Removes the binding on a property, leaving its current value in place.

        @returns True if there was a binding to remove.
    */
    bool unbind (Component& target, const Identifier& property);

    /** Removes every binding of a component, leaving the current values in place. */
    void unbindAll (Component& target);

    /** Removes every binding. */
    void clear();

    //==============================================================================
    /** Returns how many live bindings the graph holds. */
    int getNumBindings() const noexcept { return static_cast<int> (bindings.size()); }

    /** Returns the binding on a property, or nullptr if it has none. */
    const PropertyBinding* findBinding (const Component& target, const Identifier& property) const;

    /** Re-evaluates every binding, for example after the palette changed. */
    void refreshAll();

    //==============================================================================
    /** Returns the problems met while evaluating, such as a binding broken by a cycle. */
    const StringArray& getDiagnostics() const noexcept { return diagnostics; }

    /** Clears the diagnostics. */
    void clearDiagnostics();

private:
    //==============================================================================
    void componentPropertyChanged (Component& component, const Identifier& property) override;
    void componentMoved (Component& component) override;
    void componentResized (Component& component) override;
    void componentBeingDeleted (Component& component) override;

    void observe (Component& component);
    void unobserve (Component& component);
    void addObservedOf (const PropertyBinding& binding);
    void removeObservedOf (const PropertyBinding& binding);
    void removeBinding (int index);
    void invalidate (const Component& component, const Identifier& property);
    void markDirty (PropertyBinding& binding);
    void flush();

    //==============================================================================
    std::vector<std::unique_ptr<PropertyBinding>> bindings;
    std::vector<PropertyBinding*> dirty;
    std::map<Component*, int> observed;
    std::map<PropertyBinding*, int> evaluations;
    DrawingScope::ComponentResolver componentResolver;
    PropertyBinding* writingBinding = nullptr;
    StringArray diagnostics;
    bool isFlushing = false;

    static constexpr int maxEvaluationsPerFlush = 16;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BindingGraph)
};

} // namespace yup
