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

#include <functional>
#include <memory>
#include <vector>

#if YUP_DESKTOP
//==============================================================================
/**
    The plugin preferences: scanning mode, sandboxing default and the scan failures.

    The page only talks to the database directly; anything that needs the app (rescanning,
    changing the scan mode) goes through the callbacks.
*/
class PluginSettingsPage final
    : public yup::Component
    , private yup::ListBoxModel
{
public:
    explicit PluginSettingsPage (std::shared_ptr<yup::AudioPluginDatabase> databaseToUse)
        : database (std::move (databaseToUse))
    {
        outOfProcessToggle.setButtonText ("Scan plugins out of process");
        outOfProcessToggle.onClick = [this]
        {
            if (onOutOfProcessScanningChanged != nullptr)
                onOutOfProcessScanningChanged (outOfProcessToggle.getToggleState());
        };
        addAndMakeVisible (outOfProcessToggle);

        sandboxToggle.setButtonText ("Run plugins sandboxed by default");
        sandboxToggle.onClick = [this]
        {
            if (database != nullptr)
                database->setDefaultRunSandboxed (sandboxToggle.getToggleState());

            notifyPluginsChanged();
        };
        addAndMakeVisible (sandboxToggle);

        rescanButton.setButtonText ("Rescan");
        rescanButton.onClick = [this]
        {
            if (onRescanRequested != nullptr)
                onRescanRequested();
        };
        addAndMakeVisible (rescanButton);

        clearBlacklistButton.setButtonText ("Clear Blacklist");
        clearBlacklistButton.onClick = [this]
        {
            if (database != nullptr)
                database->clearBlacklist();

            refresh();
        };
        addAndMakeVisible (clearBlacklistButton);

        pruneButton.setButtonText ("Remove Missing");
        pruneButton.onClick = [this]
        {
            if (database != nullptr)
                database->pruneMissingFiles();

            refresh();
            notifyPluginsChanged();
        };
        addAndMakeVisible (pruneButton);

        addAndMakeVisible (summaryLabel);

        failuresTitle.setText ("Scan failures", yup::dontSendNotification);
        addAndMakeVisible (failuresTitle);

        failuresList.setModel (this);
        failuresList.setDragSourceEnabled (false);
        addAndMakeVisible (failuresList);
    }

    ~PluginSettingsPage() override
    {
        failuresList.setModel (nullptr);
    }

    /** Re-reads the database and the current scan mode. */
    void refresh()
    {
        const bool outOfProcess = isOutOfProcessScanning != nullptr && isOutOfProcessScanning();
        outOfProcessToggle.setToggleState (outOfProcess, yup::dontSendNotification);

        failures.clear();
        int numPlugins = 0;

        if (database != nullptr && database->isOpen())
        {
            sandboxToggle.setToggleState (database->getDefaultRunSandboxed(), yup::dontSendNotification);
            failures = database->getFailures();
            numPlugins = static_cast<int> (database->getPlugins().size());
        }

        const auto numBlacklisted = std::count_if (failures.begin(), failures.end(), [] (const auto& failure)
        {
            return failure.blacklisted;
        });

        summaryLabel.setText (yup::String (numPlugins) + " plugins known, "
                                  + yup::String (static_cast<int> (failures.size())) + " failures ("
                                  + yup::String (static_cast<int> (numBlacklisted)) + " blacklisted)",
                              yup::dontSendNotification);

        failuresList.updateContent();
    }

    /** Called when the user asks for a new scan. */
    std::function<void()> onRescanRequested;

    /** Returns whether scanning currently happens in a helper process. */
    std::function<bool()> isOutOfProcessScanning;

    /** Called when the user switches the scan mode. */
    std::function<void (bool)> onOutOfProcessScanningChanged;

    /** Called when the set of plugins or their sandboxing changed. */
    std::function<void()> onPluginsChanged;

    //==============================================================================
    void visibilityChanged() override
    {
        if (isVisible())
            refresh();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (16.0f);

        outOfProcessToggle.setBounds (bounds.removeFromTop (28.0f));
        sandboxToggle.setBounds (bounds.removeFromTop (28.0f));
        bounds.removeFromTop (12.0f);

        auto buttons = bounds.removeFromTop (30.0f);
        rescanButton.setBounds (buttons.removeFromLeft (110.0f));
        buttons.removeFromLeft (8.0f);
        clearBlacklistButton.setBounds (buttons.removeFromLeft (130.0f));
        buttons.removeFromLeft (8.0f);
        pruneButton.setBounds (buttons.removeFromLeft (140.0f));
        bounds.removeFromTop (12.0f);

        summaryLabel.setBounds (bounds.removeFromTop (24.0f));
        bounds.removeFromTop (8.0f);
        failuresTitle.setBounds (bounds.removeFromTop (24.0f));
        failuresList.setBounds (bounds);
    }

private:
    int getNumRows() override { return static_cast<int> (failures.size()); }

    yup::String getRowText (int rowIndex) override
    {
        if (! yup::isPositiveAndBelow (rowIndex, getNumRows()))
            return {};

        const auto& failure = failures[static_cast<size_t> (rowIndex)];
        auto text = failure.file.getFileName() + " - " + reasonToString (failure.reason);

        if (failure.message.isNotEmpty())
            text << ": " << failure.message;

        if (failure.blacklisted)
            text << "  [blacklisted]";

        return text;
    }

    static yup::String reasonToString (yup::AudioPluginDatabase::FailureReason reason)
    {
        switch (reason)
        {
            case yup::AudioPluginDatabase::FailureReason::crash:
                return "crashed";

            case yup::AudioPluginDatabase::FailureReason::timeout:
                return "timed out";

            default:
                return "failed";
        }
    }

    void notifyPluginsChanged()
    {
        if (onPluginsChanged != nullptr)
            onPluginsChanged();
    }

    std::shared_ptr<yup::AudioPluginDatabase> database;
    std::vector<yup::AudioPluginDatabase::Failure> failures;

    yup::ToggleButton outOfProcessToggle;
    yup::ToggleButton sandboxToggle;
    yup::TextButton rescanButton;
    yup::TextButton clearBlacklistButton;
    yup::TextButton pruneButton;
    yup::Label summaryLabel;
    yup::Label failuresTitle;
    yup::ListBox failuresList;
};

#endif

//==============================================================================
/**
    The application preferences, as a workspace page with its sections listed on the left.

    The audio device settings are always the first section; more are added with addSection().
*/
class SettingsPanel final : public yup::TabComponent
{
public:
    explicit SettingsPanel (yup::AudioDeviceManager& deviceManager)
    {
        setTabBarPlacement (Placement::left);
        setTabBarThickness (160.0f);

        auto audioPage = std::make_unique<yup::AudioDeviceManagerPanel> (deviceManager);
        midiDeviceListConnection = audioPage->makeMidiDeviceListConnection();
        addSection ("audio", "Audio", std::move (audioPage));
    }

    /** Adds a section, listed under @p name. */
    void addSection (const yup::Identifier& sectionId, const yup::String& name, std::unique_ptr<yup::Component> page)
    {
        addTab (sectionId, name, std::move (page));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff0d1117));
        g.fillAll();
    }

private:
    yup::MidiDeviceListConnection midiDeviceListConnection;
};
