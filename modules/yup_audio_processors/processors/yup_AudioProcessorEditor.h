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

class AudioParameterAttachment;

//==============================================================================
/** Base class for audio processor editors.

    This class provides the basic interface and functionality for audio processor editors,
    including handling resizing, aspect ratio preservation, and continuous rendering.
    It serves as a base class for creating custom audio processor editors within the YUP framework.
    Derived classes should implement the pure virtual methods to define their specific behavior.
    It also provides default implementations for certain behaviors, which can be overridden by
    derived classes as needed.

    @see AudioProcessor
*/
class YUP_API AudioProcessorEditor : public Component
{
public:
    //==============================================================================
    /** Constructor. */
    AudioProcessorEditor();
    
    /** Destructor. */
    virtual ~AudioProcessorEditor();

    //==============================================================================
    /** Returns whether this editor can be resized by the host or the user. */
    virtual bool isResizable() const = 0;

    /** Returns the preferred size of this editor. */
    virtual Size<int> getPreferredSize() const = 0;

    //==============================================================================
    /** Returns the size plugin wrappers and the standalone open this editor at.

        By default it returns getPreferredSize(). Resizable editors which remember the size the user left
        them at, for example in the processor state, return it here so each instance reopens at its own
        size. Wrappers pass the result through getConstrainedSize().
    */
    virtual Size<int> getInitialSize() const { return getPreferredSize(); }

    /** Returns the closest size to the requested one this editor accepts.

        Non resizable editors always return getPreferredSize(). Resizable editors which preserve their aspect
        ratio grow the requested size to match getPreferredAspectRatioSize(): a request wider than the ratio
        derives the height from the width, a taller one derives the width from the height.

        @param requestedSize  The size requested by the host or by the user.

        @returns The constrained size.
    */
    Size<int> getConstrainedSize (Size<int> requestedSize) const;

    /** Returns the largest size this editor accepts that fits inside the available one.

        Use this for sizes proposed by a host window, which the editor must fill without exceeding.
        Non resizable editors always return getPreferredSize(). Resizable editors which preserve their aspect
        ratio shrink the dimension that exceeds getPreferredAspectRatioSize(), never growing the other one.

        @param availableSize  The size offered by the host.

        @returns The constrained size.
    */
    Size<int> getConstrainedSizeToFit (Size<int> availableSize) const;

    //==============================================================================
    /** Returns whether this editor should preserve its aspect ratio when resized. */
    virtual bool shouldPreserveAspectRatio() const { return false; }

    /** Get the preferred aspect ratio size.

        This is used by resizable editors that want to maintain a specific aspect ratio.
        By default, it returns the preferred size.
    */
    virtual Size<int> getPreferredAspectRatioSize() const { return getPreferredSize(); }

    //==============================================================================
    /** Returns whether plugin wrappers should overlay a ResizableCorner on the editor.

        Hosts don't always let users resize plugin windows, so by default resizable editors get a corner
        handle in the bottom right that drives the size from within the plugin.
    */
    virtual bool shouldShowResizableCorner() const { return isResizable(); }

    //==============================================================================
    /** Returns whether this editor should render continuously. */
    virtual bool shouldRenderContinuous() const { return false; }

    //==============================================================================
    /** Services a plugin wrapper offers to its editor. */
    class YUP_API HostContext
    {
    public:
        virtual ~HostContext() = default;

        /** Asks the host to show its context menu for a parameter.

            @param parameter  The parameter the menu refers to.
            @param position   The position of the menu, in editor coordinates.

            @returns True when the host showed a menu, false when it offers none.
        */
        virtual bool showParameterContextMenu (const AudioParameter::Ptr& parameter, Point<float> position) = 0;
    };

    /** Returns the host context set by the plugin wrapper, or nullptr when the host offers none.

        Use it, for example, to show the host context menu when a control is right clicked.
    */
    HostContext* getHostContext() const noexcept { return hostContext; }

    /** Sets the host context, called by plugin wrappers. The context must outlive the editor or be reset. */
    void setHostContext (HostContext* newHostContext) noexcept { hostContext = newHostContext; }

    /** Returns the parameter controlled at a position, or nullptr when there is none.

        Hosts use this to find the control under the mouse, for example to assign it to a hardware
        controller (VST3 IParameterFinder). Override it to report the parameter of your controls.

        @param position  The position, in editor coordinates.
    */
    virtual AudioParameter::Ptr getParameterAt (Point<float> position) const;

    //==============================================================================
    /** Starts draining a processor's parameter-change queue to refresh controls reactively.

        Call this once, typically from the editor constructor, passing the processor the editor
        belongs to. A message-thread timer then drains the processor's ParameterChangeQueue at the
        given rate and notifies the AudioParameterAttachment objects bound to the changed parameters,
        so only the controls that moved are refreshed instead of polling every parameter every frame.
        Multiple editors share the drain, and every listening editor receives each change.
        The processor must outlive the editor. Call this and stopParameterListening() on
        the message thread.

        @param processor      The processor whose parameter changes drive this editor.
        @param refreshRateHz  How often, in Hz, to drain the queue on the message thread. A value of
                              zero or less binds the processor but starts no timer, leaving the caller
                              to drive dispatchPendingParameterChanges().

        @see stopParameterListening, AudioParameterAttachment, AudioProcessorBase::getParameterChangeQueue
    */
    void startParameterListening (AudioProcessorBase& processor, int refreshRateHz = 60);

    /** Stops the parameter-change timer started by startParameterListening. */
    void stopParameterListening();

    /** Drains the listened processor's parameter-change queue once and notifies attachments.

        The timer calls this on the message thread, and it is exposed so tests can drive the
        dispatch deterministically without waiting for the timer. Call on the message
        thread only. Reentrant dispatch is deferred until a later call.
    */
    void dispatchPendingParameterChanges();

    //==============================================================================
    /** @internal Registers an attachment so it is notified when its parameter changes.

        Called by AudioParameterAttachment; not intended for direct use.
    */
    void registerParameterAttachment (AudioParameter* parameter, AudioParameterAttachment* attachment);

    /** @internal Unregisters an attachment previously registered for a parameter. */
    void unregisterParameterAttachment (AudioParameter* parameter, AudioParameterAttachment* attachment);

private:
    friend class AudioProcessorBase;

    void parameterChanged (AudioParameter* parameter);

    HostContext* hostContext = nullptr;

    Size<int> constrainSize (Size<int> size, bool fitInside) const;

    AudioProcessorBase* listenedProcessor = nullptr;
    std::unique_ptr<TimedCallback> parameterDispatchTimer;
    std::unordered_map<AudioParameter*, ListenerList<AudioParameterAttachment>> parameterAttachments;
};

} // namespace yup
