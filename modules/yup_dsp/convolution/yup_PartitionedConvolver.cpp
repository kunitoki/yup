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

namespace yup
{

//==============================================================================

/** Performs Y += A * B (complex multiply accumulate) where A, B, and Y
    are arrays of interleaved complex<float> values [real, imag, real, imag...].

    @param A pointer to input complex array
    @param B pointer to input complex array
    @param Y pointer to output complex array (accumulated)
    @param complexPairs number of complex pairs (not number of floats!)
*/
static void complexMultiplyAccumulate (const float* A, const float* B, float* Y, int complexPairs) noexcept
{
    int i = 0;

#if YUP_USE_AVX_INTRINSICS
    constexpr int simdWidth = 4; // AVX2 path: process 4 complex pairs (8 floats) at a time
    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        __m256 a = _mm256_loadu_ps (A + idx);
        __m256 b = _mm256_loadu_ps (B + idx);
        __m256 y = _mm256_loadu_ps (Y + idx);

        const __m256 a_shuffled = _mm256_permute_ps (a, _MM_SHUFFLE (2, 3, 0, 1));
        const __m256 b_shuffled = _mm256_permute_ps (b, _MM_SHUFFLE (2, 3, 0, 1));

        __m256 realPart = _mm256_fmsub_ps (a, b, _mm256_mul_ps (a_shuffled, b_shuffled));
        __m256 imagPart = _mm256_fmadd_ps (a, b_shuffled, _mm256_mul_ps (a_shuffled, b));

        const __m256 interleaved = _mm256_blend_ps (realPart, imagPart, 0b10101010);

        y = _mm256_add_ps (y, interleaved);
        _mm256_storeu_ps (Y + idx, y);
    }

#elif YUP_USE_SSE_INTRINSICS
    constexpr int simdWidth = 2; // SSE path: process 2 complex pairs (4 floats) at a time
    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        __m128 a = _mm_loadu_ps (A + idx);
        __m128 b = _mm_loadu_ps (B + idx);
        __m128 y = _mm_loadu_ps (Y + idx);

        const __m128 a_shuffled = _mm_shuffle_ps (a, a, _MM_SHUFFLE (2, 3, 0, 1));
        const __m128 b_shuffled = _mm_shuffle_ps (b, b, _MM_SHUFFLE (2, 3, 0, 1));

        __m128 realPart = _mm_sub_ps (_mm_mul_ps (a, b), _mm_mul_ps (a_shuffled, b_shuffled));
        __m128 imagPart = _mm_add_ps (_mm_mul_ps (a, b_shuffled), _mm_mul_ps (a_shuffled, b));

        const __m128 interleaved = _mm_unpacklo_ps (realPart, imagPart);

        y = _mm_add_ps (y, interleaved);
        _mm_storeu_ps (Y + idx, y);
    }

#elif YUP_USE_ARM_NEON
    constexpr int simdWidth = 4;
    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        float32x4x2_t a = vld2q_f32 (A + idx);
        float32x4x2_t b = vld2q_f32 (B + idx);
        float32x4x2_t y = vld2q_f32 (Y + idx);

        float32x4_t ar = a.val[0], ai = a.val[1];
        float32x4_t br = b.val[0], bi = b.val[1];
        float32x4_t yr = y.val[0], yi = y.val[1];

        float32x4_t real = vmulq_f32 (ar, br);
        real = vfmsq_f32 (real, ai, bi);
        float32x4_t imag = vmulq_f32 (ar, bi);
        imag = vfmaq_f32 (imag, ai, br);

        yr = vaddq_f32 (yr, real);
        yi = vaddq_f32 (yi, imag);

        float32x4x2_t out = { yr, yi };
        vst2q_f32 (Y + idx, out); // interleave back
    }

#endif

    for (; i < complexPairs; ++i)
    {
        const int ri = i * 2;
        const int ii = ri + 1;

        const float ar = A[ri];
        const float ai = A[ii];
        const float br = B[ri];
        const float bi = B[ii];

        Y[ri] += ar * br - ai * bi;
        Y[ii] += ar * bi + ai * br;
    }
}

