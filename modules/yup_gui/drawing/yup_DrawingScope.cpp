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

void DrawingScope::install()
{
    context.setSymbolResolver ([this] (const String& name) -> const ExpressionType*
    {
        if (name.startsWith ("palette."))
            return typeForPaletteRole (name.substring (8));

        String propertyName;

        if (auto* target = resolveTarget (name, propertyName))
        {
            if (const auto* property = target->getTypeInfo().findProperty (Identifier (propertyName)))
                return typeForProperty (property->type);
        }

        return nullptr;
    });
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

        String propertyName;

        auto* target = resolveTarget (name, propertyName);
        if (target == nullptr)
            continue;

        const Identifier propertyId (propertyName);

        const auto* property = target->getTypeInfo().findProperty (propertyId);
        if (property == nullptr)
            continue;

        const auto value = target->findProperty (propertyId);

        if (property->type == ComponentProperty::Type::color)
            state.set (ExpressionInput<Color> { i }, VariantConverter<Color>::fromVar (value));
        else
            state.setVar (i, value);
    }
}

Component* DrawingScope::resolveTarget (const String& symbol, String& propertyName) const
{
    auto* self = component.get();
    if (self == nullptr)
        return nullptr;

    if (symbol.startsWith ("parent."))
    {
        propertyName = symbol.substring (7);
        return self->getParentComponent();
    }

    if (symbol.startsWith ("self."))
    {
        propertyName = symbol.substring (5);
        return self;
    }

    propertyName = symbol;
    return self;
}

const ExpressionType* DrawingScope::typeForProperty (ComponentProperty::Type type) const
{
    switch (type)
    {
        case ComponentProperty::Type::number:  return context.getType<float>();
        case ComponentProperty::Type::integer: return context.getType<int>();
        case ComponentProperty::Type::boolean: return context.getType<bool>();
        case ComponentProperty::Type::string:  return context.getType<String>();
        case ComponentProperty::Type::color:   return context.getType<Color>();

        default:                               return nullptr;
    }
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
