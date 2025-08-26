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

        // a = [ar0 ai0 ar1 ai1 ar2 ai2 ar3 ai3]
        // b = [br0 bi0 br1 bi1 br2 bi2 br3 bi3]

        // separate real and imag for a and b
        const __m256 a_shuffled = _mm256_permute_ps (a, _MM_SHUFFLE (2, 3, 0, 1));
        const __m256 b_shuffled = _mm256_permute_ps (b, _MM_SHUFFLE (2, 3, 0, 1));

        // real = ar*br - ai*bi
        __m256 realPart = _mm256_fmsub_ps (a, b, _mm256_mul_ps (a_shuffled, b_shuffled));

        // imag = ar*bi + ai*br
        __m256 imagPart = _mm256_fmadd_ps (a, b_shuffled, _mm256_mul_ps (a_shuffled, b));

        // interleave real/imag back
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

        // separate real and imag for a and b
        const __m128 a_shuffled = _mm_shuffle_ps (a, a, _MM_SHUFFLE (2, 3, 0, 1));
        const __m128 b_shuffled = _mm_shuffle_ps (b, b, _MM_SHUFFLE (2, 3, 0, 1));

        // real = ar*br - ai*bi
        __m128 realPart = _mm_sub_ps (_mm_mul_ps (a, b), _mm_mul_ps (a_shuffled, b_shuffled));

        // imag = ar*bi + ai*br
        __m128 imagPart = _mm_add_ps (_mm_mul_ps (a, b_shuffled), _mm_mul_ps (a_shuffled, b));

        // interleave real/imag back
        const __m128 interleaved = _mm_unpacklo_ps (realPart, imagPart);

        y = _mm_add_ps (y, interleaved);
        _mm_storeu_ps (Y + idx, y);
    }

#elif YUP_USE_ARM_NEON
    constexpr int simdWidth = 2; // NEON path: process 2 complex pairs (4 floats) at a time
    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        float32x4_t a = vld1q_f32 (A + idx); // [ar0, ai0, ar1, ai1]
        float32x4_t b = vld1q_f32 (B + idx); // [br0, bi0, br1, bi1]
        float32x4_t y = vld1q_f32 (Y + idx);

        // Shuffle a and b to get swapped real/imag for cross-multiplication
        float32x4_t a_shuf = vrev64q_f32 (a); // [ai0, ar0, ai1, ar1]
        float32x4_t b_shuf = vrev64q_f32 (b); // [bi0, br0, bi1, br1]

        // real = ar*br - ai*bi
        float32x4_t realPart = vsubq_f32 (vmulq_f32 (a, b), vmulq_f32 (a_shuf, b_shuf));

        // imag = ar*bi + ai*br
        float32x4_t imagPart = vaddq_f32 (vmulq_f32 (a, b_shuf), vmulq_f32 (a_shuf, b));

        // Interleave real and imag: [real0, imag0, real1, imag1]
        float32x2_t realLow = vget_low_f32 (realPart);
        float32x2_t imagLow = vget_low_f32 (imagPart);
        float32x2x2_t zippedLow = vzip_f32 (realLow, imagLow);

        float32x2_t realHigh = vget_high_f32 (realPart);
        float32x2_t imagHigh = vget_high_f32 (imagPart);
        float32x2x2_t zippedHigh = vzip_f32 (realHigh, imagHigh);

        float32x4_t interleaved = vcombine_f32 (zippedLow.val[0], zippedHigh.val[0]);

        y = vaddq_f32 (y, interleaved);
        vst1q_f32 (Y + idx, y);
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

        // (ar + j*ai) * (br + j*bi) = (ar*br - ai*bi) + j*(ar*bi + ai*br)
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

        tapsReversed_ = std::move (taps);
        std::reverse (tapsReversed_.begin(), tapsReversed_.end());

        numTaps_ = tapsReversed_.size();
        paddedLen_ = (numTaps_ + 3u) & ~3u;
        tapsReversed_.resize (paddedLen_, 0.0f);

        history_.assign (2 * numTaps_, 0.0f);
        writeIndex_ = 0;
    }

    void reset()
    {
        std::fill (history_.begin(), history_.end(), 0.0f);
        writeIndex_ = 0;
    }

    void process (const float* input, float* output, std::size_t numSamples) noexcept
    {
        const std::size_t M = numTaps_;
        if (M == 0)
            return;

        const float* h = tapsReversed_.data();
        for (std::size_t i = 0; i < numSamples; ++i)
        {
            const float x = input[i];

            history_[writeIndex_] = x;
            history_[writeIndex_ + M] = x;

            const float* w = history_.data() + writeIndex_ + 1;

            float sum = 0.0f;

#if YUP_ENABLE_VDSP
            vDSP_dotpr (w, 1, h, 1, &sum, M);
#else
            sum = dotProduct (w, h, M);
#endif

            output[i] += sum;

            if (++writeIndex_ == M)
                writeIndex_ = 0;
        }
    }

    std::size_t getNumTaps() const
    {
        return numTaps_;
    }

