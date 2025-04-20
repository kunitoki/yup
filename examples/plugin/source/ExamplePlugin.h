/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_processors/yup_audio_processors.h>

//==============================================================================

template <class T>
struct Array
{
    Array() = default;

    ~Array()
    {
        free();
    }

    void insert (T newItem, uintptr_t index)
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

    void remove (uintptr_t index)
    {
        std::memmove (array + index, array + index + 1, (length - index - 1) * sizeof (T));
        length--;
    }

    void add (T item)
    {
        insert (std::move (item), length);
    }

    void free()
    {
        std::free (array);

        array = nullptr;
        length = allocated = 0;
    }

    int size() const
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

class ExampleEditor;

class ExamplePlugin : public yup::AudioProcessor
{
public:
    ExamplePlugin();
    ~ExamplePlugin() override;

    void prepareToPlay (float sampleRate, int maxBlockSize) override;
    void releaseResources() override;
    void processBlock (yup::AudioSampleBuffer& audioBuffer, yup::MidiBuffer& midiBuffer) override;
    void flush() override;

    int getCurrentPreset() const noexcept override;
    void setCurrentPreset (int index) noexcept override;
    int getNumPresets() const override;
    yup::String getPresetName (int index) const override;
    void setPresetName (int index, yup::StringRef newName) override;

    yup::Result loadStateFromMemory (const yup::MemoryBlock& data) override;
    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override;

    bool hasEditor() const override;
    yup::AudioProcessorEditor* createEditor() override;

private:
    yup::AudioParameter::Ptr gainParameter;
    yup::AudioParameterHandle gainHandle;

    Array<Voice> voices;
    float sampleRate;

    int currentPreset = 0;

    struct Preset
    {
        yup::String name;
        float gainValue;
    } presets[2] = {
        { "Full Volume", 1.0f },
        { "Half Volume", 0.5f }
    };
};
