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
/** The content of a standalone plugin window.

    Shows a slim toolbar with a settings button (and a keyboard toggle for plugins
    that play notes), the plugin editor or a placeholder when there is none, an
    optional on-screen keyboard, and the audio and MIDI settings as an overlay
    drawn inside the window.
*/
class StandaloneContent final : public Component
{
public:
    //==============================================================================
    /** Creates the content and the plugin editor.

        @param processor      the processor whose editor is shown
        @param deviceManager  the device manager the settings overlay configures
        @param midiCollector  the collector the on-screen keyboard plays into
        @param canPlayNotes   true to offer the on-screen keyboard
    */
    StandaloneContent (AudioProcessor& processor,
                       AudioDeviceManager& deviceManager,
                       MidiMessageCollector& midiCollector,
                       bool canPlayNotes)
        : Component ("StandaloneContent")
        , deviceManager (deviceManager)
        , midiCollector (midiCollector)
        , playsNotes (canPlayNotes)
        , keyboard (keyboardState, MidiKeyboardComponent::horizontalKeyboard)
    {
        settingsButton.setButtonText ("Settings");
        settingsButton.onClick = [this]
        {
            showSettings();
        };
        addAndMakeVisible (settingsButton);

        keyboardButton.setButtonText ("Keyboard");
        keyboardButton.onClick = [this]
        {
            setKeyboardVisible (! isKeyboardVisible());
        };
        addChildComponent (keyboardButton);
        keyboardButton.setVisible (playsNotes);

        keyboardState.addListener (&midiCollector);
        keyboard.setMidiChannel (1);
        addChildComponent (keyboard);

        if (processor.hasEditor())
            editor.reset (processor.createEditor());

        if (editor != nullptr)
        {
            addAndMakeVisible (*editor);
        }
        else
        {
            placeholder.setText ("This plugin has no editor", dontSendNotification);
            addAndMakeVisible (placeholder);
        }
    }

    ~StandaloneContent() override
    {
        overlay.reset();

        keyboardState.removeListener (&midiCollector);
    }

    //==============================================================================
    /** Shows the audio and MIDI settings overlay. */
    void showSettings()
    {
        if (overlay != nullptr)
            return;

        overlay = std::make_unique<SettingsOverlay> (deviceManager);
        overlay->onDismiss = [weakThis = WeakReference<Component> (this)]
        {
            // Dismissal starts inside the overlay's own event handlers, so it is deleted afterwards.
            MessageManager::callAsync ([weakThis]
            {
                if (auto* content = static_cast<StandaloneContent*> (weakThis.get()))
                    content->hideSettings();
            });
        };

        addAndMakeVisible (*overlay);
        overlay->setBounds (getLocalBounds());
        overlay->takeKeyboardFocus();
    }

    /** Hides the settings overlay, calling onSettingsClosed if it was showing. */
    void hideSettings()
    {
        if (overlay == nullptr)
            return;

        removeChildComponent (*overlay);
        overlay.reset();

        if (onSettingsClosed)
            onSettingsClosed();
    }

    /** Returns true while the settings overlay is showing. */
    bool isShowingSettings() const noexcept { return overlay != nullptr; }

    //==============================================================================
    /** Shows or hides the toolbar. */
    void setToolbarVisible (bool shouldBeVisible)
    {
        if (toolbarVisible == shouldBeVisible)
            return;

        toolbarVisible = shouldBeVisible;
        settingsButton.setVisible (toolbarVisible);
        keyboardButton.setVisible (toolbarVisible && playsNotes);

        preferredSizeChanged();
    }

    /** Returns true if the toolbar is visible. */
    bool isToolbarVisible() const noexcept { return toolbarVisible; }

    /** Shows or hides the on-screen keyboard. It is only offered for plugins that play notes. */
    void setKeyboardVisible (bool shouldBeVisible)
    {
        shouldBeVisible = shouldBeVisible && playsNotes;

        if (keyboard.isVisible() == shouldBeVisible)
            return;

        keyboard.setVisible (shouldBeVisible);
        preferredSizeChanged();
    }

    /** Returns true if the on-screen keyboard is visible. */
    bool isKeyboardVisible() const noexcept { return keyboard.isVisible(); }

    //==============================================================================
    /** Returns the editor, or nullptr when the plugin has none. */
    AudioProcessorEditor* getEditor() const noexcept { return editor.get(); }

    /** Returns the size that fits the editor, the toolbar and the keyboard. */
    Size<int> getPreferredSize() const
    {
        const auto editorSize = editor != nullptr ? editor->getPreferredSize() : Size<int> (400, 300);

        return { editorSize.getWidth(),
                 editorSize.getHeight()
                     + (toolbarVisible ? toolbarHeight : 0)
                     + (isKeyboardVisible() ? keyboardHeight : 0) };
    }

