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

    Embed in an AudioDeviceManagerWindow or add directly to any Component.
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
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;

private:
    void populateFromManager();
    void repopulateDeviceDependent();
    void applyToManager();
    void handleAsyncUpdate() override;

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

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceManagerPanel)
};

} // namespace yup
