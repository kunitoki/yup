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
/** Sample rate and buffer size selectors. */
class YUP_API RateBufferSelector : public Component
{
public:
    //==============================================================================
    RateBufferSelector();

    //==============================================================================
    /** Populates both ComboBoxes.
        @param rates         Available sample rates in Hz.
        @param currentRate   Currently active sample rate.
        @param bufferSizes   Available buffer sizes in samples.
        @param currentBuffer Currently active buffer size.
    */
    void populate (const Array<double>& rates, double currentRate, const Array<int>& bufferSizes, int currentBuffer);

    //==============================================================================
    /** Called when either ComboBox changes. */
    std::function<void (double rate, int bufferSize)> onChanged;

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    void fireChanged();

    Label rateLabel;
    ComboBox rateCombo;
    Label bufferLabel;
    ComboBox bufferCombo;

    Array<double> currentRates;
    Array<int> currentBuffers;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RateBufferSelector)
};

} // namespace yup
