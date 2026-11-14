/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

//==============================================================================

ApplicationTheme::ApplicationTheme() = default;

ApplicationTheme::~ApplicationTheme() = default;

//==============================================================================

void ApplicationTheme::setGlobalTheme (ApplicationTheme::Ptr s)
{
    getGlobalThemeInstance() = std::move (s);
}

ApplicationTheme::Ptr ApplicationTheme::getGlobalTheme()
{
    return getGlobalThemeInstance();
}

ApplicationTheme::Ptr& ApplicationTheme::getGlobalThemeInstance()
{
    static ApplicationTheme::Ptr globalTheme;
    return globalTheme;
}

//==============================================================================

std::optional<Color> ApplicationTheme::findComponentColor (const Component& component, const Identifier& colorId)
{
    jassert (getGlobalThemeInstance() != nullptr);

    return getGlobalThemeInstance()->findColor (component, colorId);
}

std::optional<Color> ApplicationTheme::findColor (const Component& component, const Identifier& colorId) const
{
    if (auto color = component.findColor (colorId))
        return color;

    if (auto it = defaultColors.find (colorId); it != defaultColors.end())
        return it->second;

    if (const auto* componentPalette = component.findPalette())
        return findPaletteColor (*componentPalette, colorId);

    if (auto it = paletteColors.find (colorId); it != paletteColors.end())
        return it->second;

    return std::nullopt;
}

std::optional<Color> ApplicationTheme::findPaletteColor (const ThemePalette& componentPalette, const Identifier& colorId) const
{
    const SpinLock::ScopedLockType lock (componentPaletteColorsLock);

    auto entry = std::find_if (componentPaletteColors.begin(), componentPaletteColors.end(), [&] (const auto& candidate)
    {
        return candidate.first == componentPalette;
    });

    // Mapped once per distinct palette, by running the palette mapping on a theme of its own.
    if (entry == componentPaletteColors.end())
    {
        ApplicationTheme mapped;
        mapped.paletteMapping = paletteMapping;
        mapped.setPalette (componentPalette);
        entry = componentPaletteColors.emplace (componentPaletteColors.end(), componentPalette, std::move (mapped.paletteColors));
    }

    if (auto it = entry->second.find (colorId); it != entry->second.end())
        return it->second;

    return std::nullopt;
}

void ApplicationTheme::setColor (const Identifier& colorId, const Color& color)
{
    defaultColors.insert_or_assign (colorId, color);
}

void ApplicationTheme::setColors (std::initializer_list<std::pair<const Identifier&, const Color&>> colors)
{
    for (const auto& entry : colors)
        defaultColors.insert_or_assign (entry.first, entry.second);
}

//==============================================================================

void ApplicationTheme::setPalette (const ThemePalette& newPalette)
{
    palette = newPalette;
    paletteColors.clear();
    ++paletteGeneration;

    if (paletteMapping)
        paletteMapping (palette, *this);
}

const ThemePalette& ApplicationTheme::getPalette() const
{
    return palette;
}

const ThemePalette& ApplicationTheme::getPalette (const Component& component) const
{
    if (const auto* componentPalette = component.findPalette())
        return *componentPalette;

    return palette;
}

void ApplicationTheme::setPaletteMapping (PaletteMapping mapping)
{
    paletteMapping = std::move (mapping);
    setPalette (palette);

    const SpinLock::ScopedLockType lock (componentPaletteColorsLock);
    componentPaletteColors.clear();
}

void ApplicationTheme::setPaletteColor (const Identifier& colorId, const Color& color)
{
    paletteColors.insert_or_assign (colorId, color);
}

//==============================================================================

std::optional<var> ApplicationTheme::findComponentProperty (const Component& component, const Identifier& id)
{
    jassert (getGlobalThemeInstance() != nullptr);

    return getGlobalThemeInstance()->findProperty (component, id);
}

std::optional<var> ApplicationTheme::findProperty (const Component& component, const Identifier& id) const
{
    ignoreUnused (component);

    if (auto it = defaultProperties.find (id); it != defaultProperties.end())
        return it->second;

    return std::nullopt;
}

void ApplicationTheme::setProperty (const Identifier& id, const var& value)
{
    defaultProperties.insert_or_assign (id, value);
}

void ApplicationTheme::setProperties (std::initializer_list<std::pair<const Identifier&, var>> properties)
{
    for (const auto& entry : properties)
        defaultProperties.insert_or_assign (entry.first, entry.second);
}

//==============================================================================

void ApplicationTheme::setDefaultFont (Font font)
{
    defaultFont = std::move (font);
}

const Font& ApplicationTheme::getDefaultFont() const
{
    return defaultFont;
}

void ApplicationTheme::setDefaultIconFont (Font font)
{
    defaultIconFont = std::move (font);
}

const Font& ApplicationTheme::getDefaultIconFont() const
{
    return defaultIconFont;
}

void ApplicationTheme::setDefaultMonospaceFont (Font font)
{
    defaultMonospaceFont = std::move (font);
}

const Font& ApplicationTheme::getDefaultMonospaceFont() const
{
    return defaultMonospaceFont;
}

} // namespace yup
