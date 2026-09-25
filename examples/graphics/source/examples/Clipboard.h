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

/**
    Demonstrates copy and paste of text and images via the system clipboard.

    - Text: type into the editor, copy/paste using SystemClipboard.
    - Image: the YUP logo is loaded from disk. "Copy Image" puts the raw PNG
      bytes on the system clipboard as "image/png". "Paste Image" tries to read
      "image/png" from the system clipboard — use it after copying an image from
      another app (browser, screenshot, etc.). A local round-trip slot lets you
      test the decode/display path without leaving the app.
    - MIME types: buttons to inspect available clipboard MIME types and clear
      the clipboard.
*/
class ClipboardDemo : public yup::Component
{
public:
    ClipboardDemo()
        : yup::Component ("ClipboardDemo")
    {
        setWantsKeyboardFocus (true);

        // --- Title ---
        titleLabel = std::make_unique<yup::Label> ("titleLabel");
        titleLabel->setText ("Clipboard Demo");
        addAndMakeVisible (*titleLabel);

        // --- Text editors ---
        textSectionLabel = std::make_unique<yup::Label> ("textSectionLabel");
        textSectionLabel->setText ("Text Clipboard");
        addAndMakeVisible (*textSectionLabel);

        textEditor = std::make_unique<yup::TextEditor> ("textEditor");
        textEditor->setMultiLine (true);
        textEditor->setText ("Type some text here...");
        addAndMakeVisible (*textEditor);

        copyTextButton = std::make_unique<yup::TextButton> ("Copy");
        copyTextButton->onClick = [this]
        {
            yup::SystemClipboard::copyTextToClipboard (textEditor->getText());
            updateStatus ("Text copied to clipboard.");
        };
        addAndMakeVisible (*copyTextButton);

        pasteTextButton = std::make_unique<yup::TextButton> ("Paste");
        pasteTextButton->onClick = [this]
        {
            auto text = yup::SystemClipboard::getTextFromClipboard();
            if (text.isNotEmpty())
            {
                textEditor->setText (text);
                updateStatus ("Text pasted from clipboard.");
            }
            else
            {
                updateStatus ("Clipboard has no text.");
            }
        };
        addAndMakeVisible (*pasteTextButton);

        hasTextButton = std::make_unique<yup::TextButton> ("Has Text?");
        hasTextButton->onClick = [this]
        {
            updateStatus (yup::SystemClipboard::hasClipboardText()
                              ? "Clipboard contains text."
                              : "Clipboard has no text.");
        };
        addAndMakeVisible (*hasTextButton);

        // --- Image ---
        imageSectionLabel = std::make_unique<yup::Label> ("imageSectionLabel");
        imageSectionLabel->setText ("Image Clipboard");
        addAndMakeVisible (*imageSectionLabel);

        copyImageButton = std::make_unique<yup::TextButton> ("Copy");
        copyImageButton->setEnabled (! rawPngData.isEmpty());
        copyImageButton->onClick = [this]
        {
            if (rawPngData.isEmpty())
                return;

            yup::ClipboardData data ("image/png", rawPngData);
            if (yup::SystemClipboard::copyToClipboard (data))
            {
                // Keep a local copy for same-process round-trip testing.
                localImageCopy = rawPngData;
                updateStatus ("Logo (PNG) copied to system clipboard. Use Paste or try pasting into another app.");
            }
            else
            {
                updateStatus ("Failed to copy image to clipboard.");
            }
        };
        addAndMakeVisible (*copyImageButton);

        pasteImageButton = std::make_unique<yup::TextButton> ("Paste");
        pasteImageButton->onClick = [this]
        {
            // Try system clipboard first, then local fallback.
            auto data = yup::SystemClipboard::getFromClipboard ("image/png");

            if (data.data.isEmpty() && ! localImageCopy.isEmpty())
            {
                updateStatus ("Reading from local round-trip buffer...");
                decodeAndDisplayImage (localImageCopy);
                return;
            }

            if (data.data.isEmpty())
            {
                updateStatus ("Clipboard has no 'image/png'. Copy an image from another app first.");
                return;
            }

            decodeAndDisplayImage (data.data);
        };
        addAndMakeVisible (*pasteImageButton);

        mimeSectionLabel = std::make_unique<yup::Label> ("mimeSectionLabel");
        mimeSectionLabel->setText ("Clipboard MIME Types");
        addAndMakeVisible (*mimeSectionLabel);

        refreshMimeButton = std::make_unique<yup::TextButton> ("Refresh");
        refreshMimeButton->onClick = [this]
        {
            mimeTypes = yup::SystemClipboard::getClipboardMimeTypes();
        };
        addAndMakeVisible (*refreshMimeButton);

        clearClipboardButton = std::make_unique<yup::TextButton> ("Clear");
        clearClipboardButton->onClick = [this]
        {
            yup::SystemClipboard::clearClipboardData();
            updateStatus ("Clipboard cleared.");
        };
        addAndMakeVisible (*clearClipboardButton);

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        addAndMakeVisible (*statusLabel);
        updateStatus ("Ready.");

        loadImageAsset();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        auto area = getLocalBounds().reduced (10).to<float>();
        area.removeFromTop (widgetAreaHeight + 8.0f);
        area.removeFromBottom (4.0f);

        if (area.isEmpty())
            return;

        // MIME types text at the bottom
        g.setFillColor (yup::Colors::white.withAlpha (0.7f));
        auto mimeRow = area.removeFromBottom (20.0f);

        if (mimeTypes.isEmpty())
            g.fillFittedText ("MIME types: (none)", yup::Font().withHeight (11.0f), mimeRow, yup::Justification::centerLeft);
        else
            g.fillFittedText ("MIME types: " + mimeTypes.joinIntoString (", "), yup::Font().withHeight (11.0f), mimeRow, yup::Justification::centerLeft);

        // Image display
        auto imageArea = area.reduced (4.0f);

        if (displayedImage.isValid())
        {
            auto saved = g.saveState();
            g.setClipPath (imageArea);

            auto imgW = static_cast<float> (displayedImage.getWidth());
            auto imgH = static_cast<float> (displayedImage.getHeight());
            auto scale = std::min (imageArea.getWidth() / imgW, imageArea.getHeight() / imgH);

            yup::Rectangle<float> dest (
                imageArea.getCenterX() - imgW * scale * 0.5f,
                imageArea.getCenterY() - imgH * scale * 0.5f,
                imgW * scale,
                imgH * scale);

            g.setFillColor (yup::Colors::black);
            g.fillRect (imageArea);
            g.drawImage (displayedImage, dest);
        }
        else
        {
            g.setFillColor (yup::Colors::black.withAlpha (0.25f));
            g.fillRoundedRect (imageArea, 6.0f);
            g.setFillColor (yup::Colors::white.withAlpha (0.4f));
            g.fillFittedText ("Pasted image appears here",
                              yup::Font().withHeight (14.0f),
                              imageArea,
                              yup::Justification::center);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        const int pad = 4;
        const int btnH = 26;
        const int lblH = 18;
        const int editorH = 24;
        const int initialTop = area.getY();

        // Title
        titleLabel->setBounds (area.removeFromTop (24));

        // --- Text section ---
        textSectionLabel->setBounds (area.removeFromTop (lblH));
        area.removeFromTop (pad);

        auto textBtnRow = area.removeFromTop (btnH);
        copyTextButton->setBounds (textBtnRow.removeFromLeft (64).reduced (0, 2));
        textBtnRow.removeFromLeft (pad);
        pasteTextButton->setBounds (textBtnRow.removeFromLeft (64).reduced (0, 2));
        textBtnRow.removeFromLeft (pad);
        hasTextButton->setBounds (textBtnRow.removeFromLeft (80).reduced (0, 2));

        area.removeFromTop (pad);
        textEditor->setBounds (area.removeFromTop (editorH * 3));

        area.removeFromTop (10);

        // --- Image section ---
        imageSectionLabel->setBounds (area.removeFromTop (lblH));
        area.removeFromTop (pad);

        auto imgBtnRow = area.removeFromTop (btnH);
        copyImageButton->setBounds (imgBtnRow.removeFromLeft (64).reduced (0, 2));
        imgBtnRow.removeFromLeft (pad);
        pasteImageButton->setBounds (imgBtnRow.removeFromLeft (64).reduced (0, 2));

        area.removeFromTop (8);

        // --- MIME section ---
        mimeSectionLabel->setBounds (area.removeFromTop (lblH));
        area.removeFromTop (pad);

        auto mimeBtnRow = area.removeFromTop (btnH);
        refreshMimeButton->setBounds (mimeBtnRow.removeFromLeft (80).reduced (0, 2));
        mimeBtnRow.removeFromLeft (pad);
        clearClipboardButton->setBounds (mimeBtnRow.removeFromLeft (60).reduced (0, 2));

        widgetAreaHeight = static_cast<float> (area.getY() - initialTop);

        // Status at the bottom
        statusLabel->setBounds (getLocalBounds().reduced (10).removeFromBottom (20));
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        if (keys.getKey() == 'v' && keys.getModifiers().isCommandDown())
            pasteImageButton->onClick();
    }

private:
    void loadImageAsset()
    {
        auto basePath = yup::File (__FILE__)
                            .getParentDirectory()
                            .getParentDirectory()
                            .getChildFile ("data")
                            .getChildFile ("logo.png");

        if (basePath.loadFileAsData (rawPngData))
            updateStatus ("Logo image loaded from disk.");
        else
            updateStatus ("Could not load logo.png.");
    }

    void decodeAndDisplayImage (const yup::MemoryBlock& pngData)
    {
        if (pngData.isEmpty())
        {
            updateStatus ("No image data to display.");
            return;
        }

        auto result = yup::Image::loadFromData (pngData.asBytes());
        if (result.wasOk())
        {
            if (displayedImage.isValid())
                displayedImage.invalidateTexture();

            displayedImage = std::move (result.getReference());
            updateStatus ("Displaying image (" + yup::String (displayedImage.getWidth())
                          + " x " + yup::String (displayedImage.getHeight()) + ").");
            repaint();
        }
        else
        {
            updateStatus ("Failed to decode image data.");
        }
    }

    void updateStatus (const yup::String& text)
    {
        statusLabel->setText (text, yup::dontSendNotification);
    }

    // Widgets
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> textSectionLabel;
    std::unique_ptr<yup::Label> imageSectionLabel;
    std::unique_ptr<yup::Label> mimeSectionLabel;
    std::unique_ptr<yup::Label> statusLabel;

    std::unique_ptr<yup::TextEditor> textEditor;
    std::unique_ptr<yup::TextButton> copyTextButton;
    std::unique_ptr<yup::TextButton> pasteTextButton;
    std::unique_ptr<yup::TextButton> hasTextButton;

    std::unique_ptr<yup::TextButton> copyImageButton;
    std::unique_ptr<yup::TextButton> pasteImageButton;

    std::unique_ptr<yup::TextButton> refreshMimeButton;
    std::unique_ptr<yup::TextButton> clearClipboardButton;

    // Data
    yup::MemoryBlock rawPngData;
    yup::MemoryBlock localImageCopy;
    yup::Image displayedImage;
    yup::StringArray mimeTypes;

    float widgetAreaHeight = 0.0f;
};
