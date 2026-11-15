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

DrawingScope::DrawingScope (Component& componentToUse, ExpressionContext& contextToUse)
    : component (&componentToUse)
    , context (contextToUse)
{
}

void DrawingScope::setComponentResolver (ComponentResolver resolver)
{
    componentResolver = std::move (resolver);
    cachedPrograms.clear();
}

void DrawingScope::setModelResolver (ModelResolver resolver)
{
    modelResolver = std::move (resolver);
    cachedPrograms.clear();
}

void DrawingScope::setLocals (BindingProvider* newLocals)
{
    locals = newLocals;
    cachedPrograms.clear();
}

void DrawingScope::install()
{
    context.setSymbolResolver ([this] (const String& name)
    {
        return resolveSymbol (name);
    });
}

std::optional<BindingProvider::Property> DrawingScope::findProviderProperty (const BindingProvider& provider, const Identifier& name)
{
    return provider.findProperty (name);
}

const ExpressionType* DrawingScope::resolveSymbol (const String& symbol) const
{
    if (symbol.startsWith ("palette."))
        return typeForPaletteRole (symbol.substring (8));

    const auto target = resolveTarget (symbol);

    if (target.component != nullptr)
    {
        if (const auto* property = target.component->getTypeInfo().findProperty (target.property))
            return getExpressionType (property->type, property->elementType);
    }
    else if (target.provider != nullptr)
    {
        if (const auto property = findProviderProperty (*target.provider, target.property))
            return getExpressionType (property->type, property->elementType);
    }

    return nullptr;
}

const DrawingScope::CachedProgram& DrawingScope::getCachedProgram (const CompiledExpression& program) const
{
    for (const auto& entry : cachedPrograms)
        if (entry->program == program)
            return *entry;

    auto cached = std::make_unique<CachedProgram>();
    cached->program = program;

    for (int i = 0; i < program.getNumInputs(); ++i)
    {
        if (! program.usesInput (i))
            continue;

        const auto& name = program.getInputName (i);

        if (name.startsWith ("palette."))
        {
            const auto roleName = name.substring (8);

            if (roleName == "dark")
            {
                cached->inputs.push_back ({ .index = i, .kind = ParsedInput::Kind::paletteDark });
                cached->usesPalette = true;
            }
            else if (const auto role = ThemePalette::getRoleFromName (roleName))
            {
                cached->inputs.push_back ({ .index = i, .kind = ParsedInput::Kind::paletteRole, .paletteRole = *role });
                cached->usesPalette = true;
            }

            continue;
        }

        if (name.startsWith ("parent."))
        {
            cached->inputs.push_back ({ .index = i, .kind = ParsedInput::Kind::parentProperty, .property = Identifier (name.substring (7)) });
            continue;
        }

        if (name.startsWith ("self."))
        {
            cached->inputs.push_back ({ .index = i, .kind = ParsedInput::Kind::selfProperty, .property = Identifier (name.substring (5)) });
            continue;
        }

        const auto dot = name.indexOfChar ('.');

        if (dot > 0)
        {
            cached->inputs.push_back ({ .index = i,
                                        .kind = ParsedInput::Kind::prefixed,
                                        .prefix = name.substring (0, dot),
                                        .property = Identifier (name.substring (dot + 1)) });
            continue;
        }

        cached->inputs.push_back ({ .index = i, .kind = ParsedInput::Kind::bareProperty, .property = Identifier (name) });
    }

    auto& result = *cached;
    cachedPrograms.push_back (std::move (cached));
    return result;
}

