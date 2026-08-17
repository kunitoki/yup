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

#pragma once

#include <yup_rhi/yup_rhi.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_formats/yup_audio_formats.h>

//==============================================================================

/**
    Renders the GPU output peak meter on its own 60fps timer, independent of
    the rest of GpuAudioProcessingDemo's UI repaint cycle.
*/
class GpuPeakMeterComponent : public yup::Component
{
public:
    GpuPeakMeterComponent()
        : yup::Component ("GpuPeakMeterComponent")
    {
    }

    void setPeakLevel (float newPeak) noexcept
    {
        peakLevel.store (newPeak, std::memory_order_relaxed);
    }

    void refreshDisplay (double /*lastFrameTimeSeconds*/) override
    {
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds().to<float>();

        g.setFillColor (yup::Colors::darkgreen);
        g.fillRect (bounds);

        const auto peakWidth = bounds.getWidth() * peakLevel.load (std::memory_order_relaxed) * 4.0f;
        g.setFillColor (yup::Colors::lime);
        g.fillRect (bounds.withWidth (peakWidth));
    }

private:
    std::atomic<float> peakLevel { 0.0f };
};

//==============================================================================

/**
    Demonstrates GPU-accelerated audio effect processing using compute shaders.

    Plays a looped audio file through a GPU compute shader (gain + soft clip).
    The compute pipeline is compiled from GLSL at runtime and dispatched each
    audio callback block. GPU readback uses a synchronous staging path per
    backend.

    Requirements:
    - A GpuDevice with compute shader support (Metal, D3D11, WebGPU, GL 4.3+)
    - YUP_ENABLE_SHADER_TRANSPILER for online GLSL→native compilation
    - An audio file at examples/graphics/data/break_boomblastic_92bpm.mp3
*/
class GpuAudioProcessingDemo : public yup::Component
    , public yup::AudioIODeviceCallback
    , private yup::AsyncUpdater
{
public:
    //==============================================================================
    GpuAudioProcessingDemo()
        : yup::Component ("GpuAudioProcessingDemo")
    {
        loadAudioFile();

        peakMeter = std::make_unique<GpuPeakMeterComponent>();
        addAndMakeVisible (peakMeter.get());

        gainSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        gainSlider->setRange (0.0, 4.0);
        gainSlider->setValue (3.0);
        gainSlider->onValueChanged = [this] (double v)
        {
            gain = (float) v;
        };
        addAndMakeVisible (gainSlider.get());

        gainLabel = std::make_unique<yup::Label> ("gainLabel");
        gainLabel->setText ("Gain: 3.00", yup::dontSendNotification);
        addAndMakeVisible (gainLabel.get());

        mixSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        mixSlider->setRange (0.0, 1.0);
        mixSlider->setValue (1.0);
        mixSlider->onValueChanged = [this] (double v)
        {
            mix = (float) v;
        };
        addAndMakeVisible (mixSlider.get());

        mixLabel = std::make_unique<yup::Label> ("mixLabel");
        mixLabel->setText ("Mix: 1.00", yup::dontSendNotification);
        addAndMakeVisible (mixLabel.get());

        statusLabel = std::make_unique<yup::Label> ("status");
        statusLabel->setText ("Initializing audio and GPU...", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());

#if YUP_ENABLE_SHADER_TRANSPILER
        shaderDocument = std::make_unique<yup::CodeDocument>();
        shaderDocument->setText (yup::String::fromUTF8 (kDefaultGlslSource, sizeof (kDefaultGlslSource) - 1),
                                 yup::dontSendNotification);

        shaderEditor = std::make_unique<yup::CodeEditor> (*shaderDocument);
        shaderEditor->setSyntaxDefinition ("glsl");
        addAndMakeVisible (shaderEditor.get());

        recompileButton = std::make_unique<yup::TextButton> ("Recompile");
        recompileButton->onClick = [this]
        {
            recompileShader();
        };
        addAndMakeVisible (recompileButton.get());

        compileStatusLabel = std::make_unique<yup::Label> ("compileStatus");
        compileStatusLabel->setText ("Shader: default GLSL", yup::dontSendNotification);
        addAndMakeVisible (compileStatusLabel.get());
#endif

        yup::MessageManager::callAsync ([this]
        {
            initAudio();
        });
    }

    ~GpuAudioProcessingDemo() override
    {
        deviceManager.removeAudioCallback (this);
    }

    //==============================================================================
    // AudioIODeviceCallback
    //==============================================================================

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        if (audioBuffer.getNumSamples() == 0)
            return;

        gpuBlockSize = yup::jmin (device->getCurrentBufferSizeSamples(), kMaxGpuBlockSize);

        if (computeDevice == nullptr)
        {
            const yup::GpuPlatform platforms[] = {
#if YUP_MAC || YUP_IOS
                yup::GpuPlatform::Metal,
#else
#if YUP_WINDOWS
                yup::GpuPlatform::Direct3D,
#endif
                yup::GpuPlatform::OpenGL,
#endif
            };

            for (auto plat : platforms)
            {
                yup::GpuDevice::Options opts;
                opts.allowHeadlessRendering = true;

                computeDevice = yup::GpuDevice::create (plat, opts);
                if (computeDevice != nullptr && computeDevice->isComputeAvailable())
                    break;

                computeDevice = nullptr;
            }
        }

        if (computeDevice == nullptr || ! computeDevice->isComputeAvailable())
            return;

        const auto bufBytes = static_cast<size_t> (gpuBlockSize) * sizeof (float);
        std::vector<float> zeroData (static_cast<size_t> (gpuBlockSize), 0.0f);

        for (int i = 0; i < kRingSize; ++i)
        {
            gpuInputBuf[i] = computeDevice->createBuffer (yup::GpuBufferType::storage, zeroData.data(), bufBytes);
            gpuOutputBuf[i] = computeDevice->createBuffer (yup::GpuBufferType::storage, zeroData.data(), bufBytes);
            cpuUploadBuf[i].resize (static_cast<size_t> (gpuBlockSize));
            cpuOutputBuf[i].resize (static_cast<size_t> (gpuBlockSize));
        }

        writePos = 0;

        recompileShader();
        triggerAsyncUpdate();
    }

    void audioDeviceStopped() override
    {
        computePipeline = nullptr;

        for (int i = 0; i < kRingSize; ++i)
        {
            gpuInputBuf[i] = nullptr;
            gpuOutputBuf[i] = nullptr;
            cpuUploadBuf[i].clear();
            cpuOutputBuf[i].clear();
        }

        computeDevice = nullptr;

        triggerAsyncUpdate();
    }

    void audioDeviceIOCallbackWithContext (const float* const* /*inputChannelData*/,
                                           int /*numInputChannels*/,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            if (outputChannelData[ch] != nullptr)
                yup::FloatVectorOperations::clear (outputChannelData[ch], numSamples);
        }

        if (numOutputChannels == 0 || audioBuffer.getNumSamples() == 0)
            return;

        const int totalSamples = audioBuffer.getNumSamples();
        const int numChannels = audioBuffer.getNumChannels();
        const int processSamples = yup::jmin (numSamples, gpuBlockSize);
        const int slot = writePos % kRingSize;

        // Build input from the looped audio file into the preallocated CPU buffer.
        auto& inputBuf = cpuUploadBuf[slot];
        for (int i = 0; i < processSamples; ++i)
        {
            float audioSample = 0.0f;

            if (numChannels == 1)
                audioSample = audioBuffer.getSample (0, readPosition);
            else
                for (int ch = 0; ch < yup::jmin (2, numChannels); ++ch)
                    audioSample += audioBuffer.getSample (ch, readPosition);

            audioSample /= yup::jmin (2, numChannels);

            readPosition++;
            if (readPosition >= totalSamples)
                readPosition = 0;

            inputBuf[static_cast<size_t> (i)] = audioSample;
        }

        if (computePipeline != nullptr && computeDevice != nullptr)
        {
            // Write fresh data into the preallocated ring buffers in place —
            // no GPU allocation happens on the audio thread.
            computeDevice->updateBuffer (gpuInputBuf[slot], inputBuf.data(), inputBuf.size() * sizeof (float));

            Params params { gain, mix, 0.0f, 0.0f };

            uint32_t workgroupsX = (static_cast<uint32_t> (processSamples) + 255) / 256;

            auto pass = yup::GpuComputePass::begin (computeDevice);
            if (pass.isValid())
            {
                pass.setPipeline (computePipeline);
                pass.setStorageBuffer (0, 0, gpuInputBuf[slot]);
                pass.setStorageBuffer (0, 1, gpuOutputBuf[slot]);
                pass.setUniformBuffer (0, 2, &params, sizeof (params));
                pass.dispatch (workgroupsX, 1, 1);
                pass.finish();
            }

            // Read back from TWO slots ago — the GPU has had two full audio
            // callbacks to finish its work. No blocking wait needed.
            const int readSlot = (writePos + kRingSize - kReadLatency) % kRingSize;
            auto& outputBuf = cpuOutputBuf[readSlot];

            if (writePos >= kReadLatency
                && computeDevice->readBuffer (gpuOutputBuf[readSlot], outputBuf.data(), outputBuf.size() * sizeof (float)))
            {
                for (int ch = 0; ch < numOutputChannels; ++ch)
                    yup::FloatVectorOperations::copy (outputChannelData[ch], outputBuf.data(), processSamples);

                float peak = 0.0f;
                for (int i = 0; i < processSamples; ++i)
                    peak = yup::jmax (peak, std::abs (outputBuf[static_cast<size_t> (i)]));
                lastPeakOutput = peak;
                peakMeter->setPeakLevel (peak);
            }
            else
            {
                // Pipeline still filling — output silence.
                lastPeakOutput = 0.0f;
                peakMeter->setPeakLevel (0.0f);
            }
        }
        else
        {
            // No GPU: passthrough the looped file.
            for (int ch = 0; ch < numOutputChannels; ++ch)
                yup::FloatVectorOperations::copy (outputChannelData[ch], inputBuf.data(), processSamples);

            lastPeakOutput = 0.0f;
            peakMeter->setPeakLevel (0.0f);
        }

        writePos++;
    }

    //==============================================================================
    // Component overrides
    //==============================================================================

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::darkslategray));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        peakMeter->setBounds (bounds.removeFromTop (30.0f));
        bounds.removeFromTop (4.0f);

        statusLabel->setBounds (bounds.removeFromTop (22.0f));

        auto gainRow = bounds.removeFromTop (28.0f);
        gainLabel->setBounds (gainRow.removeFromLeft (60.0f));
        gainSlider->setBounds (gainRow);

        bounds.removeFromTop (4.0f);

        auto mixRow = bounds.removeFromTop (28.0f);
        mixLabel->setBounds (mixRow.removeFromLeft (60.0f));
        mixSlider->setBounds (mixRow);

        bounds.removeFromTop (8.0f);

