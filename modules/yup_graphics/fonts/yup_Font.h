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

class JUCE_API Font
{
public:
    //==============================================================================
    Font() = default;

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    Font (const Font& other) noexcept = default;
    Font (Font&& other) noexcept = default;
    Font& operator= (const Font& other) noexcept = default;
    Font& operator= (Font&& other) noexcept = default;

    //==============================================================================
    Result loadFromData (const MemoryBlock& fontBytes);

    //==============================================================================
    Result loadFromFile (const File& fontFile);

    //==============================================================================
    float getAscent() const;
    float getDescent() const;
    int getWeight() const;
    bool isItalic() const;

    //==============================================================================
    struct Axis
    {
        Axis() = default;

        String tagName;
        float minimumValue = 0.0f;
        float maximumValue = 0.0f;
        float defaultValue = 0.0f;
    };

    int getNumAxis() const;
    std::optional<Font::Axis> getAxisDescription (int index) const;
    std::optional<Font::Axis> getAxisDescription (StringRef tagName) const;

    float getAxisValue (int index) const;
    float getAxisValue (StringRef tagName) const;

    void setAxisValue (int index, float value);
    void setAxisValue (StringRef tagName, float value);

    Font withAxisValue (int index, float value) const;
    Font withAxisValue (StringRef tagName, float value) const;

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

    void setAxisValues (std::initializer_list<AxisOption> axisOptions);
    Font withAxisValues (std::initializer_list<AxisOption> axisOptions) const;

    void resetAxisValue (int index);
    void resetAxisValue (StringRef tagName);

    void resetAllAxisValues();

    //==============================================================================
    bool operator==(const Font& other) const;
    bool operator!=(const Font& other) const;

    //==============================================================================
    /** @internal */
    Font (rive::rcp<rive::Font> font);
    /** @internal */
    rive::rcp<rive::Font> getFont() const;

private:
    rive::rcp<rive::Font> font;
};

} // namespace yup