void DrawingScope::updateInputs (const CompiledExpression& program, ExpressionState& state) const
{
    auto* self = component.get();
    if (self == nullptr)
        return;

    const auto& cached = getCachedProgram (program);
    if (cached.inputs.empty())
        return;

    const ThemePalette* palette = nullptr;

    if (cached.usesPalette)
    {
        static const ThemePalette defaultPalette;

        const auto theme = ApplicationTheme::getGlobalTheme();
        palette = theme != nullptr ? &theme->getPalette (*self) : self->findPalette();

        if (palette == nullptr)
            palette = &defaultPalette;
    }

    for (const auto& input : cached.inputs)
    {
        if (input.kind == ParsedInput::Kind::paletteDark)
        {
            state.set (ExpressionInput<bool> { input.index }, palette->getMode() == ThemePalette::Mode::dark);
            continue;
        }

        if (input.kind == ParsedInput::Kind::paletteRole)
        {
            state.set (ExpressionInput<Color> { input.index }, palette->getColor (input.paletteRole));
            continue;
        }

        Component* targetComponent = nullptr;
        const ComponentProperty* descriptor = nullptr;
        BindingProvider* targetProvider = nullptr;

        if (input.kind == ParsedInput::Kind::selfProperty)
        {
            targetComponent = self;
            descriptor = self->getTypeInfo().findProperty (input.property);
        }
        else if (input.kind == ParsedInput::Kind::parentProperty)
        {
            if (auto* parent = self->getParentComponent())
            {
                targetComponent = parent;
                descriptor = parent->getTypeInfo().findProperty (input.property);
            }
        }
        else if (input.kind == ParsedInput::Kind::bareProperty)
        {
            if (auto* local = locals.get(); local != nullptr && findProviderProperty (*local, input.property).has_value())
            {
                targetProvider = local;
            }
            else if (const auto* own = self->getTypeInfo().findProperty (input.property))
            {
                targetComponent = self;
                descriptor = own;
            }
            else
            {
                for (auto* ancestor = self->getParentComponent(); ancestor != nullptr; ancestor = ancestor->getParentComponent())
                {
                    const auto* candidate = ancestor->getTypeInfo().findProperty (input.property);

                    if (candidate != nullptr && candidate->isStoreBacked() && candidate->isInherited)
                    {
                        targetComponent = ancestor;
                        descriptor = candidate;
                        break;
                    }
                }
            }
        }
        else if (input.kind == ParsedInput::Kind::prefixed)
        {
            targetComponent = input.cachedComponent.get();
            targetProvider = input.cachedProvider.get();

            if (targetComponent == nullptr && targetProvider == nullptr)
            {
                if (componentResolver != nullptr)
                    targetComponent = componentResolver (input.prefix);

                if (targetComponent == nullptr && modelResolver != nullptr)
                    targetProvider = modelResolver (input.prefix);

                input.cachedComponent = targetComponent;
                input.cachedProvider = targetProvider;
            }

            if (targetComponent != nullptr)
                descriptor = targetComponent->getTypeInfo().findProperty (input.property);
        }

        ComponentProperty::Type type = ComponentProperty::Type::any;
        ComponentProperty::Type elementType = ComponentProperty::Type::any;
        var value;

        if (targetComponent != nullptr)
        {
            if (descriptor == nullptr)
                continue;

            type = descriptor->type;
            elementType = descriptor->elementType;
            value = targetComponent->findProperty (input.property);
        }
        else if (targetProvider != nullptr)
        {
            const auto property = findProviderProperty (*targetProvider, input.property);
            if (! property.has_value())
                continue;

            type = property->type;
            elementType = property->elementType;
            value = targetProvider->getValue (input.property);
        }
        else
        {
            continue;
        }

        setInputValue (state, input.index, type, elementType, value, input.arrayInput);
    }
}

void DrawingScope::setInputValue (ExpressionState& state,
                                  int input,
                                  ComponentProperty::Type type,
                                  ComponentProperty::Type elementType,
                                  const var& value,
                                  ArrayInput& arrayInput)
{
    if (type == ComponentProperty::Type::array)
        arrayInput.bind (state, input, elementType, value);
    else
        withExpressionType (type, [&] (auto tag)
        {
            using Type = typename decltype (tag)::type;

            state.set (ExpressionInput<Type> { input }, VariantConverter<Type>::fromVar (value));
        });
}

