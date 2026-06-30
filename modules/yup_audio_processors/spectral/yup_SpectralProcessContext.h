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
    All inputs available to a SpectralProcessor for a single processing block.

    SpectralProcessContext<FloatType> is passed to SpectralProcessor::processBlock() and bundles:
      - the spectral buffer (in-place processing model, single or double precision),
      - sample-accurate parameter automation events, ignoring sampleOffset.

    Use SpectralProcessContext<float> for the primary single-precision processing path.
    Use SpectralProcessContext<double> for double-precision processing in processors that
    override processBlock(SpectralProcessContext<double>&) and return true from
    supportsDoublePrecisionProcessing().

    @see SpectralProcessor, ParameterChangeBuffer, AudioParameterHandle
*/
template <typename FloatType>
struct SpectralProcessContext
{
    /** Raw bins buffer in contiguous [real, imag] pairs per channel. Process in-place: read and write the same channels. */
    SpectralBuffer<FloatType>& bins;

    /** Parameter automation events for this block, ignoring sampleOffset. */
    ParameterChangeBuffer& params;
};

} // namespace yup