//==============================================================================

class PartitionedConvolver::DirectFIR
{
public:
    DirectFIR() = default;

    void setTaps (std::vector<float> taps, float scaling)
    {
        FloatVectorOperations::multiply (taps.data(), scaling, taps.size());

        tapsReversed = std::move (taps);
        std::reverse (tapsReversed.begin(), tapsReversed.end());

        numTaps = tapsReversed.size();
        paddedLen = (numTaps + 3u) & ~3u;
        tapsReversed.resize (paddedLen, 0.0f);

        history.assign (2 * numTaps, 0.0f);
        writeIndex = 0;
    }

    void reset()
    {
        std::fill (history.begin(), history.end(), 0.0f);
        writeIndex = 0;
    }

    void process (const float* input, float* output, std::size_t numSamples) noexcept
    {
        const std::size_t M = numTaps;
        if (M == 0)
            return;

        const float* h = tapsReversed.data();
        for (std::size_t i = 0; i < numSamples; ++i)
        {
            const float x = input[i];

            history[writeIndex] = x;
            history[writeIndex + M] = x;

            const float* w = history.data() + writeIndex + 1;

            float sum = 0.0f;

#if YUP_ENABLE_VDSP
            vDSP_dotpr (w, 1, h, 1, &sum, M);
#else
            sum = dotProduct (w, h, M);
#endif

            output[i] += sum;

            if (++writeIndex == M)
                writeIndex = 0;
        }
    }

    std::size_t getNumTaps() const
    {
        return numTaps;
    }

private:
    static float dotProduct (const float* __restrict a, const float* __restrict b, std::size_t len) noexcept
    {
        float acc = 0.0f;
        std::size_t i = 0;

#if YUP_USE_AVX_INTRINSICS && YUP_USE_FMA_INTRINSICS
        __m256 vacc = _mm256_setzero_ps();
        for (; i + 8 <= len; i += 8)
        {
            __m256 va = _mm256_loadu_ps (a + i);
            __m256 vb = _mm256_loadu_ps (b + i);
            vacc = _mm256_fmadd_ps (va, vb, vacc);
        }
        __m128 low = _mm256_castps256_ps128 (vacc);
        __m128 high = _mm256_extractf128_ps (vacc, 1);
        __m128 vsum = _mm_add_ps (low, high);
        vsum = _mm_hadd_ps (vsum, vsum);
        vsum = _mm_hadd_ps (vsum, vsum);
        acc += _mm_cvtss_f32 (vsum);

#elif YUP_USE_SSE_INTRINSICS
        __m128 vacc = _mm_setzero_ps();
#if YUP_USE_FMA_INTRINSICS
        for (; i + 4 <= len; i += 4)
        {
            __m128 va = _mm_loadu_ps (a + i);
            __m128 vb = _mm_loadu_ps (b + i);
            vacc = _mm_fmadd_ps (va, vb, vacc);
        }
#else
        for (; i + 4 <= len; i += 4)
        {
            __m128 va = _mm_loadu_ps (a + i);
            __m128 vb = _mm_loadu_ps (b + i);
            vacc = _mm_add_ps (vacc, _mm_mul_ps (va, vb));
        }
#endif
        __m128 shuf = _mm_shuffle_ps (vacc, vacc, _MM_SHUFFLE (2, 3, 0, 1));
        __m128 sums = _mm_add_ps (vacc, shuf);
        shuf = _mm_movehl_ps (shuf, sums);
        sums = _mm_add_ss (sums, shuf);
        acc += _mm_cvtss_f32 (sums);

#elif YUP_USE_ARM_NEON
        float32x4_t vacc = vdupq_n_f32 (0.0f);
        for (; i + 4 <= len; i += 4)
        {
            float32x4_t va = vld1q_f32 (a + i);
            float32x4_t vb = vld1q_f32 (b + i);
            vacc = vmlaq_f32 (vacc, va, vb);
        }
#if YUP_64BIT
        acc += vaddvq_f32 (vacc);
#else
        float32x2_t vlow = vget_low_f32 (vacc);
        float32x2_t vhigh = vget_high_f32 (vacc);
        float32x2_t vsum2 = vpadd_f32 (vlow, vhigh);
        vsum2 = vpadd_f32 (vsum2, vsum2);
        acc += vget_lane_f32 (vsum2, 0);
#endif

#endif

        for (; i < len; ++i)
            acc += a[i] * b[i];

        return acc;
    }