DrawingScope::ResolvedTarget DrawingScope::resolveTarget (const String& symbol) const
{
    auto* self = component.get();
    if (self == nullptr)
        return {};

    if (symbol.startsWith ("parent."))
        return { self->getParentComponent(), nullptr, Identifier (symbol.substring (7)) };

    if (symbol.startsWith ("self."))
        return { self, nullptr, Identifier (symbol.substring (5)) };

    const auto dot = symbol.indexOfChar ('.');

    if (dot > 0)
    {
        const auto prefix = symbol.substring (0, dot);
        const Identifier propertyId (symbol.substring (dot + 1));

        if (componentResolver != nullptr)
            if (auto* resolved = componentResolver (prefix))
                return { resolved, nullptr, propertyId };

        if (modelResolver != nullptr)
            if (auto* provider = modelResolver (prefix))
                return { nullptr, provider, propertyId };

        return {};
    }

    // A bare name the component doesn't declare reads the closest ancestor declaring it as inherited, like a token
    const Identifier propertyId (symbol);

    if (auto* local = locals.get(); local != nullptr && findProviderProperty (*local, propertyId).has_value())
        return { nullptr, local, propertyId };

    if (self->getTypeInfo().findProperty (propertyId) != nullptr)
        return { self, nullptr, propertyId };

    for (auto* ancestor = self->getParentComponent(); ancestor != nullptr; ancestor = ancestor->getParentComponent())
    {
        const auto* property = ancestor->getTypeInfo().findProperty (propertyId);

        if (property != nullptr && property->isStoreBacked() && property->isInherited)
            return { ancestor, nullptr, propertyId };
    }

    return { self, nullptr, propertyId };
}

std::vector<DrawingScope::Dependency> DrawingScope::getDependencies (const CompiledExpression& program) const
{
    std::vector<Dependency> dependencies;
    bool addedPaletteDependency = false;

    for (int i = 0; i < program.getNumInputs(); ++i)
    {
        const auto& name = program.getInputName (i);

        if (name.startsWith ("palette."))
        {
            if (auto* self = component.get(); self != nullptr && ! addedPaletteDependency)
            {
                static const Identifier paletteId ("palette");
                dependencies.push_back ({ self, nullptr, paletteId });
                addedPaletteDependency = true;
            }

            continue;
        }

        const auto target = resolveTarget (name);

        if (target.component != nullptr)
        {
            if (target.component->getTypeInfo().findProperty (target.property) == nullptr)
                continue;
        }
        else if (target.provider != nullptr)
        {
            if (! findProviderProperty (*target.provider, target.property).has_value())
                continue;
        }
        else
        {
            continue;
        }

        const auto isKnown = std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
        {
            return dependency.component == target.component
                && dependency.provider == target.provider
                && dependency.property == target.property;
        });

        if (! isKnown)
            dependencies.push_back ({ target.component, target.provider, target.property });
    }

    return dependencies;
}

const ExpressionType* DrawingScope::getExpressionType (ComponentProperty::Type type, ComponentProperty::Type elementType) const
{
    const ExpressionType* result = nullptr;

    if (type == ComponentProperty::Type::array)
        withExpressionType (elementType, [&] (auto tag)
        {
            result = context.addArrayType<typename decltype (tag)::type>();
        });
    else
        withExpressionType (type, [&] (auto tag)
        {
            result = context.getType<typename decltype (tag)::type>();
        });

    return result;
}

void DrawingScope::addExpressionArrayTypes (ExpressionContext& context)
{
    for (const auto type : ComponentProperty::expressionTypes)
    {
        withExpressionType (type, [&] (auto tag)
        {
            context.addArrayType<typename decltype (tag)::type>();
        });
    }
}

void DrawingScope::ArrayInput::bind (ExpressionState& state, int input, ComponentProperty::Type elementType, const var& value)
{
    withExpressionType (elementType, [&] (auto tag)
    {
        using Type = typename decltype (tag)::type;

        if (! std::holds_alternative<Array<Type>> (values))
            values = Array<Type>();

        auto& elements = std::get<Array<Type>> (values);
        elements.clearQuick();

        if (const auto* source = value.getArray())
        {
            elements.ensureStorageAllocated (source->size());

            for (const auto& element : *source)
                elements.add (VariantConverter<Type>::fromVar (element));
        }

        state.bindArray (ExpressionArray<Type> { input }, elements.data(), elements.size());
    });
}

const ExpressionType* DrawingScope::typeForPaletteRole (const String& roleName) const
{
    if (roleName == "dark")
        return context.getType<bool>();

    return ThemePalette::getRoleFromName (roleName).has_value() ? context.getType<Color>() : nullptr;
}

} // namespace yup
