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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "yup_UMPProtocols.h"
#include "yup_UMPUtils.h"
#include "yup_UMPacket.h"
#include "yup_UMPSysEx7.h"
#include "yup_UMPView.h"
#include "yup_UMPIterator.h"
#include "yup_UMPackets.h"
#include "yup_UMPFactory.h"
#include "yup_UMPConversion.h"
#include "yup_UMPMidi1ToBytestreamTranslator.h"
#include "yup_UMPMidi1ToMidi2DefaultTranslator.h"
#include "yup_UMPConverters.h"
#include "yup_UMPDispatcher.h"
#include "yup_UMPReceiver.h"

#ifndef DOXYGEN

namespace yup
{
namespace ump = universal_midi_packets;
} // namespace yup

#endif
