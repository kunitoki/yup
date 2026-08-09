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
ChannelSection::ChannelRowComponent::ChannelRowComponent()
{
    setOpaque (false);

    addAndMakeVisible (toggle);

    nameLabel.setColor (Label::Style::textFillColorId, Colors::black);
    addAndMakeVisible (nameLabel);
}

void ChannelSection::ChannelRowComponent::setup (const String& name,
                                                 bool active,
                                                 std::function<void (bool)> onToggled)
{
    toggle.setButtonText ({});
    toggle.setToggleState (active, dontSendNotification);
    toggle.onClick = [this, onToggled]
    {
        if (onToggled != nullptr)
            onToggled (toggle.getToggleState());
    };
    nameLabel.setText (name, dontSendNotification);
}

void ChannelSection::ChannelRowComponent::resized()
{
    auto bounds = getLocalBounds().reduced (2, 2);
    const int toggleW = bounds.getHeight();
    toggle.setBounds (bounds.removeFromLeft (toggleW));
    bounds.removeFromLeft (4);
    nameLabel.setBounds (bounds);
}

//==============================================================================
void ChannelSection::Model::setChannels (const StringArray& names, const BigInteger& active)
{
    channelNames = names;
    activeChannels = active;
}

BigInteger ChannelSection::Model::getActiveChannels() const
{
    return activeChannels;
}

int ChannelSection::Model::getNumRows()
{
    return channelNames.size();
}

Component* ChannelSection::Model::refreshComponentForRow (int rowIndex, Component* existing)
{
    auto* row = dynamic_cast<ChannelRowComponent*> (existing);
    if (row == nullptr)
        row = new ChannelRowComponent();

    const bool isActive = activeChannels[rowIndex];

    row->setup (channelNames[rowIndex], isActive, [this, rowIndex] (bool on)
    {
        if (on)
            activeChannels.setBit (rowIndex);
        else
            activeChannels.clearBit (rowIndex);

        if (onChannelsChanged != nullptr)
            onChannelsChanged (activeChannels);
    });

    return row;
}

//==============================================================================
ChannelSection::ChannelSection()
{
    setOpaque (false);

    listBox.setModel (&model);
    addAndMakeVisible (sectionLabel);
    addAndMakeVisible (listBox);
}

ChannelSection::~ChannelSection() = default;

void ChannelSection::setText (const String& text, NotificationType notification)
{
    sectionLabel.setText (text, notification);
}

void ChannelSection::populate (const StringArray& channelNames, const BigInteger& activeChannels)
{
    model.onChannelsChanged = [this] (const BigInteger& active)
    {
        if (onChannelsChanged != nullptr)
            onChannelsChanged (active);
    };

    model.setChannels (channelNames, activeChannels);
    listBox.updateContent();
}

BigInteger ChannelSection::getActiveChannels() const
{
    return model.getActiveChannels();
}

void ChannelSection::resized()
{
    auto bounds = getLocalBounds();
    sectionLabel.setBounds (bounds.removeFromTop (20));
    listBox.setBounds (bounds.reduced (2, 2));
}

} // namespace yup
