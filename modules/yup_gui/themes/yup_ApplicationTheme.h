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
class YUP_API ApplicationTheme final : public ReferenceCountedObject
{
public:
    /** Typedef for a reference-counted pointer to an ApplicationTheme object. */
    using Ptr = ReferenceCountedObjectPtr<ApplicationTheme>;

    /** Typedef for a reference-counted pointer to a const ApplicationTheme object. */
    using ConstPtr = ReferenceCountedObjectPtr<const ApplicationTheme>;

    //==============================================================================
    /** Constructs an ApplicationTheme object. */
    ApplicationTheme();

    /** Destructor for the ApplicationTheme object. */
    ~ApplicationTheme();

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
    static ApplicationTheme::Ptr getGlobalTheme();

    //==============================================================================
    /**
        Resolves the style for a specific component type.

        This template method returns the style for the specified component type. If a style is provided as an argument,
        it will be returned; otherwise, the global style will be used.

        @param instanceStyle  An optional style to use instead of the global style.

        @returns The resolved style for the component type.
    */
    template <class ComponentType>
    static auto findComponentStyle (ComponentType& component)
        -> std::enable_if_t<std::is_base_of_v<Component, ComponentType>, ComponentStyle::Ptr>
    {
        auto& componentStyles = getGlobalThemeInstance()->componentStyles;

        if (auto style = component.getStyle())
            return style;

        {
            auto it = componentStyles.find (std::type_index (typeid (component)));
            if (it != componentStyles.end())
                return it->second;
        }

        {
            auto it = componentStyles.find (std::type_index (typeid (ComponentType)));
            if (it != componentStyles.end())
                return it->second;
        }

        jassertfalse;
        return nullptr;
    }

    //==============================================================================
    /**
        Sets the style for a specific component type.

        This template method allows setting the style for a specific type of component. The component type must be
        part of the `componentStyles` tuple.

        @param instanceStyle  The style to set for the component type.
    */
    template <class ComponentType>
    void setComponentStyle (ComponentStyle::Ptr style)
    {
        componentStyles.try_emplace (std::type_index (typeid (ComponentType)), std::move (style));
    }

    //==============================================================================
    /** Returns a color from the global theme.

        This method looks for the color in the component's properties first, then in the global theme. If no color
        is found, it returns std::nullopt.

        @param component     The component for which to find the color.
        @param colorId       The identifier for the color to retrieve.

        @return The color associated with the given identifier, or std::nullopt if not found.
    */
    static std::optional<Color> findComponentColor (const Component& component, const Identifier& colorId);

    /** Returns a color from this theme.

        This method looks for the color in the component's properties first, then in this theme. If no color
        is found, it returns std::nullopt.

        @param component     The component for which to find the color.
        @param colorId       The identifier for the color to retrieve.

        @return The color associated with the given identifier, or std::nullopt if not found.
    */
    std::optional<Color> findColor (const Component& component, const Identifier& colorId) const;

    /** Sets a color in this theme.

        @param colorId       The identifier for the color to set.
        @param color         The color to set.
    */
    void setColor (const Identifier& colorId, const Color& color);

    /** Sets multiple colors in this theme.

        @param colors        An initializer list of color identifier and color pairs.
    */
    void setColors (std::initializer_list<std::pair<const Identifier&, const Color&>> colors);

    //==============================================================================
    /** Returns a named float metric from the global theme.
    
        This method looks for the metric in the component's properties first, then in the global theme. If no metric
        is found, it returns std::nullopt.

        @param component     The component for which to find the metric.
        @param metricId      The identifier for the metric to retrieve.

        @return The metric associated with the given identifier, or std::nullopt if not found.
    */
    static std::optional<float> findComponentMetric (const Component& component, const Identifier& metricId);

    /** Returns a named float metric from this theme.
    
        This method looks for the metric in the component's properties first, then in this theme. If no metric
        is found, it returns std::nullopt.

        @param component     The component for which to find the metric.
        @param metricId      The identifier for the metric to retrieve.

        @return The metric associated with the given identifier, or std::nullopt if not found.
    */
    std::optional<float> findMetric (const Component& component, const Identifier& metricId) const;

    /** Registers a named float metric in this theme. 
    
        @param metricId      The identifier for the metric to set.
        @param value         The value to set for the metric.
    */
    void setMetric (const Identifier& metricId, float value);

    /** Sets multiple metrics in the global theme.

        @param metrics        An initializer list of metric identifier and value pairs.
    */
    void setMetrics (std::initializer_list<std::pair<const Identifier&, float>> metrics);

    //==============================================================================
    /**
        Sets the default text font for the application theme.

        @param font  The font to set as the default font (for text).
    */
    void setDefaultFont (Font font);

    /**
        Returns the default text font for the application theme.

        @returns  The default text font.
    */
    const Font& getDefaultFont() const;

    //==============================================================================
    /**
        Sets the default icon font for the application theme.

        @param font  The font to set as the default font (for icons).
    */
    void setDefaultIconFont (Font font);

    /**
        Returns the default icon font for the application theme.

        @returns  The default icon font.
    */
    const Font& getDefaultIconFont() const;

    //==============================================================================
    /**
        Sets the default monospace text font for the application theme.

        @param font  The font to set as the default font (for monospace text).
    */
    void setDefaultMonospaceFont (Font font);

    /**
        Returns the default monospace text font for the application theme.

        @returns  The default monospace text font.
    */
    const Font& getDefaultMonospaceFont() const;

private:
    static ApplicationTheme::Ptr& getGlobalThemeInstance();

    //==============================================================================
    std::unordered_map<std::type_index, ComponentStyle::Ptr> componentStyles;
    std::unordered_map<Identifier, Color> defaultColors;
    std::unordered_map<Identifier, float> defaultMetrics;
    Font defaultFont;
    Font defaultIconFont;
    Font defaultMonospaceFont;

    YUP_LEAK_DETECTOR (ApplicationTheme)
};

} // namespace yup
