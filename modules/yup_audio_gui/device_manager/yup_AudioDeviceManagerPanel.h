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
/** A composable panel for configuring an AudioDeviceManager.

    Changes are applied to the AudioDeviceManager immediately as the user
    makes selections — no Apply button required.

    Embed in an AudioDeviceManagerWindow or add directly to any Component. Once it
    has a parent, the panel draws itself as a rounded card with a soft drop shadow,
    inside a transparent margin of its own bounds.
*/
class YUP_API AudioDeviceManagerPanel : public Component
    , private AsyncUpdater
{
public:
    //==============================================================================
    explicit AudioDeviceManagerPanel (AudioDeviceManager& manager);

    //==============================================================================
    /** Creates a MidiDeviceListConnection that keeps the MIDI device list in sync.

        The caller must hold the returned connection for as long as automatic
        refresh is desired. Destroying the connection stops notifications.
        AudioDeviceManagerWindow does this automatically; call it yourself when
        embedding the panel directly.
    */
    MidiDeviceListConnection makeMidiDeviceListConnection();

    //==============================================================================
    /** The height of every item in the panel: labels, selectors, buttons and list rows. */
    static constexpr float itemHeight = 28.0f;

    /** The space between stacked items. */
    static constexpr float itemGap = 4.0f;

    /** Returns the height that shows every row of every list.

        When the panel is given less, the lists shrink, longest first, down to one
        row each and scroll the rest.
    */
    float getPreferredHeight() const;

    /** Called when getPreferredHeight() changes, for example after selecting another device. */
    std::function<void()> onPreferredHeightChanged;

    /** Called when the Close button is clicked. The button is only shown while this is set. */
    std::function<void()> onClose;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void parentHierarchyChanged() override;

private:
    void populateFromManager();
    void repopulateDeviceDependent();
    void applyToManager();
    void handleAsyncUpdate() override;
    void listsChanged();
    std::array<int, 3> getNaturalListRows() const;
    std::array<int, 3> fitListRows (float height) const;
    float getFixedHeight() const noexcept;
    bool hasShadow() const noexcept;
    Rectangle<float> getCardBounds() const;

    static constexpr float margin = 8.0f;
    static constexpr int numFixedItems = 9;
    static constexpr int numGaps = 8;
    static constexpr float cornerRadius = 10.0f;
    static constexpr float shadowInsetSides = 24.0f;
    static constexpr float shadowInsetTop = 16.0f;
    static constexpr float shadowInsetBottom = 32.0f;

    //==============================================================================
    AudioDeviceManager& manager;

    AudioDeviceManager::AudioDeviceSetup stagedSetup;
    StringArray stagedMidiInputIds;
    String stagedMidiOutputId;

    //==============================================================================
    DeviceTypeSelector deviceTypeSelector;
    DeviceIOSelector deviceIOSelector;
    ChannelSection outputChannelSection;
    ChannelSection inputChannelSection;
    RateBufferSelector rateBufferSelector;
    MidiSection midiSection;
    TextButton closeButton;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceManagerPanel)
};

} // namespace yup
