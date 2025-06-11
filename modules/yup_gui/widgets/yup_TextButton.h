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
class YUP_API TextButton : public Button
{
public:
    //==============================================================================
    TextButton (StringRef componentID);

    //==============================================================================

    String getButtonText() const { return buttonText; }

    void setButtonText (StringRef newButtonText);

    //==============================================================================
    /** Color identifiers used by the text editor. */
    struct Colors
    {
        static const Identifier backgroundColorId;
        static const Identifier backgroundPressedColorId;
        static const Identifier textColorId;
        static const Identifier textPressedColorId;
        static const Identifier outlineColorId;
        static const Identifier outlineFocusedColorId;
    };

    Rectangle<float> getTextBounds() const;

    //==============================================================================
    /** @internal */
    void paintButton (Graphics& g) override;
    /** @internal */
    void resized() override;

    /** @internal */
    StyledText& getStyledText() const noexcept { return const_cast<StyledText&> (styledText); }

private:
    String buttonText;
    StyledText styledText;
};

} // namespace yup