#if YUP_ENABLE_SHADER_TRANSPILER
        compileStatusLabel->setBounds (bounds.removeFromTop (22.0f));
        bounds.removeFromTop (4.0f);
        recompileButton->setBounds (bounds.removeFromTop (26.0f));
        bounds.removeFromTop (4.0f);
        shaderEditor->setBounds (bounds);
#endif
    }

private:
    //==============================================================================

    void loadAudioFile()
    {
        auto dataDir = yup::File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("data");

        yup::File audioFile = dataDir.getChildFile ("break_boomblastic_92bpm.mp3");
        if (! audioFile.existsAsFile())
            return;

        yup::AudioFormatManager formatManager;
        formatManager.registerDefaultFormats();

        if (auto reader = formatManager.createReaderFor (audioFile))
        {
            audioBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&audioBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
        }
    }

    void initAudio()
    {
        auto result = deviceManager.initialiseWithDefaultDevices (0, 2);
        if (result.isNotEmpty())
        {
            statusLabel->setText ("Audio init failed: " + result, yup::dontSendNotification);
            return;
        }

        deviceManager.addAudioCallback (this);
        statusLabel->setText ("Audio + GPU compute active.", yup::dontSendNotification);
    }

    //==============================================================================

    void visibilityChanged() override
    {
        if (! isVisible())
            deviceManager.removeAudioCallback (this);
        else
            deviceManager.addAudioCallback (this);
    }

    void handleAsyncUpdate() override
    {
        gainLabel->setText ("Gain: " + yup::String (gain, 2), yup::dontSendNotification);
        mixLabel->setText ("Mix: " + yup::String (mix, 2), yup::dontSendNotification);
        repaint();
    }

    void recompileShader()
    {
        if (computeDevice == nullptr || ! computeDevice->isComputeAvailable())
            return;

#if YUP_ENABLE_SHADER_TRANSPILER
        yup::String glslSource = shaderDocument->getText().isEmpty()
                                   ? yup::String::fromUTF8 (kDefaultGlslSource, sizeof (kDefaultGlslSource) - 1)
                                   : shaderDocument->getText();

        auto result = yup::GpuComputePipeline::compileFromGlsl (computeDevice, glslSource);
        if (result.wasOk())
        {
            computePipeline = result.getValue();
            compileStatusLabel->setText ("Shader: compiled OK", yup::dontSendNotification);
        }
        else
        {
            compileStatusLabel->setText ("Shader compile error: " + result.getErrorMessage().substring (0, 80),
                                         yup::dontSendNotification);
        }
#else
        compileStatusLabel->setText ("Shader transpiler not available.", yup::dontSendNotification);
#endif
    }

    //==============================================================================

    static constexpr const char kDefaultGlslSource[] = R"glsl(#version 450
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, set = 0, binding = 0) buffer InputBuf {
    float inputData[];
};