    std::vector<float> tapsReversed;
    std::vector<float> history;
    std::size_t numTaps = 0;
    std::size_t paddedLen = 0;
    std::size_t writeIndex = 0;
};

//==============================================================================

class PartitionedConvolver::FFTLayer
{
public:
    FFTLayer() = default;
    ~FFTLayer() = default;

    FFTLayer (FFTLayer&& other) = default;
    FFTLayer& operator= (FFTLayer&& other) = default;

    void configure (int newHopSize)
    {
        hopSize = newHopSize;
        fftSize = hopSize * 2;

        fftProcessor.setSize (fftSize);
        fftProcessor.setScaling (FFTProcessor::FFTScaling::asymmetric);

        overlapBuffer.assign (static_cast<std::size_t> (hopSize), 0.0f);
        timeBuffer.assign (static_cast<std::size_t> (fftSize), 0.0f);
        frequencyBuffer.assign (static_cast<std::size_t> (fftSize) * 2, 0.0f);
        tempBuffer.assign (static_cast<std::size_t> (fftSize) * 2, 0.0f); // Must hold complex data for in-place FFT

        fdlIndex = 0;
        configured = true;
    }

    int getHopSize() const { return hopSize; }

    int getFFTSize() const { return fftSize; }

    bool isConfigured() const { return configured; }

    std::size_t setImpulseResponse (const float* impulseResponse, std::size_t length, float scaling)
    {
        jassert (configured);

        if (fftSize <= 0 || hopSize <= 0)
        {
            resetState();
            return 0;
        }

        frequencyPartitions.clear();
        frequencyDelayLine.clear();

        if (length == 0 || impulseResponse == nullptr)
        {
            resetState();
            return 0;
        }

        const auto numPartitions = (length + static_cast<std::size_t> (hopSize) - 1) / static_cast<std::size_t> (hopSize);
        if (numPartitions == 0)
        {
            resetState();
            return 0;
        }

        std::size_t processedSamples = 0;
        frequencyPartitions.reserve (numPartitions);

        for (std::size_t p = 0; p < numPartitions; ++p)
        {
            std::vector<float> partition;
            partition.resize (static_cast<std::size_t> (fftSize) * 2);

            std::fill (tempBuffer.begin(), tempBuffer.end(), 0.0f);

            const std::size_t offset = p * static_cast<std::size_t> (hopSize);
            const std::size_t copyCount = std::min (static_cast<std::size_t> (hopSize), length - offset);

            if (copyCount > 0 && offset < length)
            {
                for (std::size_t i = 0; i < copyCount && offset + i < length; ++i)
                    tempBuffer[i] = impulseResponse[offset + i] * scaling;
            }

            fftProcessor.performRealFFTForward (tempBuffer.data(), partition.data());

            frequencyPartitions.push_back (std::move (partition));

            processedSamples += copyCount;
        }

        frequencyDelayLine.assign (numPartitions, std::vector<float> (static_cast<std::size_t> (fftSize) * 2, 0.0f));
        fdlIndex = 0;

        resetState();

        return processedSamples;
    }

