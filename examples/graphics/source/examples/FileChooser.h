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

class FileChooserDemo : public yup::Component
{
public:
    FileChooserDemo()
        : Component ("FileChooserDemo")
        , openFile ("Open File")
        , openMultipleFiles ("Multiple Files")
    {
        addAndMakeVisible (openFile);
        openFile.onClick = [this]
        {
            yup::FileChooser chooser ("Select a file", yup::File::getCurrentWorkingDirectory(), "*.txt");
            if (chooser.browseForFileToOpen())
            {
                yup::File selectedFile = chooser.getResult();
                std::cout << "Selected file: " << selectedFile.getFullPathName() << std::endl;
            }
        };

        addAndMakeVisible (openMultipleFiles);
        openMultipleFiles.onClick = [this]
        {
            yup::FileChooser chooser ("Select multiple files", yup::File::getCurrentWorkingDirectory(), "*");
            if (chooser.browseForMultipleFilesToOpen())
            {
                auto selectedFiles = chooser.getResults();
                for (const auto& selectedFile : selectedFiles)
                    std::cout << "Selected file: " << selectedFile.getFullPathName() << std::endl;
            }
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

private:
    yup::TextButton openFile;
    yup::TextButton openMultipleFiles;
};
