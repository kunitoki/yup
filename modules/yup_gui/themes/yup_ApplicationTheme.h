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
/**
    Manages the application-wide theme settings.

    The ApplicationTheme class provides methods to set and retrieve component-specific
    themes and the default font. It is reference-counted for safe usage across multiple
    parts of the application.

    @tags{UI}
*/
class JUCE_API ApplicationTheme final : public ReferenceCountedObject
{
public:
    /** Typedef for a reference-counted pointer to an ApplicationTheme object. */
    using Ptr = ReferenceCountedObjectPtr<ApplicationTheme>;

    /** Typedef for a reference-counted pointer to a const ApplicationTheme object. */
    using ConstPtr = ReferenceCountedObjectPtr<const ApplicationTheme>;

    //==============================================================================
    /** Constructs an ApplicationTheme object. */
    ApplicationTheme();

    //==============================================================================
    /**
        Sets the global application theme.

        This method sets a global instance of the ApplicationTheme, which can be retrieved
        and used throughout the application.

        @param s  The ApplicationTheme object to set as the global theme.
    */
    static void setGlobalTheme (ApplicationTheme::Ptr s);

    /**
        Returns the global application theme.

        This method retrieves the global instance of the ApplicationTheme.

        @returns A reference-counted pointer to the global ApplicationTheme.
    */
    static ApplicationTheme::ConstPtr getGlobalTheme();

    //==============================================================================
    /**
        Resolves the style for a specific component type.

        This template method returns the style for the specified component type. If a style is provided as an argument,
        it will be returned; otherwise, the global style will be used.

        @param instanceStyle  An optional style to use instead of the global style.

        @returns The resolved style for the component type.
    */
    template <class T>
    static const T& findComponentStyle (const T* instanceStyle = nullptr)
    {
        return instanceStyle != nullptr
                 ? *instanceStyle
                 : std::get<T> (getGlobalThemeInstance()->componentStyles);
    }

    //==============================================================================
    /** */
    static Color findColor (const Identifier& colorId);

    /** */
    void setColor (const Identifier& colorId, const Color& color);

    //==============================================================================
    /**
        Sets the default font for the application theme.

        @param font  The font to set as the default.
    */
    void setDefaultFont (Font font);

    /**
        Returns the default font for the application theme.

        @returns  The default font.
    */
    const Font& getDefaultFont() const;

    //==============================================================================
    /**
        Sets the style for a specific component type.

        This template method allows setting the style for a specific type of component. The component type must be
        part of the `componentStyles` tuple.

        @param instanceStyle  The style to set for the component type.
    */
    template <class T>
    void setComponentStyle (T&& instanceStyle)
    {
        std::get<T> (componentStyles) = std::forward<T> (instanceStyle);
    }

private:
    static ApplicationTheme::Ptr& getGlobalThemeInstance();

    //==============================================================================
    std::tuple<
        Slider::Style,
        TextButton::Style> componentStyles;

    Font defaultFont;
    std::unordered_map<Identifier, Color> defaultColors;

    JUCE_LEAK_DETECTOR (ApplicationTheme)
};

} // namespace yup
