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
/** Font.

    This class represents a font.
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
    /** Loads a font from a memory block.

        @param fontBytes The memory block containing the font data.
        @return The result of the operation.
    */
    Result loadFromData (const MemoryBlock& fontBytes);

    //==============================================================================
    /** Loads a font from a file.

        @param fontFile The file containing the font data.
        @return The result of the operation.
    */
    Result loadFromFile (const File& fontFile);

    //==============================================================================
    /** Returns the ascent of the font. */
    float getAscent() const;

    /** Returns the descent of the font. */
    float getDescent() const;

    /** Returns the weight of the font. */
    int getWeight() const;

    /** Returns true if the font is italic. */
    bool isItalic() const;

    //==============================================================================

    float getHeight() const noexcept;

    void setHeight (float newHeight);

    Font withHeight (float height) const;

    //==============================================================================
    /** Axis.

        This struct represents an axis of the font.
    */
    struct Axis
    {
        Axis() = default;

        String tagName;
        float minimumValue = 0.0f;
        float maximumValue = 0.0f;
        float defaultValue = 0.0f;
    };

    /** Returns the number of axes in the font.

        @return The number of axes in the font.
    */
    int getNumAxis() const;

    /** Returns the description of the axis at the given index.

        @param index The index of the axis.

        @return The description of the axis.
    */
    std::optional<Font::Axis> getAxisDescription (int index) const;

    /** Returns the description of the axis with the given tag name.

        @param tagName The tag name of the axis.

        @return The description of the axis.
    */
    std::optional<Font::Axis> getAxisDescription (StringRef tagName) const;

    //==============================================================================
    /** Returns the value of the axis at the given index.

        @param index The index of the axis.

        @return The value of the axis.
    */
    float getAxisValue (int index) const;

    /** Sets the value of the axis at the given index.

        @param index The index of the axis.
        @param value The value of the axis.
    */
    void setAxisValue (int index, float value);

    /** Returns a new font with the value of the axis at the given index.

        @param index The index of the axis.
        @param value The value of the axis.

        @return A new font with the value of the axis at the given index.
    */
    Font withAxisValue (int index, float value) const;

    /** Resets the value of the axis at the given index.

        @param index The index of the axis.
    */
    void resetAxisValue (int index);

    //==============================================================================
    /** Returns the value of the axis with the given tag name.

        @param tagName The tag name of the axis.

        @return The value of the axis.
    */
    float getAxisValue (StringRef tagName) const;

    /** Sets the value of the axis with the given tag name.

        @param tagName The tag name of the axis.
        @param value The value of the axis.
    */
    void setAxisValue (StringRef tagName, float value);

    /** Returns a new font with the value of the axis with the given tag name.

        @param tagName The tag name of the axis.
        @param value The value of the axis.

        @return A new font with the value of the axis with the given tag name.
    */
    Font withAxisValue (StringRef tagName, float value) const;

    /** Resets the value of the axis with the given tag name.

        @param tagName The tag name of the axis.
    */
    void resetAxisValue (StringRef tagName);

    //==============================================================================
    /** Resets the values of all axes.

        @return A new font with the values of all axes reset.
    */
    void resetAllAxisValues();

    //==============================================================================
    /** Axis option.

        This struct represents an option of the axis.
    */
    struct AxisOption
    {
        AxisOption (StringRef tagName, float value)
            : tagName (tagName)
            , value (value)
        {
        }

        String tagName;
        float value;
    };

    /** Sets the values of the axes.

        @param axisOptions The options of the axes.
    */
    void setAxisValues (std::initializer_list<AxisOption> axisOptions);

    /** Returns a new font with the values of the axes.

        @param axisOptions The options of the axes.

        @return A new font with the values of the axes.
    */
    Font withAxisValues (std::initializer_list<AxisOption> axisOptions) const;

    //==============================================================================
    struct Feature
    {
        Feature (uint32_t tag, uint32_t value)
            : tag (tag)
            , value (value)
        {
        }

        Feature (StringRef stringTag, uint32_t value)
            : tag (0)
            , value (value)
        {
            jassert (stringTag.length() == 4);
            if (stringTag.length() == 4)
                tag = (uint32_t (stringTag.text[0]) << 24) | (uint32_t (stringTag.text[1]) << 16) | (uint32_t (stringTag.text[2]) << 8) | uint32_t (stringTag.text[3]);
        }

        uint32_t tag;
        uint32_t value;
    };

    Font withFeature (Feature feature) const;

    Font withFeatures (std::initializer_list<Feature> features) const;

    //==============================================================================
    /** Returns true if the fonts are equal. */
    bool operator== (const Font& other) const;

    /** Returns true if the fonts are not equal. */
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
