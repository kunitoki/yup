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

ComponentProperty::Type inferThemePropertyType (const var& value) noexcept
{
    if (value.isBool())
        return ComponentProperty::Type::boolean;

    if (value.isInt())
        return ComponentProperty::Type::integer;

    if (value.isDouble())
        return ComponentProperty::Type::number;

    if (value.isString())
        return ComponentProperty::Type::string;

    if (value.isInt64())
        return ComponentProperty::Type::color;

    if (const auto* array = value.getArray())
    {
        if (array->size() == 2 && ((*array)[0].isVoid() || (*array)[0].isObject()))
            return ComponentProperty::Type::font;

        if (array->size() == 2)
            return ComponentProperty::Type::point;

        if (array->size() == 4)
            return ComponentProperty::Type::rectangle;

        if (array->size() == 6)
            return ComponentProperty::Type::transform;
    }

    return ComponentProperty::Type::any;
}

} // namespace

DrawingScope::DrawingScope (Component& componentToUse, ExpressionContext& contextToUse)
    : component (&componentToUse)
    , context (contextToUse)
{
}

void DrawingScope::setComponentResolver (ComponentResolver resolver)
{
    componentResolver = std::move (resolver);
    ++resolverGeneration;
}

void DrawingScope::setLocals (Component* newLocals)
{
    locals = newLocals;
}

void DrawingScope::install()
{
    context.setSymbolResolver ([this] (const String& name)
    {
        return resolveSymbol (name);
    });
}

DrawingScope::ParsedInput DrawingScope::parse (const String& symbol, int index)
{
    if (symbol.startsWith ("_property_") || symbol.startsWith ("_hasProperty_"))
        return { .index = index, .kind = ParsedInput::Kind::invalid };

    if (symbol.startsWith ("palette."))
    {
        const auto roleName = symbol.substring (8);

        if (roleName == "dark")
            return { .index = index, .kind = ParsedInput::Kind::paletteDark };

        if (const auto role = ThemePalette::getRoleFromName (roleName))
            return { .index = index, .kind = ParsedInput::Kind::paletteRole, .paletteRole = *role };

        if (ThemePalette::isValidCustomColorName (roleName))
            return { .index = index, .kind = ParsedInput::Kind::paletteColor, .property = Identifier (roleName) };

        return { .index = index, .kind = ParsedInput::Kind::invalid };
    }

    if (symbol.startsWith ("theme."))
    {
        const auto memberName = symbol.substring (6);
        if (memberName.isEmpty())
            return { .index = index, .kind = ParsedInput::Kind::invalid };

        if (memberName == "font" || memberName == "defaultFont")
            return { .index = index, .kind = ParsedInput::Kind::themeDefaultFont };

        if (memberName == "iconFont")
            return { .index = index, .kind = ParsedInput::Kind::themeIconFont };

        if (memberName == "monospaceFont")
            return { .index = index, .kind = ParsedInput::Kind::themeMonospaceFont };

        if (memberName == "dark")
            return { .index = index, .kind = ParsedInput::Kind::themeDark };

        if (const auto role = ThemePalette::getRoleFromName (memberName))
            return { .index = index, .kind = ParsedInput::Kind::themePaletteRole, .paletteRole = *role };

        return { .index = index, .kind = ParsedInput::Kind::themeMember, .property = Identifier (memberName) };
    }

    if (symbol.isEmpty())
        return { .index = index, .kind = ParsedInput::Kind::invalid };

    if (symbol.startsWith ("parent."))
    {
        const auto rest = symbol.substring (7);
        return rest.isNotEmpty()
                 ? ParsedInput { .index = index, .kind = ParsedInput::Kind::parentProperty, .property = Identifier (rest) }
                 : ParsedInput { .index = index, .kind = ParsedInput::Kind::invalid };
    }

    if (symbol.startsWith ("self."))
    {
        const auto rest = symbol.substring (5);
        return rest.isNotEmpty()
                 ? ParsedInput { .index = index, .kind = ParsedInput::Kind::selfProperty, .property = Identifier (rest) }
                 : ParsedInput { .index = index, .kind = ParsedInput::Kind::invalid };
    }

    if (const auto dot = symbol.indexOfChar ('.'); dot >= 0)
    {
        if (dot == 0 || dot + 1 >= symbol.length())
            return { .index = index, .kind = ParsedInput::Kind::invalid };

        return {
            .index = index,
            .kind = ParsedInput::Kind::prefixed,
            .prefix = symbol.substring (0, dot),
            .property = Identifier (symbol.substring (dot + 1))
        };
    }

    return { .index = index, .kind = ParsedInput::Kind::bareProperty, .property = Identifier (symbol) };
}

