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

FrameAttachment::FrameAttachment (Component& component, Callback callbackToUse, bool shouldBeActive)
    : callback (std::make_shared<const Callback> (std::move (callbackToUse)))
    , active (shouldBeActive)
{
    jassert (*callback != nullptr); // A frame attachment needs a function to call

    if (*callback == nullptr)
        return;

    owner = std::addressof (component);
    owner->frameAttachments.add (this);

    updateNative();
}

FrameAttachment::~FrameAttachment()
{
    detach();
}

//==============================================================================

void FrameAttachment::setActive (bool shouldBeActive)
{
    if (active == shouldBeActive)
        return;

    active = shouldBeActive;

    updateNative();
}

//==============================================================================

void FrameAttachment::frameStarted (double elapsedSeconds)
{
    // The function may destroy this attachment, and must outlive its own call
    const auto function = callback;

    (*function) (elapsedSeconds);
}

//==============================================================================

void FrameAttachment::updateNative()
{
    auto* newNative = owner != nullptr && active ? owner->getNativeComponent() : nullptr;
    auto* oldNative = native.get();

    if (newNative == oldNative)
        return;

    if (oldNative != nullptr)
        oldNative->removeFrameListener (this);

    native = newNative;

    if (newNative != nullptr)
        newNative->addFrameListener (this);
}

void FrameAttachment::detach()
{
    if (owner == nullptr)
        return;

    owner->frameAttachments.removeFirstMatchingValue (this);
    owner = nullptr;

    updateNative();
}

} // namespace yup
