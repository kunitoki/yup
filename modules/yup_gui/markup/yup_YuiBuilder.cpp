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

ComponentProperty makeDeclaredProperty (const DataTree& declaration, bool isInherited)
{
    const auto type = ComponentProperty::getTypeFromName (declaration.getProperty ("type").toString());

    var defaultValue;

    switch (type)
    {
        case ComponentProperty::Type::number:  defaultValue = 0.0; break;
        case ComponentProperty::Type::integer: defaultValue = 0; break;
        case ComponentProperty::Type::boolean: defaultValue = false; break;
        case ComponentProperty::Type::string:  defaultValue = String(); break;
        case ComponentProperty::Type::color:   defaultValue = static_cast<int64> (0); break;
        default:                               break;
    }

    return ComponentProperty {
        .name = Identifier (declaration.getProperty ("name").toString()),
        .type = type,
        .defaultValue = defaultValue,
        .isInherited = isInherited
    };
}

struct Call
{
    String callee;
    StringArray arguments;
};

// Parses `callee (argument, ...)`, splitting the arguments on the commas outside strings and brackets
std::optional<Call> parseCall (const String& text)
{
    const auto trimmed = text.trim();
    const auto open = trimmed.indexOfChar ('(');

    if (open <= 0 || ! trimmed.endsWithChar (')'))
        return std::nullopt;

    Call call;
    call.callee = trimmed.substring (0, open).trim();

    if (! call.callee.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_."))
        return std::nullopt;

    const auto inner = trimmed.substring (open + 1, trimmed.length() - 1);

    auto position = inner.getCharPointer();
    auto start = position;
    yup_wchar quote = 0;
    int depth = 0;

    for (;;)
    {
        const auto current = position;
        const auto c = position.getAndAdvance();

        if (c == 0 || (c == ',' && depth == 0 && quote == 0))
        {
            const auto argument = String (start, current).trim();

            if (argument.isNotEmpty())
                call.arguments.add (argument);

            if (c == 0)
                break;

            start = position;
        }
        else if (quote != 0)
        {
            if (c == '\\' && ! position.isEmpty())
                ++position;
            else if (c == quote)
                quote = 0;
        }
        else if (c == '"' || c == '\'')
        {
            quote = c;
        }
        else if (c == '(' || c == '[')
        {
            ++depth;
        }
        else if ((c == ')' || c == ']') && depth > 0)
        {
            --depth;
        }
    }

    return call;
}

} // namespace

//==============================================================================
/** Exposes a BindingProvider as a component, so expressions and links reach it like any element. */
class YuiBuilder::Model final
    : public Component
    , private BindingProvider::Listener
    , private ComponentListener
{
public:
    explicit Model (BindingProvider& providerToUse)
        : provider (providerToUse)
        , info (Identifier ("Model"), &Component::getClassTypeInfo(), makeProperties (providerToUse))
    {
        setTypeInfo (&info);
        provider.addListener (this);
    }

    ~Model() override
    {
        stopTrackingGestures();
        provider.removeListener (this);
    }

    // The provider is told about a gesture while the component linked to one of its values is pressed
    void trackGestures (Component& component, const Identifier& property)
    {
        if (! isTracking (component))
            component.addComponentListener (this);

        tracked.push_back ({ &component, property });
    }

    void stopTrackingGestures()
    {
        while (! tracked.empty())
            stopTracking (*tracked.back().component);
    }

private:
    struct Tracked
    {
        Component* component = nullptr;
        Identifier property;
        bool isInGesture = false;
    };

    static std::vector<ComponentProperty> makeProperties (BindingProvider& provider)
    {
        std::vector<ComponentProperty> properties;

        for (const auto& property : provider.getProperties())
        {
            properties.push_back ({
                .name = property.name,
                .type = property.type,
                .getter = [name = property.name] (const Component& component)
                {
                    return static_cast<const Model&> (component).provider.getValue (name);
                },
                .setter = [name = property.name] (Component& component, const var& value)
                {
                    static_cast<Model&> (component).provider.setValue (name, value);
                },
                .isStyleable = false,
                .isInherited = false });
        }

        return properties;
    }

    bool isTracking (const Component& component) const
    {
        return std::any_of (tracked.begin(), tracked.end(), [&] (const auto& entry)
        {
            return entry.component == &component;
        });
    }

    void stopTracking (Component& component)
    {
        for (auto& entry : tracked)
            if (entry.component == &component && entry.isInGesture)
                provider.endGesture (entry.property);

        tracked.erase (std::remove_if (tracked.begin(), tracked.end(), [&] (const auto& entry)
        {
            return entry.component == &component;
        }),
                       tracked.end());

        component.removeComponentListener (this);
    }

    void bindingValueChanged (BindingProvider&, const Identifier& name) override
    {
        notifyPropertyChanged (name);
    }

    void componentPropertyChanged (Component& component, const Identifier& property) override
    {
        static const Identifier pressedProperty ("pressed");

        if (property != pressedProperty)
            return;

        const auto isPressed = component.isPressed();

        for (auto& entry : tracked)
        {
            if (entry.component != &component || entry.isInGesture == isPressed)
                continue;

            entry.isInGesture = isPressed;

            if (isPressed)
                provider.beginGesture (entry.property);
            else
                provider.endGesture (entry.property);
        }
    }

    void componentBeingDeleted (Component& component) override
    {
        stopTracking (component);
    }

    BindingProvider& provider;
    ComponentTypeInfo info;
    std::vector<Tracked> tracked;
};

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
    clear();

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

    if (! root.isValid() || ! isComponentElement (root))
    {
        clear();
        return Result::fail ("The document has no root component");
    }

    auto& scope = *scopes.emplace_back (std::make_unique<Scope>());
    auto created = createComponent (root, nullptr, scope);

    if (created.failed())
    {
        clear();
        return Result::fail (created.getErrorMessage());
    }

    scope.root = created.getValue();
    applyTokens (*scope.root, scope);

    // The tree exists before any value is applied, so expressions can reference any element
    for (const auto& entry : pending)
        applyPending (entry);

    pending.clear();
    bindings.setComponentResolver (resolverFor (scope));

    return Result::ok();
}

