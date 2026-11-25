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

namespace yup
{

//==============================================================================
/** Registers the type infos of the audio components a `.yui` document is written with: `MidiKeyboard`,
    `PitchWheel`, `ModWheel`, `SpectrumAnalyzer`, `Spectrogram`, `KMeter`, `AudioView` and `CartesianPlane`.

    A document writes how they look and behave. The state or the data each one shows belongs to the
    application, which gives it once the component is created (see YuiDocument::setInitializer).

    The yup_gui_markup module calls it in the applications linking both, so a `.yui` document
    needs no call. Call it to create the audio components by their type name without that module,
    through the ComponentTypeRegistry.

    @see ComponentTypeRegistry
*/
YUP_API void registerAudioGuiComponentTypes();

} // namespace yup