private:
    static float dotProduct (const float* __restrict a, const float* __restrict b, std::size_t len) noexcept
    {
        float acc = 0.0f;
        std::size_t i = 0;

#if YUP_USE_AVX_INTRINSICS && YUP_USE_FMA_INTRINSICS
        // 8-wide AVX2 FMA path
        __m256 vacc = _mm256_setzero_ps();
        for (; i + 8 <= len; i += 8)
        {
            __m256 va = _mm256_loadu_ps (a + i);
            __m256 vb = _mm256_loadu_ps (b + i);
            vacc = _mm256_fmadd_ps (va, vb, vacc);
        }
        // horizontal add
        __m128 low = _mm256_castps256_ps128 (vacc);
        __m128 high = _mm256_extractf128_ps (vacc, 1);
        __m128 vsum = _mm_add_ps (low, high);
        vsum = _mm_hadd_ps (vsum, vsum);
        vsum = _mm_hadd_ps (vsum, vsum);
        acc += _mm_cvtss_f32 (vsum);

#elif YUP_USE_SSE_INTRINSICS
        __m128 vacc = _mm_setzero_ps();
        std::size_t i = 0;
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
        // horizontal add
        __m128 shuf = _mm_movehdup_ps (vacc);
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

    std::vector<float> tapsReversed_;
    std::vector<float> history_;
    std::size_t numTaps_ = 0;
    std::size_t paddedLen_ = 0;
    std::size_t writeIndex_ = 0;
};

//==============================================================================

class PartitionedConvolver::FFTLayer
{
public:
    FFTLayer() = default;
    ~FFTLayer() = default;

    FFTLayer (FFTLayer&& other) = default;
    FFTLayer& operator= (FFTLayer&& other) = default;

    void configure (int hopSize)
    {
        hopSize_ = hopSize;
        fftSize_ = hopSize * 2;

        fftProcessor_.setSize (fftSize_);
        fftProcessor_.setScaling (FFTProcessor::FFTScaling::asymmetric);

        overlapBuffer_.assign (static_cast<std::size_t> (hopSize_), 0.0f);
        timeBuffer_.assign (static_cast<std::size_t> (fftSize_), 0.0f);
        frequencyBuffer_.assign (static_cast<std::size_t> (fftSize_) * 2, 0.0f);
        tempBuffer_.assign (static_cast<std::size_t> (fftSize_) * 2, 0.0f); // Must hold complex data for in-place FFT

        fdlIndex_ = 0;
        configured_ = true;
    }

    int getHopSize() const { return hopSize_; }

    int getFFTSize() const { return fftSize_; }

    bool isConfigured() const { return configured_; }

    std::size_t setImpulseResponse (const float* impulseResponse, std::size_t length, float scaling)
    {
        jassert (configured_);

        if (fftSize_ <= 0 || hopSize_ <= 0)
        {
            resetState();
            return 0;
        }

        frequencyPartitions_.clear();
        frequencyDelayLine_.clear();

        if (length == 0 || impulseResponse == nullptr)
        {
            resetState();
            return 0;
        }

        const auto numPartitions = (length + static_cast<std::size_t> (hopSize_) - 1) / static_cast<std::size_t> (hopSize_);
        if (numPartitions == 0)
        {
            resetState();
            return 0;
        }

        std::size_t processedSamples = 0;
        frequencyPartitions_.reserve (numPartitions);

        for (std::size_t p = 0; p < numPartitions; ++p)
        {
            std::vector<float> partition;
            partition.resize (static_cast<std::size_t> (fftSize_) * 2);

            std::fill (tempBuffer_.begin(), tempBuffer_.end(), 0.0f);

            const std::size_t offset = p * static_cast<std::size_t> (hopSize_);
            const std::size_t copyCount = std::min (static_cast<std::size_t> (hopSize_), length - offset);

            if (copyCount > 0 && offset < length)
            {
                for (std::size_t i = 0; i < copyCount && offset + i < length; ++i)
                    tempBuffer_[i] = impulseResponse[offset + i] * scaling;
            }

            fftProcessor_.performRealFFTForward (tempBuffer_.data(), partition.data());

            frequencyPartitions_.push_back (std::move (partition));

            processedSamples += copyCount;
        }

        frequencyDelayLine_.assign (numPartitions, std::vector<float> (static_cast<std::size_t> (fftSize_) * 2, 0.0f));
        fdlIndex_ = 0;

        resetState();

        return processedSamples;
    }