    void resetState()
    {
        fdlIndex = 0;

        for (auto& partition : frequencyDelayLine)
            std::fill (partition.begin(), partition.end(), 0.0f);

        std::fill (overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
        std::fill (timeBuffer.begin(), timeBuffer.end(), 0.0f);
        std::fill (frequencyBuffer.begin(), frequencyBuffer.end(), 0.0f);
    }

    void processHop (const float* inputHop, float* outputAccumulator)
    {
        jassert (configured);

        if (frequencyPartitions.empty())
            return;

        // 1) Transform current input hop to frequency domain
        FloatVectorOperations::copy (tempBuffer.data(), inputHop, hopSize);
        fftProcessor.performRealFFTForward (tempBuffer.data(), tempBuffer.data());

        // 2) Store in frequency delay line (circular buffer) - copy full complex buffer
        fdlIndex = (fdlIndex == 0) ? static_cast<int> (frequencyDelayLine.size()) - 1 : fdlIndex - 1;
        std::copy (tempBuffer.begin(), tempBuffer.begin() + (fftSize * 2), frequencyDelayLine[static_cast<std::size_t> (fdlIndex)].begin());

        // 3) Frequency domain convolution: Y = sum(X[k-p] * H[p])
        FloatVectorOperations::clear (frequencyBuffer.data(), fftSize * 2);

        int xIndex = fdlIndex;
        for (std::size_t p = 0; p < frequencyPartitions.size(); ++p)
        {
            const float* X = frequencyDelayLine[static_cast<std::size_t> (xIndex)].data();
            const float* H = frequencyPartitions[p].data();

            // fftSize_/2 gives the number of complex pairs for real FFT
            complexMultiplyAccumulate (X, H, frequencyBuffer.data(), fftSize / 2);

            // Move to next older spectrum
            xIndex++;
            if (xIndex >= static_cast<int> (frequencyDelayLine.size()))
                xIndex = 0;
        }

        // 4) Inverse FFT back to time domain
        fftProcessor.performRealFFTInverse (frequencyBuffer.data(), timeBuffer.data());

        // 5) Overlap-Add: output first hopSize samples, store last hopSize as overlap
        for (int i = 0; i < hopSize; ++i)
        {
            outputAccumulator[i] += timeBuffer[i] + overlapBuffer[i];
            overlapBuffer[i] = timeBuffer[i + hopSize];
        }
    }

    bool hasImpulseResponse() const { return ! frequencyPartitions.empty(); }

private:
    int hopSize = 0;
    int fftSize = 0;

    FFTProcessor fftProcessor;

    // IR partitions in frequency domain
    std::vector<std::vector<float>> frequencyPartitions;

    // Frequency Delay Line (most recent at fdlIndex)
    std::vector<std::vector<float>> frequencyDelayLine;
    int fdlIndex = 0;

    // Processing buffers
    std::vector<float> overlapBuffer;
    std::vector<float> timeBuffer;
    std::vector<float> frequencyBuffer;
    std::vector<float> tempBuffer;

    bool configured = false;
};

//==============================================================================

class PartitionedConvolver::CircularBuffer
{
public:
    CircularBuffer() = default;

    void resize (std::size_t size)
    {
        buffer.resize (size);
        clear();
    }

    void clear()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
        readIndex = 0;
        availableForRead = 0;
    }

    std::size_t getAvailableForRead() const { return availableForRead; }

    std::size_t getAvailableForWrite() const { return buffer.size() - availableForRead; }

    std::size_t getSize() const { return buffer.size(); }

    void write (const float* data, std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForWrite());
        numSamples = std::min (numSamples, getAvailableForWrite());

        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, buffer.size() - writeIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (data, data + beforeWrap, buffer.begin() + writeIndex);
        if (afterWrap > 0)
            std::copy (data + beforeWrap, data + numSamples, buffer.begin());

