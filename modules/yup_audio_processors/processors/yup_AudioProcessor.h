/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
class JUCE_API AudioProcessor
{
public:
    AudioProcessor();
    virtual ~AudioProcessor();

    virtual int getNumParameters() const = 0;
    virtual AudioProcessorParameter& getParameter (int index) = 0;

    virtual int getNumAudioOutputs() const = 0;
    virtual int getNumAudioInputs() const = 0;

    virtual void prepareToPlay (float sampleRate, int maxBlockSize) = 0;
    virtual void releaseResources() = 0;

    virtual void processBlock (yup::AudioSampleBuffer& audioBuffer, yup::MidiBuffer& midiBuffer) = 0;

    virtual void flush() {}

    virtual bool hasEditor() const = 0;
    virtual Component* createEditor() { return nullptr; }
};

} // namespace yup