    void resetState()
    {
        fdlIndex_ = 0;

        for (auto& partition : frequencyDelayLine_)
            std::fill (partition.begin(), partition.end(), 0.0f);

        std::fill (overlapBuffer_.begin(), overlapBuffer_.end(), 0.0f);
        std::fill (timeBuffer_.begin(), timeBuffer_.end(), 0.0f);
        std::fill (frequencyBuffer_.begin(), frequencyBuffer_.end(), 0.0f);
    }

    void processHop (const float* inputHop, float* outputAccumulator)
    {
        jassert (configured_);

        if (frequencyPartitions_.empty())
            return;

        // 1) Transform current input hop to frequency domain
        FloatVectorOperations::copy (tempBuffer_.data(), inputHop, hopSize_);
        fftProcessor_.performRealFFTForward (tempBuffer_.data(), tempBuffer_.data());

        // 2) Store in frequency delay line (circular buffer) - copy full complex buffer
        fdlIndex_ = (fdlIndex_ == 0) ? static_cast<int> (frequencyDelayLine_.size()) - 1 : fdlIndex_ - 1;
        std::copy (tempBuffer_.begin(), tempBuffer_.begin() + (fftSize_ * 2), frequencyDelayLine_[static_cast<std::size_t> (fdlIndex_)].begin());

        // 3) Frequency domain convolution: Y = sum(X[k-p] * H[p])
        FloatVectorOperations::clear (frequencyBuffer_.data(), fftSize_ * 2);

        int xIndex = fdlIndex_;
        for (std::size_t p = 0; p < frequencyPartitions_.size(); ++p)
        {
            const float* X = frequencyDelayLine_[static_cast<std::size_t> (xIndex)].data();
            const float* H = frequencyPartitions_[p].data();

            // fftSize_/2 gives the number of complex pairs for real FFT
            complexMultiplyAccumulate (X, H, frequencyBuffer_.data(), fftSize_ / 2);

            // Move to next older spectrum
            xIndex++;
            if (xIndex >= static_cast<int> (frequencyDelayLine_.size()))
                xIndex = 0;
        }

        // 4) Inverse FFT back to time domain
        fftProcessor_.performRealFFTInverse (frequencyBuffer_.data(), timeBuffer_.data());

        // 5) Overlap-Add: output first hopSize_ samples, store last hopSize_ as overlap
        for (int i = 0; i < hopSize_; ++i)
        {
            outputAccumulator[i] += timeBuffer_[i] + overlapBuffer_[i];
            overlapBuffer_[i] = timeBuffer_[i + hopSize_];
        }
    }

    bool hasImpulseResponse() const { return ! frequencyPartitions_.empty(); }

private:
    int hopSize_ = 0;
    int fftSize_ = 0;

    FFTProcessor fftProcessor_;

    // IR partitions in frequency domain
    std::vector<std::vector<float>> frequencyPartitions_;

    // Frequency Delay Line (most recent at fdlIndex_)
    std::vector<std::vector<float>> frequencyDelayLine_;
    int fdlIndex_ = 0;

    // Processing buffers
    std::vector<float> overlapBuffer_;
    std::vector<float> timeBuffer_;
    std::vector<float> frequencyBuffer_;
    std::vector<float> tempBuffer_;

    bool configured_ = false;
};

//==============================================================================

class PartitionedConvolver::CircularBuffer
{
public:
    CircularBuffer() = default;

    void resize (std::size_t size)
    {
        buffer_.resize (size);
        clear();
    }

