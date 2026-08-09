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

RateBufferSelector::RateBufferSelector()
{
    setOpaque (false);

    rateLabel.setText ("Sample rate:", dontSendNotification);
    bufferLabel.setText ("Audio buffer size:", dontSendNotification);

    addAndMakeVisible (rateLabel);
    addAndMakeVisible (rateCombo);
    addAndMakeVisible (bufferLabel);
    addAndMakeVisible (bufferCombo);
}

void RateBufferSelector::populate (const Array<double>& rates, double currentRate, const Array<int>& bufferSizes, int currentBuffer)
{
    rateCombo.onSelectedItemChanged = nullptr;
    bufferCombo.onSelectedItemChanged = nullptr;

    currentRates = rates;
    currentBuffers = bufferSizes;

    rateCombo.clear();
    int selectedRateId = 1;
    for (int i = 0; i < rates.size(); ++i)
    {
        rateCombo.addItem (String (static_cast<int> (rates[i])) + " Hz", i + 1);
        if (rates[i] == currentRate)
            selectedRateId = i + 1;
    }
    rateCombo.setSelectedId (selectedRateId, dontSendNotification);

    bufferCombo.clear();
    int selectedBufId = 1;
    for (int i = 0; i < bufferSizes.size(); ++i)
    {
        bufferCombo.addItem (String (bufferSizes[i]) + " samples", i + 1);
        if (bufferSizes[i] == currentBuffer)
            selectedBufId = i + 1;
    }
    bufferCombo.setSelectedId (selectedBufId, dontSendNotification);

    rateCombo.onSelectedItemChanged = [this]
    {
        fireChanged();
    };
    bufferCombo.onSelectedItemChanged = [this]
    {
        fireChanged();
    };
}

void RateBufferSelector::fireChanged()
{
    if (onChanged == nullptr)
        return;

    const int rateIdx = rateCombo.getSelectedItemIndex();
    const int bufIdx = bufferCombo.getSelectedItemIndex();

    if (rateIdx >= 0 && rateIdx < currentRates.size()
        && bufIdx >= 0 && bufIdx < currentBuffers.size())
    {
        onChanged (currentRates[rateIdx], currentBuffers[bufIdx]);
    }
}

void RateBufferSelector::resized()
{
    auto bounds = getLocalBounds();
    const int rowH = bounds.getHeight() / 2;
    const int labelW = 170; // matches shared column split used by other selectors

    auto rateRow = bounds.removeFromTop (rowH);
    rateLabel.setBounds (rateRow.removeFromLeft (labelW).reduced (0, 10));
    rateCombo.setBounds (rateRow.reduced (2, 4));

    bufferLabel.setBounds (bounds.removeFromLeft (labelW).reduced (0, 10));
    bufferCombo.setBounds (bounds.reduced (2, 4));
}

} // namespace yup
