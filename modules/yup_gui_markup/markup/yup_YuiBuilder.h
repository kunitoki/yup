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

class YuiPropertyWatcher;
class YuiDoubleClickWatcher;
class YuiHandlerArguments;

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

    Building again updates the components in place. A component is kept when its element is
    the same DataTree object as in the previous build, under the same parent and of the same
    type, which is what patching the built tree with YuiTreeDiff gives. Kept components keep
    their pointers and their state: a constant value that is still written the same isn't set
    again, so what the user changed survives, while expressions, links and handlers are always
    applied again. The values the document no longer writes go back to what a new component
    has, and the components whose elements are gone are destroyed.

    What a component needs from the application and a document can't write, such as the state a
    keyboard or a meter shows, is given by an initializer each time the component is created
    (see @ref setInitializer).

    @code
    YuiBuilder builder;
    builder.setAction ("reset", [this] (const Array<var>&) { reset(); });
    builder.setModel ("params", paramsProvider);
    builder.setInitializer<MyMeter> ([this] (MyMeter& meter) { meter.setSource (&levels); });

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

        @returns A failure when a component can't be created, in which case what was built
                 before is kept unchanged. The failure and the properties that can't be set
                 are reported by @ref getDiagnostics.
    */
    Result build (const DataTree& document);

    /** Destroys every component the builder created. Actions, models and initializers are kept. */
    void clear();

    //==============================================================================
    /** Sets the base directory used to resolve relative asset paths (the files of `font ("...")` values). */
    void setBaseDirectory (const File& directory)
    {
        baseDirectory = directory;
        bindings.setBaseDirectory (directory);
    }

    /** Returns the base directory used to resolve relative asset paths. */
    const File& getBaseDirectory() const noexcept { return baseDirectory; }

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

    //==============================================================================
    /** Returns the document element a built component came from, or an invalid tree.

        Development tools use this to map a component picked on screen back to the markup that
        describes it, so an edit can be written to the document rather than to the component.

        For an instance of a composite type this is the use site element, not the body of the
        declaration.

        @param component The component to look up.
    */
    DataTree findElementFor (const Component* component) const;

    /** Returns the component built from a document element, or nullptr.

        @param element The element to look up.
    */
    Component* findComponentFor (const DataTree& element) const;

    /** Returns the problems met by the last build.

        There is one per value that couldn't be applied, plus the error that made the build fail.
        Each holds the element of the document and the property it is about, which the
        DataTreeSourceMap of the parsed document turns into a line and a column.
    */
    const std::vector<DataTreeDiagnostic>& getDiagnostics() const noexcept { return diagnostics; }

    /** Optional sink notified when a live binding reports a problem after or during build. */
    std::function<void (const DataTreeDiagnostic&)> onDiagnostic;

    /** Optional hook invoked once the built DataTree was mutated in place, when its changes are
        applied (see @ref applyPendingChanges). When null, the builder builds its watched DataTree
        again itself.
    */
    std::function<void()> onTreeMutated;

    /** Optional hook invoked when the components of the `repeat` blocks have to be made again,
        as the range of one changed. When null, the builder builds its document again itself.
    */
    std::function<void()> onRangeChanged;

    /** Optional hook invoked each time a build has made, or kept, the root of its components:
        before the components under it are made and before any of them is given its values.
    */
    std::function<void (Component&)> onRootCreated;

    /** Applies, now, the changes waiting for the message loop.

        The builder watches the tree it built, and the ranges of its `repeat` blocks of components,
        which can read what changes, such as the property of a model. When the tree is mutated in
        place or a range changes, the document is built again once the change is over, from the
        message loop, keeping the components that are still described. However many changes are
        made before that, they lead to one build. A tool or a test with no message loop calls this
        to have it done. It does nothing when nothing changed.

        The changes are not applied as they are made so that a handler or an action can make them:
        the components it runs for are still there when it returns.
    */
    void applyPendingChanges();

    /** Temporarily suppresses automatic rebuilds triggered by DataTree mutations. */
    void setIgnoreTreeChanges (bool shouldIgnore) noexcept { ignoreTreeChanges = shouldIgnore; }

    /** Returns true when DataTree mutation notifications are currently suppressed. */
    bool isIgnoringTreeChanges() const noexcept { return ignoreTreeChanges; }

    /** Returns true when elements were added to the built DataTree, removed from it or moved in
        it since the last build, done or failed.

        A DataTreeSourceMap made for the tree no longer places its elements then, as it knows
        them by their index among their siblings.
    */
    bool hasTreeStructureChanged() const noexcept;

    //==============================================================================
    /** Registers the function run by `action ("name", ...)` handlers, or removes it when null.

        Actions are looked up when the handler runs, so they can be set before or after building.
        An action can change the built DataTree, which is applied once the handler is over (see
        @ref applyPendingChanges). Don't build again from inside an action: it ends the handler,
        whose calls left are not run, and the component emitting the signal may be destroyed
        while it runs.
    */
    void setAction (const String& name, Action action);

    /** The function run for the actions no function is registered for, given the name of the action and its arguments. */
    using FallbackAction = std::function<void (const String& name, const Array<var>& arguments)>;

    /** Sets the function run by the `action ("name", ...)` handlers that setAction() registered
        no function for, or none when null, which leaves them doing nothing.

        It lets a tool showing a document of an application it does not know tell which
        actions the interface calls.
    */
    void setFallbackAction (FallbackAction action);

    /** Makes the values of a provider readable and writable as `name.property`.

        Set models before building: replacing or removing one drops the bindings and links
        made to the previous one. The provider must outlive the builder or be removed first.
    */
    void setModel (const String& name, BindingProvider& provider);

    /** Removes a model, dropping the bindings and links made to it. */
    void removeModel (const String& name);

    //==============================================================================
    /** A function setting up a component of a type the builder created. */
    template <class ComponentType>
    using Initializer = std::function<void (ComponentType&)>;

    /** Sets the function setting up every component of a type the builder creates, or removes it when null.

        This is how a component is given what a document can't write, such as the object of the
        application it shows. The builder creates the components again when the document changes,
        so they are handed it each time, which a call made once after building wouldn't do.

        @code
        builder.setInitializer<MidiKeyboardComponent> ([this] (MidiKeyboardComponent& keyboard)
        {
            keyboard.setState (&processor.getKeyboardState());
        });
        @endcode

        The function runs for the components of the type and of the ones derived from it, which
        the instances of a `component` block based on the type are. It runs once the whole tree is
        created and before any value of the document is applied, so the document has the last
        word on what both set. A component kept by a build isn't set up again, unless its ID
        changed, in which case it is given all its values again too.

        Initializers can be set before or after building: one set later runs right away on the
        components already built. They run in the order they were set, the ones of an ID after
        the others, and are kept by @ref clear. What a function captures must outlive the builder,
        or the function be removed first. Don't build, clear or set an initializer from inside one.

        @param initializer  The function, given each component of the type.
    */
    template <class ComponentType>
    void setInitializer (Initializer<ComponentType> initializer)
    {
        setInitializer<ComponentType> (String(), std::move (initializer));
    }

    /** Sets the function setting up the components of a type that have a given ID, or removes it when null.

        It runs after the initializers of the type that name no ID, whatever the order they were
        set in, so it wins over them. The ID is the one the element is written with, which makes
        it reach the components `findById` can't: those inside a `component` block or a `repeat`.

        @code
        builder.setInitializer<SpectrumAnalyzerComponent> ("output", [this] (SpectrumAnalyzerComponent& analyzer)
        {
            analyzer.setState (&processor.getOutputAnalyzerState());
        });
        @endcode

        @param id           The ID of the components to set up.
        @param initializer  The function, given each component of the type with that ID.
    */
    template <class ComponentType>
    void setInitializer (const String& id, Initializer<ComponentType> initializer)
    {
        ComponentInitializer::Function function;

        if (initializer != nullptr)
        {
            function = [initializer = std::move (initializer)] (Component& component)
            {
                auto* target = dynamic_cast<ComponentType*> (&component);

                if (target != nullptr)
                    initializer (*target);

                return target != nullptr;
            };
        }

        setComponentInitializer ({ std::type_index (typeid (ComponentType)), id, std::move (function) });
    }

    //==============================================================================
    /** Returns the bindings keeping the built properties up to date. */
    BindingGraph& getBindings() noexcept { return bindings; }

    /** Returns the bindings keeping the built properties up to date, to read them. */
    const BindingGraph& getBindings() const noexcept { return bindings; }

    /** Gives the components whose `palette` names one with a dark and a light version the
        version for the mode of the application theme. Call it when that mode changes, as
        YuiDocument does.
    */
    void refreshThemePalettes();

    /** Returns a resolver looking up components and models in the top-level document scope. */
    DrawingScope::ComponentResolver getComponentResolver() const;

