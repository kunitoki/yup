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

#pragma once

namespace yup
{

//==============================================================================
/**
    Calls a function once per frame of the window that shows a component.

    The attachment registers with the native window the component is in and follows the component
    when it is added to or removed from the desktop, moved to another window or deleted. While the
    component is not in a window the function is not called.

    The function is called before the frame is painted, with the time elapsed since the previous
    frame. It only gets a frame painted by calling Component::repaint().

    @code
    class Spinner : public Component
    {
    public:
        void paint (Graphics& g) override;

    private:
        void advance (double elapsedSeconds)
        {
            rotation += 360.0f * static_cast<float> (elapsedSeconds);
            repaint();
        }

        float rotation = 0.0f;
        FrameAttachment frameAttachment { *this, bindFront (&Spinner::advance, this) };
    };
    @endcode

    To receive frames only for a while, such as during an animation, make the attachment inactive
    with setActive() when done and active again when needed: a window in which no attachment is
    active skips the frames it has nothing to paint in, without stalling the message thread. The
    function may stop or destroy its own attachment.

    Threading: on platforms with a render thread the function runs on that thread while it holds
    the message manager lock, so it never runs at the same time as the message thread or as
    Component::paint(), and can use the component freely. On the web it runs on the main thread.
    The graphics context is not current during the call. Create and destroy attachments on the
    message thread, or with the message manager locked.

    @see ComponentNative::FrameListener

    @tags{UI}
*/
class YUP_API FrameAttachment final : private ComponentNative::FrameListener
{
public:
    //==============================================================================
    /** The function called for every frame, with the seconds elapsed since the previous one. */
    using Callback = std::function<void (double elapsedSeconds)>;

    //==============================================================================
    /** Creates an attachment calling a function for every frame of the window of a component.

        @param component      The component whose window drives the function. It may be deleted before
                              the attachment, which then stops calling the function.
        @param callback       The function to call, which must not be empty.
        @param shouldBeActive Whether the function is called from the start, or only once
                              setActive() says so.
    */
    FrameAttachment (Component& component, Callback callback, bool shouldBeActive = true);

    /** Destructor, after which the function is no longer called. */
    ~FrameAttachment() override;

    //==============================================================================
    /** Starts or stops calling the function.

        An attachment that is not active is not registered with the window of its component: it
        costs nothing, and does not keep that window from skipping the frames it has nothing to
        paint in. The first call after it is made active again is given the time of one frame, not
        the time it was stopped for.

        Call it on the message thread, or with the message manager locked, as from the function
        itself.

        @param shouldBeActive Whether the function is called for the frames of the window.
    */
    void setActive (bool shouldBeActive);

    /** Returns true when the function is called for the frames of the window its component is in. */
    bool isActive() const noexcept { return active; }

private:
    //==============================================================================
    friend class Component;

    void frameStarted (double elapsedSeconds) override;

    void updateNative();
    void detach();

    Component* owner = nullptr;
    WeakReference<ComponentNative> native;
    std::shared_ptr<const Callback> callback;
    bool active = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrameAttachment)
};

} // namespace yup