layout(std430, set = 0, binding = 1) buffer OutputBuf {
    float outputData[];
};

layout(std140, set = 0, binding = 2) uniform Params {
    float gain;
    float mix;
    float pad0;
    float pad1;
} params;

float excite(float x, float g)
{
    float y = x;
    y = tanh(g * y);
    y += 0.20 * sin(7.0 * y);
    y += 0.08 * sin(19.0 * y);
    y *= 1.0 + 0.3 * abs(y);
    y = y / (1.0 + abs(y));
    return y;
}

void main()
{
    uint idx = gl_GlobalInvocationID.x;

    float s = inputData[idx];
    float d = excite(s, params.gain);

    outputData[idx] = d * params.mix + s * (1.0 - params.mix);
}
)glsl";

    //==============================================================================

    struct alignas (16) Params
    {
        float gainVal;
        float mixVal;
        float pad0;
        float pad1;
    };

    static constexpr int kMaxGpuBlockSize = 4096;
    static constexpr int kRingSize = 4;
    static constexpr int kReadLatency = 2;

    // Audio file playback.
    yup::AudioDeviceManager deviceManager;
    yup::AudioBuffer<float> audioBuffer;
    int readPosition = 0;
    int gpuBlockSize = 0;

    // GPU compute.
    yup::GpuDevice::Ptr computeDevice;
    yup::GpuComputePipeline::Ptr computePipeline;
    yup::GpuBuffer::Ptr gpuInputBuf[kRingSize];
    yup::GpuBuffer::Ptr gpuOutputBuf[kRingSize];
    std::vector<float> cpuUploadBuf[kRingSize];
    std::vector<float> cpuOutputBuf[kRingSize];
    int writePos = 0;

    // Parameters.
    float gain = 3.0f;
    float mix = 1.0f;

    // UI.
    float lastPeakOutput = 0.0f;
    std::unique_ptr<GpuPeakMeterComponent> peakMeter;
    std::unique_ptr<yup::Slider> gainSlider;
    std::unique_ptr<yup::Label> gainLabel;
    std::unique_ptr<yup::Slider> mixSlider;
    std::unique_ptr<yup::Label> mixLabel;
    std::unique_ptr<yup::Label> statusLabel;

#if YUP_ENABLE_SHADER_TRANSPILER
    std::unique_ptr<yup::CodeDocument> shaderDocument;
    std::unique_ptr<yup::CodeEditor> shaderEditor;
    std::unique_ptr<yup::TextButton> recompileButton;
    std::unique_ptr<yup::Label> compileStatusLabel;
#endif
};