        writeIndex = (writeIndex + numSamples) % buffer.size();
        availableForRead += numSamples;
    }

    void read (float* data, std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead());

        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, buffer.size() - readIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (buffer.begin() + readIndex, buffer.begin() + readIndex + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (buffer.begin(), buffer.begin() + afterWrap, data + beforeWrap);

        readIndex = (readIndex + numSamples) % buffer.size();
        availableForRead -= numSamples;
    }

    void peek (float* data, std::size_t numSamples, std::size_t offset = 0) const
    {
        jassert (numSamples + offset <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead() - offset);

        if (numSamples == 0)
            return;

        const std::size_t startIndex = (readIndex + offset) % buffer.size();
        const std::size_t beforeWrap = std::min (numSamples, buffer.size() - startIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (buffer.begin() + startIndex, buffer.begin() + startIndex + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (buffer.begin(), buffer.begin() + afterWrap, data + beforeWrap);
    }

    void skip (std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead());

        readIndex = (readIndex + numSamples) % buffer.size();
        availableForRead -= numSamples;
    }

private:
    std::vector<float> buffer;
    std::size_t writeIndex = 0;
    std::size_t readIndex = 0;
    std::size_t availableForRead = 0;
};

//==============================================================================

class PartitionedConvolver::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    void configureLayers (std::size_t directFIRTaps, const std::vector<LayerSpec>& newLayers)
    {
        directFIRTapCount = directFIRTaps;

        layers.clear();
        layers.resize (newLayers.size());

        std::size_t maximumHopSize = 0;

        baseHopSize = newLayers.empty() ? 0 : newLayers.front().hopSize;
        for (std::size_t i = 0; i < newLayers.size(); ++i)
        {
            layers[i].configure (newLayers[i].hopSize);
            if (i == 0)
                baseHopSize = newLayers[i].hopSize;
            else
                baseHopSize = std::min (baseHopSize, newLayers[i].hopSize);

            maximumHopSize = std::max (maximumHopSize, static_cast<std::size_t> (newLayers[i].hopSize));
        }

        maxHopSize = maximumHopSize;

        // Clear staging buffers - will be allocated in prepare()
        inputStaging.clear();
        outputStaging.clear();

        // Resize per-layer circular buffers - will be allocated in prepare()
        layerInputBuffers.resize (layers.size());
        layerOutputBuffers.resize (layers.size());

        layerTempOutput.clear();
        tempLayerHop.clear();

        // Clear working buffers - will be allocated in prepare()
        workingOutput.clear();

        isPrepared = false;
    }

    void prepare (std::size_t maxBlockSize)
    {
        this->maxBlockSize = maxBlockSize;

        // Prepare main input staging - needs to accumulate up to baseHopSize samples plus incoming block
        const std::size_t inputStagingSize = static_cast<std::size_t> (baseHopSize) + maxBlockSize;
        inputStaging.resize (inputStagingSize);
        outputStaging.assign (static_cast<std::size_t> (baseHopSize), 0.0f);

        // Prepare per-layer circular buffers with layer-specific sizing
        for (std::size_t i = 0; i < layerInputBuffers.size(); ++i)
        {
            const std::size_t layerHopSize = static_cast<std::size_t> (layers[i].getHopSize());

            // Input buffer: needs to accumulate up to layerHopSize samples plus incoming block
            const std::size_t layerInputBufferSize = layerHopSize + maxBlockSize;
            layerInputBuffers[i].resize (layerInputBufferSize);

            // Output buffer: needs to handle bursts of layerHopSize samples
            // Size it to handle multiple hops since read rate (baseHopSize) may be much smaller than write rate (layerHopSize)
            const std::size_t layerOutputBufferSize = layerHopSize * ((layerHopSize / static_cast<std::size_t> (baseHopSize)) + 2);
            layerOutputBuffers[i].resize (layerOutputBufferSize);
        }

        // Allocate temp buffers
        if (maxHopSize > 0)
        {
            layerTempOutput.resize (maxHopSize);
            tempLayerHop.resize (maxHopSize);
        }

        // Allocate working buffers
        workingOutput.resize (maxBlockSize);

        isPrepared = true;
    }

    std::size_t trimSilenceFromEnd (const float* impulseResponse, std::size_t length, float thresholdDb)
    {
        if (impulseResponse == nullptr || length == 0)
            return 0;

        const float threshold = std::pow (10.0f, thresholdDb / 20.0f);

        // For short IRs, use smaller window size and be more conservative
        const std::size_t minRetainLength = std::max (std::size_t (32), length / 4);
        const std::size_t windowSize = std::min (std::size_t (1024), std::max (std::size_t (64), length / 20));

        // First pass: scan from end to find significant content
        std::size_t significantContentEnd = 0;
        for (std::size_t i = length; i > windowSize; i -= windowSize)
        {
            const std::size_t startIdx = i - windowSize;
            const std::size_t endIdx = std::min (i, length);
            const std::size_t samples = endIdx - startIdx;

            if (samples == 0)
                continue;

            float rmsSquared = 0.0f;
            for (std::size_t j = startIdx; j < endIdx; ++j)
                rmsSquared += impulseResponse[j] * impulseResponse[j];

            const float rms = std::sqrt (rmsSquared / static_cast<float> (samples));
            if (rms >= threshold)
            {
                significantContentEnd = endIdx;
                break;
            }
        }

        // If no significant content found, check the beginning more carefully
        if (significantContentEnd == 0)
        {
            const std::size_t checkLength = std::min (minRetainLength, length);
            float rmsSquared = 0.0f;
            for (std::size_t j = 0; j < checkLength; ++j)
                rmsSquared += impulseResponse[j] * impulseResponse[j];

            const float rms = std::sqrt (rmsSquared / static_cast<float> (checkLength));
            if (rms < threshold)
                return 1;
        }

        // Return the found significant content end, but respect minimum for short IRs
        if (length <= 200) // Short IR protection
            return std::max (significantContentEnd, minRetainLength);
        else
            return std::max (significantContentEnd, windowSize);
    }

    void setImpulseResponse (const float* impulseResponse, std::size_t length, const PartitionedConvolver::IRLoadOptions& options)
    {
        DirectFIR newFIR;
        std::vector<FFTLayer> newLayers (layers.size());

        std::size_t trimmedLength = length;

        // Safety check
        if (impulseResponse != nullptr && trimmedLength > 0)
        {
            // Trim end silence if requested
            if (options.trimEndSilenceBelowDb)
                trimmedLength = trimSilenceFromEnd (impulseResponse, length, *options.trimEndSilenceBelowDb);

            // Always apply peak headroom
            float headroomScale = std::pow (10.0f, options.headroomDb / 20.0f);
            if (options.normalize)
            {
                const auto minMax = FloatVectorOperations::findMinAndMax (impulseResponse, trimmedLength);

                const float peak = std::max (std::abs (minMax.getStart()), std::abs (minMax.getEnd()));
                if (peak > 0.0f)
                    headroomScale /= peak;
            }

            // Update DirectFIR in-place
            std::vector<float> directTaps;

            const auto directTapsCount = std::min (directFIRTapCount, trimmedLength);
            if (directTapsCount > 0)
            {
                directTaps.reserve (directTapsCount);
                directTaps.assign (impulseResponse, impulseResponse + directTapsCount);
            }

            newFIR.setTaps (std::move (directTaps), headroomScale);

            // Update FFT layers
            std::size_t consumed = directTapsCount;
            for (std::size_t i = 0; i < newLayers.size(); ++i)
            {
                auto& layer = newLayers[i];
                layer.configure (layers[i].getHopSize());

                const std::size_t remaining = (consumed < trimmedLength) ? (trimmedLength - consumed) : 0;
                if (remaining == 0)
                {
                    layer.setImpulseResponse (nullptr, 0, headroomScale);
                    continue;
                }

                consumed += layer.setImpulseResponse (impulseResponse + consumed, remaining, headroomScale);
            }
        }

        {
            SpinLock::ScopedLockType lock (processingLock);

            directFIR = std::move (newFIR);
            layers = std::move (newLayers);

            resetStateUnsafe();
        }
    }

    void reset()
    {
        SpinLock::ScopedLockType lock (processingLock);

        resetStateUnsafe();
    }

    void process (const float* input, float* output, std::size_t numSamples)
    {
        if (numSamples == 0)
            return;

        SpinLock::ScopedLockType lock (processingLock);

        processUnsafe (input, output, numSamples);
    }

