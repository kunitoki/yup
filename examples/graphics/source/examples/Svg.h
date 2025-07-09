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

class SvgDemo : public yup::Component
{
public:
    SvgDemo()
    {
        updateListOfSvgFiles();

        parseSvgFile (currentSvgFileIndex);
    }

    void resized() override
    {
        //drawable.setBounds (getLocalBounds());
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        ++currentSvgFileIndex;

        parseSvgFile (currentSvgFileIndex);
    }

    void paint (Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        drawable.paint (g, getLocalBounds());
    }

private:
    void updateListOfSvgFiles()
    {
        yup::File riveBasePath = yup::File (__FILE__)
                                     .getParentDirectory()
                                     .getParentDirectory()
                                     .getParentDirectory();

        auto files = riveBasePath.getChildFile ("data/svg").findChildFiles (yup::File::findFiles, false, "*.svg");
        if (files.isEmpty())
            return;

        for (const auto& svgFile : files)
            svgFiles.add (svgFile);
    }

    void parseSvgFile (int index)
    {
        if (svgFiles.isEmpty())
            return;

        if (index < 0)
            index = svgFiles.size() - 1;

        if (index >= svgFiles.size())
            index = 0;

        currentSvgFileIndex = index;

        YUP_DBG ("Showing " << svgFiles[currentSvgFileIndex].getFullPathName());

        drawable.clear();
        drawable.parseSVG (svgFiles[currentSvgFileIndex]);

        repaint();
    }

    yup::Drawable drawable;
    Array<yup::File> svgFiles;
    int currentSvgFileIndex = 0;
};

} // namespace yup
