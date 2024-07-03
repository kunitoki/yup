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

#include "oboe.h"

#if __ANDROID__
#include "upstream/src/aaudio/AAudioLoader.cpp"
#include "upstream/src/aaudio/AudioStreamAAudio.cpp"
#include "upstream/src/common/AdpfWrapper.cpp"
#include "upstream/src/common/AudioSourceCaller.cpp"
#include "upstream/src/common/AudioStream.cpp"
#include "upstream/src/common/AudioStreamBuilder.cpp"
#include "upstream/src/common/DataConversionFlowGraph.cpp"
#include "upstream/src/common/FilterAudioStream.cpp"
#include "upstream/src/common/FixedBlockAdapter.cpp"
#include "upstream/src/common/FixedBlockReader.cpp"
#include "upstream/src/common/FixedBlockWriter.cpp"
#include "upstream/src/common/LatencyTuner.cpp"
#include "upstream/src/common/OboeExtensions.cpp"
#include "upstream/src/common/SourceFloatCaller.cpp"
#include "upstream/src/common/SourceI16Caller.cpp"
#include "upstream/src/common/SourceI24Caller.cpp"
#include "upstream/src/common/SourceI32Caller.cpp"
#include "upstream/src/common/Utilities.cpp"
#include "upstream/src/common/QuirksManager.cpp"
#include "upstream/src/fifo/FifoBuffer.cpp"
#include "upstream/src/fifo/FifoController.cpp"
#include "upstream/src/fifo/FifoControllerBase.cpp"
#include "upstream/src/fifo/FifoControllerIndirect.cpp"
#include "upstream/src/flowgraph/FlowGraphNode.cpp"
#include "upstream/src/flowgraph/ChannelCountConverter.cpp"
#include "upstream/src/flowgraph/ClipToRange.cpp"
#include "upstream/src/flowgraph/Limiter.cpp"
#include "upstream/src/flowgraph/ManyToMultiConverter.cpp"
#include "upstream/src/flowgraph/MonoBlend.cpp"
#include "upstream/src/flowgraph/MonoToMultiConverter.cpp"
#include "upstream/src/flowgraph/MultiToManyConverter.cpp"
#include "upstream/src/flowgraph/MultiToMonoConverter.cpp"
#include "upstream/src/flowgraph/RampLinear.cpp"
#include "upstream/src/flowgraph/SampleRateConverter.cpp"
#include "upstream/src/flowgraph/SinkFloat.cpp"
#include "upstream/src/flowgraph/SinkI16.cpp"
#include "upstream/src/flowgraph/SinkI24.cpp"
#include "upstream/src/flowgraph/SinkI32.cpp"
#include "upstream/src/flowgraph/SourceFloat.cpp"
#include "upstream/src/flowgraph/SourceI16.cpp"
#include "upstream/src/flowgraph/SourceI24.cpp"
#include "upstream/src/flowgraph/SourceI32.cpp"
#include "upstream/src/flowgraph/resampler/IntegerRatio.cpp"
#include "upstream/src/flowgraph/resampler/LinearResampler.cpp"
#include "upstream/src/flowgraph/resampler/MultiChannelResampler.cpp"
#include "upstream/src/flowgraph/resampler/PolyphaseResampler.cpp"
#include "upstream/src/flowgraph/resampler/PolyphaseResamplerMono.cpp"
#include "upstream/src/flowgraph/resampler/PolyphaseResamplerStereo.cpp"
#include "upstream/src/flowgraph/resampler/SincResampler.cpp"
#include "upstream/src/flowgraph/resampler/SincResamplerStereo.cpp"
#include "upstream/src/opensles/AudioInputStreamOpenSLES.cpp"
#include "upstream/src/opensles/AudioOutputStreamOpenSLES.cpp"
#include "upstream/src/opensles/AudioStreamBuffered.cpp"
#include "upstream/src/opensles/AudioStreamOpenSLES.cpp"
#include "upstream/src/opensles/EngineOpenSLES.cpp"
#include "upstream/src/opensles/OpenSLESUtilities.cpp"
#include "upstream/src/opensles/OutputMixerOpenSLES.cpp"
#include "upstream/src/common/StabilizedCallback.cpp"
#include "upstream/src/common/Trace.cpp"
#include "upstream/src/common/Version.cpp"
#endif
