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
    /** Returns true if the fonts are equal. */
    bool operator== (const Font& other) const;

    /** Returns true if the fonts are not equal. */
    bool operator!= (const Font& other) const;

    //==============================================================================
    /** @internal */
    Font (rive::rcp<rive::Font> font);
    /** @internal */
    rive::rcp<rive::Font> getFont() const;

private:
    rive::rcp<rive::Font> font;
};

} // namespace yup
