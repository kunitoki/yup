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

#if YUP_MODULE_AVAILABLE_yup_renderer
// Defined by yup_renderer: declared here rather than including that module for one function
YUP_API void registerSceneComponentTypes();
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_gui
// Defined by yup_audio_gui: declared here rather than including that module for one function
YUP_API void registerAudioGuiComponentTypes();
#endif

void registerYuiComponentTypes()
{
    registerBuiltInComponentTypes();

#if YUP_MODULE_AVAILABLE_yup_renderer
    static std::once_flag once;
    std::call_once (once, registerSceneComponentTypes);
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_gui
    static std::once_flag audioOnce;
    std::call_once (audioOnce, registerAudioGuiComponentTypes);
#endif
}

namespace
{

const Identifier& idProperty()
{
    static const Identifier identifier ("id");
    return identifier;
}

const Identifier& classProperty()
{
    static const Identifier identifier ("class");
    return identifier;
}

const Identifier& paletteProperty()
{
    static const Identifier identifier ("palette");
    return identifier;
}

const Identifier& drawingProperty()
{
    static const Identifier identifier ("drawing");
    return identifier;
}

const Identifier& repeatType()
{
    static const Identifier identifier ("repeat");
    return identifier;
}

constexpr int maxRepeatedComponents = 1024;
constexpr int maximumRangePasses = 8;

// Where an element is in its document, as the indices leading to it from the root
String makeElementKey (const DataTree& element)
{
    String key;

    for (auto node = element; node.getParent().isValid(); node = node.getParent())
        key = String (node.getParent().indexOf (node)) + (key.isEmpty() ? String() : "_" + key);

    return key;
}

Identifier makeOptionalIdentifier (const var& value)
{
    const auto text = value.toString();
    return text.isNotEmpty() ? Identifier (text) : Identifier();
}

// The names an `id` or a `class` is written with: a bare name, or the ones in a string, separated by spaces or commas.
// Nothing when the value is not that, as an expression isn't
std::optional<StringArray> readNames (const String& value)
{
    const auto text = value.trim();

    auto names = StringArray::fromTokens (text.unquoted(), " ,", "");
    names.removeEmptyStrings();

    if (! text.isQuotedString() && names.size() > 1)
        return std::nullopt;

    for (const auto& name : names)
        if (! Identifier::isValidIdentifier (name))
            return std::nullopt;

    return names;
}

DataTree findElementById (const DataTree& element, const String& id)
{
    if (detail::getYuiElementId (element) == id)
        return element;

    for (const auto& child : element)
    {
        if (! detail::isYuiComponentElement (child))
            continue;

        if (auto found = findElementById (child, id); found.isValid())
            return found;
    }

    return {};
}

std::size_t declarationRankOf (const ComponentTypeInfo& type, const Identifier& name)
{
    std::vector<const ComponentTypeInfo*> chain;

    for (const auto* info = &type; info != nullptr; info = info->getBaseTypeInfo())
        chain.insert (chain.begin(), info);

    std::size_t rank = 0;

    for (const auto* info : chain)
    {
        for (const auto& property : info->getOwnProperties())
        {
            if (property.name == name)
                return rank;

            ++rank;
        }
    }

    if (name == paletteProperty())
        return std::numeric_limits<std::size_t>::max() - 2;

    if (name == drawingProperty())
        return std::numeric_limits<std::size_t>::max() - 1;

    return std::numeric_limits<std::size_t>::max();
}

// Like C++ members, values apply in the order the type declares them (bases first), so a property constraining
// another one, a Slider range, applies before it wherever the document writes it; undeclared names come last
template <class Value>
std::vector<const Value*> inDeclarationOrder (const ComponentTypeInfo& type, const std::vector<Value>& values)
{
    std::vector<std::pair<std::size_t, const Value*>> ranked;
    ranked.reserve (values.size());

    for (const auto& value : values)
        ranked.emplace_back (declarationRankOf (type, value.name), &value);

    std::stable_sort (ranked.begin(), ranked.end(), [] (const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<const Value*> result;
    result.reserve (ranked.size());

    for (const auto& [rank, value] : ranked)
        result.push_back (value);

    return result;
}

ComponentProperty makeDeclaredProperty (const DataTree& declaration, bool isInherited)
{
    const auto typeName = declaration.getProperty ("type").toString();
    const auto type = ComponentProperty::getTypeFromName (typeName);

    var defaultValue;

    if (type == ComponentProperty::Type::array)
        defaultValue = var (Array<var>());
    else if (type == ComponentProperty::Type::drawing)
        defaultValue = var (String());
    else
        DrawingScope::withExpressionType (type, [&] (auto tag)
        {
            defaultValue = VariantConverter<typename decltype (tag)::type>::toVar ({});
        });

    return ComponentProperty {
        .name = makeOptionalIdentifier (declaration.getProperty ("name")),
        .type = type,
        .elementType = ComponentProperty::getElementTypeFromName (typeName),
        .defaultValue = defaultValue,
        .isInherited = isInherited
    };
}

} // namespace

//==============================================================================

struct YuiBuilder::TreeWatcher final : DataTreeListener
{
    explicit TreeWatcher (YuiBuilder& ownerToUse)
        : owner (ownerToUse)
    {
    }

    ~TreeWatcher() override
    {
        detachAll();
    }

    void resetRoot (const DataTree& root)
    {
        detachAll();

        if (! root.isValid())
            return;

        watchedRoot = root;
        attachSubtree (watchedRoot);
    }

    void detachAll()
    {
        for (auto& node : watchedNodes)
            if (node.isValid())
                node.removeListener (this);

        watchedNodes.clear();
        watchedRoot = {};
        hasStructureChanged = false;
    }

    void propertyChanged (DataTree&, const Identifier&) override { handleMutation (false); }
    void childAdded (DataTree&, DataTree&) override { handleMutation (true); }
    void childRemoved (DataTree&, DataTree&, int) override { handleMutation (true); }
    void childMoved (DataTree&, DataTree&, int, int) override { handleMutation (true); }

    void attachSubtree (DataTree node)
    {
        if (! node.isValid())
            return;

        node.addListener (this);
        watchedNodes.push_back (node);

        for (int i = 0; i < node.getNumChildren(); ++i)
            attachSubtree (node.getChild (i));
    }

    void handleMutation (bool isStructural)
    {
        if (owner.ignoreTreeChanges || ! watchedRoot.isValid())
            return;

        hasStructureChanged |= isStructural;
        owner.treeMutated();
    }

    YuiBuilder& owner;
    DataTree watchedRoot;
    std::vector<DataTree> watchedNodes;
    bool hasStructureChanged = false;
};

//==============================================================================
/* Has the changes of the tree and of the ranges of the repeats applied once they are over. */
struct YuiBuilder::ChangeUpdater final : AsyncUpdater
{
    explicit ChangeUpdater (YuiBuilder& ownerToUse)
        : owner (ownerToUse)
    {
    }

    ~ChangeUpdater() override
    {
        cancelPendingUpdate();
    }

    void handleAsyncUpdate() override
    {
        owner.applyPendingChanges();
    }

    YuiBuilder& owner;
};

//==============================================================================

YuiBuilder::YuiBuilder()
    : treeWatcher (std::make_unique<TreeWatcher> (*this))
    , changeUpdater (std::make_unique<ChangeUpdater> (*this))
{
    registerYuiComponentTypes();

    bindings.onDiagnostic = [this] (Component* target, const Identifier& property, const String& message)
    {
        DataTree element;
        Identifier sourceProperty;
        String prefix;

        if (target != nullptr)
        {
            prefix = target->getTypeInfo().getTypeName().toString() + "." + property.toString() + ": ";

            if (const auto compIt = applied.find (target); compIt != applied.end())
            {
                if (const auto valIt = compIt->second.find (property); valIt != compIt->second.end())
                {
                    element = valIt->second.element;
                    sourceProperty = valIt->second.property;
                }
            }
        }

        const DataTreeDiagnostic diagnostic {
            .message = prefix + message,
            .element = element,
            .property = sourceProperty
        };

        diagnostics.push_back (diagnostic);

        if (onDiagnostic != nullptr)
            onDiagnostic (diagnostic);
    };
}

YuiBuilder::~YuiBuilder()
{
    clear();
}

//==============================================================================

Result YuiBuilder::build (const DataTree& document)
{
    const ScopedValueSetter<bool> guard (ignoreTreeChanges, true);
    const auto previousWatchedRoot = treeWatcher != nullptr ? treeWatcher->watchedRoot : DataTree();

    if (treeWatcher != nullptr)
        treeWatcher->detachAll();

    // The previous declarations stay alive until the reused components have their new types
    auto previousComposites = std::exchange (composites, {});
    auto previousPalettes = std::exchange (palettes, {});
    auto previousDrawings = std::exchange (drawings, {});
    auto previousShaders = std::exchange (shaders, {});
    auto previousDeclaredProperties = std::exchange (declaredProperties, {});
    auto previousTokens = std::exchange (tokens, {});
    auto previousTokensInfo = std::move (tokensInfo);
    diagnostics.clear();

    auto root = document;

    if (document.getType().toString() == "yui")
    {
        collectDeclarations (document);

        root = {};

        for (const auto& element : document)
        {
            if (detail::isYuiComponentElement (element))
            {
                root = element;
                break;
            }
        }
    }

    const auto checked = root.isValid() && detail::isYuiComponentElement (root)
                           ? checkTypes (root)
                           : report ("The document has no root component", document);

    if (checked.failed())
    {
        composites = std::move (previousComposites);
        palettes = std::move (previousPalettes);
        drawings = std::move (previousDrawings);
        shaders = std::move (previousShaders);
        declaredProperties = std::move (previousDeclaredProperties);
        tokens = std::move (previousTokens);
        tokensInfo = std::move (previousTokensInfo);

        if (treeWatcher != nullptr)
            treeWatcher->resetRoot (previousWatchedRoot);

        return checked;
    }

    const ScopedValueSetter<bool> building (isBuilding, true);
    const auto numDeclarationDiagnostics = static_cast<std::ptrdiff_t> (diagnostics.size());

    // What was waiting to be applied is: the tree and the ranges are read as they are now
    hasPendingTreeChange = false;
    hasPendingRangeChange = false;
    changeUpdater->cancelPendingUpdate();

    // The range of a repeat is only known once the values it reads are applied. When it is not the one its
    // components were made for, they are made again, which keeps those still described
    for (int pass = 0;; ++pass)
    {
        diagnostics.erase (diagnostics.begin() + numDeclarationDiagnostics, diagnostics.end());
        resetBindings();

        for (auto& [name, model] : models)
            model->refreshProperties();

        scopes.clear();
        ranges.clear();
        previousComponents = std::exchange (components, {});

        {
            // A container is laid out once its children are made and have their values, not at each of them
            const LayoutComponent::ScopedLayoutBatch layouts;

            auto& scope = *scopes.emplace_back (std::make_shared<Scope>());
            scope.root = &createComponent (root, nullptr, scope, {});
            applyTokens (*scope.root, scope);

            // What the code sets up comes before the values of the document, on the components that are new and on
            // the ones kept under another id, which are then given all their values again
            for (const auto& record : components)
            {
                auto& component = *record.component;

                if (record.previousId == component.getComponentID() || ! initialize (component, false))
                    continue;

                if (const auto values = applied.find (&component); values != applied.end())
                {
                    for (auto& [name, value] : values->second)
                        value.isConstant = false;
                }
            }

            // The tree exists before any value is applied, so expressions can reference any element
            applyValues (std::exchange (applied, {}));

            // Children go first, so that no parent is destroyed while still holding them
            while (! previousComponents.empty())
                previousComponents.pop_back();
        }

        if (! bindRanges())
            break;

        if (pass == maximumRangePasses)
        {
            report ("The ranges of the repeats keep changing while their components are made", root);
            break;
        }
    }

    if (treeWatcher != nullptr)
        treeWatcher->resetRoot (document);

    return Result::ok();
}

void YuiBuilder::applyPendingChanges()
{
    // A change of the tree is the one applied: the build it leads to makes the components of the repeats again too
    const auto hasTreeChange = std::exchange (hasPendingTreeChange, false);

    if (! hasTreeChange && ! std::exchange (hasPendingRangeChange, false))
        return;

    changeUpdater->cancelPendingUpdate();

    // Nothing of the builder is read once a hook returns, as what it tells may destroy the builder
    if (const auto& hook = hasTreeChange ? onTreeMutated : onRangeChanged; hook != nullptr)
    {
        hook();
        return;
    }

    if (treeWatcher != nullptr && treeWatcher->watchedRoot.isValid())
    {
        const auto rootCopy = treeWatcher->watchedRoot;
        build (rootCopy);
    }
}

void YuiBuilder::treeMutated()
{
    hasPendingTreeChange = true;
    changeUpdater->triggerAsyncUpdate();
}

bool YuiBuilder::hasTreeStructureChanged() const noexcept
{
    return treeWatcher != nullptr && treeWatcher->hasStructureChanged;
}

void YuiBuilder::rangeChanged()
{
    // A range changing while its components are made is seen by the build itself
    if (isBuilding)
        return;

    hasPendingRangeChange = true;
    changeUpdater->triggerAsyncUpdate();
}

bool YuiBuilder::bindRanges()
{
    bool hasChanged = false;

    for (auto& range : ranges)
    {
        const auto bindEnd = [&] (const Identifier& name, const char* end, const var& used)
        {
            const auto bound = bindings.bind (*range.parent, name, range.element.getProperty (end).toString(), ComponentProperty::Type::integer, resolverFor (*range.scope), nullptr, range.scope->locals.get());

            if (bound.failed())
            {
                report (String ("repeat.") + end + ": " + bound.getErrorMessage(), range.element, Identifier (end));
                range.parent->setProperty (name, 0);
            }

            // An end is an integer, so it only changes when the repeat has other turns to run
            watchers.push_back (std::make_unique<YuiPropertyWatcher> (*range.parent, name, [this] (const Array<var>&)
            {
                rangeChanged();
            }));

            return range.parent->getProperty (name) != used;
        };

        hasChanged |= bindEnd (range.fromName, "from", range.usedFrom);
        hasChanged |= bindEnd (range.toName, "to", range.usedTo);
    }

    return hasChanged;
}

void YuiBuilder::clear()
{
    if (treeWatcher != nullptr)
        treeWatcher->detachAll();

    resetBindings();

    {
        // The containers go away with their children: none is laid out again for each child it loses
        const LayoutComponent::ScopedLayoutBatch layouts;

        while (! components.empty())
            components.pop_back();
    }

    applied.clear();
    scopes.clear();
    ranges.clear();
    hasPendingTreeChange = false;
    hasPendingRangeChange = false;
    changeUpdater->cancelPendingUpdate();
    composites.clear();
    palettes.clear();
    drawings.clear();
    shaders.clear();
    declaredProperties.clear();
    tokens.clear();
    tokensInfo.reset();
    diagnostics.clear();
}

void YuiBuilder::resetBindings()
{
    bindings.clear();
    bindings.clearDiagnostics();
    pending.clear();
    connections.clear();
    watchers.clear();

    for (auto& [name, model] : models)
        model->stopTrackingGestures();
}

//==============================================================================

Component* YuiBuilder::getRoot() const noexcept
{
    return components.empty() ? nullptr : components.front().component.get();
}

Component* YuiBuilder::findById (const String& id) const
{
    if (scopes.empty())
        return nullptr;

    const auto& ids = scopes.front()->ids;
    const auto found = ids.find (id);
    return found != ids.end() ? found->second : nullptr;
}

const ComponentTypeInfo* YuiBuilder::findTypeInfo (const Identifier& typeName)
{
    const auto resolved = resolveType (typeName);
    return resolved.wasOk() ? resolved.getValue() : nullptr;
}

DataTree YuiBuilder::findElementFor (const Component* component) const
{
    if (component == nullptr)
        return {};

    for (const auto& record : components)
    {
        if (record.component.get() == component)
            return record.element;
    }

    return {};
}

Component* YuiBuilder::findComponentFor (const DataTree& element) const
{
    if (! element.isValid())
        return nullptr;

    for (const auto& record : components)
    {
        if (record.element == element)
            return record.component.get();
    }

    return nullptr;
}

//==============================================================================

void YuiBuilder::setAction (const String& name, Action action)
{
    if (action != nullptr)
        actions[name] = std::move (action);
    else
        actions.erase (name);
}

void YuiBuilder::setFallbackAction (FallbackAction action)
{
    fallbackAction = std::move (action);
}

void YuiBuilder::setModel (const String& name, BindingProvider& provider)
{
    models[name] = std::make_unique<BindingModel> (provider);
}

void YuiBuilder::removeModel (const String& name)
{
    models.erase (name);
}

void YuiBuilder::setComponentInitializer (ComponentInitializer initializer)
{
    std::erase_if (initializers, [&] (const ComponentInitializer& other)
    {
        return other.type == initializer.type && other.id == initializer.id;
    });

    if (initializer.function == nullptr)
        return;

    const auto& added = initializers.emplace_back (std::move (initializer));

    // The components built already are set up as the ones made later will be: after an initializer of the type,
    // the ones of their id run again to keep the last word
    for (const auto& record : components)
    {
        auto& component = *record.component;

        if (added.id.isNotEmpty() && added.id != component.getComponentID())
            continue;

        if (added.function (component) && added.id.isEmpty())
            initialize (component, true);
    }
}

bool YuiBuilder::initialize (Component& component, bool onlyWithId) const
{
    bool hasRun = false;

    // The initializers of an id run after the others, whatever the order they were set in
    for (const auto withId : { false, true })
    {
        if (onlyWithId && ! withId)
            continue;

        for (const auto& initializer : initializers)
        {
            if (initializer.id.isNotEmpty() == withId && (! withId || initializer.id == component.getComponentID()))
                hasRun = initializer.function (component) || hasRun;
        }
    }

    return hasRun;
}

//==============================================================================

void YuiBuilder::collectStyleDeclarations (const DataTree& parent)
{
    const auto collectProps = [this] (const DataTree& container)
    {
        for (const auto& child : container)
        {
            if (child.getType().toString() != "property")
                continue;

            const auto rawName = child.getProperty ("name").toString().trim();
            const auto dot = rawName.indexOfChar ('.');
            const Identifier unqualified (dot >= 0 ? rawName.substring (dot + 1) : rawName);
            const auto propTypeName = child.getProperty ("type").toString().trim();

            if (unqualified.isValid() && propTypeName.isNotEmpty())
            {
                const auto type = ComponentProperty::getTypeFromName (propTypeName);
                const auto elementType = ComponentProperty::getElementTypeFromName (propTypeName);

                const auto [declared, isNew] = declaredProperties.try_emplace (unqualified, ComponentProperty {
                    .name = unqualified,
                    .type = type,
                    .elementType = elementType,
                    .isInherited = true
                });

                // The first declaration of a name is the one read, whatever declares it again
                if (! isNew && (declared->second.type != type || declared->second.elementType != elementType))
                    report ("The property '" + unqualified.toString() + "' is already declared with another type", child);
            }
        }
    };

    for (const auto& element : parent)
    {
        const auto kind = element.getType().toString();

        if (kind == "style")
        {
            collectProps (element);
            collectStyleDeclarations (element);
        }
        else if (kind == "rule")
        {
            collectProps (element);
        }
        else if (kind == "shader")
        {
            const auto name = element.getProperty ("name").toString().trim();

            if (name.isNotEmpty() && ! shaders.insert_or_assign (name, element).second)
                report ("The shader '" + name + "' is already declared", element);

            collectProps (element);
        }
        else if (kind == "palette")
        {
            if (auto problem = StyleSheet::readPaletteElement (element, palettes))
                diagnostics.push_back (*problem);
        }
        else if (kind == "drawing")
        {
            const auto name = element.getProperty ("name").toString().trim();

            if (name.isNotEmpty() && ! drawings.insert_or_assign (name, element).second)
                report ("The drawing '" + name + "' is already declared", element);

            collectProps (element);
        }
    }
}

void YuiBuilder::collectDeclarations (const DataTree& document)
{
    collectStyleDeclarations (document);

    for (const auto& element : document)
    {
        const auto kind = element.getType().toString();

        if (kind == "import")
        {
            // Left by a parse that reads no file: what the file declares is missing
            report ("Unresolved import '" + element.getProperty ("path").toString() + "'", element);
            continue;
        }

        if (kind == "tokens")
        {
            for (const auto& token : element)
                tokens.push_back (token);

            continue;
        }

        if (kind != "component")
            continue;

        const auto name = makeOptionalIdentifier (element.getProperty ("name"));
        if (! name.isValid())
            continue;

        if (composites.count (name) > 0 || ComponentTypeRegistry::getInstance().findType (name) != nullptr)
        {
            report ("component " + name.toString() + ": the type is already declared", element);
            continue;
        }

        Composite composite;
        composite.declaration = element;

        for (const auto& child : element)
            if (detail::isYuiComponentElement (child))
                composite.body = child;

        composites.emplace (name, std::move (composite));
    }
}

ResultValue<const ComponentTypeInfo*> YuiBuilder::resolveType (const Identifier& typeName)
{
    if (const auto found = composites.find (typeName); found != composites.end())
    {
        if (auto result = resolveComposite (typeName, found->second); result.failed())
            return makeResultValueFail (result.getErrorMessage());

        return makeResultValueOk (static_cast<const ComponentTypeInfo*> (found->second.info.get()));
    }

    if (const auto* info = ComponentTypeRegistry::getInstance().findType (typeName))
        return makeResultValueOk (info);

    return makeResultValueFail ("Unknown component type '" + typeName.toString() + "'");
}

Result YuiBuilder::resolveComposite (const Identifier& typeName, Composite& composite)
{
    if (composite.info != nullptr)
        return Result::ok();

    if (composite.error.isNotEmpty())
        return Result::fail (composite.error);

    if (composite.isResolving)
        return Result::fail ("The component '" + typeName.toString() + "' is based on itself");

    const ScopedValueSetter<bool> resolving (composite.isResolving, true);

    const auto base = resolveType (composite.body.getType());

    if (base.failed())
    {
        composite.error = base.getErrorMessage();
        return Result::fail (composite.error);
    }

    const auto where = "component " + typeName.toString();
    std::vector<ComponentProperty> properties;
    std::vector<ComponentSignal> signals;

    for (const auto& declaration : composite.declaration)
    {
        if (detail::isYuiComponentElement (declaration))
            continue;

        const auto kind = declaration.getType().toString();
        const auto name = makeOptionalIdentifier (declaration.getProperty ("name"));
        if (! name.isValid())
            continue;

        if (kind == "property")
        {
            properties.push_back (makeDeclaredProperty (declaration, false));

            if (declaration.hasProperty ("value"))
                composite.defaults.push_back ({ name, declaration.getProperty ("value").toString(), declaration });
        }
        else if (kind == "signal")
        {
            std::vector<ComponentSignal::Argument> arguments;

            for (const auto& argument : declaration)
            {
                const auto argumentName = makeOptionalIdentifier (argument.getProperty ("name"));
                if (! argumentName.isValid())
                    continue;

                const auto argumentType = argument.getProperty ("type").toString();

                arguments.push_back ({ argumentName,
                                       ComponentProperty::getTypeFromName (argumentType),
                                       ComponentProperty::getElementTypeFromName (argumentType) });
            }

            signals.push_back ({ .name = name,
                                 .connect = [this, name] (Component& component, ComponentSignal::Handler handler)
                                 {
                                     // A signal has one handler, as the one of a widget: the last connected, or none
                                     std::erase_if (connections, [&] (const Connection& connection)
                                     {
                                         return connection.component == &component && connection.signal == name;
                                     });

                                     if (handler != nullptr)
                                         connections.push_back ({ &component, name, std::move (handler) });
                                 },
                                 .arguments = std::move (arguments) });
        }
        else if (kind == "alias")
        {
            const auto target = declaration.getProperty ("target").toString();
            const auto split = detail::splitYuiPath (target);
            const auto element = split.has_value() ? findElementById (composite.body, split->first) : DataTree();

            if (! element.isValid())
            {
                report (where + "." + name.toString() + ": unknown element in '" + target + "'", declaration);
                continue;
            }

            const auto elementType = resolveType (element.getType());

            if (elementType.failed())
                continue;

            const auto* aliased = elementType.getValue()->findProperty (split->second);

            if (aliased == nullptr)
            {
                report (where + "." + name.toString() + ": " + element.getType().toString() + " has no property '" + split->second.toString() + "'", declaration);
                continue;
            }

            auto alias = *aliased;
            alias.name = name;
            alias.storageId = {};
            alias.getter = nullptr;
            alias.arrayGetter = nullptr;
            alias.setter = nullptr;
            alias.isState = false;
            alias.isInherited = false;
            alias.layoutEffect = ComponentProperty::LayoutEffect::none;
            properties.push_back (std::move (alias));
            composite.aliases.push_back ({ name, target, declaration });
        }
    }

    composite.info = std::make_unique<ComponentTypeInfo> (typeName, base.getValue(), std::move (properties), std::move (signals));
    return Result::ok();
}

//==============================================================================

Result YuiBuilder::checkTypes (const DataTree& element)
{
    const auto typeName = element.getType();

    if (const auto found = composites.find (typeName); found != composites.end())
    {
        auto& composite = found->second;

        // A composite that can't be resolved is reported where its base type is written
        if (auto result = resolveComposite (typeName, composite); result.failed())
            return report (result.getErrorMessage(), composite.body);

        if (composite.isInstantiating)
            return report ("The component '" + typeName.toString() + "' contains itself", element);

        const ScopedValueSetter<bool> instantiating (composite.isInstantiating, true);

        if (auto result = checkTypes (composite.body); result.failed())
            return result;
    }
    else if (const auto* typeInfo = ComponentTypeRegistry::getInstance().findType (typeName); typeInfo == nullptr)
    {
        return report ("Unknown component type '" + typeName.toString() + "'", element);
    }
    else if (! typeInfo->hasFactory())
    {
        return report ("The component type '" + typeName.toString() + "' can't be created from .yui", element);
    }

    return checkChildTypes (element);
}

Result YuiBuilder::checkChildTypes (const DataTree& parent)
{
    for (const auto& child : parent)
    {
        // What a repeat holds are children of the element the repeat is in
        const auto isRepeat = child.getType() == repeatType();

        if (! isRepeat && ! detail::isYuiComponentElement (child))
            continue;

        if (auto result = isRepeat ? checkChildTypes (child) : checkTypes (child); result.failed())
            return result;
    }

    return Result::ok();
}

Component& YuiBuilder::createComponent (const DataTree& element, Component* parent, Scope& scope, const DataTree& recordElement, const String& turn)
{
    const auto typeName = element.getType();
    const auto& key = recordElement.isValid() ? recordElement : element;
    Component* component = nullptr;

    if (const auto found = composites.find (typeName); found != composites.end())
    {
        auto& composite = found->second;

        // The body is created in a scope of its own, where `root` is the instance
        auto& bodyScope = *scopes.emplace_back (std::make_shared<Scope>());
        const auto declarationsIndex = static_cast<std::ptrdiff_t> (pending.size());

        component = &createComponent (composite.body, parent, bodyScope, key, turn);
        bodyScope.root = component;
        component->setTypeInfo (composite.info.get());

        // The declared defaults and aliases come before the values of the body and of the use site
        pending.insert (pending.begin() + declarationsIndex, Pending { component, &bodyScope, composite.defaults, composite.aliases });
        applyIdentity (*component, element, scope, true);
    }
    else
    {
        // checkTypes made sure the type exists and can be created
        const auto& type = *ComponentTypeRegistry::getInstance().findType (typeName);

        auto created = reuseComponent (key, parent, type, turn);
        const auto isReused = created != nullptr;
        const auto previousId = isReused ? std::optional<String> (created->getComponentID()) : std::nullopt;

        if (! isReused)
            created = type.createInstance();

        component = created.get();

        // The identity comes first, so a container placing its children itself knows them
        applyIdentity (*component, element, scope, false);

        if (parent != nullptr)
            placeChild (*parent, *component, isReused);

        components.push_back ({ key, parent, &type, std::move (created), turn, previousId });

        // The components made under the root can look for what builds them from here on
        if (parent == nullptr && onRootCreated != nullptr)
            onRootCreated (*component);
    }

    static const Identifier bindType ("bind");
    static const Identifier changedType ("changed");

    Pending values { component, &scope };

    for (int i = 0; i < element.getNumProperties(); ++i)
    {
        const auto name = element.getPropertyName (i);

        if (name != idProperty() && name != classProperty())
            values.values.push_back ({ name, element.getProperty (name).toString(), element, name });
    }

    for (const auto& child : element)
    {
        if (child.getType() == bindType)
        {
            if (const auto prop = makeOptionalIdentifier (child.getProperty ("property")); prop.isValid())
                values.links.push_back ({ prop, child.getProperty ("target").toString(), child });
        }
        else if (child.getType() == changedType)
        {
            if (const auto prop = makeOptionalIdentifier (child.getProperty ("property")); prop.isValid())
                values.changes.push_back ({ prop, child.getProperty ("handler").toString(), child, Identifier ("handler") });
        }
    }

    pending.push_back (std::move (values));

    for (const auto& child : element)
    {
        if (child.getType() == bindType || child.getType() == changedType)
            continue;

        if (child.getType() == repeatType())
        {
            createRepeated (child, *component, scope, {});
            continue;
        }

        if (! detail::isYuiComponentElement (child))
        {
            report ("'" + child.getType().toString() + "' is not a component type", child);
            continue;
        }

        createComponent (child, component, scope, {});
    }

    return *component;
}

void YuiBuilder::createRepeated (const DataTree& repeat, Component& parent, Scope& scope, const String& turn)
{
    const auto name = repeat.getProperty ("name").toString();
    const auto key = "__repeat_" + makeElementKey (repeat) + (turn.isNotEmpty() ? "_" + turn.replaceCharacter ('/', '_') : String());
    const Identifier fromName (key + "_from");
    const Identifier toName (key + "_to");

    // An end of the range is what it was when it was last known, else the number it is written as
    const auto readEnd = [&] (const Identifier& property, const char* end)
    {
        if (const auto known = parent.getProperty (property); ! known.isVoid())
            return known;

        const auto text = repeat.getProperty (end).toString().trim();
        return text.isNotEmpty() && text.containsOnly ("0123456789") ? var (text.getIntValue()) : var();
    };

    const auto from = readEnd (fromName, "from");
    const auto to = readEnd (toName, "to");

    ranges.push_back ({ &parent, &scope, repeat, fromName, toName, from, to });

    if (from.isVoid() || to.isVoid())
        return;

    const auto first = static_cast<int> (from);
    auto count = static_cast<int64> (static_cast<int> (to)) - first;

    // Repeats inside one another run their turns together, so they share what one of them may run
    const auto limit = maxRepeatedComponents / numRepeatTurns;

    if (count > limit)
    {
        report ("A repeat runs at most " + String (limit) + " times" + (numRepeatTurns > 1 ? " inside the " + String (numRepeatTurns) + " turns around it" : String()) + ", not " + String (count), repeat, Identifier ("to"));
        count = limit;
    }

    const ScopedValueSetter<int> turns (numRepeatTurns, numRepeatTurns * jmax (1, static_cast<int> (count)));

    for (int offset = 0; offset < count; ++offset)
    {
        const auto index = first + offset;
        const auto ownTurn = turn.isEmpty() ? String (index) : turn + "/" + String (index);

        auto& turnScope = *scopes.emplace_back (std::make_shared<Scope>());
        turnScope.outer = &scope;
        turnScope.indexNames = scope.indexNames;
        turnScope.indexValues = scope.indexValues;

        // An index hides one of the same name around it
        if (const auto hidden = turnScope.indexNames.indexOf (name); hidden >= 0)
        {
            turnScope.indexNames.remove (hidden);
            turnScope.indexValues.remove (hidden);
        }

        turnScope.indexNames.add (name);
        turnScope.indexValues.add (index);

        std::vector<ComponentSignal::Argument> indices;

        for (const auto& indexName : turnScope.indexNames)
            indices.push_back ({ Identifier (indexName), ComponentProperty::Type::integer });

        turnScope.locals = std::make_shared<YuiHandlerArguments> (turnScope.indexNames, indices);
        turnScope.locals->setValues (turnScope.indexValues);

        for (const auto& child : repeat)
        {
            if (child.getType() == repeatType())
            {
                createRepeated (child, parent, turnScope, ownTurn);
            }
            else if (detail::isYuiComponentElement (child))
            {
                createComponent (child, &parent, turnScope, {}, ownTurn);
            }
            else if (offset == 0)
            {
                report ("'" + child.getType().toString() + "' is not a component type", child);
            }
        }
    }
}

std::unique_ptr<Component> YuiBuilder::reuseComponent (const DataTree& element, Component* parent, const ComponentTypeInfo& type, const String& turn)
{
    for (auto& record : previousComponents)
    {
        if (record.component == nullptr || record.element != element || record.parent != parent || record.type != &type || record.turn != turn)
            continue;

        // A composite type info of the previous build is about to go away
        record.component->setTypeInfo (nullptr);
        return std::move (record.component);
    }

    return nullptr;
}

void YuiBuilder::placeChild (Component& parent, Component& child, bool isReused)
{
    const auto& parentType = parent.getTypeInfo();

    if (! isReused)
    {
        parentType.attachChild (parent, child);
        return;
    }

    // A container that places its children itself keeps them where it put them
    if (parentType.hasChildAttacher())
        return;

    // A kept child only moves when it isn't after its previous sibling, so its subtree isn't notified for nothing
    for (auto it = components.rbegin(); it != components.rend(); ++it)
    {
        if (it->parent != &parent)
            continue;

        if (parent.getIndexOfChildComponent (&child) < parent.getIndexOfChildComponent (it->component.get()))
            parent.addChildComponent (child);

        break;
    }
}

void YuiBuilder::applyIdentity (Component& component, const DataTree& element, Scope& scope, bool isUseSite)
{
    if (! isUseSite)
    {
        if (auto* imageComponent = dynamic_cast<ImageComponent*> (&component))
            imageComponent->setBaseDirectory (baseDirectory);
    }

    String id;

    if (element.hasProperty (idProperty()))
    {
        const auto names = readNames (element.getProperty (idProperty()).toString());

        if (names.has_value() && names->size() == 1)
            id = (*names)[0];
        else
            report (element.getType().toString() + ".id: expected a name, such as knob", element, idProperty());
    }

    // A kept component loses the identity it had, unless it's a composite instance keeping the one of its body
    if (id.isNotEmpty())
    {
        component.setComponentID (id);

        if (! scope.ids.emplace (id, &component).second)
            report ("The id '" + id + "' is used more than once", element, idProperty());
    }
    else if (! isUseSite)
    {
        component.setComponentID ({});
    }

    Array<Identifier> classes;

    if (const auto names = readNames (element.getProperty (classProperty()).toString()))
    {
        for (const auto& styleClass : *names)
            classes.add (Identifier (styleClass));
    }
    else
    {
        report (element.getType().toString() + ".class: expected a name or names in a string, such as \"knob large\"", element, classProperty());
    }

    if (! isUseSite)
        component.setStyleClasses (classes);
    else
        for (const auto& styleClass : classes)
            component.addStyleClass (styleClass);
}

void YuiBuilder::applyTokens (Component& root, Scope& scope)
{
    if (tokens.empty())
        return;

    std::vector<ComponentProperty> properties;
    Pending values { &root, &scope };

    for (const auto& token : tokens)
    {
        auto property = makeDeclaredProperty (token, true);
        if (! property.name.isValid())
            continue;

        const auto isDeclared = std::any_of (properties.begin(), properties.end(), [&] (const auto& declared)
        {
            return declared.name == property.name;
        });

        if (isDeclared)
        {
            report ("tokens: '" + property.name.toString() + "' is declared more than once", token);
            continue;
        }

        if (token.hasProperty ("value"))
            values.values.push_back ({ property.name, token.getProperty ("value").toString(), token });

        properties.push_back (std::move (property));
    }

    // The tokens extend the type of the root, so they read and cascade like any of its properties
    tokensInfo = std::make_unique<ComponentTypeInfo> (root.getTypeInfo().getTypeName(), &root.getTypeInfo(), std::move (properties));
    root.setTypeInfo (tokensInfo.get());

    pending.insert (pending.begin(), std::move (values));
}

//==============================================================================

void YuiBuilder::refreshThemePalettes()
{
    for (const auto& record : components)
    {
        auto* component = record.component.get();
        const auto values = applied.find (component);

        if (values == applied.end())
            continue;

        const auto value = values->second.find (paletteProperty());

        if (value == values->second.end())
            continue;

        const auto found = palettes.find (value->second.text.unquoted().trim());

        if (found != palettes.end() && found->second.followsTheme())
            component->setPalette (*found->second.findForTheme());
    }
}

void YuiBuilder::applyValues (const AppliedValues& previous)
{
    // Later values of a property override earlier ones, so the last text is the one applied
    for (const auto& entry : pending)
    {
        auto& values = applied[entry.component];

        for (const auto& value : entry.values)
            values[value.name] = { value.text, value.element, value.property, false };

        for (const auto& link : entry.links)
            values[link.name] = { link.text, link.element, link.property, true };
    }

    for (const auto& record : components)
    {
        auto* component = record.component.get();
        const auto found = previous.find (component);

        if (found == previous.end())
            continue;

        auto& values = applied[component];
        std::vector<Identifier> removed;

        for (const auto& [name, before] : found->second)
        {
            if (const auto now = values.find (name); now == values.end())
            {
                removed.push_back (name);
            }
            else if (before.isConstant && ! before.isLink && ! now->second.isLink && before.text == now->second.text)
            {
                now->second.isKept = true;
                now->second.isConstant = true;
            }
        }

        std::sort (removed.begin(), removed.end(), [&record] (const Identifier& a, const Identifier& b)
        {
            return declarationRankOf (*record.type, a) < declarationRankOf (*record.type, b);
        });

        for (const auto& name : removed)
            resetValue (*component, *record.type, name);
    }

    for (const auto& entry : pending)
        applyPending (entry);

    pending.clear();
}

void YuiBuilder::resetValue (Component& component, const ComponentTypeInfo& type, const Identifier& name)
{
    const auto& typeInfo = component.getTypeInfo();

    if (const auto* signal = typeInfo.findSignal (name))
    {
        signal->connect (component, nullptr);
        return;
    }

    const auto* descriptor = typeInfo.findProperty (name);

    if (descriptor == nullptr && name == paletteProperty())
    {
        component.setPalette (std::nullopt);
        return;
    }

    if (descriptor == nullptr && name == drawingProperty())
    {
        component.setStyle (nullptr);
        return;
    }

    if (descriptor == nullptr || ! descriptor->isStateBacked())
    {
        component.removeProperty (name);
        return;
    }

    // A state-backed value goes back to the one a new component has. It is shown when placed, a new instance is still hidden
    static const Identifier visible ("visible");

    if (! descriptor->isReadOnly())
        component.setProperty (name, name == visible ? var (true) : type.createInstance()->getProperty (name));

    // Nor has a new instance the bounds a layout container gives its items: the container gives them again
    // shortcut: a TabComponent shows and places its pages itself and is not asked, upgrade if documents set those
    if (auto* parent = component.getParentComponent())
        parent->childLayoutChanged (component);
}

void YuiBuilder::applyPending (const Pending& entry)
{
    auto& values = applied[entry.component];

    for (const auto* entryValue : inDeclarationOrder (entry.component->getTypeInfo(), entry.values))
    {
        auto& value = values[entryValue->name];

        if (value.isKept)
            continue;

        value.element = entryValue->element;
        value.property = entryValue->property;

        const auto outcome = applyProperty (*entry.component, *entryValue, *entry.scope);
        value.isConstant = ! value.isLink && outcome == ApplyOutcome::constant;
    }

    for (const auto& link : entry.links)
    {
        auto& value = values[link.name];
        value.element = link.element;
        value.property = link.property;
        value.isLink = true;
        value.isConstant = false;
        applyLink (*entry.component, link, *entry.scope);
    }

    for (const auto& change : entry.changes)
        applyChanged (*entry.component, change, *entry.scope);
}

YuiBuilder::ApplyOutcome YuiBuilder::applyProperty (Component& component, const Value& value, Scope& scope)
{
    const auto& typeInfo = component.getTypeInfo();
    const auto& name = value.name;

    if (const auto* signal = typeInfo.findSignal (name))
    {
        connectHandler (component, *signal, value, scope);
        return ApplyOutcome::live;
    }

    const auto* descriptor = typeInfo.findProperty (name);

    if (descriptor == nullptr)
    {
        if (const auto* drawingStyle = dynamic_cast<const DrawingStyle*> (component.getStyle().get()))
        {
            for (const auto& property : drawingStyle->getProperties())
            {
                if (property.name == name)
                {
                    descriptor = &property;
                    break;
                }
            }
        }
    }

    if (descriptor == nullptr)
    {
        for (const auto* ancestor = component.getParentComponent(); ancestor != nullptr; ancestor = ancestor->getParentComponent())
        {
            const auto* candidate = ancestor->getTypeInfo().findProperty (name);
            if (candidate != nullptr && candidate->isStoreBacked() && candidate->isInherited)
            {
                descriptor = candidate;
                break;
            }
        }
    }

    if (descriptor == nullptr)
    {
        if (const auto found = declaredProperties.find (name); found != declaredProperties.end())
            descriptor = &found->second;
    }

    if (descriptor != nullptr && descriptor->isReadOnly())
    {
        report (component, name, "the property is read-only", value);
        return ApplyOutcome::failed;
    }

    if (descriptor == nullptr && name == paletteProperty())
    {
        const auto paletteName = value.text.unquoted().trim();

        if (paletteName.isEmpty() || paletteName == "none")
        {
            component.setPalette (std::nullopt);
            return ApplyOutcome::constant;
        }

        if (const auto found = palettes.find (paletteName); found != palettes.end())
        {
            component.setPalette (*found->second.findForTheme());
            return ApplyOutcome::constant;
        }

        report (component, name, "unknown palette '" + paletteName + "'", value);
        return ApplyOutcome::failed;
    }

    if (descriptor == nullptr && name == drawingProperty())
    {
        const auto drawingName = value.text.unquoted().trim();

        if (drawingName.isEmpty() || drawingName == "none")
        {
            component.setStyle (nullptr);
            return ApplyOutcome::constant;
        }

        const auto installNamedDrawing = [&] (const String& targetName) -> ApplyOutcome
        {
            if (targetName.isEmpty() || targetName == "none")
            {
                component.setStyle (nullptr);
                return ApplyOutcome::constant;
            }

            const auto found = drawings.find (targetName);
            if (found == drawings.end())
            {
                report (component, name, "unknown drawing '" + targetName + "'", value);
                return ApplyOutcome::failed;
            }

            const auto findDrawing = [this] (const String& usedName)
            {
                const auto used = drawings.find (usedName);
                return used != drawings.end() ? used->second : DataTree();
            };

            // A shader is known by its block here. The stylesheet styling the component loads it, and makes the drawing
            // again with it when it restyles the component: until then the drawing runs nothing where it uses it
            const auto findShader = [this] (const String& usedName)
            {
                const auto used = shaders.find (usedName);
                return DrawingStyle::Shader { used != shaders.end() ? used->second : DataTree(), nullptr };
            };

            DataTreeDiagnostic failure;
            auto created = DrawingStyle::create (component, found->second, resolverFor (scope), &failure, findDrawing, findShader);

            if (created.failed())
            {
                diagnostics.push_back ({
                    .message = component.getTypeInfo().getTypeName().toString() + "." + name.toString() + ": " + created.getErrorMessage(),
                    .element = failure.element.isValid() ? failure.element : value.element,
                    .property = failure.element.isValid() ? failure.property : value.property
                });
                return ApplyOutcome::failed;
            }

            component.setStyle (created.getReference());

            if (! created.getReference()->getShaders().empty())
                if (auto* context = component.findStyleContext())
                    context->markDirty (component);

            // Made again by each build: it finds components in the scopes of the build that made it
            return ApplyOutcome::live;
        };

        auto outcome = ApplyOutcome::failed;

        if (drawings.find (drawingName) != drawings.end())
            outcome = installNamedDrawing (drawingName);
        else if (auto compiled = PropertyExpression::compile (component, value.text, ComponentProperty::Type::string, resolverFor (scope), scope.locals.get()); compiled.wasOk())
            outcome = installNamedDrawing (compiled.getReference()->evaluate().toString());
        else
            report (component, name, "unknown drawing '" + drawingName + "'", value);

        // A drawing that can't be installed leaves none. The one a component kept of the build before is the drawing as
        // it was, reading the components of that build
        if (outcome == ApplyOutcome::failed)
            component.setStyle (nullptr);

        return outcome;
    }

    if (descriptor != nullptr && descriptor->type == ComponentProperty::Type::drawing)
    {
        const auto drawingName = value.text.unquoted().trim();

        if (drawingName.isEmpty() || drawingName == "none")
        {
            component.setProperty (name, String());
            return ApplyOutcome::constant;
        }

        // The name of a drawing or of a shader the document declares is written bare
        if (drawings.find (drawingName) != drawings.end() || shaders.find (drawingName) != shaders.end())
        {
            component.setProperty (name, drawingName);
            return ApplyOutcome::constant;
        }

        bool isLive = false;
        if (bindings.bind (component, name, value.text, ComponentProperty::Type::string, resolverFor (scope), &isLive, scope.locals.get()).wasOk())
            return isLive ? ApplyOutcome::live : ApplyOutcome::constant;

        report (component, name, "unknown drawing or shader '" + drawingName + "'", value);
        return ApplyOutcome::failed;
    }

    if (descriptor != nullptr && descriptor->type == ComponentProperty::Type::enumeration)
    {
        const auto enumValue = value.text.unquoted().trim();

        if (! descriptor->enumValues.contains (enumValue))
        {
            report (component, name, "expected one of " + descriptor->enumValues.joinIntoString (", "), value);
            return ApplyOutcome::failed;
        }

        component.setProperty (name, enumValue);
        return ApplyOutcome::constant;
    }

    std::optional<ComponentProperty::Type> type;

    if (descriptor != nullptr)
    {
        if (ComponentProperty::isExpressionType (descriptor->type) || descriptor->type == ComponentProperty::Type::array)
        {
            type = descriptor->type;
        }
        else if (descriptor->type != ComponentProperty::Type::any)
        {
            report (component, name, "properties of this type can't be set from .yui yet", value);
            return ApplyOutcome::failed;
        }
    }

    bool isLive = false;

    if (auto result = bindings.bind (component, name, value.text, type, resolverFor (scope), &isLive, scope.locals.get()); result.failed())
    {
        report (component, name, result.getErrorMessage(), value);
        return ApplyOutcome::failed;
    }

    static const Identifier cornerRadiusName ("cornerRadius");
    if (descriptor == nullptr && name != cornerRadiusName)
    {
        const DataTreeDiagnostic warning {
            .message = typeInfo.getTypeName().toString() + "." + name.toString() + ": undeclared property '" + name.toString() + "'",
            .element = value.element,
            .property = value.property,
            .severity = DataTreeDiagnostic::Severity::warning
        };

        diagnostics.push_back (warning);

        if (onDiagnostic != nullptr)
            onDiagnostic (warning);
    }

    return isLive ? ApplyOutcome::live : ApplyOutcome::constant;
}

void YuiBuilder::applyLink (Component& component, const Value& link, Scope& scope)
{
    const auto& name = link.name;
    auto path = link.text;
    int index = -1;

    // An element of a list: its index is read once, when the link is made
    if (const auto open = path.indexOfChar ('['); open >= 0)
    {
        const auto indexText = path.substring (open + 1).upToLastOccurrenceOf ("]", false, false);
        path = path.substring (0, open).trim();

        auto compiled = PropertyExpression::compile (component, indexText, ComponentProperty::Type::integer, resolverFor (scope), scope.locals.get(), baseDirectory);

        if (compiled.failed())
        {
            report (component, name, compiled.getErrorMessage(), link);
            return;
        }

        index = static_cast<int> (compiled.getReference()->evaluate());

        if (index < 0)
        {
            report (component, name, "the index of an element is 0 or more", link);
            return;
        }
    }

    const auto split = detail::splitYuiPath (path);

    if (! split.has_value())
    {
        report (component, name, "expected element.property after '<=>'", link);
        return;
    }

    auto* source = resolve (scope, split->first, &component);

    if (source == nullptr)
    {
        report (component, name, "unknown element '" + split->first + "'", link);
        return;
    }

    const auto& sourceProperty = split->second;

    auto* model = dynamic_cast<BindingModel*> (source);
    if (model != nullptr && source->getTypeInfo().findProperty (sourceProperty) == nullptr)
        model->refreshProperties();

    if (source->getTypeInfo().findProperty (sourceProperty) == nullptr)
    {
        report (component, name, source->getTypeInfo().getTypeName().toString() + " has no property '" + sourceProperty.toString() + "'", link);
        return;
    }

    if (const auto* descriptor = component.getTypeInfo().findProperty (name); descriptor != nullptr && descriptor->isReadOnly())
    {
        report (component, name, "the property is read-only", link);
        return;
    }

    if (index >= 0 && source->getTypeInfo().findProperty (sourceProperty)->type != ComponentProperty::Type::array)
    {
        report (component, name, "'" + path + "' is not a list", link);
        return;
    }

    bindings.link (component, name, *source, sourceProperty, index);

    if (model != nullptr)
        model->trackGestures (component, sourceProperty);
}

//==============================================================================

void YuiBuilder::applyChanged (Component& component, const Value& change, Scope& scope)
{
    const auto* descriptor = component.getTypeInfo().findProperty (change.name);
    const Identifier changedName ("changed " + change.name.toString());

    if (descriptor == nullptr)
    {
        report (component, changedName, "unknown property '" + change.name.toString() + "'", Value { changedName, {}, change.element });
        return;
    }

    // A changed handler is the handler of a signal emitted with the new value
    const ComponentSignal signal {
        .name = changedName,
        .connect = [this, property = change.name] (Component& target, ComponentSignal::Handler handler)
        {
            watchers.push_back (std::make_unique<YuiPropertyWatcher> (target, property, std::move (handler)));
        },
        .arguments = { { change.name, descriptor->type, descriptor->elementType } }
    };

    connectHandler (component, signal, change, scope);
}

void YuiBuilder::connectHandler (Component& component, const ComponentSignal& signal, const Value& value, Scope& scope)
{
    const auto fail = [&] (const String& message)
    {
        report (component, signal.name, message, value);
    };

    const auto parsed = YuiHandler::parse (value.text);

    if (parsed.failed())
        return fail (parsed.getErrorMessage());

    const auto& handler = parsed.getReference();

    if (handler.argumentNames.size() > static_cast<int> (signal.arguments.size()))
        return fail ("expected at most " + String (static_cast<int> (signal.arguments.size())) + " argument names");

    // The names the handler gives the arguments of the signal, then the indices of the repeats it is in, which an
    // argument of the same name hides
    auto localNames = handler.argumentNames;
    std::vector<ComponentSignal::Argument> localTypes (signal.arguments.begin(), signal.arguments.begin() + handler.argumentNames.size());
    const auto numArguments = handler.argumentNames.size();
    Array<var> indices;

    for (int i = 0; i < scope.indexNames.size(); ++i)
    {
        const auto& indexName = scope.indexNames[i];

        if (handler.argumentNames.contains (indexName))
            continue;

        localNames.add (indexName);
        localTypes.push_back ({ Identifier (indexName), ComponentProperty::Type::integer });
        indices.add (scope.indexValues[i]);
    }

    auto locals = std::make_shared<YuiHandlerArguments> (localNames, localTypes);

    struct Step
    {
        String actionName;
        WeakReference<Component> emitter;
        Identifier emitted;
        std::vector<std::shared_ptr<PropertyExpression>> values;
    };

    auto steps = std::make_shared<std::vector<Step>>();

    for (const auto& call : handler.calls)
    {
        Step step;
        std::size_t firstArgument = 0;
        const ComponentSignal* emitted = nullptr;

        if (call.target.isEmpty() && call.name == "action")
        {
            if (call.arguments.empty()
                || call.arguments[0].getKind() != Expression::Kind::constant
                || ! call.arguments[0].getConstantValue().isString())
                return fail ("action expects the name of the action as a string");

            step.actionName = call.arguments[0].getConstantValue().toString();
            firstArgument = 1;
        }
        else
        {
            const auto callee = call.target.isEmpty() ? call.name : call.target + "." + call.name;
            auto* target = call.target.isNotEmpty() ? resolve (scope, call.target, &component) : nullptr;

            if (target == nullptr)
                return fail ("unknown function '" + callee + "'");

            step.emitted = Identifier (call.name);
            emitted = findDeclaredSignal (*target, step.emitted);

            if (emitted == nullptr)
                return fail ("'" + callee + "' is not a signal declared by a component");

            step.emitter = target;
        }

        // The arguments are evaluated each time the signal is emitted, typed as the emitted signal declares them
        for (std::size_t i = firstArgument; i < call.arguments.size(); ++i)
        {
            const auto argIndex = i - firstArgument;
            std::optional<ComponentProperty::Type> type;

            if (emitted != nullptr && argIndex < emitted->arguments.size())
                if (const auto declared = emitted->arguments[argIndex].type; ComponentProperty::isExpressionType (declared))
                    type = declared;

            auto compiled = PropertyExpression::compile (component, call.arguments[i].toString(), type, resolverFor (scope), locals.get(), baseDirectory);

            if (compiled.failed())
                return fail (compiled.getErrorMessage());

            step.values.push_back (std::move (compiled.getReference()));
        }

        steps->push_back (std::move (step));
    }

    signal.connect (component, [this, locals, numArguments, indices, madeFor = scope.weak_from_this(), steps = std::shared_ptr<const std::vector<Step>> (std::move (steps))] (const Array<var>& signalArguments)
    {
        // A call may have the document built again, which destroys this function while it runs: what it reads is held here
        auto& owner = *this;
        const auto heldLocals = locals;
        const auto heldSteps = steps;
        const auto heldScope = madeFor;

        auto localValues = signalArguments;
        localValues.resize (numArguments);
        localValues.addArray (indices);

        for (const auto& step : *heldSteps)
        {
            // The scope goes with the build the handler was made by, and the calls left are of an interface that is gone
            if (heldScope.expired())
                return;

            // Set for each call, as a call emitting a signal can run this handler again
            heldLocals->setValues (localValues);

            auto callArguments = signalArguments;

            if (! step.values.empty())
            {
                callArguments.clearQuick();

                for (const auto& argument : step.values)
                    callArguments.add (argument->evaluate());
            }

            if (step.actionName.isEmpty())
            {
                if (auto* target = step.emitter.get())
                    owner.emit (*target, step.emitted, callArguments);
            }
            else if (const auto found = owner.actions.find (step.actionName); found != owner.actions.end())
            {
                const auto action = found->second;
                action (callArguments);
            }
            else if (owner.fallbackAction != nullptr)
            {
                const auto action = owner.fallbackAction;
                action (step.actionName, callArguments);
            }
        }
    });
}

void YuiBuilder::emit (Component& component, const Identifier& signal, const Array<var>& arguments)
{
    std::vector<ComponentSignal::Handler> handlers;

    for (const auto& connection : connections)
        if (connection.component == &component && connection.signal == signal)
            handlers.push_back (connection.handler);

    for (const auto& handler : handlers)
        handler (arguments);
}

const ComponentSignal* YuiBuilder::findDeclaredSignal (const Component& component, const Identifier& signal) const
{
    for (const auto* info = &component.getTypeInfo(); info != nullptr; info = info->getBaseTypeInfo())
    {
        const auto isDeclared = std::any_of (composites.begin(), composites.end(), [info] (const auto& entry)
        {
            return entry.second.info.get() == info;
        });

        if (! isDeclared)
            continue;

        for (const auto& declared : info->getOwnSignals())
            if (declared.name == signal)
                return &declared;
    }

    return nullptr;
}

Result YuiBuilder::report (const String& message, const DataTree& element, const Identifier& property)
{
    diagnostics.push_back ({ .message = message, .element = element, .property = property });
    return Result::fail (message);
}

void YuiBuilder::report (const Component& component, const Identifier& name, const String& message, const Value& value)
{
    report (component.getTypeInfo().getTypeName().toString() + "." + name.toString() + ": " + message, value.element, value.property);
}

//==============================================================================

Component* YuiBuilder::resolveComponent (const Scope& scope, const String& name, Component* self) const
{
    if (self != nullptr && name == "self")
        return self;

    if (self != nullptr && name == "parent")
        return self->getParentComponent();

    // A turn of a repeat reads what the scope around it holds
    if (scope.outer != nullptr)
    {
        if (const auto found = scope.ids.find (name); found != scope.ids.end())
            return found->second;

        return resolveComponent (*scope.outer, name);
    }

    if (name == "root")
        return scope.root;

    if (const auto found = scope.ids.find (name); found != scope.ids.end())
        return found->second;

    return nullptr;
}

Component* YuiBuilder::resolve (const Scope& scope, const String& name, Component* self) const
{
    if (auto* component = resolveComponent (scope, name, self))
        return component;

    if (const auto found = models.find (name); found != models.end())
        return found->second.get();

    return nullptr;
}

DrawingScope::ComponentResolver YuiBuilder::resolverFor (const Scope& scope) const
{
    // What is given the resolver may outlive the build its scope is of, as a drawing something still holds does: it
    // then finds nothing
    return [this, scopeToUse = scope.weak_from_this()] (const String& name) -> Component*
    {
        const auto alive = scopeToUse.lock();
        return alive != nullptr ? resolve (*alive, name) : nullptr;
    };
}

DrawingScope::ComponentResolver YuiBuilder::getComponentResolver() const
{
    return [this] (const String& name) -> Component*
    {
        if (! scopes.empty())
            if (auto* component = resolveComponent (*scopes.front(), name))
                return component;

        if (const auto found = models.find (name); found != models.end())
            return found->second.get();

        return nullptr;
    };
}

} // namespace yup
