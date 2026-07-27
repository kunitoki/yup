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

//==============================================================================

class FileChooserDemo : public yup::Component
{
public:
    FileChooserDemo()
        : Component ("FileChooserDemo")
        , openFile ("Open File")
        , openMultipleFiles ("Multiple Files")
    {
        setOpaque (false);

        addAndMakeVisible (openFile);
        openFile.onClick = [this]
        {
            auto chooser = yup::FileChooser::create ("Select a file", yup::File::getCurrentWorkingDirectory(), "*.txt");
            chooser->browseForFileToOpen ([] (bool success, const yup::Array<yup::File>& results)
            {
                if (success && results.size() > 0)
                {
                    yup::File selectedFile = results[0];
                    std::cout << "Selected file: " << selectedFile.getFullPathName() << std::endl;
                }
                else
                {
                    std::cout << "Failure selecting file !" << std::endl;
                }
            });
        };

        addAndMakeVisible (openMultipleFiles);
        openMultipleFiles.onClick = [this]
        {
            auto chooser = yup::FileChooser::create ("Select multiple files", yup::File::getCurrentWorkingDirectory(), "*");
            chooser->browseForMultipleFilesToOpen ([] (bool success, const yup::Array<yup::File>& results)
            {
                if (success && results.size() > 0)
                {
                    for (const auto& selectedFile : results)
                        std::cout << "Selected file: " << selectedFile.getFullPathName() << std::endl;
                }
                else
                {
                    std::cout << "Failure selecting files !" << std::endl;
                }
            });
        };
    }

    void resized() override
    {
        constexpr int margin = 5;
        constexpr int buttonWidth = 100;
        constexpr int buttonHeight = 30;

        auto bounds = getLocalBounds().reduced (margin);

        auto buttons1 = bounds.removeFromTop (buttonHeight);
        openFile.setBounds (buttons1.removeFromLeft (buttonWidth));
        buttons1.removeFromLeft (margin);
        openMultipleFiles.setBounds (buttons1.removeFromLeft (buttonWidth));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

private:
    yup::TextButton openFile;
    yup::TextButton openMultipleFiles;
};