void YuiBuilder::clear()
{
    bindings.clear();
    bindings.clearDiagnostics();
    bindings.setComponentResolver (nullptr);
    pending.clear();
    connections.clear();

    for (auto& [name, model] : models)
        model->stopTrackingGestures();

    // Children go first, so that no parent is destroyed while still holding them
    while (! components.empty())
        components.pop_back();

    scopes.clear();
    composites.clear();
    tokens.clear();
    tokensInfo.reset();
    diagnostics.clear();
}

//==============================================================================

Component* YuiBuilder::getRoot() const noexcept
{
    return components.empty() ? nullptr : components.front().get();
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
    models[name] = std::make_unique<Model> (provider);
}

void YuiBuilder::removeModel (const String& name)
{
    models.erase (name);
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
            diagnostics.add ("component " + name.toString() + ": the type is already declared");
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
            signals.push_back ({ .name = name,
                                 .connect = [this, name] (Component& component, ComponentSignal::Handler handler)
                                 {
                                     connections.push_back ({ &component, name, std::move (handler) });
                                 } });
        }
        else if (kind == "alias")
        {
            const auto target = declaration.getProperty ("target").toString();
            const auto dot = target.indexOfChar ('.');
            const auto element = dot > 0 ? findElementById (composite.body, target.substring (0, dot)) : DataTree();

            if (! element.isValid())
            {
                diagnostics.add (where + "." + name.toString() + ": unknown element in '" + target + "'");
                continue;
            }

            const auto elementType = resolveType (element.getType());

            if (elementType.failed())
                continue;

            const auto* aliased = elementType.getValue()->findProperty (Identifier (target.substring (dot + 1)));

            if (aliased == nullptr)
            {
                diagnostics.add (where + "." + name.toString() + ": " + element.getType().toString() + " has no property '" + target.substring (dot + 1) + "'");
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

ResultValue<Component*> YuiBuilder::createComponent (const DataTree& element, Component* parent, Scope& scope)
{
    const auto typeName = element.getType();
    Component* component = nullptr;

    if (const auto found = composites.find (typeName); found != composites.end())
    {
        auto& composite = found->second;

        if (auto result = resolveComposite (typeName, composite); result.failed())
            return makeResultValueFail (result.getErrorMessage());

        if (composite.isInstantiating)
            return makeResultValueFail ("The component '" + typeName.toString() + "' contains itself");

        // The body is created in a scope of its own, where `root` is the instance
        auto& bodyScope = *scopes.emplace_back (std::make_unique<Scope>());
        const auto declarationsIndex = static_cast<std::ptrdiff_t> (pending.size());

        auto body = [&]
        {
            const ScopedValueSetter<bool> instantiating (composite.isInstantiating, true);
            return createComponent (composite.body, parent, bodyScope);
        }();

        if (body.failed())
            return body;

        component = body.getValue();
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
                declarations.values.emplace_back (name, declaration.getProperty ("value").toString());
            else if (kind == "alias" && composite.info->findProperty (name) != nullptr)
                declarations.links.emplace_back (name, declaration.getProperty ("target").toString());
        }

        pending.insert (pending.begin() + declarationsIndex, std::move (declarations));
        applyIdentity (*component, element, scope, true);
    }
    else
    {
        const auto* typeInfo = ComponentTypeRegistry::getInstance().findType (typeName);

        if (typeInfo == nullptr)
            return makeResultValueFail ("Unknown component type '" + typeName.toString() + "'");

        auto created = typeInfo->createInstance();

        if (created == nullptr)
            return makeResultValueFail ("The component type '" + typeName.toString() + "' can't be created from .yui");

        component = created.get();
        components.push_back (std::move (created));

        if (parent != nullptr)
            parent->addAndMakeVisible (*component);

        applyIdentity (*component, element, scope, false);
    }

    static const Identifier bindType ("bind");

    Pending values { component, &scope };

    for (int i = 0; i < element.getNumProperties(); ++i)
    {
        const auto name = element.getPropertyName (i);

        if (name != idProperty() && name != classProperty())
            values.values.emplace_back (name, element.getProperty (name).toString());
    }

    for (const auto& child : element)
    {
        if (child.getType() == bindType)
            values.links.emplace_back (Identifier (child.getProperty ("property").toString()), child.getProperty ("target").toString());
    }

    pending.push_back (std::move (values));

    for (const auto& child : element)
    {
        if (child.getType() == bindType)
            continue;

        if (! isComponentElement (child))
        {
            diagnostics.add ("'" + child.getType().toString() + "' is not a component type");
            continue;
        }

        if (auto created = createComponent (child, component, scope); created.failed())
            return created;
    }

    return makeResultValueOk (component);
}

void YuiBuilder::applyIdentity (Component& component, const DataTree& element, Scope& scope, bool addClasses)
{
    if (element.hasProperty (idProperty()))
    {
        const auto id = getElementId (element);
        component.setComponentID (id);

        if (! scope.ids.emplace (id, &component).second)
            diagnostics.add ("The id '" + id + "' is used more than once");
    }

    if (element.hasProperty (classProperty()))
    {
        Array<Identifier> classes;

        for (const auto& styleClass : StringArray::fromTokens (element.getProperty (classProperty()).toString().unquoted(), " ,", ""))
            if (styleClass.isNotEmpty())
                classes.add (Identifier (styleClass));

        if (! addClasses)
            component.setStyleClasses (classes);
        else
            for (const auto& styleClass : classes)
                component.addStyleClass (styleClass);
    }
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
            diagnostics.add ("tokens: '" + property.name.toString() + "' is declared more than once");
            continue;
        }

        if (token.hasProperty ("value"))
            values.values.emplace_back (property.name, token.getProperty ("value").toString());

        properties.push_back (std::move (property));
    }

    // The tokens extend the type of the root, so they read and cascade like any of its properties
    tokensInfo = std::make_unique<ComponentTypeInfo> (root.getTypeInfo().getTypeName(), &root.getTypeInfo(), std::move (properties));
    root.setTypeInfo (tokensInfo.get());

    pending.insert (pending.begin(), std::move (values));
}