    /** Returns the width over height the window should keep, when the editor keeps its
        aspect ratio: the preferred size, toolbar and keyboard included. The toolbar and
        keyboard keep their heights as the window scales, so the editor fits itself into
        what is left.
    */
    std::optional<float> getPreferredAspectRatio() const
    {
        if (editor == nullptr || ! editor->shouldPreserveAspectRatio())
            return std::nullopt;

        const auto editorSize = editor->getPreferredAspectRatioSize();
        const auto height = editorSize.getHeight() + (toolbarVisible ? toolbarHeight : 0) + (isKeyboardVisible() ? keyboardHeight : 0);

        if (editorSize.getWidth() <= 0 || height <= 0)
            return std::nullopt;

        return static_cast<float> (editorSize.getWidth()) / static_cast<float> (height);
    }

    //==============================================================================
    /** Called when getPreferredSize() changes, so the window can follow. */
    std::function<void()> onPreferredSizeChanged;

    /** Called after the settings overlay is closed. */
    std::function<void()> onSettingsClosed;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override
    {
        g.setFillColor (ApplicationTheme::findComponentColor (*this, DocumentWindow::Style::backgroundColorId).value_or (Colors::dimgray));
        g.fillAll();
    }

    /** @internal */
    void resized() override
    {
        auto bounds = getLocalBounds();

        if (toolbarVisible)
        {
            auto toolbar = bounds.removeFromTop (static_cast<float> (toolbarHeight)).reduced (4.0f, 3.0f);
            settingsButton.setBounds (toolbar.removeFromRight (90.0f));
            toolbar.removeFromRight (4.0f);
            keyboardButton.setBounds (toolbar.removeFromRight (90.0f));
        }

        if (isKeyboardVisible())
            keyboard.setBounds (bounds.removeFromBottom (static_cast<float> (keyboardHeight)));

        if (editor != nullptr)
            editor->setBounds (bounds);
        else
            placeholder.setBounds (bounds.withSizeKeepingCenter (220.0f, 24.0f));

        if (overlay != nullptr)
            overlay->setBounds (getLocalBounds());
    }

private:
    //==============================================================================
    class SettingsOverlay final : public Component
    {
    public:
        explicit SettingsOverlay (AudioDeviceManager& manager)
            : Component ("StandaloneSettingsOverlay")
            , panel (manager)
        {
            setOpaque (false);
            setWantsKeyboardFocus (true);

            panel.onClose = [this]
            {
                dismiss();
            };
            panel.onPreferredHeightChanged = [this]
            {
                resized();
                repaint();
            };
            addAndMakeVisible (panel);
            midiDeviceListConnection = panel.makeMidiDeviceListConnection();
        }

        std::function<void()> onDismiss;

        void paint (Graphics& g) override
        {
            g.setFillColor (Colors::black.withAlpha (0.6f));
            g.fillAll();
        }

        void resized() override
        {
            const auto area = getLocalBounds();

            panel.setBounds (area.withSizeKeepingCenter (jmin (area.getWidth(), panelWidth),
                                                         jmin (area.getHeight(), panel.getPreferredHeight())));
        }

        void mouseDown (const MouseEvent& event) override
        {
            if (! panel.getBounds().contains (event.getPosition()))
                dismiss();
        }

        bool keyDown (const KeyPress& key, const Point<float>&) override
        {
            if (key.getKey() != KeyPress::escapeKey)
                return false;

            dismiss();
            return true;
        }

    private:
        void dismiss()
        {
            if (onDismiss)
                onDismiss();
        }

        // Includes the transparent margin the panel keeps around its card for the drop shadow.
        static constexpr float panelWidth = 520.0f;

        AudioDeviceManagerPanel panel;
        MidiDeviceListConnection midiDeviceListConnection;
    };

    //==============================================================================
    void preferredSizeChanged()
    {
        resized();

        if (onPreferredSizeChanged)
            onPreferredSizeChanged();
    }

    //==============================================================================
    static constexpr int toolbarHeight = 28;
    static constexpr int keyboardHeight = 80;

    AudioDeviceManager& deviceManager;
    MidiMessageCollector& midiCollector;
    const bool playsNotes;

    MidiKeyboardState keyboardState;
    MidiKeyboardComponent keyboard;

    TextButton settingsButton;
    TextButton keyboardButton;
    Label placeholder;

    std::unique_ptr<AudioProcessorEditor> editor;
    std::unique_ptr<SettingsOverlay> overlay;

    bool toolbarVisible = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneContent)
};

} // namespace yup
