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
const Identifier ProgressBar::Style::backgroundColorId { "backgroundColorId" };
const Identifier ProgressBar::Style::foregroundColorId { "foregroundColorId" };

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
    // Check if entering indeterminate mode
    if (newProgress < 0.0)
    {
        const bool wasIndeterminate = indeterminate.get();
        indeterminate.set (true);
        currentProgress.set (-1.0);

        if (! wasIndeterminate)
        {
            animationPhase = 0.0;
            lastAnimationTime = Time::getCurrentTime();
        }

        updateProgress (-1.0, notification);
        return;
    }

    // Normal progress mode - clamp to valid range
    newProgress = jlimit (0.0, 1.0, newProgress);

    const bool wasIndeterminate = indeterminate.get();
    indeterminate.set (false);

    const double oldProgress = currentProgress.get();
    currentProgress.set (newProgress);

    if (wasIndeterminate || ! approximatelyEqual (oldProgress, newProgress))
    {
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
