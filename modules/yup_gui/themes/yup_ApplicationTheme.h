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
    /** Paints a part of a component: what its type declares it shows beside its shapes (see ComponentPart).

        @param g           The Graphics to paint with, holding the paint the part uses.
        @param theme       The theme painting the part.
        @param component   The component, of the type the part was set for.
        @param area        Where the part goes, in the coordinates of the component.
    */
    using ComponentPartPainter = std::function<void (Graphics& g, const ApplicationTheme& theme, const Component& component, const Rectangle<float>& area)>;

    /**
        Sets how this theme paints a part of a component type.

        The type declares its parts (see ComponentTypeBuilder::part) and the theme paints them, as it
        paints the component: the style of the type calls the painter where it wants the part, and a
        `.yui` drawing places the same one with a `part` element. A painter set again replaces the
        one before.

        @code
        theme->setComponentPart<Label> (Label::Style::textPartId, [] (Graphics& g, const ApplicationTheme&, const Label& label, const Rectangle<float>& area)
        {
            g.fillFittedText (label.getStyledText(), area);
        });
        @endcode

        @param partName  The name of a part @p ComponentType declares in its `getClassTypeInfo()`.
        @param painter   Paints it: `void (Graphics&, const ApplicationTheme&, const ComponentType&, const Rectangle<float>& area)`.

        @see ComponentPart, paintPart
    */
    template <class ComponentType, class F>
        requires std::invocable<F&, Graphics&, const ApplicationTheme&, const ComponentType&, const Rectangle<float>&>
    void setComponentPart (const Identifier& partName, F&& painter)
    {
        setComponentPart (ComponentType::getClassTypeInfo(), partName, [painter = std::forward<F> (painter)] (Graphics& g, const ApplicationTheme& theme, const Component& component, const Rectangle<float>& area) mutable
        {
            std::invoke (painter, g, theme, static_cast<const ComponentType&> (component), area);
        });
    }

    /**
        Sets how this theme paints a part of the components of a type info.

        As the overload naming a class, for a type that tells its type info another way than with
        `getClassTypeInfo()`. The painter is given the components whose type info is @p type or derives from it.

        @param type      The type info declaring the part.
        @param partName  The name of the part.
        @param painter   Paints it.
    */
    void setComponentPart (const ComponentTypeInfo& type, const Identifier& partName, ComponentPartPainter painter);

    /**
        Paints a part of a component as the global theme does.

        @param g          The Graphics to paint with, holding the paint the part uses.
        @param component  The component to paint the part of.
        @param partName   The name of the part.
        @param area       Where the part goes, in the coordinates of the component.

        @returns False, having painted nothing, when the global theme paints no such part.

        @see Component::paintPart
    */
    static bool paintComponentPart (Graphics& g, const Component& component, const Identifier& partName, const Rectangle<float>& area);

    /**
        Paints a part of a component as this theme does.

        The painter is the one set for the type of the component, or else for the closest of its base types.

        @param g          The Graphics to paint with, holding the paint the part uses.
        @param component  The component to paint the part of.
        @param partName   The name of the part.
        @param area       Where the part goes, in the coordinates of the component.

        @returns False, having painted nothing, when this theme paints no such part.
    */
    bool paintPart (Graphics& g, const Component& component, const Identifier& partName, const Rectangle<float>& area) const;

    //==============================================================================
    /** Returns a color from the global theme.

        This method looks for the color in the component's properties first (walking up its parents), then in
        the colors set explicitly with setColor(), then in the colors derived from the palette. If no color is
        found, it returns std::nullopt.

        @param component     The component for which to find the color.
        @param colorId       The identifier for the color to retrieve.

        @return The color associated with the given identifier, or std::nullopt if not found.
    */
    static std::optional<Color> findComponentColor (const Component& component, const Identifier& colorId);

    /** Returns a color from this theme.

        This method looks for the color in the component's properties first (walking up its parents), then in
        the colors set explicitly with setColor(), then in the colors derived from the palette. The palette is
        the closest one set with Component::setPalette() on the component or a parent, mapped with this theme's
        palette mapping, or else the palette of this theme. If no color is found, it returns std::nullopt.

        @param component     The component for which to find the color.
        @param colorId       The identifier for the color to retrieve.

        @return The color associated with the given identifier, or std::nullopt if not found.
    */
    std::optional<Color> findColor (const Component& component, const Identifier& colorId) const;

    /** Returns the theme default or palette-derived color for @p colorId without checking @p component's
        own property or style store.

        Explicit colors set with ApplicationTheme::setColor() take precedence over palette colors mapped
        from @p component's closest Component::findPalette() or this theme's palette.

        @param component     The component used to locate any scoped palette.
        @param colorId       The identifier for the color to retrieve.

        @return The theme or palette color, or std::nullopt if none is registered.
    */
    std::optional<Color> findDefaultColor (const Component& component, const Identifier& colorId) const;

    /** Sets a color in this theme.

        Colors set this way take precedence over the colors derived from the palette, and are kept when the
        palette changes.

        @param colorId       The identifier for the color to set.
        @param color         The color to set.
    */
    void setColor (const Identifier& colorId, const Color& color);

    /** Sets multiple colors in this theme.

        @param colors        An initializer list of color identifier and color pairs.
    */
    void setColors (std::initializer_list<std::pair<const Identifier&, const Color&>> colors);

    //==============================================================================
    /** Sets the palette of this theme.

        The palette mapping (see setPaletteMapping()) is run to rebuild the palette colors, which are used by
        findColor() for every identifier that has no explicit color set with setColor(). Components are not
        repainted automatically.

        @param newPalette    The palette to use.

        @see ThemePalette, setPaletteMapping
    */
    void setPalette (const ThemePalette& newPalette);

    /** Returns the palette of this theme. */
    const ThemePalette& getPalette() const;

    /** Returns the palette a component is drawn with: the closest one set with Component::setPalette()
        on the component or a parent, or else the palette of this theme.

        @param component     The component being drawn.
    */
    const ThemePalette& getPalette (const Component& component) const;

    /** Returns a number that changes every time the theme does: its palette or its mapping, its
        colors, its properties or its fonts.

        No two themes share a value, so it also tells when another theme became the global one.
        Stylesheets compare it between frames to restyle the components whose computed values read
        palette roles, and components to paint again what they keep in a texture, without
        listening to the theme.

        @see setPalette, setPaletteMapping
    */
    uint32 getPaletteGeneration() const noexcept { return paletteGeneration; }

    /** Returns a number that changes whenever a theme changes, or another one becomes the global one.

        It is one number for all the themes, and can be read from any thread without a lock: a
        window polls it to tell, without stopping the message thread, that the stylesheets of its
        components have to look at the theme again.

        @see getPaletteGeneration
    */
    static uint32 getLastGeneration() noexcept;

    /** A function that assigns theme colors from the roles of a palette, by calling setPaletteColor(). */
    using PaletteMapping = std::function<void (const ThemePalette&, ApplicationTheme&)>;

    /** Sets the function that turns the palette into theme colors, and runs it on the current palette.

        Themes set this up when they are created, so that a new palette recolors all their components. The
        same mapping derives the colors of the palettes set on components.

        @param mapping       The mapping function.
    */
    void setPaletteMapping (PaletteMapping mapping);

    /** The maximum number of per-component palette mappings kept cached at once. */
    static constexpr size_t maxCachedComponentPalettes = 32;

    /** Returns the number of per-component palette mappings currently cached on this theme. */
    size_t getNumCachedComponentPalettes() const noexcept;

    /** Sets a palette derived color in this theme.

        This is meant to be called from a palette mapping function. Palette colors are cleared every time the
        palette changes, and are only used when there is no explicit color set with setColor().

        @param colorId       The identifier for the color to set.
        @param color         The color to set.
    */
    void setPaletteColor (const Identifier& colorId, const Color& color);

    //==============================================================================
    /** Returns a named property value from the global theme, used as a default fallback.

        This is consulted by Component::findProperty() after the component and its parents have
        been checked, and before the descriptor default. Themes register defaults here with
        setProperty().

        @param component     The component for which to find the property.
        @param id            The storage id for the property to retrieve.

        @return The property value, or std::nullopt if not found.
    */
    static std::optional<var> findComponentProperty (const Component& component, const Identifier& id);

    /** Returns a named property value from this theme.

        @param component     The component for which to find the property.
        @param id            The storage id for the property to retrieve.

        @return The property value, or std::nullopt if not found.
    */
    std::optional<var> findProperty (const Component& component, const Identifier& id) const;

    /** Registers a named theme-default property value.

        @param id            The storage id for the property to set.
        @param value         The value to set for the property.
    */
    void setProperty (const Identifier& id, const var& value);

    /** Sets multiple theme-default property values.

        @param properties    An initializer list of property id and value pairs.
    */
    void setProperties (std::initializer_list<std::pair<const Identifier&, var>> properties);

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
    std::optional<Color> findPaletteColor (const ThemePalette& componentPalette, const Identifier& colorId) const;
    static std::atomic<uint32>& getGenerationCounter() noexcept;
    static uint32 makeGeneration() noexcept;

    //==============================================================================
    std::unordered_map<std::type_index, ComponentStyle::Ptr> componentStyles;
    std::unordered_map<const ComponentTypeInfo*, std::unordered_map<Identifier, ComponentPartPainter>> componentParts;
    std::unordered_map<Identifier, Color> defaultColors;
    std::unordered_map<Identifier, Color> paletteColors;
    ThemePalette palette;
    PaletteMapping paletteMapping;
    uint32 paletteGeneration = makeGeneration();
    mutable std::vector<std::pair<ThemePalette, std::unordered_map<Identifier, Color>>> componentPaletteColors;
    mutable SpinLock componentPaletteColorsLock;
    std::unordered_map<Identifier, var> defaultProperties;
    Font defaultFont;
    Font defaultIconFont;
    Font defaultMonospaceFont;

    YUP_LEAK_DETECTOR (ApplicationTheme)
};

} // namespace yup
