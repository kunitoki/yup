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
SpectralBridge::SpectralBridge (StringRef name, AudioBusLayout busLayout)
    : AudioProcessor (name, std::move (busLayout))
{
    allocateResources();
    setLatencySamples (fftSize);
}

SpectralBridge::~SpectralBridge() = default;

//==============================================================================
void SpectralBridge::setSpectralProcessor (std::shared_ptr<SpectralProcessor> processor)
{
    spectralProcessor = std::move (processor);

    if (spectralProcessor != nullptr)
    {
        for (auto& param : spectralProcessor->getParameters())
            addParameter (param);
    }
}

std::shared_ptr<SpectralProcessor> SpectralBridge::getSpectralProcessor() const noexcept
{
    return spectralProcessor;
}

//==============================================================================
void SpectralBridge::setFFTSize (int newFFTSize)
{
    jassert (newFFTSize > 0 && (newFFTSize & (newFFTSize - 1)) == 0);

    if (newFFTSize == fftSize)
        return;

    auto suspension = ScopedProcessSuspension (*this, false);
    if (isPrepared())
        suspension.setSuspended();

    fftSize = newFFTSize;
    hopSize = fftSize / overlapFactor;
    numBins = fftSize / 2 + 1;

    reconfigureIfPrepared();
    setLatencySamples (fftSize);
}

int SpectralBridge::getFFTSize() const noexcept
{
    return fftSize;
}

//==============================================================================
void SpectralBridge::setOverlapFactor (int newOverlapFactor)
{
    jassert (newOverlapFactor >= 1);

    if (newOverlapFactor == overlapFactor)
        return;

    auto suspension = ScopedProcessSuspension (*this, false);
    if (isPrepared())
        suspension.setSuspended();

    overlapFactor = newOverlapFactor;
    hopSize = fftSize / overlapFactor;

    reconfigureIfPrepared();
}

int SpectralBridge::getOverlapFactor() const noexcept
{
    return overlapFactor;
}

//==============================================================================
void SpectralBridge::setWindowType (WindowType type)
{
    if (windowType == type)
        return;

    auto suspension = ScopedProcessSuspension (*this, false);
    if (isPrepared())
        suspension.setSuspended();

    windowType = type;
    reconfigureIfPrepared();
}

WindowType SpectralBridge::getWindowType() const noexcept
{
    return windowType;
}

//==============================================================================
void SpectralBridge::setWindowParameter (float parameter)
{
    if (windowParameter == parameter)
        return;

    auto suspension = ScopedProcessSuspension (*this, false);
    if (isPrepared())
        suspension.setSuspended();

    windowParameter = parameter;
    reconfigureIfPrepared();
}

float SpectralBridge::getWindowParameter() const noexcept
{
    return windowParameter;
}

//==============================================================================
void SpectralBridge::prepareToPlay (const AudioSpec& spec)
{
    const float rate = jmax (1.0f, spec.sampleRate);
    const int blockSize = jmax (1, spec.maxBlockSize);

    AudioProcessorBase::setPlaybackConfiguration (rate, blockSize);

    allocateResources();
    setLatencySamples (fftSize);

    if (spectralProcessor != nullptr)
    {
        const auto spectralSpec = SpectralSpec (getSampleRate(), getSamplesPerBlock(), static_cast<int> (channelState.size()), fftSize);
        spectralProcessor->prepareToPlay (spectralSpec);
    }

    spectralParams.reserve (512);
}

void SpectralBridge::releaseResources()
{
    for (auto& ch : channelState)
        ch = {};

    window = {};
    colaCompensation = {};
    spectralBuffer = {};

    fftSize = 1024;
    hopSize = 256;
    numBins = 0;
    overlapFactor = 4;

    if (spectralProcessor != nullptr)
        spectralProcessor->releaseResources();
}

//==============================================================================
void SpectralBridge::flush()
{
    for (auto& ch : channelState)
    {
        std::fill (ch.inputRing.begin(), ch.inputRing.end(), 0.0f);
        std::fill (ch.outputRing.begin(), ch.outputRing.end(), 0.0f);
        std::fill (ch.workBuf.begin(), ch.workBuf.end(), 0.0f);

        ch.samplesWritten = 0;
        ch.nextFrameStart = 0;
    }

    if (spectralProcessor != nullptr)
        spectralProcessor->flush();
}

