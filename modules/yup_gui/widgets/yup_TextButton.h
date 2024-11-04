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
class JUCE_API TextButton : public Button
{
public:
    //==============================================================================
    TextButton (StringRef componentID);

    //==============================================================================
    struct Style : ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<const Style>;

        Style() = default;

        Style (std::function<void (Graphics&, const TextButton&, bool, bool)> p)
            : onPaint (std::move (p))
        {
        }

        std::function<void (Graphics&, const TextButton&, bool, bool)> onPaint;
    };

    void setStyle (Style::Ptr newStyle);
    Style::Ptr getStyle() const;

    //==============================================================================
    void paintButton (Graphics& g, bool isButtonOver, bool isButtonDown) override;

private:
    Style::Ptr style;
};

} // namespace yup
