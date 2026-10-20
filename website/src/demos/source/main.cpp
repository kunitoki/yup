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

#include <yup_core/yup_core.h>
#include <yup_events/yup_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>
#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_dsp/yup_dsp.h>

#include <emscripten/emscripten.h>

#include <Snippets/AppSnippet.h>
#include <ShaderBundles/RippleShader.h>
#include <ShaderBundles/SurfaceShader.h>

#include <memory>
#include <optional>

#include "common/Gpu.h"
#include "common/Lift.h"
#include "common/Palettes.h"
#include "common/AudioAnalysis.h"
#include "common/Sound.h"

#include "common/Demo.h"
#include "common/SurfaceView.h"

#include "facet/FacetSound.h"
#include "facet/Facet.h"
#include "keys/KeysSound.h"
#include "keys/Keys.h"
#include "layers/LayersSound.h"
#include "layers/Layers.h"
#include "reel/ReelSound.h"
#include "reel/Reel.h"
#include "ripple/RippleEffect.h"
#include "ripple/RippleSound.h"
#include "ripple/Ripple.h"
#include "start/Start.h"

#include "Stage.h"

//==============================================================================
// Called by the website page, see website/src/home.js.
extern "C"
{
    void EMSCRIPTEN_KEEPALIVE yupWebsiteSetDemo (int index)
    {
        Stage::requestDemo (index);
    }

    void EMSCRIPTEN_KEEPALIVE yupWebsiteSetOpen (int isOpen)
    {
        Stage::requestOpen (isOpen != 0);
    }
}

//==============================================================================
class Application : public yup::YUPApplication
{
public:
    Application() = default;

    yup::String getApplicationName() override
    {
        return "yup website demos";
    }

    yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String&) override
    {
        applyPalette (coldPalette);

        yup::MessageManager::callAsync ([this]
        {
            stage = std::make_unique<Stage>();
            stage->centreWithSize ({ 1280, 800 });
            stage->setVisible (true);
        });
    }

    void shutdown() override
    {
        stage.reset();
    }

private:
    std::unique_ptr<Stage> stage;
};

START_YUP_APPLICATION (Application)