//==============================================================================
void SpectralBridge::processBlock (AudioProcessContext<float>& context)
{
    if (fftSize <= 0 || hopSize <= 0 || fft.getSize() != fftSize)
        return;

    auto& audioBuffer = context.audio;
    const int numSamples = audioBuffer.getNumSamples();
    if (numSamples <= 0)
        return;

    jassert (numSamples <= getSamplesPerBlock());
    if (numSamples > getSamplesPerBlock())
        return;

    const ScopedTryLock lock (getProcessLock());
    if (! lock.isLocked())
    {
        context.audio.clear();
        return;
    }

    if (isSuspended())
    {
        context.audio.clear();
        return;
    }

    const int numChannels = jmin (audioBuffer.getNumChannels(), static_cast<int> (channelState.size()));

    const int64 blockStart = channelState[0].samplesWritten;
    const int64 blockEnd = blockStart + static_cast<int64> (numSamples);

    for (size_t ch = 0; ch < channelState.size(); ++ch)
    {
        auto& st = channelState[ch];

        if (st.inputRing.empty())
            continue;

        const int inputRingSize = static_cast<int> (st.inputRing.size());
        const auto* readPtr = ch < static_cast<size_t> (numChannels)
                                ? audioBuffer.getReadPointer (static_cast<int> (ch))
                                : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            const float sample = readPtr != nullptr ? readPtr[s] : 0.0f;
            st.inputRing[wrapIndex (blockStart + static_cast<int64> (s), inputRingSize)] = sample;
        }

        st.samplesWritten = blockEnd;
    }

    processAvailableFrames (context);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& st = channelState[static_cast<size_t> (ch)];

        auto* writePtr = audioBuffer.getWritePointer (ch);
        if (writePtr == nullptr || st.outputRing.empty())
            continue;

        const int outputRingSize = static_cast<int> (st.outputRing.size());

        for (int s = 0; s < numSamples; ++s)
        {
            const int64 outputPosition = blockStart + static_cast<int64> (s);
            const size_t outputIndex = wrapIndex (outputPosition, outputRingSize);

            writePtr[s] = st.outputRing[outputIndex];
            st.outputRing[outputIndex] = 0.0f;
        }
    }
}

//==============================================================================
bool SpectralBridge::hasEditor() const { return false; }

int SpectralBridge::getCurrentPreset() const noexcept { return 0; }

void SpectralBridge::setCurrentPreset (int) noexcept {}

int SpectralBridge::getNumPresets() const { return 0; }

String SpectralBridge::getPresetName (int) const { return {}; }

void SpectralBridge::setPresetName (int, StringRef) {}

bool SpectralBridge::supportsDataTreeState() const noexcept { return true; }

Result SpectralBridge::loadStateFromDataTree (const DataTree&)
{
    return Result::ok();
}

Result SpectralBridge::saveStateIntoDataTree (DataTree&)
{
    return Result::ok();
}

//==============================================================================
void SpectralBridge::allocateResources()
{
    const int inputRingSize = fftSize + getSamplesPerBlock() + hopSize;
    const int outputRingSize = fftSize * 2 + getSamplesPerBlock() * 2 + hopSize;

    numBins = fftSize / 2 + 1;
    hopSize = fftSize / overlapFactor;

    fft.setSize (fftSize);
    fft.setScaling (FFTProcessor::FFTScaling::asymmetric);

    buildWindows();

    const auto numChannels = static_cast<size_t> (jmax (getBusLayout().getNumAudioInputChannels(),
                                                        getBusLayout().getNumAudioOutputChannels(),
                                                        1));
    channelState.resize (numChannels);
    spectralBuffer.setSize (static_cast<int> (numChannels), numBins, false, false, false);

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto& st = channelState[ch];

        st.inputRing.resize (static_cast<size_t> (inputRingSize));
        std::fill (st.inputRing.begin(), st.inputRing.end(), 0.0f);

        st.outputRing.resize (static_cast<size_t> (outputRingSize));
        std::fill (st.outputRing.begin(), st.outputRing.end(), 0.0f);

        st.workBuf.resize (static_cast<size_t> (fftSize * 2));

        st.samplesWritten = 0;
        st.nextFrameStart = 0;
    }
}