    void clear()
    {
        std::fill (buffer_.begin(), buffer_.end(), 0.0f);
        writeIndex_ = 0;
        readIndex_ = 0;
        availableForRead_ = 0;
    }

    std::size_t getAvailableForRead() const { return availableForRead_; }

    std::size_t getAvailableForWrite() const { return buffer_.size() - availableForRead_; }

    std::size_t getSize() const { return buffer_.size(); }

    void write (const float* data, std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForWrite());
        numSamples = std::min (numSamples, getAvailableForWrite());

        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, buffer_.size() - writeIndex_);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (data, data + beforeWrap, buffer_.begin() + writeIndex_);
        if (afterWrap > 0)
            std::copy (data + beforeWrap, data + numSamples, buffer_.begin());

        writeIndex_ = (writeIndex_ + numSamples) % buffer_.size();
        availableForRead_ += numSamples;
    }

    void read (float* data, std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead());

        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, buffer_.size() - readIndex_);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (buffer_.begin() + readIndex_, buffer_.begin() + readIndex_ + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (buffer_.begin(), buffer_.begin() + afterWrap, data + beforeWrap);

        readIndex_ = (readIndex_ + numSamples) % buffer_.size();
        availableForRead_ -= numSamples;
    }

    void peek (float* data, std::size_t numSamples, std::size_t offset = 0) const
    {
        jassert (numSamples + offset <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead() - offset);

        if (numSamples == 0)
            return;

        const std::size_t startIndex = (readIndex_ + offset) % buffer_.size();
        const std::size_t beforeWrap = std::min (numSamples, buffer_.size() - startIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (buffer_.begin() + startIndex, buffer_.begin() + startIndex + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (buffer_.begin(), buffer_.begin() + afterWrap, data + beforeWrap);
    }

    void skip (std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead());

        readIndex_ = (readIndex_ + numSamples) % buffer_.size();
        availableForRead_ -= numSamples;
    }

private:
    std::vector<float> buffer_;
    std::size_t writeIndex_ = 0;
    std::size_t readIndex_ = 0;
    std::size_t availableForRead_ = 0;
};

//==============================================================================

