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

class JUCE_API MouseListener
{
public:
    virtual ~MouseListener() {}

    virtual void mouseEnter (const MouseEvent& event) {}
    virtual void mouseExit (const MouseEvent& event) {}
    virtual void mouseDoubleClick(const MouseEvent& event) {}
    virtual void mouseDown (const MouseEvent& event) {}
    virtual void mouseMove (const MouseEvent& event) {}
    virtual void mouseDrag (const MouseEvent& event) {}
    virtual void mouseUp (const MouseEvent& event) {}
    virtual void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) {}

private:
    JUCE_DECLARE_WEAK_REFERENCEABLE (MouseListener)
};

} // namespace yup
