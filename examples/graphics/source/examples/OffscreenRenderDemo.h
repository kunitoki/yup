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

//==============================================================================

/**
    Demonstrates offscreen GPU render-to-texture via Graphics(context, image).

    The demo renders shapes to a 256x256 offscreen texture outside the main
    frame cycle, then composites the result onto the screen via drawImage().
    A button allows saving the raw pixel data to disk.
*/
class OffscreenRenderDemo : public yup::Component
{
public:
    OffscreenRenderDemo()
        : yup::Component ("OffscreenRenderDemo")
    {
        renderButton = std::make_unique<yup::TextButton> ("Re-render offscreen");
        renderButton->onClick = [this]
        {
            // Button callbacks run on the message thread, which is outside the render cycle.
            renderOffscreen();
        };
        addAndMakeVisible (renderButton.get());

        saveButton = std::make_unique<yup::TextButton> ("Save pixels to file");
        saveButton->onClick = [this]
        {
            savePixelsToFile();
        };
        saveButton->setEnabled (false);
        addAndMakeVisible (saveButton.get());

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        statusLabel->setText ("Not yet rendered.", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());
    }

    void paint (yup::Graphics& g) override
    {
        // Fill background
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        // Capture context pointer for use outside paint() (safe: context lives with the window).
        // Schedule the first offscreen render asynchronously so it runs OUTSIDE the begin/end cycle.
        if (capturedContext == nullptr)
        {
            capturedContext = &g.getGraphicsContext();
            yup::MessageManager::callAsync ([this]
            {
                renderOffscreen();
            });
        }

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromTop (75.0f); // leave room for buttons and status label

        if (offscreenImage.isValid() && offscreenImage.getTexture() != nullptr)
        {
            // GPU-direct path: the image holds a GPU texture; drawImage samples it without CPU upload.
            const auto imgW = static_cast<float> (offscreenImage.getWidth());
            const auto imgH = static_cast<float> (offscreenImage.getHeight());
            const auto scale = std::min (bounds.getWidth() / imgW, bounds.getHeight() / imgH) * 0.8f;

            yup::Rectangle<float> destRect (
                bounds.getCenterX() - imgW * scale * 0.5f,
                bounds.getCenterY() - imgH * scale * 0.5f,
                imgW * scale,
                imgH * scale);

            g.drawImage (offscreenImage, destRect);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        auto buttonRow = bounds.removeFromTop (35.0f);
        renderButton->setBounds (buttonRow.removeFromLeft (200.0f).reduced (0.0f, 2.0f));
        buttonRow.removeFromLeft (10.0f);
        saveButton->setBounds (buttonRow.removeFromLeft (200.0f).reduced (0.0f, 2.0f));

        statusLabel->setBounds (bounds.removeFromTop (30.0f));
    }

private:
    void renderOffscreen()
    {
        if (capturedContext == nullptr)
            return;

        constexpr int offscreenSize = 256;

        offscreenImage = yup::Image (offscreenSize, offscreenSize);

        // Render to offscreen image — this happens OUTSIDE the main begin()/end() cycle.
        {
            yup::Graphics g (*capturedContext, offscreenImage, 0xff1a1a2e);

            if (! g.isOffscreen())
            {
                statusLabel->setText ("Offscreen not supported on this platform.", yup::dontSendNotification);
                return;
            }

            // Background gradient circle
            yup::ColorGradient radial {
                yup::Color (0xff16213e), 128.0f, 128.0f, yup::Color (0xff0f3460), 256.0f, 128.0f, yup::ColorGradient::Radial
            };
            g.setFillColorGradient (radial);
            g.fillRect (0.0f, 0.0f, 256.0f, 256.0f);

            // Outer ring
            g.setStrokeColor (yup::Color (0xffe94560));
            g.setStrokeWidth (4.0f);
            g.strokeEllipse (16.0f, 16.0f, 224.0f, 224.0f);

            // Concentric circles
            g.setStrokeColor (yup::Color (0x8053d8c5));
            g.setStrokeWidth (2.0f);
            for (float r = 50.0f; r < 120.0f; r += 25.0f)
            {
                g.strokeEllipse (128.0f - r, 128.0f - r, r * 2.0f, r * 2.0f);
            }

            // Filled corner decorations
            g.setFillColor (yup::Color (0xffe94560));
            g.fillRoundedRect (10.0f, 10.0f, 40.0f, 40.0f, 6.0f);
            g.fillRoundedRect (206.0f, 10.0f, 40.0f, 40.0f, 6.0f);
            g.fillRoundedRect (10.0f, 206.0f, 40.0f, 40.0f, 6.0f);
            g.fillRoundedRect (206.0f, 206.0f, 40.0f, 40.0f, 6.0f);

            // Center diamond path
            yup::Path diamond;
            diamond.moveTo (128.0f, 60.0f);
            diamond.lineTo (196.0f, 128.0f);
            diamond.lineTo (128.0f, 196.0f);
            diamond.lineTo (60.0f, 128.0f);
            diamond.close();

            g.setFillColor (yup::Color (0x4053d8c5));
            g.fillPath (diamond);
            g.setStrokeColor (yup::Color (0xff53d8c5));
            g.setStrokeWidth (3.0f);
            g.strokePath (diamond);

            // Commit: flush GPU commands and set GPU texture on the image.
            // After this, offscreenImage.getTexture() is non-null and ready for drawImage.
            g.commitToImage();

            // Also pull CPU pixels so the save button can write them to disk.
            g.readPixelsToImage();
        }

        saveButton->setEnabled (true);
        statusLabel->setText ("Rendered to 256x256 GPU texture. GPU draw active.", yup::dontSendNotification);

        repaint();
    }

    void savePixelsToFile()
    {
        if (! offscreenImage.isValid())
            return;

        // Choose a save path next to the executable (or home directory on macOS).
        yup::File outputFile = yup::File::getSpecialLocation (yup::File::userHomeDirectory)
                                   .getChildFile ("offscreen_render.raw");

        auto span = offscreenImage.getRawData();
        if (span.empty())
        {
            statusLabel->setText ("No pixel data — readPixelsToImage not called or failed.", yup::dontSendNotification);
            return;
        }

        yup::FileOutputStream stream (outputFile);
        if (! stream.openedOk())
        {
            statusLabel->setText ("Failed to open output file.", yup::dontSendNotification);
            return;
        }

        // Write a tiny PPM header + raw RGB data for quick preview with any image viewer.
        const int w = offscreenImage.getWidth();
        const int h = offscreenImage.getHeight();
        const int numPixels = w * h;

        stream << "P6\n"
               << w << " " << h << "\n255\n";

        const auto* src = span.data();
        for (int i = 0; i < numPixels; ++i)
        {
            stream.writeByte (static_cast<char> (src[0])); // R
            stream.writeByte (static_cast<char> (src[1])); // G
            stream.writeByte (static_cast<char> (src[2])); // B
            src += 4;                                      // skip alpha
        }

        statusLabel->setText ("Saved PPM: " + outputFile.getFullPathName(), yup::dontSendNotification);
    }

    yup::GraphicsContext* capturedContext = nullptr;
    yup::Image offscreenImage;

    std::unique_ptr<yup::TextButton> renderButton;
    std::unique_ptr<yup::TextButton> saveButton;
    std::unique_ptr<yup::Label> statusLabel;
};
