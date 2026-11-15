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
    graph.bind (label, "text", "format (\"gain %.2f\", knob.value)");
    @endcode

    Updates are synchronous: changing a property re-evaluates everything that depends on it
    before the call returns, in dependency order, iteratively rather than recursively. A
    binding that keeps retriggering itself is broken after a few passes and reported through
    @ref getDiagnostics, so a cycle cannot hang the UI.

    A binding is dropped as soon as something else writes its property, whether from code or
    from the user interacting with the widget: an explicit value always wins over a binding.
    Bindings are also dropped when the component they write to, or any component they read
    from, is deleted.

    Two properties can also be linked with @ref link, which keeps them equal in both
    directions, as `.yui` does for `value <=> model.cutoff`.

    Expressions must be free of side effects, since the graph decides when and how often they
    run. Symbols resolve through DrawingScope: bare names and `self.` on the target, `parent.`
    on its parent, `palette.` on the palette, and any other prefix through the resolver.

    @see PropertyBinding, PropertyExpression, DrawingScope, YuiBuilder

    @tags{UI}
*/
class YUP_API BindingGraph
    : private ComponentListener
    , private BindingProvider::Listener
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

    /** Sets the resolver turning a name into a BindingProvider, for expressions referencing models.

        It applies to the bindings created after it is set.
    */
    void setModelResolver (DrawingScope::ModelResolver resolver);

    //==============================================================================
    /** Gives a property of a component its value from an expression.

        The expression is evaluated immediately. If it reads anything that can change, it is
        kept as a live binding, replacing any binding already on that property.

        @param target    The component whose property is written.
        @param property  The name of the property that is written.
        @param text      The expression text.
        @param type      The type of the value, or nothing to probe it from the text.
        @param isLive    Optional flag set to true when the expression reads dynamic inputs, false when constant.

        @returns A failure if the expression does not compile, otherwise ok.
    */
    Result bind (Component& target,
                 const Identifier& property,
                 const String& text,
                 std::optional<ComponentProperty::Type> type = std::nullopt,
                 bool* isLive = nullptr);

    /** Removes the binding on a property, leaving its current value in place.

        @returns True if there was a binding to remove.
    */
    bool unbind (Component& target, const Identifier& property);

    /** Removes every binding of a component, leaving the current values in place. */
    void unbindAll (Component& target);

    /** Removes every binding reading from a provider, leaving the current values in place. */
    void forgetProvider (BindingProvider& provider);

    /** Removes every binding and every link. */
    void clear();

    //==============================================================================
    /** Keeps two properties equal, whichever of them changes.

        The target property takes the value of the source property immediately, then a change
        of either is copied to the other. This is the `property <=> other.property` of `.yui`.
        Unlike a binding, a link is not dropped when one of its properties is written, only
        when one of the two components is deleted or by @ref unlink.

        @param target          The component whose property takes the value first.
        @param property        The property of the target.
        @param source          The component the initial value comes from.
        @param sourceProperty  The property of the source.
    */
    void link (Component& target, const Identifier& property, Component& source, const Identifier& sourceProperty);

    /** Removes every link of a property, leaving the current values in place.

        @returns True if there was a link to remove.
    */
    bool unlink (Component& component, const Identifier& property);

    /** Returns how many links the graph holds. */
    int getNumLinks() const noexcept { return static_cast<int> (links.size()); }

    //==============================================================================
    /** Returns how many live bindings the graph holds. */
    int getNumBindings() const noexcept { return static_cast<int> (bindings.size()); }

    /** Returns the binding on a property, or nullptr if it has none. */
    const PropertyBinding* findBinding (const Component& target, const Identifier& property) const;

    /** Re-evaluates every binding, for example after the palette changed. */
    void refreshAll();

    /** Re-evaluates only the bindings that read a role of the palette. */
    void refreshPaletteBindings();

    //==============================================================================
    /** Returns the problems met while evaluating, such as a binding broken by a cycle. */
    const StringArray& getDiagnostics() const noexcept { return diagnostics; }

    /** Clears the diagnostics. */
    void clearDiagnostics();

    /** Optional sink notified whenever an evaluation-time problem occurs, such as a cycle. */
    std::function<void (Component* target, const Identifier& property, const String& message)> onDiagnostic;

private:
    //==============================================================================
    struct Link
    {
        Component* first = nullptr;
        Identifier firstProperty;
        Component* second = nullptr;
        Identifier secondProperty;
        bool isSyncing = false;

        bool touches (const Component& component, const Identifier& property) const noexcept
        {
            return (first == &component && firstProperty == property) || (second == &component && secondProperty == property);
        }
    };

    void componentPropertyChanged (Component& component, const Identifier& property) override;
    void componentBeingDeleted (Component& component) override;
    void bindingValueChanged (BindingProvider& provider, const Identifier& name) override;
    void bindingProviderBeingDeleted (BindingProvider& provider) override;

    void observe (Component& component);
    void unobserve (Component& component);
    void observe (BindingProvider& provider);
    void unobserve (BindingProvider& provider);
    void addObservedOf (const PropertyBinding& binding);
    void removeObservedOf (const PropertyBinding& binding);
    void removeBinding (int index);
    void removeLink (int index);
    void syncLinks (Component& component, const Identifier& property);
    void invalidate (const Component& component, const Identifier& property);
    void invalidate (const BindingProvider& provider, const Identifier& property);
    void markDirty (PropertyBinding& binding);
    void flush();

    //==============================================================================
    std::vector<std::unique_ptr<PropertyBinding>> bindings;
    std::vector<std::shared_ptr<Link>> links;
    std::vector<PropertyBinding*> dirty;
    std::map<Component*, int> observed;
    std::map<BindingProvider*, int> observedProviders;
    std::map<PropertyBinding*, int> evaluations;
    DrawingScope::ComponentResolver componentResolver;
    DrawingScope::ModelResolver modelResolver;
    PropertyBinding* writingBinding = nullptr;
    StringArray diagnostics;
    bool isFlushing = false;

    static constexpr int maxEvaluationsPerFlush = 16;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BindingGraph)
};

} // namespace yup
