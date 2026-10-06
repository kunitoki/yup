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
    A typeface at a given height, with optional variable axis values and OpenType features.

    A Font is a light value: copies share the loaded font data. The height is only a size
    carried along for text layout, while axis values and features make a new variation of
    the font data.

    @code
    auto font = Font::loadFontFromFile (file).valueOr (Font())
                    .withHeight (18.0f)
                    .withFeature ({ "tnum", 1 });
    @endcode

    Fonts usually come from the ApplicationTheme rather than being loaded inline.

    @see StyledText, ApplicationTheme

    @tags{Graphics}
*/
class YUP_API Font
{
public:
    //==============================================================================
    /** Creates an empty font. */
    Font() = default;

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    Font (const Font& other) noexcept = default;
    Font (Font&& other) noexcept = default;
    Font& operator= (const Font& other) noexcept = default;
    Font& operator= (Font&& other) noexcept = default;

    //==============================================================================
    /** Loads a font from a memory block holding a TrueType or OpenType font.

        The data is copied, so the block can be released afterwards.

        @param fontBytes The memory block containing the font data.
        @return The result of the operation, holding the loaded font on success.
    */
    static ResultValue<Font> loadFontFromData (const MemoryBlock& fontBytes);

    /** Loads a font from bytes holding a TrueType or OpenType font.

        The data is copied, so the bytes can be released afterwards.

        @param fontBytes The span containing the font data.
        @return The result of the operation, holding the loaded font on success.
    */
    static ResultValue<Font> loadFontFromData (const Span<const uint8>& fontBytes);

    //==============================================================================
    /** Loads a font from a TrueType or OpenType font file.

        The whole file is read into memory.

        @param fontFile The file containing the font data.
        @return The result of the operation, holding the loaded font on success.
    */
    static ResultValue<Font> loadFontFromFile (const File& fontFile);

    /** Loads the first font found among the given file paths, in order.

        @param fontPaths The file paths to try in order.
        @return The result of the operation, holding the loaded font on success,
                or a failure if none of the files could be loaded.
    */
    static ResultValue<Font> loadFontFromFirstAvailableFile (std::initializer_list<const char*> fontPaths);

    /** Attempts to load the platform's regular system text font.

        Despite the name, this is the font the platform uses for its own interface, which is
        sans-serif everywhere: the CoreText system UI font on macOS and iOS, Segoe UI on
        Windows, Roboto on Android and the first of Noto Sans, DejaVu Sans, Liberation Sans
        and Ubuntu found on Linux. WebAssembly has no system fonts, so this always fails there.

        @return The result of the operation, holding the loaded font on success.
    */
    static ResultValue<Font> loadSerifSystemTextFont();

    /** Attempts to load the platform's monospace system text font.

        On macOS and iOS this uses the CoreText monospaced system UI font, elsewhere
        well-known system monospace font files are tried in order. WebAssembly has no system
        fonts, so this always fails there.

        @return The result of the operation, holding the loaded font on success.
    */
    static ResultValue<Font> loadMonospaceSystemTextFont();

    /** Attempts to load the platform's color emoji system font.

        On macOS/iOS this uses CoreText's Apple Color Emoji, on Windows Segoe UI Emoji and on
        Linux and Android Noto Color Emoji. WebAssembly has no system fonts, so this always
        fails there: ship an emoji font with the app and pass it to setColorEmojiFallbackFont().

        @return The result of the operation, holding the loaded font on success, or a failure
                if no font with color glyphs could be found.
    */
    static ResultValue<Font> loadColorEmojiSystemFont();

    /** Sets the font that draws characters missing from a text's own font, typically emoji.

        Shaping text falls back to this font for a run of characters the text's font lacks,
        when this font has the first of them. That covers StyledText, every component drawing
        text and Rive artboard text, and color glyphs are drawn in their own colors.

        The default is the system color emoji font (see loadColorEmojiSystemFont()), loaded
        on first use. Pass an empty Font to turn the fallback off. The setting is process-wide,
        can be changed from any thread, and only affects text shaped after the call.

        @param font The fallback font, or an empty Font for none.
    */
    static void setColorEmojiFallbackFont (const Font& font);

