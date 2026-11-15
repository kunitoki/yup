/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
/** A component that displays text.

    The Label component can display text with optional styling including font,
    colors, and stroke effects. It efficiently caches the styled text layout
    for better performance.

    @see Component, StyledText
*/
class YUP_API Label : public Component
{
public:
    //==============================================================================
    /** Creates an empty label. */
    Label (StringRef componentID = {});

    //==============================================================================
    /** Returns the label's current text.

        @returns The text displayed in the label
    */
    String getText() const;

    /** Changes the label's text.

        @param newText          The new text to display
        @param notification     Whether property listeners are told that `text` changed
    */
    void setText (String newText, NotificationType notification = sendNotification);

    //==============================================================================
    /** Returns the current font.

        @returns The font used to display the text
    */
    std::optional<Font> getFont() const;

    /** Sets the font to use.

        @param newFont    The new font to use
    */
    void setFont (Font newFont);

    /** Reset the font to the theme one. */
    void resetFont();

    //==============================================================================
    /** Returns the stroke width used for the text outline.

        @returns The width of the text outline
    */
    float getStrokeWidth() const noexcept { return strokeWidth; }

    /** Sets the stroke width for the text outline.

        @param newWidth    The new width to use for the text outline
    */
    void setStrokeWidth (float newWidth);

    //==============================================================================
    /** Returns the justification used for the text.

        @returns The current text justification
    */
    Justification getJustification() const { return justification; }

    /** Sets the justification for the text.

        @param newJustification    The new justification to use
    */
    void setJustification (Justification newJustification);

    //==============================================================================
    /** Style identifiers for customizing the appearance of the Label. */
    struct Style
    {
        static const Identifier textFillColorId;       /**< Text fill color */
        static const Identifier textStrokeColorId;     /**< Text stroke color */
        static const Identifier backgroundColorId;     /**< Background color */
        static const Identifier outlineColorId;        /**< Outline color */

        static const Identifier textHeightProportionId; /**< Text height proportion */
    };

    //==============================================================================
    /** Returns the static type info describing the Label type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override;

    //==============================================================================
    /** @internal */
    StyledText& getStyledText() const noexcept { return const_cast<StyledText&> (styledText); }

    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void styleChanged() override;

private:
    void prepareText();
    void invalidateCache();

    String text;
    StyledText styledText;
    float strokeWidth = 0.0f;
    Justification justification { Justification::left };
    std::optional<Font> font;
    Font preparedFont;
    Size<float> preparedSize { -1.0f, -1.0f };
    Justification preparedJustification { Justification::left };
    bool needsUpdate = true;
    bool textDirty = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Label)
};

} // namespace yup
