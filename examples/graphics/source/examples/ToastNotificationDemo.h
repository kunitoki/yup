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

/**
    Demonstrates the cross-platform ToastNotification utility.

    - Simple: sends a two-line notification through sendNotification().
    - Rich: sends a ToastTemplate with an attribution line, two action
      buttons, a reminder scenario, a long duration and a 15 s expiration, and
      wires up the activation/dismissal callbacks.
    - With Image: sends an imageAndText02 template using the YUP logo from the
      demo assets.
    - Hide Last / Clear: hides the last shown notification id, or all
      notifications.

    Notifications are delivered by the platform backend (Windows toasts,
    Apple UserNotifications, Android NotificationManager, Linux
    notify-send/zenity, or the browser Web Notifications API on Emscripten),
    so the visible result depends on the OS and, on the web, on the user
    granting notification permission.
*/
class ToastNotificationDemo : public yup::Component
{
public:
    ToastNotificationDemo()
        : yup::Component ("ToastNotificationDemo")
    {
        auto* toasts = yup::ToastNotification::getInstance();
        toasts->setAppName ("YUP Graphics Demo");
        toasts->setAppUserModelId (yup::ToastNotification::configureAUMI ("org.yup", "yup_graphics"));

        // --- Title ---
        titleLabel = std::make_unique<yup::Label> ("titleLabel");
        titleLabel->setText ("Toast Notifications");
        addAndMakeVisible (*titleLabel);

        // --- Actions ---
        simpleButton = std::make_unique<yup::TextButton> ("Simple");
        simpleButton->onClick = [this]
        {
            auto* toasts = yup::ToastNotification::getInstance();

            toasts->sendNotification ("Hello from YUP",
                                      "This is a simple toast notification.",
                                      [this] (const yup::Result& result)
            {
                updateStatusAsync (result.wasOk()
                                       ? "Simple notification sent."
                                       : "Simple notification failed: " + result.getErrorMessage());
            },
                                      std::nullopt);
        };
        addAndMakeVisible (*simpleButton);

        richButton = std::make_unique<yup::TextButton> ("Rich");
        richButton->onClick = [this]
        {
            auto* toasts = yup::ToastNotification::getInstance();

            if (auto result = toasts->initialize(); result.failed())
            {
                updateStatus ("Initialization failed: " + result.getErrorMessage());
                return;
            }

            yup::ToastTemplate toast;
            toast.setFirstLine ("Backup complete");
            toast.setSecondLine ("42 files were synced.");
            toast.setAttributionText ("via YUP Graphics Demo");
            toast.addAction ("View");
            toast.addAction ("Dismiss");
            toast.setScenario (yup::ToastTemplate::Scenario::reminder);
            toast.setDuration (yup::ToastTemplate::Duration::long_);
            toast.setExpiration (15000);

            toast.onActivated = [this]
            {
                updateStatusAsync ("Toast activated.");
            };
            toast.onActivatedWithAction = [this] (int index)
            {
                updateStatusAsync ("Action " + yup::String (index) + " activated.");
            };
            toast.onDismissed = [this] (yup::ToastTemplate::DismissalReason)
            {
                updateStatusAsync ("Toast dismissed.");
            };
            toast.onFailed = [this]
            {
                updateStatusAsync ("Toast failed to display.");
            };

            auto result = toasts->showToast (toast);

            if (result.wasOk())
            {
                lastToastId = result.getValue();
                updateStatus ("Rich notification sent (id " + yup::String (lastToastId) + ").");
            }
            else
            {
                updateStatus ("Failed to send rich notification: " + result.getErrorMessage());
            }
        };
        addAndMakeVisible (*richButton);

        imageButton = std::make_unique<yup::TextButton> ("With Image");
        imageButton->onClick = [this]
        {
            auto* toasts = yup::ToastNotification::getInstance();

            if (auto result = toasts->initialize(); result.failed())
            {
                updateStatus ("Initialization failed: " + result.getErrorMessage());
                return;
            }

            yup::ToastTemplate toast (yup::ToastTemplate::TemplateType::imageAndText02);
            toast.setFirstLine ("Look at this image");
            toast.setSecondLine ("The YUP logo, straight from the demo assets.");
            toast.setImagePath (getAssetPath ("data/logo.png"));

            auto result = toasts->showToast (toast);

            // Report the resolved path too, so an unreachable asset is visible.
            const yup::String imagePath = toast.getImagePath().getFullPathName();

            if (result.wasOk())
            {
                lastToastId = result.getValue();
                updateStatus ("Image notification sent (id " + yup::String (lastToastId) + ") [img: " + imagePath + "].");
            }
            else
            {
                updateStatus ("Failed to send image notification: " + result.getErrorMessage() + " [img: " + imagePath + "]");
            }
        };
        addAndMakeVisible (*imageButton);

        hideButton = std::make_unique<yup::TextButton> ("Hide Last");
        hideButton->onClick = [this]
        {
            if (lastToastId < 0)
            {
                updateStatus ("No toast to hide yet.");
                return;
            }

            if (yup::ToastNotification::getInstance()->hideToast (lastToastId))
                updateStatus ("Hidden toast " + yup::String (lastToastId) + ".");
            else
                updateStatus ("Toast " + yup::String (lastToastId) + " not found (already gone).");
        };
        addAndMakeVisible (*hideButton);

        clearButton = std::make_unique<yup::TextButton> ("Clear");
        clearButton->onClick = [this]
        {
            yup::ToastNotification::getInstance()->clear();
            updateStatus ("Cleared all notifications.");
        };
        addAndMakeVisible (*clearButton);

        permissionButton = std::make_unique<yup::TextButton> ("Permission");
        permissionButton->onClick = [this]
        {
            yup::ToastNotification::requestPermission ([this] (yup::ToastNotification::PermissionState state)
            {
                updateStatusAsync (permissionStateDescription (state));
            });
        };
        addAndMakeVisible (*permissionButton);

        hintLabel = std::make_unique<yup::Label> ("hintLabel");
        hintLabel->setText ("Delivered by the platform notification backend; on the web, permission must be granted first.");
        addAndMakeVisible (*hintLabel);

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        addAndMakeVisible (*statusLabel);
        updateStatus ("Ready.");
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        const int pad = 4;
        const int btnH = 26;
        const int lblH = 18;

        // Title
        titleLabel->setBounds (area.removeFromTop (24));
        area.removeFromTop (8);

        // First row: Simple / Rich / With Image
        auto row1 = area.removeFromTop (btnH);
        simpleButton->setBounds (row1.removeFromLeft (90).reduced (0, 2));
        row1.removeFromLeft (pad);
        richButton->setBounds (row1.removeFromLeft (90).reduced (0, 2));
        row1.removeFromLeft (pad);
        imageButton->setBounds (row1.removeFromLeft (100).reduced (0, 2));

        area.removeFromTop (pad);

        // Second row: Permission / Hide Last / Clear
        auto row2 = area.removeFromTop (btnH);
        permissionButton->setBounds (row2.removeFromLeft (100).reduced (0, 2));
        row2.removeFromLeft (pad);
        hideButton->setBounds (row2.removeFromLeft (90).reduced (0, 2));
        row2.removeFromLeft (pad);
        clearButton->setBounds (row2.removeFromLeft (90).reduced (0, 2));

        area.removeFromTop (10);

        // Hint text
        hintLabel->setBounds (area.removeFromTop (lblH * 2));

        // Status at the bottom
        statusLabel->setBounds (getLocalBounds().reduced (10).removeFromBottom (20));
    }

private:
    static yup::String permissionStateDescription (yup::ToastNotification::PermissionState state)
    {
        switch (state)
        {
            case yup::ToastNotification::PermissionState::notDetermined:
                return "Permission not determined yet.";
            case yup::ToastNotification::PermissionState::granted:
                return "Permission granted.";
            case yup::ToastNotification::PermissionState::denied:
                return "Permission denied.";
        }

        return {};
    }

    void updateStatusAsync (const yup::String& text)
    {
        // Notification callbacks can arrive on a platform-dependent thread.
        yup::MessageManager::callAsync ([this, text]
        {
            updateStatus (text);
        });
    }

    void updateStatus (const yup::String& text)
    {
        statusLabel->setText (text, yup::dontSendNotification);
    }

    // Widgets
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> hintLabel;
    std::unique_ptr<yup::Label> statusLabel;

    std::unique_ptr<yup::TextButton> simpleButton;
    std::unique_ptr<yup::TextButton> richButton;
    std::unique_ptr<yup::TextButton> imageButton;
    std::unique_ptr<yup::TextButton> hideButton;
    std::unique_ptr<yup::TextButton> clearButton;
    std::unique_ptr<yup::TextButton> permissionButton;

    // Data
    yup::int64 lastToastId = -1;
};