    /** Returns the font that draws characters missing from a text's own font.

        @return The font set with setColorEmojiFallbackFont(), otherwise the system color emoji
                font, or an empty Font when there is none.
    */
    static Font getColorEmojiFallbackFont();

    //==============================================================================
    /** Returns true if the font is empty (no font data loaded). */
    bool isEmpty() const noexcept { return font == nullptr; }

    /** Returns the distance from the baseline to the top of the font, as a fraction of its height.

        The value is negative, since up is -Y: multiply by getHeight() for the size in pixels.
        Returns 0 for an empty font.
    */
    float getAscent() const;

    /** Returns the distance from the baseline to the bottom of the font, as a fraction of its height.

        The value is positive: multiply by getHeight() for the size in pixels. Returns 0 for
        an empty font.
    */
    float getDescent() const;

    /** Returns the weight of the font, from 100 (thin) to 900 (black), 400 being regular.

        Returns 0 for an empty font.
    */
    int getWeight() const;

    /** Returns true if the font is italic. */
    bool isItalic() const;

    //==============================================================================
    /** Returns the height the font is laid out at: the size of its em square, 12 by default. */
    float getHeight() const noexcept;

    /** Sets the height the font is laid out at.

        @param newHeight The size of the em square.
    */
    void setHeight (float newHeight);

    /** Returns a copy of the font at another height.

        @param height The size of the em square.
        @return A font sharing this font's data, at the given height.
    */
    Font withHeight (float height) const;

    //==============================================================================
    /** Describes a variation axis of a variable font. */
    struct Axis
    {
        Axis() = default;

        String tagName;            ///< The four letter tag of the axis, like "wght" or "wdth".
        float minimumValue = 0.0f; ///< The smallest value the axis takes.
        float maximumValue = 0.0f; ///< The largest value the axis takes.
        float defaultValue = 0.0f; ///< The value of the axis when it is not set.
    };

    /** Returns the number of variation axes, 0 for fonts that are not variable. */
    int getNumAxis() const;

    /** Returns the description of the axis at the given index.

        @param index The index of the axis, from 0 to getNumAxis() - 1.
        @return The description of the axis, or nothing when the index is out of range.
    */
    std::optional<Font::Axis> getAxisDescription (int index) const;

    /** Returns the description of the axis with the given tag.

        @param tagName The four letter tag of the axis, like "wght".
        @return The description of the axis, or nothing when the font has no such axis.
    */
    std::optional<Font::Axis> getAxisDescription (StringRef tagName) const;

    //==============================================================================
    /** Returns the value of the axis at the given index.

        @param index The index of the axis, from 0 to getNumAxis() - 1.
        @return The value of the axis, or 0 when the index is out of range.
    */
    float getAxisValue (int index) const;

    /** Sets the value of the axis at the given index.

        The value is limited to the axis range, and an index out of range is ignored.

        @param index The index of the axis, from 0 to getNumAxis() - 1.
        @param value The new value of the axis.
    */
    void setAxisValue (int index, float value);

    /** Returns a copy of the font with the axis at the given index set.

        @param index The index of the axis, from 0 to getNumAxis() - 1.
        @param value The new value of the axis, limited to the axis range.
        @return A font with the axis set and the other settings kept, or an empty font when
                the index is out of range.
    */
    Font withAxisValue (int index, float value) const;

    /** Sets the axis at the given index back to its default value.

        @param index The index of the axis, from 0 to getNumAxis() - 1.
    */
    void resetAxisValue (int index);

    //==============================================================================
    /** Returns the value of the axis with the given tag.

        @param tagName The four letter tag of the axis, like "wght".
        @return The value of the axis.
    */
    float getAxisValue (StringRef tagName) const;

    /** Sets the value of the axis with the given tag.

        The value is limited to the axis range, and a tag the font has no axis for is ignored.

        @param tagName The four letter tag of the axis, like "wght".
        @param value   The new value of the axis.
    */
    void setAxisValue (StringRef tagName, float value);

