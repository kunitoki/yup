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
/**
    A progress bar component that displays progress from 0% to 100%.

    The ProgressBar supports three modes:
    - Normal mode (0.0 to 1.0): Shows a filled bar representing the progress
    - Indeterminate mode (-1.0): Shows an animated bar with moving ribbons

    The component is thread-safe and can be updated from any thread. Updates
    from non-GUI threads are automatically marshalled to the message thread.

    The appearance is controlled by the ApplicationTheme, with customizable
    colors for the background and foreground (filled portion).

    @see Component, ApplicationTheme
*/
class YUP_API ProgressBar : public Component
{
public:
    //==============================================================================
    /** Style identifiers for theme customization. */
    struct Style
    {
        /** The background color of the progress bar track. */
        static const Identifier backgroundColorId;

        /** The foreground color of the filled progress indicator. */
        static const Identifier foregroundColorId;
    };

    //==============================================================================
    /** Creates a ProgressBar.

        @param componentID  An optional identifier for this component
    */
    explicit ProgressBar (StringRef componentID = {});

    /** Destructor. */
    ~ProgressBar() override;

    //==============================================================================
    /** Sets the current progress value.

        The value should be in the range 0.0 (0%) to 1.0 (100%). Values outside
        this range will be clamped. Pass -1.0 to enter indeterminate mode with
        animated moving ribbons.

        This method is thread-safe and can be called from any thread.

        @param newProgress      The new progress value (0.0 to 1.0, or -1.0 for indeterminate)
        @param notification     Whether to notify listeners of the change
    */
    void setProgress (double newProgress, NotificationType notification = sendNotificationAsync);

    /** Returns the current progress value.

        @return The current progress (0.0 to 1.0), or -1.0 if in indeterminate mode
    */
    double getProgress() const noexcept;

    /** Returns whether the progress bar is in indeterminate mode.

        @return true if in indeterminate mode (showing animated ribbons)
    */
    bool isIndeterminate() const noexcept;

    //==============================================================================
    /** Called when the progress value changes.

        Override this method to be notified when the progress changes.
    */
    virtual void progressChanged();

    /** Callback that's invoked when the progress changes.

        You can assign a lambda or function to this to be notified of changes:
        @code
        progressBar.onProgressChanged = [](double progress)
        {
            DBG ("Progress: " << (progress * 100.0) << "%");
        };
        @endcode
    */
    std::function<void (double)> onProgressChanged;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;

    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;

private:
    //==============================================================================
    void updateProgress (double newProgress, NotificationType notification);
    void sendProgressChanged (NotificationType notification);

    //==============================================================================
    Atomic<double> currentProgress { 0.0 };
    Atomic<bool> indeterminate { false };

    // Animation state for indeterminate mode
    double animationPhase = 0.0;
    Time lastAnimationTime;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgressBar)
};

} // namespace yup