class PartitionedConvolver::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    void configureLayers (std::size_t directFIRTaps, const std::vector<LayerSpec>& layers)
    {
        directFIRTapCount_ = directFIRTaps;

        layers_.clear();
        layers_.resize (layers.size());

        std::size_t maximumHopSize = 0;

        baseHopSize_ = layers.empty() ? 0 : layers.front().hopSize;
        for (std::size_t i = 0; i < layers.size(); ++i)
        {
            layers_[i].configure (layers[i].hopSize);
            if (i == 0)
                baseHopSize_ = layers[i].hopSize;
            else
                baseHopSize_ = std::min (baseHopSize_, layers[i].hopSize);

            maximumHopSize = std::max (maximumHopSize, static_cast<std::size_t> (layers[i].hopSize));
        }

        maxHopSize_ = maximumHopSize;

        // Clear staging buffers - will be allocated in prepare()
        inputStaging_.clear();
        outputStaging_.clear();

        // Resize per-layer circular buffers - will be allocated in prepare()
        layerInputBuffers_.resize (layers.size());
        layerOutputBuffers_.resize (layers.size());

        layerTempOutput_.clear();
        tempLayerHop_.clear();

        // Clear working buffers - will be allocated in prepare()
        workingInput_.clear();
        workingOutput_.clear();

        isPrepared_ = false;
    }

    void prepare (std::size_t maxBlockSize)
    {
        maxBlockSize_ = maxBlockSize;

        // Prepare main input staging - needs to accumulate up to baseHopSize samples plus incoming block
        const std::size_t inputStagingSize = static_cast<std::size_t> (baseHopSize_) + maxBlockSize;
        inputStaging_.resize (inputStagingSize);
        outputStaging_.assign (static_cast<std::size_t> (baseHopSize_), 0.0f);

        // Prepare per-layer circular buffers with layer-specific sizing
        for (std::size_t i = 0; i < layerInputBuffers_.size(); ++i)
        {
            const std::size_t layerHopSize = static_cast<std::size_t> (layers_[i].getHopSize());

            // Input buffer: needs to accumulate up to layerHopSize samples plus incoming block
            const std::size_t layerInputBufferSize = layerHopSize + maxBlockSize;
            layerInputBuffers_[i].resize (layerInputBufferSize);

            // Output buffer: needs to handle bursts of layerHopSize samples
            // Size it to handle multiple hops since read rate (baseHopSize) may be much smaller than write rate (layerHopSize)
            const std::size_t layerOutputBufferSize = layerHopSize * ((layerHopSize / static_cast<std::size_t> (baseHopSize_)) + 2);
            layerOutputBuffers_[i].resize (layerOutputBufferSize);
        }

        // Allocate temp buffers
        if (maxHopSize_ > 0)
        {
            layerTempOutput_.resize (maxHopSize_);
            tempLayerHop_.resize (maxHopSize_);
        }

        // Allocate working buffers
        workingInput_.resize (maxBlockSize);
        workingOutput_.resize (maxBlockSize);

        isPrepared_ = true;
    }

    void setImpulseResponse (const float* impulseResponse, std::size_t length, const PartitionedConvolver::IRLoadOptions& options)
    {
        DirectFIR newFIR;
        std::vector<FFTLayer> newLayers (layers_.size());

        // Safety check
        if (impulseResponse != nullptr && length > 0)
        {
            // Always apply peak headroom
            float headroomScale = std::pow (10.0f, options.headroomDb / 20.0f);
            if (options.normalize)
            {
                const auto minMax = FloatVectorOperations::findMinAndMax (impulseResponse, length);

                const float peak = std::max (std::abs (minMax.getStart()), std::abs (minMax.getEnd()));
                if (peak > 0.0f)
                    headroomScale /= peak;
            }

            // Update DirectFIR in-place
            std::vector<float> directTaps;

            const auto directTapsCount = std::min (directFIRTapCount_, length);
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
                layer.configure (layers_[i].getHopSize());

                const std::size_t remaining = (consumed < length) ? (length - consumed) : 0;
                if (remaining == 0)
                {
                    layer.setImpulseResponse (nullptr, 0, headroomScale);
                    continue;
                }

                consumed += layer.setImpulseResponse (impulseResponse + consumed, remaining, headroomScale);
            }
        }

        {
            SpinLock::ScopedLockType lock (processingLock_);

            directFIR_ = std::move (newFIR);
            layers_ = std::move (newLayers);

            resetStateUnsafe();
        }
    }

    void reset()
    {
        SpinLock::ScopedLockType lock (processingLock_);

        resetStateUnsafe();
    }

    void process (const float* input, float* output, std::size_t numSamples)
    {
        if (numSamples == 0)
            return;

        SpinLock::ScopedLockType lock (processingLock_);

        processUnsafe (input, output, numSamples);
    }

