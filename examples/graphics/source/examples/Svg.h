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

class SvgDemo : public yup::Component
{
public:
    SvgDemo()
    {
        updateListOfSvgFiles();
        loadDemoFont();

        parseSvgFile (currentSvgFileIndex);
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        if (event.isLeftButtonDown())
            ++currentSvgFileIndex;
        else if (event.isRightButtonDown())
            --currentSvgFileIndex;

        parseSvgFile (currentSvgFileIndex);
    }

    void paint (yup::Graphics& g) override
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

        dataDirectory = riveBasePath.getChildFile ("data");

        auto files = riveBasePath.getChildFile ("data/svg").findChildFiles (yup::File::findFiles, false, "*.svg");
        if (files.isEmpty())
            return;

        for (const auto& svgFile : files)
        {
            //if (svgFile.getFileName() == "mozilla2.svg")
            svgFiles.add (svgFile);
        }
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
        drawable.parseSVG (svgFiles[currentSvgFileIndex], createParseOptions (svgFiles[currentSvgFileIndex]));

        repaint();
    }

    void loadDemoFont()
    {
        yup::Font font;
        if (font.loadFromFile (dataDirectory.getChildFile ("RobotoFlex-VariableFont.ttf")).wasOk())
            demoFont = std::move (font);
    }

    yup::Drawable::ParseOptions createParseOptions (const yup::File& svgFile)
    {
        yup::Drawable::ParseOptions options;
        options.baseDirectory = svgFile.getParentDirectory();
        options.fontResolver = [this] (yup::StringRef, float fontSize, int weight, bool italic) -> std::optional<yup::Font>
        {
            if (demoFont)
            {
                auto font = *demoFont;
                font.setAxisValue ("wght", static_cast<float> (weight));
                if (italic)
                    font.setAxisValue ("slnt", -10.0f);
                else
                    font.setAxisValue ("slnt", 0.0f);
                return font.withHeight (fontSize);
            }

            if (auto theme = yup::ApplicationTheme::getGlobalTheme())
                return theme->getDefaultFont().withHeight (fontSize);

            return std::nullopt;
        };

        return options;
    }

    yup::Drawable drawable;
    yup::Array<yup::File> svgFiles;
    yup::File dataDirectory;
    std::optional<yup::Font> demoFont;
    int currentSvgFileIndex = 0;
};