//==============================================================================

void YuiBuilder::applyPending (const Pending& entry)
{
    bindings.setComponentResolver (resolverFor (*entry.scope));

    for (const auto& [name, text] : entry.values)
        applyProperty (*entry.component, name, text, *entry.scope);

    for (const auto& [name, path] : entry.links)
        applyLink (*entry.component, name, path, *entry.scope);
}

void YuiBuilder::applyProperty (Component& component, const Identifier& name, const String& valueText, Scope& scope)
{
    const auto& typeInfo = component.getTypeInfo();

    if (const auto* signal = typeInfo.findSignal (name))
    {
        connectHandler (component, *signal, valueText, scope);
        return;
    }

    const auto* descriptor = typeInfo.findProperty (name);
    const auto where = typeInfo.getTypeName().toString() + "." + name.toString();

    if (descriptor != nullptr && descriptor->isReadOnly())
    {
        diagnostics.add (where + ": the property is read-only");
        return;
    }

    if (descriptor != nullptr && descriptor->type == ComponentProperty::Type::enumeration)
    {
        const auto value = valueText.unquoted().trim();

        if (! descriptor->enumValues.contains (value))
            diagnostics.add (where + ": expected one of " + descriptor->enumValues.joinIntoString (", "));
        else
            component.setProperty (name, value);

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
            case ComponentProperty::Type::color:   type = descriptor->type; break;
            case ComponentProperty::Type::any:     break;

            default:
                diagnostics.add (where + ": properties of this type can't be set from .yui yet");
                return;
        }
    }

    if (auto result = bindings.bind (component, name, valueText, type); result.failed())
        diagnostics.add (where + ": " + result.getErrorMessage());
}

