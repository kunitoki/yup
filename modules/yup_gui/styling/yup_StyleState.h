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

#pragma once

namespace yup
{

//==============================================================================
/**
    The styling a StyleContext computed for one component.

    A component allocates one lazily, the first time a stylesheet, an animation or a style
    palette gives it a value of its own. It holds everything a stylesheet owns, kept apart
    from the values set explicitly in C++ or in a `.yui` instance so that the two never
    fight: Component::findProperty() reads the explicit value first, then the animated one
    a running transition or animation writes, then the computed one, and
    Component::setProperty() leaves the style layers untouched.

    @see StyleContext, StyleSheet, Component::setStyleSheet

    @tags{UI}
*/
class YUP_API StyleState
{
public:
    //==============================================================================
    /** Constructor. */
    StyleState() = default;

    //==============================================================================
    /** Sets the value a stylesheet computed for a property.

        @param storageId The storage id of the property, as resolved by its descriptor.
        @param value     The computed value.

        @returns True if the value changed.
    */
    bool setComputedValue (const Identifier& storageId, const var& value);

    /** Removes a computed value.

        @param storageId The storage id of the property.

        @returns True if a value was removed.
    */
    bool removeComputedValue (const Identifier& storageId);

    /** Returns the computed value of a property, or nullptr when it has none.

        @param storageId The storage id of the property.
    */
    const var* getComputedValue (const Identifier& storageId) const;

    /** Removes every computed value.

        @returns True if any value was removed.
    */
    bool clearComputedValues();

    /** Returns the computed values, keyed by storage id. */
    const NamedValueSet& getComputedValues() const noexcept { return computedValues; }

    //==============================================================================
    /** Sets the value a running transition or animation shows for a property.

        The animated value hides the computed one until it is removed.

        @param storageId The storage id of the property, as resolved by its descriptor.
        @param value     The animated value.

        @returns True if the value changed.
    */
    bool setAnimatedValue (const Identifier& storageId, const var& value);

    /** Removes an animated value, uncovering the computed one.

        @param storageId The storage id of the property.

        @returns True if a value was removed.
    */
    bool removeAnimatedValue (const Identifier& storageId);

    /** Returns the animated value of a property, or nullptr when it has none.

        @param storageId The storage id of the property.
    */
    const var* getAnimatedValue (const Identifier& storageId) const;

    /** Removes every animated value.

        @returns True if any value was removed.
    */
    bool clearAnimatedValues();

    /** Returns the value the style layer shows for a property: the animated one, else the computed one.

        @param storageId The storage id of the property.

        @returns The value, or nullptr when the style layer has none.
    */
    const var* getStyleValue (const Identifier& storageId) const;

    //==============================================================================
    /** Scopes a palette to this component and its children.

        @param newPalette The palette a `palette:` declaration matched, or std::nullopt for none.

        @returns True if the palette changed.
    */
    bool setStylePalette (std::optional<ThemePalette> newPalette);

    /** Returns the palette a stylesheet scoped to this component, or nullptr. */
    const ThemePalette* getStylePalette() const noexcept { return stylePalette.get(); }

    //==============================================================================
    /** Marks this component as reading palette roles, so palette switches restyle it.

        @param isDependent True when a computed value reads `palette.*`.
    */
    void setPaletteDependent (bool isDependent) noexcept { paletteDependent = isDependent; }

    /** Returns true when a computed value of this component reads a palette role. */
    bool isPaletteDependent() const noexcept { return paletteDependent; }

    //==============================================================================
    /** One layer of a `shadow:` declaration, painted behind the component. */
    struct Shadow
    {
        /** How far the layer is moved from the component bounds. */
        Point<float> offset;

        /** The feather softening the layer edges. */
        float blur = 0.0f;

        /** How far the layer grows past the component bounds, or shrinks when negative. */
        float spread = 0.0f;

        /** The layer color. */
        Color color;

        bool operator== (const Shadow& other) const = default;
    };

    /** Sets the shadow layers, the first one painted on top.

        @param newShadows The layers, empty for none.

        @returns True if the layers changed.
    */
    bool setShadows (std::vector<Shadow> newShadows);

    /** Returns the shadow layers, the first one painted on top. */
    const std::vector<Shadow>& getShadows() const noexcept { return shadows; }

    /** Paints the shadow layers as feathered rounded rectangles.

        @param g            The graphics context, in the component coordinates.
        @param bounds       The component bounds the layers follow.
        @param cornerRadius The corner radius of the component bounds.
    */
    void paintShadows (Graphics& g, Rectangle<float> bounds, float cornerRadius) const;

    //==============================================================================
    /** Returns true when the component holds nothing a stylesheet computed. */
    bool isEmpty() const noexcept;

private:
    NamedValueSet computedValues;
    NamedValueSet animatedValues;
    std::unique_ptr<ThemePalette> stylePalette;
    std::vector<Shadow> shadows;
    bool paletteDependent = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleState)
};

} // namespace yup
