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
class YUP_API Button : public Component
{
public:
    //==============================================================================
    Button (StringRef componentID);

    //==============================================================================
    bool isButtonDown() const { return isButtonCurrentlyDown; }
    bool isButtonOver() const { return isButtonCurrentlyDown; }

    //==============================================================================
    virtual void paintButton (Graphics& g) = 0;

    //==============================================================================
    std::function<void()> onClick;

    //==============================================================================
    void paint (Graphics& g) override;
    void mouseEnter (const MouseEvent& event) override;
    void mouseExit (const MouseEvent& event) override;
    void mouseDown (const MouseEvent& event) override;
    void mouseUp (const MouseEvent& event) override;

private:
    bool isButtonCurrentlyOver = false;
    bool isButtonCurrentlyDown = false;
};

} // namespace yup