void YuiBuilder::applyLink (Component& component, const Identifier& name, const String& path, Scope& scope)
{
    const auto where = component.getTypeInfo().getTypeName().toString() + "." + name.toString();
    const auto dot = path.indexOfChar ('.');

    if (dot <= 0)
    {
        diagnostics.add (where + ": expected element.property after '<=>'");
        return;
    }

    const auto elementName = path.substring (0, dot);
    auto* source = resolve (scope, elementName, &component);

    if (source == nullptr)
    {
        diagnostics.add (where + ": unknown element '" + elementName + "'");
        return;
    }

    const Identifier sourceProperty (path.substring (dot + 1));

    if (source->getTypeInfo().findProperty (sourceProperty) == nullptr)
    {
        diagnostics.add (where + ": " + source->getTypeInfo().getTypeName().toString() + " has no property '" + sourceProperty.toString() + "'");
        return;
    }

    if (const auto* descriptor = component.getTypeInfo().findProperty (name); descriptor != nullptr && descriptor->isReadOnly())
    {
        diagnostics.add (where + ": the property is read-only");
        return;
    }

    bindings.link (component, name, *source, sourceProperty);

    if (auto* model = dynamic_cast<Model*> (source))
        model->trackGestures (component, sourceProperty);
}

//==============================================================================

void YuiBuilder::connectHandler (Component& component, const ComponentSignal& signal, const String& text, Scope& scope)
{
    const auto where = component.getTypeInfo().getTypeName().toString() + "." + signal.name.toString();
    const auto call = parseCall (text);

    if (! call.has_value())
    {
        diagnostics.add (where + ": expected a call, like action (\"name\") or root.signal()");
        return;
    }

    auto arguments = call->arguments;
    String actionName;
    WeakReference<Component> emitter;
    Identifier emitted;

    if (call->callee == "action")
    {
        if (arguments.isEmpty() || ! arguments[0].isQuotedString())
        {
            diagnostics.add (where + ": action expects the name of the action as a string");
            return;
        }

        actionName = arguments[0].unquoted();
        arguments.remove (0);
    }
    else
    {
        const auto dot = call->callee.lastIndexOfChar ('.');
        auto* target = dot > 0 ? resolve (scope, call->callee.substring (0, dot), &component) : nullptr;

        if (target == nullptr)
        {
            diagnostics.add (where + ": unknown function '" + call->callee + "'");
            return;
        }

        emitted = Identifier (call->callee.substring (dot + 1));

        if (! isDeclaredSignal (*target, emitted))
        {
            diagnostics.add (where + ": '" + call->callee + "' is not a signal declared by a component");
            return;
        }

        emitter = target;
    }

    // The arguments are evaluated each time the signal is emitted
    std::vector<std::shared_ptr<PropertyExpression>> values;

    for (const auto& argument : arguments)
    {
        auto compiled = PropertyExpression::compile (component, argument, std::nullopt, resolverFor (scope));

        if (compiled.failed())
        {
            diagnostics.add (where + ": " + compiled.getErrorMessage());
            return;
        }

        values.push_back (std::move (compiled.getReference()));
    }

    signal.connect (component, [this, actionName, emitter, emitted, values] (const Array<var>& signalArguments)
    {
        auto callArguments = signalArguments;

        if (! values.empty())
        {
            callArguments.clearQuick();

            for (const auto& value : values)
                callArguments.add (value->evaluate());
        }

        if (actionName.isEmpty())
        {
            if (auto* target = emitter.get())
                emit (*target, emitted, callArguments);
        }
        else if (const auto found = actions.find (actionName); found != actions.end())
        {
            const auto action = found->second;
            action (callArguments);
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

bool YuiBuilder::isDeclaredSignal (const Component& component, const Identifier& signal) const
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
                return true;
    }

    return false;
}

//==============================================================================

Component* YuiBuilder::resolve (const Scope& scope, const String& name, Component* self) const
{
    if (self != nullptr && name == "self")
        return self;

    if (self != nullptr && name == "parent")
        return self->getParentComponent();

    if (name == "root")
        return scope.root;

    if (const auto found = scope.ids.find (name); found != scope.ids.end())
        return found->second;

    if (const auto found = models.find (name); found != models.end())
        return found->second.get();

    return nullptr;
}

DrawingScope::ComponentResolver YuiBuilder::resolverFor (const Scope& scope) const
{
    return [this, scopeToUse = &scope] (const String& name)
    {
        return resolve (*scopeToUse, name);
    };
}

} // namespace yup