private:
    void resetStateUnsafe()
    {
        directFIR.reset();
        inputStagingReadIndex = 0;
        inputStagingWriteIndex = 0;
        inputStagingAvailable = 0;
        std::fill (outputStaging.begin(), outputStaging.end(), 0.0f);

        for (auto& buffer : layerInputBuffers)
            buffer.clear();

        for (auto& buffer : layerOutputBuffers)
            buffer.clear();

        for (auto& layer : layers)
            layer.resetState();
    }

    void processUnsafe (const float* input, float* output, std::size_t numSamples)
    {
        jassert (isPrepared);
        jassert (numSamples <= maxBlockSize);
        if (! isPrepared || numSamples > maxBlockSize)
            return;

        FloatVectorOperations::clear (workingOutput.data(), numSamples);

        // Process direct FIR (no block size constraints)
        directFIR.process (input, workingOutput.data(), numSamples);
        if (layers.empty())
        {
            FloatVectorOperations::add (output, workingOutput.data(), numSamples);
            return;
        }

        // Add input to main input staging buffer using circular buffer logic
        writeToInputStaging (input, numSamples);

        std::size_t outputSamplesProduced = 0;
        while (getInputStagingAvailable() >= static_cast<std::size_t> (baseHopSize))
        {
            const std::size_t hopSize = static_cast<std::size_t> (baseHopSize);

            // Read hop from input staging
            readFromInputStaging (tempLayerHop.data(), hopSize);
            FloatVectorOperations::clear (outputStaging.data(), outputStaging.size());

            for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex)
            {
                auto& layer = layers[layerIndex];
                if (! layer.hasImpulseResponse())
                    continue;

                const int layerHopSize = layer.getHopSize();
                auto& inputBuffer = layerInputBuffers[layerIndex];
                auto& outputBuffer = layerOutputBuffers[layerIndex];

                // Write input hop to layer's input buffer
                inputBuffer.write (tempLayerHop.data(), hopSize);

                // Process complete layer hops
                while (inputBuffer.getAvailableForRead() >= static_cast<std::size_t> (layerHopSize))
                {
                    // Read a full hop for this layer
                    inputBuffer.read (tempLayerHop.data(), static_cast<std::size_t> (layerHopSize));
                    FloatVectorOperations::clear (layerTempOutput.data(), layerHopSize);

                    // Process hop
                    layer.processHop (tempLayerHop.data(), layerTempOutput.data());

                    // Write output to layer's output buffer
                    outputBuffer.write (layerTempOutput.data(), static_cast<std::size_t> (layerHopSize));
                }

                // Mix available output from this layer
                if (outputBuffer.getAvailableForRead() >= hopSize)
                {
                    outputBuffer.read (layerTempOutput.data(), hopSize);
                    FloatVectorOperations::add (outputStaging.data(), layerTempOutput.data(), hopSize);
                }
            }

            // Add staging output to main output
            const std::size_t samplesToWrite = std::min (hopSize, numSamples - outputSamplesProduced);
            FloatVectorOperations::add (workingOutput.data() + outputSamplesProduced, outputStaging.data(), samplesToWrite);
            outputSamplesProduced += samplesToWrite;
        }

        // Copy final result to output (accumulate)
        FloatVectorOperations::add (output, workingOutput.data(), numSamples);
    }

