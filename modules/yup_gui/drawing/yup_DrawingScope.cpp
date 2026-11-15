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
}

void DrawingScope::setModelResolver (ModelResolver resolver)
{
    modelResolver = std::move (resolver);
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
    for (const auto& property : provider.getProperties())
        if (property.name == name)
            return property;

    return std::nullopt;
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

void DrawingScope::updateInputs (const CompiledExpression& program, ExpressionState& state) const
{
    for (int i = 0; i < program.getNumInputs(); ++i)
    {
        const auto& name = program.getInputName (i);

        if (name.startsWith ("palette."))
        {
            if (const auto* self = component.get())
                updatePaletteInput (*self, name.substring (8), i, state);

            continue;
        }

        const auto target = resolveTarget (name);

        ComponentProperty::Type type = ComponentProperty::Type::any;
        ComponentProperty::Type elementType = ComponentProperty::Type::any;
        var value;

        if (target.component != nullptr)
        {
            const auto* property = target.component->getTypeInfo().findProperty (target.property);
            if (property == nullptr)
                continue;

            type = property->type;
            elementType = property->elementType;
            value = target.component->findProperty (target.property);
        }
        else if (target.provider != nullptr)
        {
            const auto property = findProviderProperty (*target.provider, target.property);
            if (! property.has_value())
                continue;

            type = property->type;
            elementType = property->elementType;
            value = target.provider->getValue (target.property);
        }
        else
        {
            continue;
        }

        if (type == ComponentProperty::Type::array)
            arrayInputs[name].bind (state, i, elementType, value);
        else
            withExpressionType (type, [&] (auto tag)
            {
                using Type = typename decltype (tag)::type;

                state.set (ExpressionInput<Type> { i }, VariantConverter<Type>::fromVar (value));
            });
    }
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

    for (int i = 0; i < program.getNumInputs(); ++i)
    {
        const auto& name = program.getInputName (i);

        if (name.startsWith ("palette."))
            continue;

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
    for (const auto type : { ComponentProperty::Type::number,
                             ComponentProperty::Type::integer,
                             ComponentProperty::Type::boolean,
                             ComponentProperty::Type::string,
                             ComponentProperty::Type::color,
                             ComponentProperty::Type::point,
                             ComponentProperty::Type::rectangle })
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

void DrawingScope::updatePaletteInput (const Component& target, const String& roleName, int index, ExpressionState& state)
{
    static const ThemePalette defaultPalette;

    const auto theme = ApplicationTheme::getGlobalTheme();
    const auto* palette = theme != nullptr ? &theme->getPalette (target) : target.findPalette();

    if (palette == nullptr)
        palette = &defaultPalette;

    if (roleName == "dark")
        state.set (ExpressionInput<bool> { index }, palette->getMode() == ThemePalette::Mode::dark);
    else if (const auto role = ThemePalette::getRoleFromName (roleName))
        state.set (ExpressionInput<Color> { index }, palette->getColor (*role));
}

} // namespace yup
