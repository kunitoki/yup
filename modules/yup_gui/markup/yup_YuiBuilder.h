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
    children, and owns them. Each element is a component of the type named by the element,
    either registered in the ComponentTypeRegistry or declared by a `component` block of the
    document, and its properties are set on it.

    The whole tree is created first, so an expression can reference any element of the
    document whatever the order they are written in, and the values are then applied parents
    before children. An expression that reads something which can change becomes a live
    binding, re-evaluated whenever what it reads changes (see BindingGraph).

    A `component Name : Base { ... }` block declares a type made of other components, with its
    own typed properties, aliases of the properties of its parts and signals. An instance is a
    component of the base type extended with a type info of its own (see
    Component::setTypeInfo), so stylesheets and expressions see it as `Name`. Inside the block,
    `root` is the instance and the IDs are private to it; the values written where the
    component is used win over the ones in the block, and the children written there are added
    after the block's own.

    The `tokens` block declares typed properties of the root component, which every component
    of the document reads by their bare name unless it declares a property of the same name.

    @code
    YuiBuilder builder;
    builder.setAction ("reset", [this] (const Array<var>&) { reset(); });
    builder.setModel ("params", paramsProvider);

    const auto parsed = YuiParser::parse (text);
    if (parsed.wasOk() && builder.build (parsed.document).wasOk())
        addAndMakeVisible (builder.getRoot());
    @endcode

    @see YuiDocument, YuiParser, ComponentTypeRegistry, PropertyExpression, BindingGraph, BindingProvider

    @tags{UI}
*/
class YUP_API YuiBuilder
{
public:
    //==============================================================================
    /** A function run by a handler such as `onClick: action ("reset");`, receiving the values of the call. */
    using Action = std::function<void (const Array<var>& arguments)>;

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

    /** Destroys every component the builder created. Actions and models are kept. */
    void clear();

    //==============================================================================
    /** Returns the root component, or nullptr if nothing is built. */
    Component* getRoot() const noexcept;

    /** Returns the component with the given ID, or nullptr.

        Only the IDs of the document itself are found: those inside `component` blocks are
        private to each instance.
    */
    Component* findById (const String& id) const;

    /** Returns the component with the given ID and type, or nullptr. */
    template <class T>
    T* findById (const String& id) const
    {
        return dynamic_cast<T*> (findById (id));
    }

    /** Returns the type info of a type declared by the built document or registered, or nullptr. */
    const ComponentTypeInfo* findTypeInfo (const Identifier& typeName);

    /** Returns the problems met by the last build, one per value that couldn't be applied. */
    const StringArray& getDiagnostics() const noexcept { return diagnostics; }

    //==============================================================================
    /** Registers the function run by `action ("name", ...)` handlers, or removes it when null.

        Actions are looked up when the handler runs, so they can be set before or after building.
        Don't rebuild synchronously from inside an action, as the component emitting the signal
        would be destroyed while it runs.
    */
    void setAction (const String& name, Action action);

    /** Makes the values of a provider readable and writable as `name.property`.

        Set models before building: replacing or removing one drops the bindings and links
        made to the previous one. The provider must outlive the builder or be removed first.
    */
    void setModel (const String& name, BindingProvider& provider);

    /** Removes a model, dropping the bindings and links made to it. */
    void removeModel (const String& name);

    //==============================================================================
    /** Returns the bindings keeping the built properties up to date. */
    BindingGraph& getBindings() noexcept { return bindings; }

private:
    //==============================================================================
    class Model;

    struct Scope
    {
        Component* root = nullptr;
        std::map<String, Component*> ids;
    };

    struct Composite
    {
        DataTree declaration;
        DataTree body;
        std::unique_ptr<ComponentTypeInfo> info;
        String error;
        bool isResolving = false;
        bool isInstantiating = false;
    };

    struct Pending
    {
        Component* component = nullptr;
        Scope* scope = nullptr;
        std::vector<std::pair<Identifier, String>> values;
        std::vector<std::pair<Identifier, String>> links;
    };

    struct Connection
    {
        Component* component = nullptr;
        Identifier signal;
        ComponentSignal::Handler handler;
    };

    //==============================================================================
    void collectDeclarations (const DataTree& document);
    ResultValue<const ComponentTypeInfo*> resolveType (const Identifier& typeName);
    Result resolveComposite (const Identifier& typeName, Composite& composite);
    ResultValue<Component*> createComponent (const DataTree& element, Component* parent, Scope& scope);
    void applyIdentity (Component& component, const DataTree& element, Scope& scope, bool addClasses);
    void applyTokens (Component& root, Scope& scope);
    void applyPending (const Pending& pending);
    void applyProperty (Component& component, const Identifier& name, const String& valueText, Scope& scope);
    void applyLink (Component& component, const Identifier& name, const String& path, Scope& scope);
    void connectHandler (Component& component, const ComponentSignal& signal, const String& text, Scope& scope);
    void emit (Component& component, const Identifier& signal, const Array<var>& arguments);
    bool isDeclaredSignal (const Component& component, const Identifier& signal) const;
    Component* resolve (const Scope& scope, const String& name, Component* self = nullptr) const;
    DrawingScope::ComponentResolver resolverFor (const Scope& scope) const;

    std::vector<std::unique_ptr<Component>> components;
    std::vector<std::unique_ptr<Scope>> scopes;
    std::map<Identifier, Composite> composites;
    std::unique_ptr<ComponentTypeInfo> tokensInfo;
    std::vector<DataTree> tokens;
    std::vector<Pending> pending;
    std::vector<Connection> connections;
    std::map<String, Action> actions;
    std::map<String, std::unique_ptr<Model>> models;
    BindingGraph bindings;
    StringArray diagnostics;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiBuilder)
};

} // namespace yup
