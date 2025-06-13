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
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    getGlobalThemeInstance() = std::move (s);
}

ApplicationTheme::ConstPtr ApplicationTheme::getGlobalTheme()
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    return getGlobalThemeInstance();
}

ApplicationTheme::Ptr& ApplicationTheme::getGlobalThemeInstance()
{
    static ApplicationTheme::Ptr globalTheme;
    return globalTheme;
}

//==============================================================================

Color ApplicationTheme::findColor (const Identifier& colorId)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    auto it = getGlobalThemeInstance()->defaultColors.find (colorId);
    return it != getGlobalThemeInstance()->defaultColors.end() ? it->second : Color();
}

void ApplicationTheme::setColor (const Identifier& colorId, const Color& color)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    defaultColors.insert_or_assign (colorId, color);
}

void ApplicationTheme::setColors (std::initializer_list<std::pair<const Identifier&, const Color&>> colors)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    for (const auto& entry : colors)
        defaultColors.insert_or_assign (entry.first, entry.second);
}

//==============================================================================

void ApplicationTheme::setDefaultFont (Font font)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    defaultFont = std::move (font);
}

const Font& ApplicationTheme::getDefaultFont() const
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    return defaultFont;
}

} // namespace yup
