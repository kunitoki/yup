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

bool isComponentElement (const DataTree& element)
{
    const auto type = element.getType().toString();
    return type.isNotEmpty() && CharacterFunctions::isUpperCase (type[0]);
}

String getElementId (const DataTree& element)
{
    return element.getProperty (idProperty()).toString().unquoted().trim();
}

DataTree findElementById (const DataTree& element, const String& id)
{
    if (getElementId (element) == id)
        return element;

    for (const auto& child : element)
    {
        if (! isComponentElement (child))
            continue;

        if (auto found = findElementById (child, id); found.isValid())
            return found;
    }

    return {};
}

// Like C++ members, values apply in the order the type declares them (bases first), so a property constraining
// another one, a Slider range, applies before it wherever the document writes it; undeclared names come last
template <class Value>
std::vector<const Value*> inDeclarationOrder (const ComponentTypeInfo& type, const std::vector<Value>& values)
{
    std::vector<const ComponentTypeInfo*> chain;

    for (const auto* info = &type; info != nullptr; info = info->getBaseTypeInfo())
        chain.insert (chain.begin(), info);

    const auto rankOf = [&chain] (const Identifier& name)
    {
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

        return std::numeric_limits<std::size_t>::max();
    };

    std::vector<std::pair<std::size_t, const Value*>> ranked;
    ranked.reserve (values.size());

    for (const auto& value : values)
        ranked.emplace_back (rankOf (value.name), &value);

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

    switch (type)
    {
        case ComponentProperty::Type::number:    defaultValue = 0.0; break;
        case ComponentProperty::Type::integer:   defaultValue = 0; break;
        case ComponentProperty::Type::boolean:   defaultValue = false; break;
        case ComponentProperty::Type::string:    defaultValue = String(); break;
        case ComponentProperty::Type::color:     defaultValue = static_cast<int64> (0); break;
        case ComponentProperty::Type::point:     defaultValue = VariantConverter<Point<float>>::toVar ({}); break;
        case ComponentProperty::Type::rectangle: defaultValue = VariantConverter<Rectangle<float>>::toVar ({}); break;
        case ComponentProperty::Type::array:     defaultValue = var (Array<var>()); break;
        default:                                 break;
    }

    return ComponentProperty {
        .name = Identifier (declaration.getProperty ("name").toString()),
        .type = type,
        .elementType = ComponentProperty::getElementTypeFromName (typeName),
        .defaultValue = defaultValue,
        .isInherited = isInherited
    };
}

} // namespace

//==============================================================================

YuiBuilder::YuiBuilder()
{
    registerBuiltInComponentTypes();
}

YuiBuilder::~YuiBuilder()
{
    clear();
}

//==============================================================================

Result YuiBuilder::build (const DataTree& document)
{
    // The previous declarations stay alive until the reused components have their new types
    auto previousComposites = std::exchange (composites, {});
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
            if (isComponentElement (element))
            {
                root = element;
                break;
            }
        }
    }

    const auto checked = root.isValid() && isComponentElement (root)
                           ? checkTypes (root)
                           : report ("The document has no root component", document);

    if (checked.failed())
    {
        composites = std::move (previousComposites);
        tokens = std::move (previousTokens);
        tokensInfo = std::move (previousTokensInfo);
        return checked;
    }

    resetBindings();

    for (auto& [name, model] : models)
        model->refreshProperties();

    const auto previousScopes = std::exchange (scopes, {});
    previousComponents = std::exchange (components, {});

    auto& scope = *scopes.emplace_back (std::make_unique<Scope>());
    scope.root = &createComponent (root, nullptr, scope, {});
    applyTokens (*scope.root, scope);

    // The tree exists before any value is applied, so expressions can reference any element
    applyValues (std::exchange (applied, {}));
    bindings.setComponentResolver (resolverFor (scope));
    bindings.setModelResolver (getModelResolver());

    // Children go first, so that no parent is destroyed while still holding them
    while (! previousComponents.empty())
        previousComponents.pop_back();

    return Result::ok();
}

void YuiBuilder::clear()
{
    resetBindings();

    while (! components.empty())
        components.pop_back();

    applied.clear();
    scopes.clear();
    composites.clear();
    tokens.clear();
    tokensInfo.reset();
    diagnostics.clear();
}

