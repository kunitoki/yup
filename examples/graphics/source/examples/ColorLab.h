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

#pragma once

#include "colorlab/ColorChannelEditor.h"
#include "colorlab/BlendSpacesPage.h"
#include "colorlab/GradientEditorPage.h"
#include "colorlab/ColorPickerPage.h"
#include "colorlab/PalettesPage.h"

//==============================================================================
/**
    Explores colors, one page per topic:

    - Blend spaces: the same two colors mixed in sRGB, linear RGB and spectral space.
    - Gradient: an interactive linear and radial gradient editor.
    - Picker: a color wheel with RGBA, HSL, HSV and HSLuv channel editors.
    - Palettes: HSLuv palettes, spectral mixing and gradients.
*/
class ColorLabDemo final : public yup::Component
{
public:
    ColorLabDemo()
        : yup::Component ("ColorLabDemo")
    {
        setTitle ("Color Lab");

        tabs.addTab ("blend", "Blend spaces", std::make_unique<colorlab::BlendSpacesPage>());
        tabs.addTab ("gradient", "Gradient", std::make_unique<colorlab::GradientEditorPage>());
        tabs.addTab ("picker", "Picker", std::make_unique<colorlab::ColorPickerPage>());
        tabs.addTab ("palettes", "Palettes", std::make_unique<colorlab::PalettesPage>());
        tabs.getTabBar().setVariant (yup::TabBar::Variant::underline);
        addAndMakeVisible (tabs);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (colorlab::paletteColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        tabs.setBounds (getLocalBounds());
    }

private:
    yup::TabComponent tabs { "colorLabTabs" };
};