private:
    void resetStateUnsafe()
    {
        directFIR_.reset();
        inputStagingReadIndex_ = 0;
        inputStagingWriteIndex_ = 0;
        inputStagingAvailable_ = 0;
        std::fill (outputStaging_.begin(), outputStaging_.end(), 0.0f);

        for (auto& buffer : layerInputBuffers_)
            buffer.clear();

        for (auto& buffer : layerOutputBuffers_)
            buffer.clear();

        for (auto& layer : layers_)
            layer.resetState();
    }

    void processUnsafe (const float* input, float* output, std::size_t numSamples)
    {
        jassert (isPrepared_);
        jassert (numSamples <= maxBlockSize_);
        if (! isPrepared_ || numSamples > maxBlockSize_)
            return;

        FloatVectorOperations::copy (workingInput_.data(), input, numSamples);
        FloatVectorOperations::clear (workingOutput_.data(), numSamples);

        // Process direct FIR (no block size constraints)
        directFIR_.process (workingInput_.data(), workingOutput_.data(), numSamples);
        if (layers_.empty())
        {
            FloatVectorOperations::add (output, workingOutput_.data(), numSamples);
            return;
        }

        // Add input to main input staging buffer using circular buffer logic
        writeToInputStaging (workingInput_.data(), numSamples);

        std::size_t outputSamplesProduced = 0;
        while (getInputStagingAvailable() >= static_cast<std::size_t> (baseHopSize_))
        {
            const std::size_t hopSize = static_cast<std::size_t> (baseHopSize_);

            // Read hop from input staging
            readFromInputStaging (tempLayerHop_.data(), hopSize);
            FloatVectorOperations::clear (outputStaging_.data(), outputStaging_.size());

            for (std::size_t layerIndex = 0; layerIndex < layers_.size(); ++layerIndex)
            {
                auto& layer = layers_[layerIndex];
                if (! layer.hasImpulseResponse())
                    continue;

                const int layerHopSize = layer.getHopSize();
                auto& inputBuffer = layerInputBuffers_[layerIndex];
                auto& outputBuffer = layerOutputBuffers_[layerIndex];

                // Write input hop to layer's input buffer
                inputBuffer.write (tempLayerHop_.data(), hopSize);

                // Process complete layer hops
                while (inputBuffer.getAvailableForRead() >= static_cast<std::size_t> (layerHopSize))
                {
                    // Read a full hop for this layer
                    inputBuffer.read (tempLayerHop_.data(), static_cast<std::size_t> (layerHopSize));
                    FloatVectorOperations::clear (layerTempOutput_.data(), layerHopSize);

                    // Process hop
                    layer.processHop (tempLayerHop_.data(), layerTempOutput_.data());

                    // Write output to layer's output buffer
                    outputBuffer.write (layerTempOutput_.data(), static_cast<std::size_t> (layerHopSize));
                }

                // Mix available output from this layer
                if (outputBuffer.getAvailableForRead() >= hopSize)
                {
                    outputBuffer.read (layerTempOutput_.data(), hopSize);
                    FloatVectorOperations::add (outputStaging_.data(), layerTempOutput_.data(), hopSize);
                }
            }

            // Add staging output to main output
            const std::size_t samplesToWrite = std::min (hopSize, numSamples - outputSamplesProduced);
            FloatVectorOperations::add (workingOutput_.data() + outputSamplesProduced, outputStaging_.data(), samplesToWrite);
            outputSamplesProduced += samplesToWrite;
        }

        // Copy final result to output (accumulate)
        FloatVectorOperations::add (output, workingOutput_.data(), numSamples);
    }

private:
    void writeToInputStaging (const float* data, std::size_t numSamples)
    {
        const std::size_t available = inputStaging_.size() - inputStagingAvailable_;
        jassert (numSamples <= available);
        numSamples = std::min (numSamples, available);
        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, inputStaging_.size() - inputStagingWriteIndex_);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (data, data + beforeWrap, inputStaging_.begin() + inputStagingWriteIndex_);
        if (afterWrap > 0)
            std::copy (data + beforeWrap, data + numSamples, inputStaging_.begin());

        inputStagingWriteIndex_ = (inputStagingWriteIndex_ + numSamples) % inputStaging_.size();
        inputStagingAvailable_ += numSamples;
    }

    void readFromInputStaging (float* data, std::size_t numSamples)
    {
        jassert (numSamples <= inputStagingAvailable_);
        numSamples = std::min (numSamples, inputStagingAvailable_);
        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, inputStaging_.size() - inputStagingReadIndex_);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (inputStaging_.begin() + inputStagingReadIndex_, inputStaging_.begin() + inputStagingReadIndex_ + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (inputStaging_.begin(), inputStaging_.begin() + afterWrap, data + beforeWrap);

        inputStagingReadIndex_ = (inputStagingReadIndex_ + numSamples) % inputStaging_.size();
        inputStagingAvailable_ -= numSamples;
    }

    std::size_t getInputStagingAvailable() const { return inputStagingAvailable_; }

    std::size_t directFIRTapCount_ = 0;
    int baseHopSize_ = 0;
    std::size_t maxHopSize_ = 0;
    std::size_t maxBlockSize_ = 0;
    bool isPrepared_ = false;

    DirectFIR directFIR_;
    std::vector<FFTLayer> layers_;

    // Working buffers
    std::vector<float> workingInput_;
    std::vector<float> workingOutput_;

    // Input staging with circular buffer management
    std::vector<float> inputStaging_;
    std::size_t inputStagingReadIndex_ = 0;
    std::size_t inputStagingWriteIndex_ = 0;
    std::size_t inputStagingAvailable_ = 0;
    std::vector<float> outputStaging_;

    // Per-layer circular buffering
    std::vector<CircularBuffer> layerInputBuffers_;
    std::vector<CircularBuffer> layerOutputBuffers_;
    std::vector<float> tempLayerHop_;
    std::vector<float> layerTempOutput_;

    mutable SpinLock processingLock_;
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
        layerSpecs.push_back ({ hop });

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
