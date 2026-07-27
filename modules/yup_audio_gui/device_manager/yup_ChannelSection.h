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
/** A ListBox that shows audio channels as toggleable rows.

    Bit N of the BigInteger corresponds to channelNames[N].
*/
class YUP_API ChannelSection : public Component
{
public:
    //==============================================================================
    ChannelSection();
    ~ChannelSection() override;

    //==============================================================================
    /** Sets the label text displayed above the channel list. */
    void setText (const String& text, NotificationType notification = dontSendNotification);

    /** Populates the list from channel names and an active-channel bitmask. */
    void populate (const StringArray& channelNames, const BigInteger& activeChannels);

    /** Returns the current active-channel bitmask as edited by the user. */
    BigInteger getActiveChannels() const;

    //==============================================================================
    /** Called when the user toggles any channel. */
    std::function<void (const BigInteger& active)> onChannelsChanged;

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    //==============================================================================
    class ChannelRowComponent : public Component
    {
    public:
        ChannelRowComponent();
        void setup (const String& name, bool active, std::function<void (bool)> onToggled);
        void resized() override;

    private:
        ToggleButton toggle;
        Label nameLabel;
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelRowComponent)
    };

    //==============================================================================
    class Model : public ListBoxModel
    {
    public:
        Model() = default;

        void setChannels (const StringArray& names, const BigInteger& active);
        BigInteger getActiveChannels() const;
        std::function<void (const BigInteger&)> onChannelsChanged;

        int getNumRows() override;
        Component* refreshComponentForRow (int rowIndex, Component* existing) override;

    private:
        StringArray channelNames;
        BigInteger activeChannels;
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
    };

    //==============================================================================
    Label sectionLabel;
    ListBox listBox;
    Model model;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelSection)
};

} // namespace yup
