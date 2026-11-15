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

namespace
{

struct ProgressBarIds
{
    static inline const Identifier progress { "progress" };
    static inline const Identifier indeterminate { "indeterminate" };
};

} // namespace

//==============================================================================
const Identifier ProgressBar::Style::backgroundColorId { "progressBarBackground" };
const Identifier ProgressBar::Style::foregroundColorId { "progressBarForeground" };
const Identifier ProgressBar::Style::cornerRadiusId { "progressBarCornerRadius" };

//==============================================================================
const ComponentTypeInfo& ProgressBar::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ProgressBar> (Identifier ("ProgressBar"))
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("foregroundColor", Style::foregroundColorId)
                                 .number ("cornerRadius", Style::cornerRadiusId, {}, "The corner radius, unset for a pill.")
                                 .property (ProgressBarIds::progress, &ProgressBar::getProgress, [] (ProgressBar& p, double v) { p.setProgress (v); })
                                 .readOnly (ProgressBarIds::indeterminate, &ProgressBar::isIndeterminate)
                                 .build();

    return info;
}

//==============================================================================
ProgressBar::ProgressBar (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
    lastAnimationTime = Time::getCurrentTime();
}

ProgressBar::~ProgressBar()
{
}

//==============================================================================
void ProgressBar::setProgress (double newProgress, NotificationType notification)
{
    const bool wasIndeterminate = indeterminate.get();
    const double oldProgress = currentProgress.get();

    // Check if entering indeterminate mode
    if (newProgress < 0.0)
    {
        indeterminate.set (true);
        currentProgress.set (-1.0);

        if (! wasIndeterminate)
        {
            animationPhase = 0.0;
            lastAnimationTime = Time::getCurrentTime();
        }

        const BailOutChecker bailOutChecker (this);

        if (! approximatelyEqual (oldProgress, -1.0))
        {
            notifyPropertyChanged (ProgressBarIds::progress);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        if (! wasIndeterminate)
        {
            notifyPropertyChanged (ProgressBarIds::indeterminate);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        updateProgress (-1.0, notification);
        return;
    }

    // Normal progress mode - clamp to valid range
    newProgress = jlimit (0.0, 1.0, newProgress);
    indeterminate.set (false);
    currentProgress.set (newProgress);

    const bool progressDidChange = ! approximatelyEqual (oldProgress, newProgress);

    if (wasIndeterminate || progressDidChange)
    {
        const BailOutChecker bailOutChecker (this);

        if (progressDidChange)
        {
            notifyPropertyChanged (ProgressBarIds::progress);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        if (wasIndeterminate)
        {
            notifyPropertyChanged (ProgressBarIds::indeterminate);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        updateProgress (newProgress, notification);
    }
}

double ProgressBar::getProgress() const noexcept
{
    return currentProgress.get();
}

bool ProgressBar::isIndeterminate() const noexcept
{
    return indeterminate.get();
}

//==============================================================================
void ProgressBar::progressChanged()
{
}

//==============================================================================
void ProgressBar::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
    {
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
    }
}

void ProgressBar::refreshDisplay (double lastFrameTimeSeconds)
{
    Component::refreshDisplay (lastFrameTimeSeconds);

    // Animate in indeterminate mode
    if (indeterminate.get())
    {
        auto currentTime = Time::getCurrentTime();
        auto deltaTime = (currentTime - lastAnimationTime).inSeconds();
        lastAnimationTime = currentTime;

        // Update animation phase (full cycle every 2 seconds)
        animationPhase += deltaTime * 0.5;
        while (animationPhase >= 1.0)
            animationPhase -= 1.0;

        repaint();
    }
}

//==============================================================================
void ProgressBar::updateProgress (double newProgress, NotificationType notification)
{
    WeakReference<Component> self = this;

    MessageManager::callAsync ([this, self, notification]
    {
        if (self.get() == nullptr)
            return;

        repaint();
        sendProgressChanged (notification);
    });
}

void ProgressBar::sendProgressChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        progressChanged();

        if (onProgressChanged)
            onProgressChanged (currentProgress.get());
    });
}

} // namespace yup
