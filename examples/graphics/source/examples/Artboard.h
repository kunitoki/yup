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

namespace yup
{

class ArtboardDemo : public yup::Component
{
public:
    ArtboardDemo()
    {
        setWantsKeyboardFocus (true);
    }

    bool loadArtboard()
    {
        auto factory = getNativeComponent()->getFactory();
        if (factory == nullptr)
            return false;

#if JUCE_ANDROID
        yup::MemoryInputStream is (yup::RiveFile_data, yup::RiveFile_size, false);
        auto artboardFile = yup::ArtboardFile::load (is, *factory);

#else
        yup::File riveBasePath;
#if YUP_WASM
        riveBasePath = yup::File ("/");
#else
        riveBasePath = yup::File (__FILE__)
                           .getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory();
#endif

        auto artboardFile = yup::ArtboardFile::load (riveBasePath.getChildFile (YUP_EXAMPLE_GRAPHICS_RIVE_FILE), *factory);
#endif
        if (! artboardFile)
            return false;

        // Setup artboards
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto art = artboards.add (std::make_unique<yup::Artboard> (yup::String ("art") + yup::String (i)));
            addAndMakeVisible (art);

            art->setFile (artboardFile.getValue());

            art->advanceAndApply (i * art->durationSeconds());
        }

        return true;
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        repaint();
    }

    void resized() override
    {
        //for (int i = 0; i < totalRows * totalColumns; ++i)
        //    artboards.getUnchecked (i)->setBounds (getLocalBounds().reduced (100.0f));

        if (artboards.size() != totalRows * totalColumns)
            return;

        auto bounds = getLocalBounds().reduced (10, 20);
        auto width = bounds.getWidth() / totalColumns;
        auto height = bounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows; ++i)
        {
            auto row = bounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                artboards.getUnchecked (j * totalRows + i)->setBounds (col.largestFittingSquare());
            }
        }
    }

private:
    yup::OwnedArray<yup::Artboard> artboards;
    int totalRows = 1;
    int totalColumns = 1;
};

} // namespace yup
