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

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_gui/yup_gui.h>

#include <BinaryData.h>

//==============================================================================

template <class T>
struct Array
{
    Array() = default;

    ~Array()
    {
        Free();
    }

    void Insert (T newItem, uintptr_t index)
    {
        if (length + 1 > allocated)
        {
            allocated *= 2;
            if (length + 1 > allocated)
                allocated = length + 1;

            array = (T*) std::realloc (array, allocated * sizeof (T));
        }

        length++;
        std::memmove (array + index + 1, array + index, (length - index - 1) * sizeof (T));
        array[index] = std::move (newItem);
    }

    void Delete (uintptr_t index)
    {
        std::memmove (array + index, array + index + 1, (length - index - 1) * sizeof (T));
        length--;
    }

    void Add (T item)
    {
        Insert (std::move (item), length);
    }

    void Free()
    {
        std::free (array);

        array = nullptr;
        length = allocated = 0;
    }

    int Length() const
    {
        return static_cast<int> (length);
    }

    T& operator[] (uintptr_t index)
    {
        jassert (index < length);
        return array[index];
    }

private:
    T* array = nullptr;
    size_t length = 0;
    size_t allocated = 0;
};

//==============================================================================

// Parameters.
#define P_VOLUME (0)
#define P_COUNT (1)

struct Voice
{
    bool held;
    int16_t channel;
    int16_t key;
    float phase;
    float parameterOffsets[P_COUNT];
};

//==============================================================================

class MyPlugin;

struct MyEditor : public yup::AudioProcessorEditor
{
    MyEditor (MyPlugin& processor);

    bool isResizable() const override
    {
        return true;
    }

    bool shouldPreserveAspectRatio() const override
    {
        return false;
    }

    yup::Size<int> getPreferredSize() const override
    {
        return { 600, 400 };
    }

    void resized() override
    {
        x->setBounds (getLocalBounds().largestFittingSquare());
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (0xff404040);
        g.fillAll();
    }

    MyPlugin& audioProcessor;
    std::unique_ptr<yup::Slider> x;
};

//==============================================================================

class MyPlugin : public yup::AudioProcessor
{
public:
    MyPlugin()
        : yup::AudioProcessor ("MyPlugin",
                               yup::AudioBusLayout ({}, { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        addParameter (gainParameter = yup::AudioParameterBuilder()
                                          .withID ("volume")
                                          .withName ("Volume")
                                          .withRange (0.0f, 1.0f)
                                          .withDefault (0.5f)
                                          .withSmoothing (20.0f)
                                          .build());
    }

    ~MyPlugin()
    {
        voices.Free();
    }

    void prepareToPlay (float sampleRate, int maxBlockSize) override
    {
        this->sampleRate = sampleRate;

        gainHandle = yup::AudioParameterHandle (*gainParameter, sampleRate);
    }

    void releaseResources() override
    {
    }

    void processBlock (yup::AudioSampleBuffer& audioBuffer, yup::MidiBuffer& midiBuffer) override
    {
        int numSamples = audioBuffer.getNumSamples();
        float* outputL = audioBuffer.getWritePointer (0);
        float* outputR = audioBuffer.getWritePointer (1);

        int nextEventSample = midiBuffer.getNumEvents() ? 0 : numSamples;
        auto midiIterator = midiBuffer.begin();

        gainHandle.update(); // call once per block

        for (int currentSample = 0; currentSample < numSamples;)
        {
            while (midiIterator != midiBuffer.end() && nextEventSample == currentSample)
            {
                const auto& event = *midiIterator;

                if (event.samplePosition != currentSample)
                {
                    nextEventSample = event.samplePosition;
                    break;
                }

                ++midiIterator;

                const auto& message = event.getMessage();

                // Look through our voices array, and if the event matches any of them, it must have been released.
                if (message.isNoteOff())
                {
                    for (int i = 0; i < voices.Length(); i++)
                    {
                        Voice* voice = &voices[i];

                        if (voice->key == message.getNoteNumber() && voice->channel == message.getChannel())
                        {
                            if (message.getVelocity() == 0)
                                voices.Delete (i--); // Stop the voice immediately; don't process the release segment of any ADSR envelopes.
                            else
                                voice->held = false;
                        }
                    }
                }

                // If this is a note on event, create a new voice and add it to our array.
                if (message.isNoteOn())
                {
                    Voice voice;
                    voice.held = true;
                    voice.channel = static_cast<int16_t> (message.getChannel());
                    voice.key = static_cast<int16_t> (message.getNoteNumber());
                    voice.phase = 0.0f;
                    voices.Add (voice);
                }

                // If this is a controller, set the corresponding parameter
                /*
                if (message.isController())
                {
                    const int controllerNumber = message.getControllerNumber();
                    if (yup::isPositiveAndBelow (controllerNumber, getNumParameters()))
                    {
                        getParameter (controllerNumber).setValue (message.getControllerValue() / 127.0f);
                    }
                }
                */

                // TODO - clap supports per voice modulations: clap_event_param_mod_t
            }

            if (midiIterator == midiBuffer.end())
                nextEventSample = numSamples;

            int remainingSamples = nextEventSample - currentSample;
            currentSample += remainingSamples;

            while (--remainingSamples >= 0)
            {
                float gainValue = gainHandle.getNextValue();

                float sum = 0.0f;

                for (int i = 0; i < voices.Length(); i++)
                {
                    Voice* voice = &voices[i];
                    if (! voice->held)
                        continue;

                    float volume = yup::jlimit (0.0f, 1.0f, gainValue + 0.0f); // parameterOffsets[P_VOLUME]);
                    sum += std::sin (voice->phase * 2.0f * 3.14159f) * 0.2f * volume;

                    voice->phase += 440.0f * std::exp2 ((voice->key - 57.0f) / 12.0f) / sampleRate;
                    voice->phase -= std::floor (voice->phase);
                }

                *outputL++ = sum;
                *outputR++ = sum;
            }
        }

        midiBuffer.clear();

        for (int i = 0; i < voices.Length(); i++)
        {
            Voice* voice = &voices[i];

            if (voice->held)
                continue;

            midiBuffer.addEvent (yup::MidiMessage::noteOff (voice->channel, voice->key), 0);

            voices.Delete (i--);
        }
    }

    void flush() override
    {
        voices.Free();
    }

    bool hasEditor() const override
    {
        return true;
    }

    yup::AudioProcessorEditor* createEditor() override
    {
        return new MyEditor (*this);
    }

private:
    friend MyEditor;
    yup::AudioParameter::Ptr gainParameter;
    yup::AudioParameterHandle gainHandle;

    Array<Voice> voices;
    float sampleRate;
};

MyEditor::MyEditor (MyPlugin& processor)
    : audioProcessor (processor)
{
    x = std::make_unique<yup::Slider> ("Slider", yup::Font());
    x->setValue (audioProcessor.gainParameter->getValue());
    x->onValueChanged = [this] (float value)
    {
        audioProcessor.gainParameter->setValueNotifyingHost (value);
    };
    addAndMakeVisible (*x);

    setSize (getPreferredSize().to<float>());
}

extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new MyPlugin();
}