private:
    void writeToInputStaging (const float* data, std::size_t numSamples)
    {
        const std::size_t available = inputStaging.size() - inputStagingAvailable;
        jassert (numSamples <= available);
        numSamples = std::min (numSamples, available);
        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, inputStaging.size() - inputStagingWriteIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (data, data + beforeWrap, inputStaging.begin() + inputStagingWriteIndex);
        if (afterWrap > 0)
            std::copy (data + beforeWrap, data + numSamples, inputStaging.begin());

        inputStagingWriteIndex = (inputStagingWriteIndex + numSamples) % inputStaging.size();
        inputStagingAvailable += numSamples;
    }

    void readFromInputStaging (float* data, std::size_t numSamples)
    {
        jassert (numSamples <= inputStagingAvailable);
        numSamples = std::min (numSamples, inputStagingAvailable);
        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, inputStaging.size() - inputStagingReadIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (inputStaging.begin() + inputStagingReadIndex, inputStaging.begin() + inputStagingReadIndex + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (inputStaging.begin(), inputStaging.begin() + afterWrap, data + beforeWrap);

        inputStagingReadIndex = (inputStagingReadIndex + numSamples) % inputStaging.size();
        inputStagingAvailable -= numSamples;
    }

    std::size_t getInputStagingAvailable() const { return inputStagingAvailable; }

    std::size_t directFIRTapCount = 0;
    int baseHopSize = 0;
    std::size_t maxHopSize = 0;
    std::size_t maxBlockSize = 0;
    bool isPrepared = false;

    DirectFIR directFIR;
    std::vector<FFTLayer> layers;

    // Working buffers
    std::vector<float> workingOutput;

    // Input staging with circular buffer management
    std::vector<float> inputStaging;
    std::size_t inputStagingReadIndex = 0;
    std::size_t inputStagingWriteIndex = 0;
    std::size_t inputStagingAvailable = 0;
    std::vector<float> outputStaging;

    // Per-layer circular buffering
    std::vector<CircularBuffer> layerInputBuffers;
    std::vector<CircularBuffer> layerOutputBuffers;
    std::vector<float> tempLayerHop;
    std::vector<float> layerTempOutput;

    mutable SpinLock processingLock;
};

