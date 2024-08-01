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

class JUCE_API ApplicationTheme final : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ApplicationTheme>;
    using ConstPtr = ReferenceCountedObjectPtr<const ApplicationTheme>;

    //==============================================================================
    ApplicationTheme();

    //==============================================================================
    template <class T>
    void setComponentTheme (T&& instanceTheme)
    {
        std::get<T>(componentThemes) = std::forward<T>(instanceTheme);
    }

    //==============================================================================
    template <class T>
    static const T& resolveTheme (const T* instanceTheme = nullptr)
    {
        return instanceTheme != nullptr
            ? *instanceTheme
            : std::get<T>(getGlobalThemeInstance()->componentThemes);
    }

    //==============================================================================
    void setDefaultFont (Font font);
    const Font& getDefaultFont() const;

    //==============================================================================
    static void setGlobalTheme (ApplicationTheme::Ptr s);
    static ApplicationTheme::ConstPtr getGlobalTheme();

private:
    static ApplicationTheme::Ptr& getGlobalThemeInstance();

    std::tuple<
        Slider::Theme
    > componentThemes;

    Font defaultFont;

    JUCE_LEAK_DETECTOR (ApplicationTheme)
};

} // namespace yup