DrawingScope::ResolvedProperty DrawingScope::resolve (const ParsedInput& input) const
{
    auto* self = component.get();
    if (self == nullptr)
        return {};

    switch (input.kind)
    {
        case ParsedInput::Kind::selfProperty:
        {
            const auto* descriptor = self->getTypeInfo().findProperty (input.property);
            return descriptor != nullptr ? ResolvedProperty { self, descriptor } : ResolvedProperty {};
        }

        case ParsedInput::Kind::parentProperty:
        {
            auto* parent = self->getParentComponent();
            const auto* descriptor = parent != nullptr ? parent->getTypeInfo().findProperty (input.property) : nullptr;
            return descriptor != nullptr ? ResolvedProperty { parent, descriptor } : ResolvedProperty {};
        }

        case ParsedInput::Kind::bareProperty:
        {
            if (auto* local = locals.get())
                if (const auto* descriptor = local->getTypeInfo().findProperty (input.property))
                    return { local, descriptor };

            if (const auto* own = self->getTypeInfo().findProperty (input.property))
                return { self, own };

            for (auto* ancestor = self->getParentComponent(); ancestor != nullptr; ancestor = ancestor->getParentComponent())
            {
                const auto* candidate = ancestor->getTypeInfo().findProperty (input.property);

                if (candidate != nullptr && candidate->isStoreBacked() && candidate->isInherited)
                    return { ancestor, candidate };
            }

            return {};
        }

        case ParsedInput::Kind::prefixed:
        {
            auto* target = input.cachedResolverGeneration == resolverGeneration
                             ? input.cachedComponent.get()
                             : nullptr;

            if (target == nullptr)
            {
                if (componentResolver != nullptr)
                    target = componentResolver (input.prefix);

                input.cachedComponent = target;
                input.cachedResolverGeneration = resolverGeneration;
            }

            if (target == nullptr)
                return {};

            const auto* descriptor = target->getTypeInfo().findProperty (input.property);
            return descriptor != nullptr ? ResolvedProperty { target, descriptor } : ResolvedProperty {};
        }

        case ParsedInput::Kind::invalid:
        case ParsedInput::Kind::paletteDark:
        case ParsedInput::Kind::paletteRole:
        case ParsedInput::Kind::paletteColor:
        case ParsedInput::Kind::themeDefaultFont:
        case ParsedInput::Kind::themeIconFont:
        case ParsedInput::Kind::themeMonospaceFont:
        case ParsedInput::Kind::themeDark:
        case ParsedInput::Kind::themePaletteRole:
        case ParsedInput::Kind::themeMember:
            return {};
    }

    return {};
}

const ExpressionType* DrawingScope::resolveSymbol (const String& symbol) const
{
    const auto input = parse (symbol);

    switch (input.kind)
    {
        case ParsedInput::Kind::paletteDark:
        case ParsedInput::Kind::themeDark:
            return context.getType<bool>();

        case ParsedInput::Kind::paletteRole:
        case ParsedInput::Kind::paletteColor:
        case ParsedInput::Kind::themePaletteRole:
            return context.getType<Color>();

        case ParsedInput::Kind::themeDefaultFont:
        case ParsedInput::Kind::themeIconFont:
        case ParsedInput::Kind::themeMonospaceFont:
            return context.getType<Font>();

        case ParsedInput::Kind::themeMember:
        {
            const auto theme = ApplicationTheme::getGlobalTheme();
            auto* self = component.get();
            if (theme == nullptr || self == nullptr)
                return nullptr;

            if (theme->findDefaultColor (*self, input.property).has_value())
                return context.getType<Color>();

            if (const auto prop = theme->findProperty (*self, input.property); prop.has_value())
                return getExpressionType (inferThemePropertyType (*prop), ComponentProperty::Type::any);

            return nullptr;
        }

        default:
            break;
    }

    const auto resolved = resolve (input);
    return resolved.descriptor != nullptr
             ? getExpressionType (resolved.descriptor->type, resolved.descriptor->elementType)
             : nullptr;
}

DrawingScope::BoundInputs DrawingScope::bindInputs (const CompiledExpression& program) const
{
    BoundInputs bound;
    const auto theme = ApplicationTheme::getGlobalTheme();
    auto* self = component.get();

    for (int i = context.getNumInputs(); i < program.getNumInputs(); ++i)
    {
        if (! program.usesInput (i))
            continue;

        auto input = parse (program.getInputName (i), i);
        if (input.kind == ParsedInput::Kind::invalid)
            continue;

        if (input.isPaletteOrTheme())
        {
            bound.paletteDependent = true;

            if (input.kind == ParsedInput::Kind::themeMember && theme != nullptr && self != nullptr)
            {
                if (theme->findDefaultColor (*self, input.property).has_value())
                {
                    input.isThemeColor = true;
                    input.themeType = ComponentProperty::Type::color;
                }
                else if (const auto prop = theme->findProperty (*self, input.property); prop.has_value())
                {
                    input.themeType = inferThemePropertyType (*prop);
                }
            }
        }

        bound.inputs.push_back (std::move (input));
    }

    return bound;
}

