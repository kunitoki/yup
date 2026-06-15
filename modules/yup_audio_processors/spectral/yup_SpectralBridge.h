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
/**
    A time-domain AudioProcessor that bridges to a frequency-domain SpectralProcessor
    via STFT (Short-Time Fourier Transform) with weighted overlap-add.

    The bridge performs real FFT/IFFT conversion, windowing, and overlap-add so that
    the wrapped SpectralProcessor can operate purely in the frequency domain. Set the
    FFT size and overlap factor to control the time-frequency resolution tradeoff.

    Latency is equal to the current FFT size.

    Typical usage:
    @code
    auto bridge = std::make_unique<SpectralBridge> ("MyBridge", AudioBusLayout { ... });
    bridge->setFFTSize (1024);
    bridge->setOverlapFactor (4);       // 75% overlap, hop = fftSize / 4
    bridge->setSpectralProcessor (mySpectralProcessor);
    bridge->prepareToPlay (AudioSpec (44100.0f, 512, 2));
    bridge->processBlock (context);
    @endcode

    @tags{Audio, Spectral, Realtime}
*/
class YUP_API SpectralBridge : public AudioProcessor
{
public:
    //==============================================================================
    /** Constructs a SpectralBridge with the given name and bus layout.

        @param name       The processor name.
        @param busLayout  The input/output bus layout.
    */
    SpectralBridge (StringRef name, AudioBusLayout busLayout);

    /** Destructor. */
    ~SpectralBridge() override;

    //==============================================================================
    /** Assigns the spectral processor to be applied in the frequency domain.

        The bridge takes shared ownership of the processor. Parameters from the
        spectral processor are registered on the bridge so they are visible to hosts.

        @param processor  The spectral processor, or nullptr to disable processing.
    */
    void setSpectralProcessor (std::shared_ptr<SpectralProcessor> processor);

    /** Returns the current spectral processor, or nullptr. */
    std::shared_ptr<SpectralProcessor> getSpectralProcessor() const noexcept;

    //==============================================================================
    /** Sets the FFT size (must be a power of two).

        The change takes effect on the next call to prepareToPlay(). The processor
        latency is updated immediately for host discovery.

        @param fftSize  The new FFT size.
    */
    void setFFTSize (int fftSize);

    /** Returns the current FFT size. */
    int getFFTSize() const noexcept;

    //==============================================================================
    /** Sets the overlap factor for the STFT.

        The hop size is computed as fftSize / overlapFactor. Higher values give
        smoother time-varying effects at the cost of increased CPU usage.
        Common values: 4 (75% overlap), 2 (50% overlap).

        The change takes effect on the next call to prepareToPlay().

        @param overlapFactor  The overlap factor (must be >= 1).
    */
    void setOverlapFactor (int overlapFactor);

    /** Returns the current overlap factor. */
    int getOverlapFactor() const noexcept;

    //==============================================================================
    /** Sets the window type used for both analysis and synthesis windowing.

        The change takes effect on the next call to prepareToPlay(). The default
        is WindowType::hann, which provides good frequency selectivity and COLA
        properties with 75% overlap.

        @param type  The window type to use.
    */
    void setWindowType (WindowType type);

    /** Returns the current window type. */
    WindowType getWindowType() const noexcept;

    //==============================================================================
    /** Sets an optional parameter used by parameterizable window types.

        Relevant windows:
        - Kaiser: beta (default 8.0, higher = more sidelobe attenuation)
        - Gaussian: sigma (default 0.4, controls width)
        - Tukey: alpha (default 0.5, taper ratio 0..1)
        - RakshitUllah: r (default 1.0, controlling parameter)

        The change takes effect on the next call to prepareToPlay().

        @param parameter  The window-specific parameter value.
    */
    void setWindowParameter (float parameter);

    /** Returns the current window parameter. */
    float getWindowParameter() const noexcept;

    //==============================================================================
    /** @internal */
    void prepareToPlay (const AudioSpec& spec) override;
    /** @internal */
    void releaseResources() override;
    /** @internal */
    void processBlock (AudioProcessContext<float>& context) override;
    /** @internal */
    void flush() override;

    /** @internal */
    int getLatencySamples() override { return fftSize; }

    /** @internal */
    bool hasEditor() const override;
    /** @internal */
    int getCurrentPreset() const noexcept override;
    /** @internal */
    void setCurrentPreset (int) noexcept override;
    /** @internal */
    int getNumPresets() const override;
    /** @internal */
    String getPresetName (int) const override;
    /** @internal */
    void setPresetName (int, StringRef) override;
    /** @internal */
    bool supportsDataTreeState() const noexcept override;
    /** @internal */
    Result loadStateFromDataTree (const DataTree& state) override;
    /** @internal */
    Result saveStateIntoDataTree (DataTree& state) override;

private:
    //==============================================================================
    struct ChannelState
    {
        std::vector<float> inputRing;
        std::vector<float> outputRing;
        std::vector<float> workBuf;

        int64 samplesWritten = 0;
        int64 nextFrameStart = 0;
    };

    //==============================================================================
    static size_t wrapIndex (int64 position, int size) noexcept
    {
        jassert (position >= 0);
        jassert (size > 0);

        return static_cast<size_t> (position % static_cast<int64> (size));
    }

    //==============================================================================
    void allocateResources();
    void buildWindows();
    bool isPrepared() const;
    void reconfigureIfPrepared();
    void processAvailableFrames (AudioProcessContext<float>& context);

    //==============================================================================
    FFTProcessor fft;
    std::shared_ptr<SpectralProcessor> spectralProcessor;
    SpectralBuffer<float> spectralBuffer;

    std::vector<float> window;
    std::vector<float> colaCompensation;

    int fftSize = 1024;
    int hopSize = 256;
    int overlapFactor = 4;
    int numBins = 0;
    WindowType windowType = WindowType::hann;
    float windowParameter = 8.0f;

    std::vector<ChannelState> channelState;

    ParameterChangeBuffer spectralParams;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralBridge)
};

} // namespace yup