private:
    //==============================================================================
    // The names the expressions of a part of the document read. It is shared so that what reads it past the build
    // it is of, which frees it, can tell
    struct Scope : std::enable_shared_from_this<Scope>
    {
        Component* root = nullptr;
        std::map<String, Component*> ids;

        // A turn of a repeat: the scope around it, where the ids it does not hold are looked for, and
        // the indices of the repeats it is in, which its expressions read by their name
        const Scope* outer = nullptr;
        StringArray indexNames;
        Array<var> indexValues;
        std::shared_ptr<YuiHandlerArguments> locals;
    };

    // A value or link written by the document, with the element and property it was written in
    struct Value
    {
        Identifier name;
        String text;
        DataTree element;
        Identifier property;
    };

    struct Composite
    {
        DataTree declaration;
        DataTree body;
        std::unique_ptr<ComponentTypeInfo> info;
        std::vector<Value> defaults;
        std::vector<Value> aliases;
        String error;
        bool isResolving = false;
        bool isInstantiating = false;
    };

    // A component, identified by the element it was built from (the use site for a composite), its parent
    // and, for one a repeat makes, the turn of the repeat it is made in. A kept one tells the id it had,
    // which is the one the initializers set it up with
    struct Record
    {
        DataTree element;
        Component* parent = nullptr;
        const ComponentTypeInfo* type = nullptr;
        std::unique_ptr<Component> component;
        String turn;
        std::optional<String> previousId;
    };

    // What sets up the components of a type, the ones of an id when it names one. The function tells
    // whether the component is of the type
    struct ComponentInitializer
    {
        using Function = std::function<bool (Component&)>;

        std::type_index type;
        String id;
        Function function;
    };

    // The range of a repeat of components: the properties of the parent holding its ends, which are
    // bound to what the document says once the values are applied, and what they were when the
    // components were made
    struct RepeatRange
    {
        Component* parent = nullptr;
        Scope* scope = nullptr;
        DataTree element;
        Identifier fromName;
        Identifier toName;
        var usedFrom;
        var usedTo;
    };

    enum class ApplyOutcome
    {
        failed,
        constant,
        live
    };

    // The final text of a value written by the document, and whether it was applied as a constant
    struct Applied
    {
        String text;
        DataTree element;
        Identifier property;
        bool isLink = false;
        bool isConstant = false;
        bool isKept = false;
    };

    using AppliedValues = std::unordered_map<Component*, std::unordered_map<Identifier, Applied>>;

    struct Pending
    {
        Component* component = nullptr;
        Scope* scope = nullptr;
        std::vector<Value> values;
        std::vector<Value> links;
        std::vector<Value> changes;
    };

    struct Connection
    {
        Component* component = nullptr;
        Identifier signal;
        ComponentSignal::Handler handler;
    };

    // The child last placed in a parent by the build under way, and its index among the children when it is known
    struct Placed
    {
        Component* child = nullptr;
        int index = -1;
    };

    // A component of each type as it is when new, made for the first value read from it
    using PristineInstances = std::unordered_map<const ComponentTypeInfo*, std::unique_ptr<Component>>;

    struct TreeWatcher;
    struct ChangeUpdater;

    //==============================================================================
    void collectDeclarations (const DataTree& document);
    void collectStyleDeclarations (const DataTree& parent);
    void resetBindings();
    ResultValue<const ComponentTypeInfo*> resolveType (const Identifier& typeName);
    Result resolveComposite (const Identifier& typeName, Composite& composite);
    Result checkTypes (const DataTree& element);
    Result checkChildTypes (const DataTree& parent);
    Component& createComponent (const DataTree& element, Component* parent, Scope& scope, const DataTree& recordElement, const String& turn = {});
    void createRepeated (const DataTree& repeat, Component& parent, Scope& scope, const String& turn);
    bool bindRanges();
    void treeMutated();
    void rangeChanged();
    std::unique_ptr<Component> reuseComponent (const DataTree& element, Component* parent, const ComponentTypeInfo& type, const String& turn);
    void placeChild (Component& parent, Component& child, bool isReused);
    void setComponentInitializer (ComponentInitializer initializer);
    bool initialize (Component& component, bool onlyWithId) const;
    void applyIdentity (Component& component, const DataTree& element, Scope& scope, bool isUseSite);
    void applyTokens (Component& root, Scope& scope);
    void applyValues (const AppliedValues& previous);
    void resetValue (Component& component, const ComponentTypeInfo& type, const Identifier& name, PristineInstances& pristine);
    const std::vector<Identifier>& declarationOrderOf (const ComponentTypeInfo& type);
    std::vector<const Value*> inDeclarationOrder (const ComponentTypeInfo& type, const std::vector<Value>& values);
    void applyPending (const Pending& pending);
    ApplyOutcome applyProperty (Component& component, const Value& value, Scope& scope);
    void applyLink (Component& component, const Value& link, Scope& scope);
    void applyChanged (Component& component, const Value& change, Scope& scope);
    void connectHandler (Component& component, const ComponentSignal& signal, const Value& value, Scope& scope);
    Result report (const String& message, const DataTree& element, const Identifier& property = {});
    void report (const Component& component, const Identifier& name, const String& message, const Value& value);
    void emit (Component& component, const Identifier& signal, const Array<var>& arguments);
    const ComponentSignal* findDeclaredSignal (const Component& component, const Identifier& signal) const;
    Component* resolveComponent (const Scope& scope, const String& name, Component* self = nullptr) const;
    Component* resolve (const Scope& scope, const String& name, Component* self = nullptr) const;
    DrawingScope::ComponentResolver resolverFor (const Scope& scope) const;

    std::vector<Record> components;
    std::vector<Record> previousComponents;

    // What a pass of a build finds things by, all cleared with it: the components of the pass before by the hash of what
    // they were made from, the child last placed in each parent, and the names each type declares in their order
    std::vector<std::pair<std::size_t, std::size_t>> previousByHash;
    std::unordered_map<Component*, Placed> lastPlaced;
    std::unordered_map<const ComponentTypeInfo*, std::vector<Identifier>> declarationOrders;

    AppliedValues applied;
    std::vector<std::shared_ptr<Scope>> scopes;
    std::vector<RepeatRange> ranges;
    std::map<Identifier, Composite> composites;
    StyleSheet::Palettes palettes;
    std::map<String, DataTree> drawings;
    std::map<String, DataTree> shaders;
    std::map<Identifier, ComponentProperty> declaredProperties;
    std::unique_ptr<ComponentTypeInfo> tokensInfo;
    std::vector<DataTree> tokens;
    std::vector<Pending> pending;
    std::vector<Connection> connections;
    std::vector<std::unique_ptr<YuiPropertyWatcher>> watchers;
    std::vector<std::unique_ptr<YuiDoubleClickWatcher>> doubleClickWatchers;
    InPlaceTextEditor valueEditor;
    std::unique_ptr<TreeWatcher> treeWatcher;
    std::unique_ptr<ChangeUpdater> changeUpdater;
    std::map<String, Action> actions;
    FallbackAction fallbackAction;
    std::vector<ComponentInitializer> initializers;
    std::map<String, std::unique_ptr<BindingModel>> models;
    BindingGraph bindings;
    std::vector<DataTreeDiagnostic> diagnostics;
    File baseDirectory;
    bool ignoreTreeChanges = false;
    bool isBuilding = false;
    bool hasPendingTreeChange = false;
    bool hasPendingRangeChange = false;
    int numRepeatTurns = 1;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiBuilder)
};

//==============================================================================
/** Registers the component types a `.yui` document can be written with.

    These are the built-in types of yup_gui (see registerBuiltInComponentTypes), and, in the
    applications linking yup_renderer too, its 3D components: `Scene3D`, `Node3D`, `Camera3D`,
    `Light3D`, `Mesh3D`, `Model3D` and `Material3D`.

    In the applications linking yup_audio_gui they are its components too: `MidiKeyboard`,
    `PitchWheel`, `ModWheel`, `SpectrumAnalyzer`, `Spectrogram`, `KMeter`, `AudioView` and
    `CartesianPlane`, which are given their state or their data in C++ (see
    YuiBuilder::setInitializer).

    YuiBuilder calls it, so a document needs no call. It is for the code listing the types
    before anything is built, such as a schema or a tool.
*/
YUP_API void registerYuiComponentTypes();

} // namespace yup
