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

#include "images/ImageViewerPage.h"
#include "images/ImageMeshPage.h"
#include "images/OffscreenRenderPage.h"

//==============================================================================
/**
    Demonstrates images, one page per topic:

    - Viewer: loading, saving and playing animated images in every registered format.
    - Mesh: warping an image with Graphics::drawImageMesh() and drawing thousands of
      sprites with one Graphics::drawImageMeshInstanced() call.
    - Offscreen: rendering into a GPU texture with Graphics (context, image), then drawing it.
*/
class ImagesDemo final : public yup::Component
{
public:
    ImagesDemo()
        : yup::Component ("ImagesDemo")
    {
        setTitle ("Images");

        tabs.addTab ("viewer", "Viewer", std::make_unique<ImageViewerPage>());
        tabs.addTab ("mesh", "Mesh", std::make_unique<ImageMeshPage>());
        tabs.addTab ("offscreen", "Offscreen", std::make_unique<OffscreenRenderPage>());
        tabs.getTabBar().setVariant (yup::TabBar::Variant::underline);
        addAndMakeVisible (tabs);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        tabs.setBounds (getLocalBounds());
    }

private:
    yup::TabComponent tabs { "imagesTabs" };
};
