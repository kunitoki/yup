/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

void TextInputTarget::requestTextInput()
{
    if (textInputActive)
        return;

    textInputActive = true;

    if (auto* component = dynamic_cast<Component*> (this))
    {
        if (auto* nativeComponent = component->getNativeComponent())
            nativeComponent->startTextInput (*component);
    }
}

void TextInputTarget::relinquishTextInput()
{
    if (! textInputActive)
        return;

    textInputActive = false;

    if (auto* component = dynamic_cast<Component*> (this))
    {
        if (auto* nativeComponent = component->getNativeComponent())
            nativeComponent->stopTextInput (*component);
    }
}

void TextInputTarget::updateTextInputRect()
{
    if (! textInputActive)
        return;

    if (auto* component = dynamic_cast<Component*> (this))
    {
        if (auto* nativeComponent = component->getNativeComponent())
            nativeComponent->updateTextInputRect (*component);
    }
}

bool TextInputTarget::isTextInputActive() const noexcept
{
    return textInputActive;
}

} // namespace yup
