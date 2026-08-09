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
/** MIDI input list (toggleable rows) and MIDI output selector. */
class YUP_API MidiSection : public Component
{
public:
    //==============================================================================
    MidiSection();
    ~MidiSection() override;

    //==============================================================================
    /** Populates the MIDI input list.
        @param devices     All available MIDI input devices.
        @param enabledIds  Identifiers of currently-enabled MIDI inputs.
    */
    void populateInputs (const Array<MidiDeviceInfo>& devices, const StringArray& enabledIds);

    /** Populates the MIDI output ComboBox.
        @param devices    All available MIDI output devices.
        @param selectedId Identifier of the currently selected MIDI output (empty = none).
    */
    void populateOutput (const Array<MidiDeviceInfo>& devices, const String& selectedId);

    //==============================================================================
    /** Called when the user toggles any MIDI input. Receives the full set of enabled IDs. */
    std::function<void (const StringArray& enabledIds)> onInputsChanged;

    /** Called when the user picks a different MIDI output. */
    std::function<void (const String& outputId)> onOutputChanged;

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    //==============================================================================
    class MidiInputRowComponent : public Component
    {
    public:
        MidiInputRowComponent();
        void setup (const String& name, bool enabled, std::function<void (bool)> onToggled);
        void resized() override;

    private:
        ToggleButton toggle;
        Label nameLabel;
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputRowComponent)
    };

    //==============================================================================
    class InputModel : public ListBoxModel
    {
    public:
        InputModel() = default;

        void setDevices (const Array<MidiDeviceInfo>& devices, const StringArray& enabledIds);
        StringArray getEnabledIds() const;
        std::function<void (const StringArray&)> onInputsChanged;

        int getNumRows() override;
        Component* refreshComponentForRow (int rowIndex, Component* existing) override;

    private:
        Array<MidiDeviceInfo> midiDevices;
        StringArray enabledIds;
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputModel)
    };

    //==============================================================================
    Label inputsLabel;
    ListBox inputsListBox;
    InputModel inputsModel;
    Label outputLabel;
    ComboBox outputCombo;

    Array<MidiDeviceInfo> outputDevices;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSection)
};

} // namespace yup