void SpectralBridge::buildWindows()
{
    window.resize (static_cast<size_t> (fftSize));
    colaCompensation.resize (static_cast<size_t> (fftSize));

    WindowFunctions<float>::generate (windowType, window, windowParameter);

    for (int n = 0; n < fftSize; ++n)
    {
        float overlapSum = 0.0f;

        for (int overlapIndex = n % hopSize; overlapIndex < fftSize; overlapIndex += hopSize)
        {
            const float w = window[static_cast<size_t> (overlapIndex)];
            overlapSum += w * w;
        }

        colaCompensation[static_cast<size_t> (n)] = overlapSum > 0.0f
                                                      ? window[static_cast<size_t> (n)] / overlapSum
                                                      : 0.0f;
    }
}

bool SpectralBridge::isPrepared() const
{
    return ! channelState.empty() && ! channelState[0].inputRing.empty();
}

void SpectralBridge::reconfigureIfPrepared()
{
    if (! isPrepared())
        return;

    flush();
    allocateResources();

    if (spectralProcessor != nullptr)
    {
        spectralProcessor->releaseResources();

        const auto spectralSpec = SpectralSpec (getSampleRate(), getSamplesPerBlock(), static_cast<int> (channelState.size()), fftSize);
        spectralProcessor->prepareToPlay (spectralSpec);
    }

    spectralParams.reserve (512);
}

void SpectralBridge::processAvailableFrames (AudioProcessContext<float>& context)
{
    ScopedNoDenormals noDenormals;

    const int numChannels = jmin (context.audio.getNumChannels(), static_cast<int> (channelState.size()));

    spectralParams.clear();

    auto& st0 = channelState[0];

    while (st0.nextFrameStart + static_cast<int64> (fftSize) <= st0.samplesWritten)
    {
        const int64 frameStart = st0.nextFrameStart;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& st = channelState[static_cast<size_t> (ch)];
            const int inputRingSize = static_cast<int> (st.inputRing.size());

            for (int n = 0; n < fftSize; ++n)
            {
                const int64 inputPosition = frameStart + static_cast<int64> (n);
                st.workBuf[static_cast<size_t> (n)] = st.inputRing[wrapIndex (inputPosition, inputRingSize)]
                                                    * window[static_cast<size_t> (n)];
            }

            fft.performRealFFTForward (st.workBuf.data(), st.workBuf.data());

            const int numBinsFloats = numBins * 2;
            auto* specPtr = spectralBuffer.getWritePointer (ch);

            std::memcpy (specPtr, st.workBuf.data(), numBinsFloats * sizeof (float));
        }

        if (spectralProcessor != nullptr)
        {
            SpectralProcessContext<float> spectralCtx { spectralBuffer, spectralParams };
            spectralProcessor->processBlock (spectralCtx);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& st = channelState[static_cast<size_t> (ch)];
            const int outputRingSize = static_cast<int> (st.outputRing.size());

            const int numBinsFloats = numBins * 2;
            const auto* specPtr = spectralBuffer.getReadPointer (ch);

            std::memcpy (st.workBuf.data(), specPtr, numBinsFloats * sizeof (float));
            std::memset (st.workBuf.data() + numBinsFloats, 0, (fftSize * 2 - numBinsFloats) * sizeof (float));

            fft.performRealFFTInverse (st.workBuf.data(), st.workBuf.data());

            const int64 outputStart = frameStart + static_cast<int64> (fftSize);

            for (int n = 0; n < fftSize; ++n)
            {
                const int64 outputPosition = outputStart + static_cast<int64> (n);
                st.outputRing[wrapIndex (outputPosition, outputRingSize)] += st.workBuf[static_cast<size_t> (n)] * colaCompensation[static_cast<size_t> (n)];
            }
        }

        const int64 nextFrameStart = frameStart + static_cast<int64> (hopSize);
        for (auto& st : channelState)
            st.nextFrameStart = nextFrameStart;
    }
}

} // namespace yup