void DrawingScope::updateInputs (BoundInputs& bound, ExpressionState& state) const
{
    auto* self = component.get();
    if (self == nullptr || bound.inputs.empty())
        return;

    static const ThemePalette defaultPalette;
    const auto theme = bound.paletteDependent ? ApplicationTheme::getGlobalTheme() : nullptr;
    const ThemePalette* palette = nullptr;

    if (bound.paletteDependent)
    {
        palette = theme != nullptr ? &theme->getPalette (*self) : self->findPalette();

        if (palette == nullptr)
            palette = &defaultPalette;
    }

    const ThemePalette& globalPalette = theme != nullptr ? theme->getPalette() : defaultPalette;

    for (auto& input : bound.inputs)
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

        if (input.kind == ParsedInput::Kind::paletteColor)
        {
            state.set (ExpressionInput<Color> { input.index }, palette->findCustomColor (input.property.toString()).value_or (Color (0x00000000)));
            continue;
        }

        if (input.kind == ParsedInput::Kind::themeDefaultFont)
        {
            state.set (ExpressionInput<Font> { input.index }, theme != nullptr ? theme->getDefaultFont() : Font());
            continue;
        }

        if (input.kind == ParsedInput::Kind::themeIconFont)
        {
            state.set (ExpressionInput<Font> { input.index }, theme != nullptr ? theme->getDefaultIconFont() : Font());
            continue;
        }

        if (input.kind == ParsedInput::Kind::themeMonospaceFont)
        {
            state.set (ExpressionInput<Font> { input.index }, theme != nullptr ? theme->getDefaultMonospaceFont() : Font());
            continue;
        }

        if (input.kind == ParsedInput::Kind::themeDark)
        {
            state.set (ExpressionInput<bool> { input.index }, globalPalette.getMode() == ThemePalette::Mode::dark);
            continue;
        }

        if (input.kind == ParsedInput::Kind::themePaletteRole)
        {
            state.set (ExpressionInput<Color> { input.index }, globalPalette.getColor (input.paletteRole));
            continue;
        }

        if (input.kind == ParsedInput::Kind::themeMember)
        {
            if (theme == nullptr)
                continue;

            if (input.isThemeColor)
            {
                state.set (ExpressionInput<Color> { input.index }, theme->findDefaultColor (*self, input.property).value_or (Color()));
            }
            else if (input.themeType != ComponentProperty::Type::any)
            {
                const auto prop = theme->findProperty (*self, input.property).value_or (var());
                setInputValue (state, input.index, input.themeType, ComponentProperty::Type::any, prop, input.arrayInput);
            }

            continue;
        }

        const auto resolved = resolve (input);
        if (resolved.descriptor == nullptr)
            continue;

        setInputValue (state,
                       input.index,
                       resolved.descriptor->type,
                       resolved.descriptor->elementType,
                       resolved.component->findProperty (input.property),
                       input.arrayInput);
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
    else if (type == ComponentProperty::Type::drawing || type == ComponentProperty::Type::enumeration)
        state.set (ExpressionInput<String> { input }, value.toString());
    else
        withExpressionType (type, [&] (auto tag)
        {
            using Type = typename decltype (tag)::type;

            state.set (ExpressionInput<Type> { input }, VariantConverter<Type>::fromVar (value));
        });
}

std::vector<DrawingScope::Dependency> DrawingScope::getDependencies (const CompiledExpression& program) const
{
    std::vector<Dependency> dependencies;

    for (int i = context.getNumInputs(); i < program.getNumInputs(); ++i)
    {
        if (! program.usesInput (i))
            continue;

        const auto input = parse (program.getInputName (i), i);

        if (input.isPaletteOrTheme())
            continue;

        const auto resolved = resolve (input);
        if (resolved.component == nullptr)
            continue;

        const auto isKnown = std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
        {
            return dependency.component == resolved.component && dependency.property == input.property;
        });

        if (! isKnown)
            dependencies.push_back ({ resolved.component, input.property });
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
    else if (type == ComponentProperty::Type::drawing || type == ComponentProperty::Type::enumeration)
        result = context.getType<String>();
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

} // namespace yup