    /** Returns a copy of the font with the axis with the given tag set.

        @param tagName The four letter tag of the axis, like "wght".
        @param value   The new value of the axis, limited to the axis range.
        @return A font with the axis set and the other settings kept, or an empty font when
                the font has no such axis.
    */
    Font withAxisValue (StringRef tagName, float value) const;

    /** Sets the axis with the given tag back to its default value.

        @param tagName The four letter tag of the axis, like "wght".
    */
    void resetAxisValue (StringRef tagName);

    //==============================================================================
    /** Sets every axis back to its default value. */
    void resetAllAxisValues();

    //==============================================================================
    /** A value for the axis with a given tag, for setting several axes at once. */
    struct AxisOption
    {
        /** Creates an option for an axis.

            @param tagName The four letter tag of the axis, like "wght".
            @param value   The value of the axis.
        */
        AxisOption (StringRef tagName, float value)
            : tagName (tagName)
            , value (value)
        {
        }

        String tagName; ///< The four letter tag of the axis.
        float value;    ///< The value of the axis.
    };

    /** Sets several axes at once.

        Values are limited to their axis range, and tags the font has no axis for are ignored.

        @param axisOptions The axes to set and their values.
    */
    void setAxisValues (std::initializer_list<AxisOption> axisOptions);

    /** Returns a copy of the font with several axes set.

        Tags the font has no axis for are ignored.

        @param axisOptions The axes to set and their values, limited to their axis range.
        @return A font with the axes set and the other settings kept, or an empty font when
                none of the axes exist.
    */
    Font withAxisValues (std::initializer_list<AxisOption> axisOptions) const;

    //==============================================================================
    /** An OpenType feature and its value, like { "liga", 0 } to turn ligatures off. */
    struct Feature
    {
        /** Creates a feature from its numeric tag.

            @param tag   The four letter tag packed in big endian order.
            @param value The value of the feature: 0 turns it off, 1 on, higher values pick alternates.
        */
        Feature (uint32_t tag, uint32_t value)
            : tag (tag)
            , value (value)
        {
        }

        /** Creates a feature from its four letter tag.

            @param stringTag The four letter tag of the feature, like "liga" or "tnum".
            @param value     The value of the feature: 0 turns it off, 1 on, higher values pick alternates.
        */
        Feature (StringRef stringTag, uint32_t value)
            : tag (0)
            , value (value)
        {
            jassert (stringTag.length() == 4);
            if (stringTag.length() == 4)
                tag = (uint32_t (stringTag.text[0]) << 24) | (uint32_t (stringTag.text[1]) << 16) | (uint32_t (stringTag.text[2]) << 8) | uint32_t (stringTag.text[3]);
        }

        uint32_t tag;   ///< The four letter tag packed in big endian order.
        uint32_t value; ///< The value of the feature.
    };

    /** Returns a copy of the font with an OpenType feature set.

        Features set before are kept, as are the height and axis values.

        @param feature The feature and its value.
        @return A font with the feature set, or an empty font when this font is empty.
    */
    Font withFeature (Feature feature) const;

    /** Returns a copy of the font with several OpenType features set.

        Features set before are kept, as are the height and axis values.

        @param features The features and their values.
        @return A font with the features set, or an empty font when this font is empty.
    */
    Font withFeatures (std::initializer_list<Feature> features) const;

    //==============================================================================
    /** Returns true if both fonts share the same loaded font variation.

        Setting axis values or features makes a new variation, so fonts set up separately
        compare different even with the same settings. The height is not compared.
    */
    bool operator== (const Font& other) const;

    /** Returns true if the fonts do not share the same loaded font variation. */
    bool operator!= (const Font& other) const;

    //==============================================================================
    /** @internal */
    Font (rive::rcp<rive::Font> font);
    /** @internal */
    Font (rive::rcp<rive::Font> font, float height);
    /** @internal */
    rive::rcp<rive::Font> getFont() const;

private:
    rive::rcp<rive::Font> font;
    float height = 12.0f;
};

} // namespace yup
