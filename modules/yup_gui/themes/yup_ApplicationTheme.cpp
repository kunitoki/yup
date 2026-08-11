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

std::optional<float> ApplicationTheme::findComponentMetric (const Component& component, const Identifier& metricId)
{
    jassert (getGlobalThemeInstance() != nullptr);

    return getGlobalThemeInstance()->findMetric (component, metricId);
}

std::optional<float> ApplicationTheme::findMetric (const Component& component, const Identifier& metricId) const
{
    if (auto metric = component.findMetric (metricId))
        return metric;

    if (auto it = defaultMetrics.find (metricId); it != defaultMetrics.end())
        return it->second;

    return std::nullopt;
}

void ApplicationTheme::setMetric (const Identifier& metricId, float value)
{
    defaultMetrics.insert_or_assign (metricId, value);
}

void ApplicationTheme::setMetrics (std::initializer_list<std::pair<const Identifier&, float>> metrics)
{
    for (const auto& entry : metrics)
        defaultMetrics.insert_or_assign (entry.first, entry.second);
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