//==============================================================================
// PartitionedConvolver implementation
//==============================================================================

PartitionedConvolver::PartitionedConvolver()
    : pImpl (std::make_unique<Impl>())
{
}

PartitionedConvolver::~PartitionedConvolver() = default;

PartitionedConvolver::PartitionedConvolver (PartitionedConvolver&& other) noexcept
    : pImpl (std::move (other.pImpl))
{
}

PartitionedConvolver& PartitionedConvolver::operator= (PartitionedConvolver&& other) noexcept
{
    if (this != &other)
        pImpl = std::move (other.pImpl);
    return *this;
}

void PartitionedConvolver::configureLayers (std::size_t directFIRTaps, const std::vector<LayerSpec>& layers)
{
    pImpl->configureLayers (directFIRTaps, layers);
}

void PartitionedConvolver::setTypicalLayout (std::size_t directTaps, const std::vector<int>& hops)
{
    std::vector<LayerSpec> layerSpecs;
    layerSpecs.reserve (hops.size());

    for (int hop : hops)
    {
        if (hop < 64)
            directTaps += static_cast<std::size_t> (hop);
        else
            layerSpecs.push_back ({ nextPowerOfTwo (hop) });
    }

    configureLayers (directTaps, layerSpecs);
}

void PartitionedConvolver::setImpulseResponse (const float* impulseResponse, std::size_t length, const IRLoadOptions& options)
{
    pImpl->setImpulseResponse (impulseResponse, length, options);
}

void PartitionedConvolver::setImpulseResponse (const std::vector<float>& impulseResponse, const IRLoadOptions& options)
{
    setImpulseResponse (impulseResponse.data(), impulseResponse.size(), options);
}

void PartitionedConvolver::prepare (std::size_t maxBlockSize)
{
    pImpl->prepare (maxBlockSize);
}

void PartitionedConvolver::reset()
{
    pImpl->reset();
}

void PartitionedConvolver::process (const float* input, float* output, std::size_t numSamples)
{
    pImpl->process (input, output, numSamples);
}

} // namespace yup
