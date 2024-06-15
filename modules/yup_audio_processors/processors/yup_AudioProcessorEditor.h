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
class JUCE_API AudioProcessorEditor : public Component
{
public:
    AudioProcessorEditor();
    virtual ~AudioProcessorEditor();

    virtual bool isResizable() const = 0;
    virtual Size<int> getPreferredSize() const = 0;

    virtual bool shouldPreserveAspectRatio() const { return false; }
    virtual Size<int> getPreferredAspectRatioSize() const { return getPreferredSize(); }

    virtual bool shouldRenderContinuous() const { return false; }

    virtual void attachedToWindow() {} // TODO
};

} // namespace yup