void YuiBuilder::resetBindings()
{
    bindings.clear();
    bindings.clearDiagnostics();
    bindings.setComponentResolver (nullptr);
    bindings.setModelResolver (nullptr);
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

//==============================================================================

void YuiBuilder::setAction (const String& name, Action action)
{
    if (action != nullptr)
        actions[name] = std::move (action);
    else
        actions.erase (name);
}

void YuiBuilder::setModel (const String& name, BindingProvider& provider)
{
    if (const auto found = models.find (name); found != models.end())
    {
        if (auto* existing = found->second->getProvider())
            bindings.forgetProvider (*existing);
    }

    models[name] = std::make_unique<Model> (provider);
}

void YuiBuilder::removeModel (const String& name)
{
    if (const auto found = models.find (name); found != models.end())
    {
        if (auto* existing = found->second->getProvider())
            bindings.forgetProvider (*existing);

        models.erase (found);
    }
}

//==============================================================================

void YuiBuilder::collectDeclarations (const DataTree& document)
{
    for (const auto& element : document)
    {
        const auto kind = element.getType().toString();

        if (kind == "tokens")
        {
            for (const auto& token : element)
                tokens.push_back (token);

            continue;
        }

        if (kind != "component")
            continue;

        const Identifier name (element.getProperty ("name").toString());

        if (composites.count (name) > 0 || ComponentTypeRegistry::getInstance().findType (name) != nullptr)
        {
            report ("component " + name.toString() + ": the type is already declared", element);
            continue;
        }

        Composite composite;
        composite.declaration = element;

        for (const auto& child : element)
            if (isComponentElement (child))
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
        if (isComponentElement (declaration))
            continue;

        const auto kind = declaration.getType().toString();
        const Identifier name (declaration.getProperty ("name").toString());

        if (kind == "property")
        {
            properties.push_back (makeDeclaredProperty (declaration, false));
        }
        else if (kind == "signal")
        {
            std::vector<ComponentSignal::Argument> arguments;

            for (const auto& argument : declaration)
            {
                const auto argumentType = argument.getProperty ("type").toString();

                arguments.push_back ({ Identifier (argument.getProperty ("name").toString()),
                                       ComponentProperty::getTypeFromName (argumentType),
                                       ComponentProperty::getElementTypeFromName (argumentType) });
            }

            signals.push_back ({ .name = name,
                                 .connect = [this, name] (Component& component, ComponentSignal::Handler handler)
                                 {
                                     connections.push_back ({ &component, name, std::move (handler) });
                                 },
                                 .arguments = std::move (arguments) });
        }
        else if (kind == "alias")
        {
            const auto target = declaration.getProperty ("target").toString();
            const auto dot = target.indexOfChar ('.');
            const auto element = dot > 0 ? findElementById (composite.body, target.substring (0, dot)) : DataTree();

            if (! element.isValid())
            {
                report (where + "." + name.toString() + ": unknown element in '" + target + "'", declaration);
                continue;
            }

            const auto elementType = resolveType (element.getType());

            if (elementType.failed())
                continue;

            const auto* aliased = elementType.getValue()->findProperty (Identifier (target.substring (dot + 1)));

            if (aliased == nullptr)
            {
                report (where + "." + name.toString() + ": " + element.getType().toString() + " has no property '" + target.substring (dot + 1) + "'", declaration);
                continue;
            }

            auto alias = *aliased;
            alias.name = name;
            alias.storageId = {};
            alias.getter = nullptr;
            alias.setter = nullptr;
            alias.isState = false;
            alias.isInherited = false;
            alias.affectsParentLayout = false;
            properties.push_back (std::move (alias));
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

    for (const auto& child : element)
    {
        if (! isComponentElement (child))
            continue;

        if (auto result = checkTypes (child); result.failed())
            return result;
    }

    return Result::ok();
}

Component& YuiBuilder::createComponent (const DataTree& element, Component* parent, Scope& scope, const DataTree& recordElement)
{
    const auto typeName = element.getType();
    const auto& key = recordElement.isValid() ? recordElement : element;
    Component* component = nullptr;

    if (const auto found = composites.find (typeName); found != composites.end())
    {
        auto& composite = found->second;

        // The body is created in a scope of its own, where `root` is the instance
        auto& bodyScope = *scopes.emplace_back (std::make_unique<Scope>());
        const auto declarationsIndex = static_cast<std::ptrdiff_t> (pending.size());

        component = &createComponent (composite.body, parent, bodyScope, key);
        bodyScope.root = component;
        component->setTypeInfo (composite.info.get());

        // The declared defaults and aliases come before the values of the body and of the use site
        Pending declarations { component, &bodyScope };

        for (const auto& declaration : composite.declaration)
        {
            if (isComponentElement (declaration))
                continue;

            const auto kind = declaration.getType().toString();
            const Identifier name (declaration.getProperty ("name").toString());

            if (kind == "property" && declaration.hasProperty ("value"))
                declarations.values.push_back ({ name, declaration.getProperty ("value").toString(), declaration });
            else if (kind == "alias" && composite.info->findProperty (name) != nullptr)
                declarations.links.push_back ({ name, declaration.getProperty ("target").toString(), declaration });
        }

        pending.insert (pending.begin() + declarationsIndex, std::move (declarations));
        applyIdentity (*component, element, scope, true);
    }
    else
    {
        // checkTypes made sure the type exists and can be created
        const auto& type = *ComponentTypeRegistry::getInstance().findType (typeName);

        auto created = reuseComponent (key, parent, type);
        const auto isReused = created != nullptr;

        if (! isReused)
            created = type.createInstance();

        component = created.get();

        // The identity comes first, so a container placing its children itself knows them
        applyIdentity (*component, element, scope, false);

        if (parent != nullptr)
            placeChild (*parent, *component, isReused);

        components.push_back ({ key, parent, &type, std::move (created) });
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
            values.links.push_back ({ Identifier (child.getProperty ("property").toString()), child.getProperty ("target").toString(), child });
        else if (child.getType() == changedType)
            values.changes.push_back ({ Identifier (child.getProperty ("property").toString()), child.getProperty ("handler").toString(), child, Identifier ("handler") });
    }

    pending.push_back (std::move (values));

    for (const auto& child : element)
    {
        if (child.getType() == bindType || child.getType() == changedType)
            continue;

        if (! isComponentElement (child))
        {
            report ("'" + child.getType().toString() + "' is not a component type", child);
            continue;
        }

        createComponent (child, component, scope, {});
    }

    return *component;
}

std::unique_ptr<Component> YuiBuilder::reuseComponent (const DataTree& element, Component* parent, const ComponentTypeInfo& type)
{
    for (auto& record : previousComponents)
    {
        if (record.component == nullptr || record.element != element || record.parent != parent || record.type != &type)
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
    // A kept component loses the identity it had, unless it's a composite instance keeping the one of its body
    if (element.hasProperty (idProperty()))
    {
        const auto id = getElementId (element);
        component.setComponentID (id);

        if (! scope.ids.emplace (id, &component).second)
            report ("The id '" + id + "' is used more than once", element, idProperty());
    }
    else if (! isUseSite)
    {
        component.setComponentID ({});
    }

    Array<Identifier> classes;

    for (const auto& styleClass : StringArray::fromTokens (element.getProperty (classProperty()).toString().unquoted(), " ,", ""))
        if (styleClass.isNotEmpty())
            classes.add (Identifier (styleClass));

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

void YuiBuilder::applyValues (const AppliedValues& previous)
{
    const auto linkText = [] (const String& path)
    {
        return "<=> " + path;
    };

    // Later values of a property override earlier ones, so the last text is the one applied
    for (const auto& entry : pending)
    {
        for (const auto& value : entry.values)
            applied[entry.component][value.name.toString()] = { value.text };

        for (const auto& link : entry.links)
            applied[entry.component][link.name.toString()] = { linkText (link.text) };
    }

    for (const auto& record : components)
    {
        auto* component = record.component.get();
        const auto found = previous.find (component);

        if (found == previous.end())
            continue;

        auto& values = applied[component];

        for (const auto& [name, before] : found->second)
        {
            if (const auto now = values.find (name); now == values.end())
                resetValue (*component, *record.type, Identifier (name));
            else
                now->second.isKept = before.isConstant && before.text == now->second.text;
        }
    }

    for (const auto& entry : pending)
        applyPending (entry);

    pending.clear();

    // Constants applied again with the same text are skipped next time, keeping what the user changed since
    for (auto& [component, values] : applied)
    {
        for (auto& [name, value] : values)
        {
            const Identifier property (name);

            value.isConstant = value.isKept
                            || (! value.hasFailed
                                && ! value.text.startsWith ("<=>")
                                && component->getTypeInfo().findSignal (property) == nullptr
                                && bindings.findBinding (*component, property) == nullptr);
        }
    }
}

void YuiBuilder::resetValue (Component& component, const ComponentTypeInfo& type, const Identifier& name)
{
    const auto& typeInfo = component.getTypeInfo();

    if (const auto* signal = typeInfo.findSignal (name))
    {
        signal->connect (component, [] (const Array<var>&) {});
        return;
    }

    const auto* descriptor = typeInfo.findProperty (name);

    if (descriptor == nullptr || ! descriptor->isStateBacked())
    {
        component.removeProperty (name);
        return;
    }

    // A state-backed value goes back to the one a new component has
    if (! descriptor->isReadOnly())
        component.setProperty (name, type.createInstance()->getProperty (name));
}

void YuiBuilder::applyPending (const Pending& entry)
{
    bindings.setComponentResolver (resolverFor (*entry.scope));
    bindings.setModelResolver (getModelResolver());
    auto& values = applied[entry.component];

    for (const auto* entryValue : inDeclarationOrder (entry.component->getTypeInfo(), entry.values))
    {
        auto& value = values[entryValue->name.toString()];

        if (value.isKept)
            continue;

        const auto numDiagnostics = diagnostics.size();
        applyProperty (*entry.component, *entryValue, *entry.scope);
        value.hasFailed = value.hasFailed || diagnostics.size() > numDiagnostics;
    }

    for (const auto& link : entry.links)
        applyLink (*entry.component, link, *entry.scope);

    for (const auto& change : entry.changes)
        applyChanged (*entry.component, change, *entry.scope);
}

void YuiBuilder::applyProperty (Component& component, const Value& value, Scope& scope)
{
    const auto& typeInfo = component.getTypeInfo();
    const auto& name = value.name;

    if (const auto* signal = typeInfo.findSignal (name))
    {
        connectHandler (component, *signal, value, scope);
        return;
    }

    const auto* descriptor = typeInfo.findProperty (name);
    const auto where = typeInfo.getTypeName().toString() + "." + name.toString();

    if (descriptor != nullptr && descriptor->isReadOnly())
    {
        report (where + ": the property is read-only", value.element, value.property);
        return;
    }

    if (descriptor != nullptr && descriptor->type == ComponentProperty::Type::enumeration)
    {
        const auto enumValue = value.text.unquoted().trim();

        if (! descriptor->enumValues.contains (enumValue))
            report (where + ": expected one of " + descriptor->enumValues.joinIntoString (", "), value.element, value.property);
        else
            component.setProperty (name, enumValue);

        return;
    }

    std::optional<ComponentProperty::Type> type;

    if (descriptor != nullptr)
    {
        switch (descriptor->type)
        {
            case ComponentProperty::Type::number:
            case ComponentProperty::Type::integer:
            case ComponentProperty::Type::boolean:
            case ComponentProperty::Type::string:
            case ComponentProperty::Type::color:
            case ComponentProperty::Type::point:
            case ComponentProperty::Type::rectangle:
            case ComponentProperty::Type::array:   type = descriptor->type; break;
            case ComponentProperty::Type::any:     break;

            default:
                report (where + ": properties of this type can't be set from .yui yet", value.element, value.property);
                return;
        }
    }

    if (auto result = bindings.bind (component, name, value.text, type); result.failed())
        report (where + ": " + result.getErrorMessage(), value.element, value.property);
}

void YuiBuilder::applyLink (Component& component, const Value& link, Scope& scope)
{
    const auto& name = link.name;
    const auto& path = link.text;
    const auto where = component.getTypeInfo().getTypeName().toString() + "." + name.toString();
    const auto dot = path.indexOfChar ('.');

    if (dot <= 0)
    {
        report (where + ": expected element.property after '<=>'", link.element, link.property);
        return;
    }

    const auto elementName = path.substring (0, dot);
    auto* source = resolve (scope, elementName, &component);

    if (source == nullptr)
    {
        report (where + ": unknown element '" + elementName + "'", link.element, link.property);
        return;
    }

    const Identifier sourceProperty (path.substring (dot + 1));

    auto* model = dynamic_cast<Model*> (source);
    if (model != nullptr && source->getTypeInfo().findProperty (sourceProperty) == nullptr)
        model->refreshProperties();

    if (source->getTypeInfo().findProperty (sourceProperty) == nullptr)
    {
        report (where + ": " + source->getTypeInfo().getTypeName().toString() + " has no property '" + sourceProperty.toString() + "'", link.element, link.property);
        return;
    }

    if (const auto* descriptor = component.getTypeInfo().findProperty (name); descriptor != nullptr && descriptor->isReadOnly())
    {
        report (where + ": the property is read-only", link.element, link.property);
        return;
    }

    bindings.link (component, name, *source, sourceProperty);

    if (model != nullptr)
        model->trackGestures (component, sourceProperty);
}

//==============================================================================

void YuiBuilder::applyChanged (Component& component, const Value& change, Scope& scope)
{
    const auto* descriptor = component.getTypeInfo().findProperty (change.name);

    if (descriptor == nullptr)
    {
        report (component.getTypeInfo().getTypeName().toString() + ".changed " + change.name.toString() + ": unknown property '" + change.name.toString() + "'", change.element);
        return;
    }

    // A changed handler is the handler of a signal emitted with the new value
    const ComponentSignal signal {
        .name = Identifier ("changed " + change.name.toString()),
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
    const auto where = component.getTypeInfo().getTypeName().toString() + "." + signal.name.toString();

    const auto fail = [&] (const String& message)
    {
        report (where + ": " + message, value.element, value.property);
    };

    const auto parsed = YuiHandler::parse (value.text);

    if (parsed.failed())
        return fail (parsed.getErrorMessage());

    const auto& handler = parsed.getReference();

    if (handler.argumentNames.size() > static_cast<int> (signal.arguments.size()))
        return fail ("expected at most " + String (static_cast<int> (signal.arguments.size())) + " argument names");

    auto locals = std::make_shared<YuiHandlerArguments> (handler.argumentNames, signal.arguments);

    struct Step
    {
        String actionName;
        WeakReference<Component> emitter;
        Identifier emitted;
        std::vector<std::shared_ptr<PropertyExpression>> values;
    };

    std::vector<Step> steps;

    for (const auto& call : handler.calls)
    {
        Step step;
        auto arguments = call.arguments;
        const ComponentSignal* emitted = nullptr;

        if (call.callee == "action")
        {
            if (arguments.isEmpty() || ! arguments[0].isQuotedString())
                return fail ("action expects the name of the action as a string");

            step.actionName = arguments[0].unquoted();
            arguments.remove (0);
        }
        else
        {
            const auto dot = call.callee.lastIndexOfChar ('.');
            auto* target = dot > 0 ? resolve (scope, call.callee.substring (0, dot), &component) : nullptr;

            if (target == nullptr)
                return fail ("unknown function '" + call.callee + "'");

            step.emitted = Identifier (call.callee.substring (dot + 1));

            emitted = findDeclaredSignal (*target, step.emitted);

            if (emitted == nullptr)
                return fail ("'" + call.callee + "' is not a signal declared by a component");

            step.emitter = target;
        }

        // The arguments are evaluated each time the signal is emitted, typed as the emitted signal declares them
        for (int i = 0; i < arguments.size(); ++i)
        {
            std::optional<ComponentProperty::Type> type;

            if (emitted != nullptr && i < static_cast<int> (emitted->arguments.size()))
                if (const auto declared = emitted->arguments[static_cast<size_t> (i)].type; ComponentProperty::isExpressionType (declared))
                    type = declared;

            auto compiled = PropertyExpression::compile (component, arguments[i], type, resolverFor (scope), getModelResolver(), locals.get());

            if (compiled.failed())
                return fail (compiled.getErrorMessage());

            step.values.push_back (std::move (compiled.getReference()));
        }

        steps.push_back (std::move (step));
    }

    signal.connect (component, [this, locals, steps] (const Array<var>& signalArguments)
    {
        for (const auto& step : steps)
        {
            // Set for each call, as a call emitting a signal can run this handler again
            locals->setValues (signalArguments);

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
                    emit (*target, step.emitted, callArguments);
            }
            else if (const auto found = actions.find (step.actionName); found != actions.end())
            {
                const auto action = found->second;
                action (callArguments);
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

//==============================================================================

Component* YuiBuilder::resolveComponent (const Scope& scope, const String& name, Component* self) const
{
    if (self != nullptr && name == "self")
        return self;

    if (self != nullptr && name == "parent")
        return self->getParentComponent();

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
    return [this, scopeToUse = &scope] (const String& name)
    {
        return resolveComponent (*scopeToUse, name);
    };
}

DrawingScope::ComponentResolver YuiBuilder::getComponentResolver() const
{
    return [this] (const String& name) -> Component*
    {
        if (scopes.empty())
            return nullptr;

        return resolveComponent (*scopes.front(), name);
    };
}

DrawingScope::ModelResolver YuiBuilder::getModelResolver() const
{
    return [this] (const String& name) -> BindingProvider*
    {
        if (const auto found = models.find (name); found != models.end())
            return found->second->getProvider();

        return nullptr;
    };
}

} // namespace yup
