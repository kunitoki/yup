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
    Demonstrates image loading, saving, rendering, and animated image playback.

    Supports all registered image formats (PNG, JPEG, BMP, WebP, GIF, PPM).
    Uses FileChooser to pick files. Automatically detects animated images (GIF)
    and plays them frame-by-frame with on-demand decoding — only one Image is
    kept in memory at a time. A synthetic animated image is generated as the
    default, also rendered on-demand per frame.
*/
class ImagesDemo : public yup::Component
    , public yup::Timer
{
public:
    ImagesDemo()
        : yup::Component ("ImagesDemo")
    {
        formatManager.registerDefaultFormats();

        loadButton = std::make_unique<yup::TextButton> ("Load Image...");
        loadButton->onClick = [this]
        {
            browseForImageToLoad();
        };
        addAndMakeVisible (loadButton.get());

        saveButton = std::make_unique<yup::TextButton> ("Save As...");
        saveButton->onClick = [this]
        {
            browseForImageToSave();
        };
        addAndMakeVisible (saveButton.get());

        playPauseButton = std::make_unique<yup::TextButton> ("Play");
        playPauseButton->onClick = [this]
        {
            animationPlaying = ! animationPlaying;
            playPauseButton->setButtonText (animationPlaying ? "Pause" : "Play");
        };
        addAndMakeVisible (playPauseButton.get());

        loopButton = std::make_unique<yup::TextButton> ("Loop: ON");
        loopButton->onClick = [this]
        {
            animationLooping = ! animationLooping;
            loopButton->setButtonText (animationLooping ? "Loop: ON" : "Loop: OFF");
        };
        addAndMakeVisible (loopButton.get());

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        addAndMakeVisible (statusLabel.get());

        infoLabel = std::make_unique<yup::Label> ("infoLabel");
        addAndMakeVisible (infoLabel.get());

        // Set up a single reusable image buffer for the default animation.
        // All frames are rendered into this same buffer on each timer tick.
        currentImage = yup::Image (defaultAnimSize, defaultAnimSize, yup::PixelFormat::RGBA);
        currentImageLabel = "Default animation";

        animationIsAnimated = true;
        animationFrameCount = defaultAnimFrames;
        animationPlaying = true;
        animationLooping = true;
        animationCurrentFrame = 0;
        animationElapsedMs = 0;

        for (int i = 0; i < defaultAnimFrames; ++i)
            animationFrameDelays.push_back (defaultAnimDelayMs);

        renderCurrentFrame();
        updateAnimationControls();
        updateStatus ("Ready. Click 'Load Image...' or watch the default animation.");

        startTimerHz (30);
    }

    ~ImagesDemo() override
    {
        stopTimer();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromTop (75.0f);

        {
            g.setFillColor (yup::Colors::black.withAlpha (0.3f));
            g.fillRoundedRect (bounds, 8.0f);

            yup::String label = currentImage.isValid() ? currentImageLabel : yup::String ("No image");
            if (animationIsAnimated && animationFrameCount > 0)
                label << "  (frame " << (animationCurrentFrame + 1) << "/" << animationFrameCount << ")";

            g.fillFittedText (label, yup::Font().withHeight (14.0f), bounds.removeFromTop (24.0f).translated (0.0f, 8.0f), yup::Justification::center);

            auto imageArea = bounds.reduced (8.0f);
            imageArea.removeFromTop (8.0f);

            if (currentImage.isValid())
                drawImageFitted (g, currentImage, imageArea);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        auto buttonRow = bounds.removeFromTop (35.0f);
        loadButton->setBounds (buttonRow.removeFromLeft (130.0f).reduced (0.0f, 2.0f));
        buttonRow.removeFromLeft (6.0f);
        saveButton->setBounds (buttonRow.removeFromLeft (130.0f).reduced (0.0f, 2.0f));
        buttonRow.removeFromLeft (6.0f);
        playPauseButton->setBounds (buttonRow.removeFromLeft (80.0f).reduced (0.0f, 2.0f));
        buttonRow.removeFromLeft (6.0f);
        loopButton->setBounds (buttonRow.removeFromLeft (90.0f).reduced (0.0f, 2.0f));

        statusLabel->setBounds (bounds.removeFromTop (20.0f).toNearestInt());
        infoLabel->setBounds (bounds.removeFromTop (18.0f).toNearestInt());
    }

    void timerCallback() override
    {
        if (! animationPlaying || ! animationIsAnimated || animationFrameCount <= 1)
            return;

        animationElapsedMs += 33;

        auto delay = animationFrameDelays[static_cast<size_t> (animationCurrentFrame)];

        if (animationElapsedMs >= delay)
        {
            animationElapsedMs = 0;
            ++animationCurrentFrame;

            if (animationCurrentFrame >= animationFrameCount)
            {
                if (animationLooping)
                    animationCurrentFrame = 0;
                else
                {
                    animationCurrentFrame = animationFrameCount - 1;
                    animationPlaying = false;
                    playPauseButton->setButtonText ("Play");
                }
            }

            renderCurrentFrame();
            repaint();
        }
    }

private:
    //==============================================================================
    static yup::String getImageFileFilter()
    {
        return "*.png;*.jpg;*.jpeg;*.jpe;*.bmp;*.gif;*.webp;*.ppm;*.pgm;*.pbm";
    }

    void browseForImageToLoad()
    {
        auto chooser = yup::FileChooser::create ("Open Image File",
                                                 yup::File::getCurrentWorkingDirectory(),
                                                 getImageFileFilter());
        chooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (success && ! results.isEmpty())
                loadImageFromFile (results[0]);
        });
    }

    void browseForImageToSave()
    {
        if (! currentImage.isValid())
        {
            updateStatus ("Nothing to save.");
            return;
        }

        auto chooser = yup::FileChooser::create ("Save Image As",
                                                 yup::File::getCurrentWorkingDirectory(),
                                                 getImageFileFilter());
        chooser->browseForFileToSave ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (success && ! results.isEmpty())
                saveImageToFile (results[0]);
        },
                                      true);
    }

    //==============================================================================
    void loadImageFromFile (const yup::File& file)
    {
        // Release GPU texture before destroying the current image.
        if (currentImage.isValid())
            currentImage.invalidateTexture();

        imageFormatReader.reset();
        currentImage = {};
        currentImageLabel = {};
        animationFrameDelays.clear();
        animationFrameCount = 0;
        animationCurrentFrame = 0;
        animationElapsedMs = 0;
        isDefaultAnimation = false;

        auto reader = formatManager.createReaderFor (file);
        if (reader == nullptr)
        {
            updateStatus ("Unrecognised format: " + file.getFileName());
            updateAnimationControls();
            repaint();
            return;
        }

        yup::String format = reader->getFormatName();
        int w = reader->width;
        int h = reader->height;

        animationIsAnimated = reader->isAnimated();
        animationFrameCount = reader->getFrameCount();
        bool infinite = reader->getLoopCount() == 0;
        bool hasAnimation = animationIsAnimated && animationFrameCount > 1;

        if (hasAnimation)
        {
            // Animated: keep the reader, decode frames on-demand into a reusable buffer.
            currentImage = yup::Image (w, h, reader->pixelFormat);

            animationFrameDelays.clear();
            for (int fi = 0; fi < animationFrameCount; ++fi)
                animationFrameDelays.push_back (reader->getFrameDelayMs (fi));

            animationCurrentFrame = 0;
            animationElapsedMs = 0;
            animationPlaying = true;
            animationLooping = infinite;

            imageFormatReader = std::move (reader);

            // Decode frame 0 directly into currentImage's buffer.
            bool ok = imageFormatReader->readFrame (0, currentImage);
            if (ok)
                currentImage.invalidateTexture();

            updateStatus (yup::String ("Loaded ") + format + ": "
                          + yup::String (animationFrameCount) + " frames, "
                          + yup::String (w) + "x" + yup::String (h)
                          + (infinite ? ", loops infinitely" : ", plays once"));
        }
        else
        {
            // Still image: use readImage() directly — no reader kept.
            currentImage = reader->readImage();

            animationIsAnimated = false;
            animationFrameCount = 1;
            animationPlaying = false;
            animationLooping = false;

            updateStatus (yup::String ("Loaded ") + format + ": "
                          + yup::String (w) + "x" + yup::String (h));
        }

        currentImageLabel = file.getFileName() + " (" + format + ")";

        updateAnimationControls();
        updateInfo (file.getFullPathName());
        repaint();
    }

    void saveImageToFile (const yup::File& file)
    {
        auto writer = formatManager.createWriterFor (file, yup::PixelFormat::RGBA);
        if (writer == nullptr)
        {
            updateStatus ("No writer available for: " + file.getFileName());
            return;
        }

        yup::String format = writer->getFormatName();

        if (animationIsAnimated && animationFrameCount > 1 && writer->supportsAnimation())
        {
            if (! writer->beginAnimation (animationLooping ? 0 : 1))
            {
                updateStatus ("Failed to begin animation for " + format);
                return;
            }

            yup::Image frame (currentImage.getWidth(), currentImage.getHeight(), currentImage.getPixelFormat());

            for (int fi = 0; fi < animationFrameCount; ++fi)
            {
                if (! renderFrameInto (fi, frame))
                {
                    updateStatus ("Failed to render frame " + yup::String (fi));
                    return;
                }

                if (! writer->writeFrame (frame, animationFrameDelays[static_cast<size_t> (fi)]))
                {
                    updateStatus ("Failed to write frame " + yup::String (fi) + " for " + format);
                    return;
                }
            }

            if (! writer->endAnimation())
            {
                updateStatus ("Failed to finalise animation for " + format);
                return;
            }

            updateStatus (yup::String ("Saved animated ") + format + ": "
                          + yup::String (animationFrameCount) + " frames -> "
                          + file.getFileName());
        }
        else
        {
            if (! writer->writeImage (currentImage))
            {
                updateStatus ("Failed to write " + format);
                return;
            }

            updateStatus ("Saved " + format + ": " + file.getFileName());
        }

        updateInfo (file.getFullPathName());
    }

    //==============================================================================
    /** Decodes or renders the current frame into currentImage and invalidates the GPU texture. */
    void renderCurrentFrame()
    {
        if (! currentImage.isValid())
            return;

        bool ok = renderFrameInto (animationCurrentFrame, currentImage);

        if (ok)
            currentImage.invalidateTexture();
    }

    /** Renders frame index into dest. Returns true on success. */
    bool renderFrameInto (int frameIndex, yup::Image& dest)
    {
        if (imageFormatReader != nullptr)
            return imageFormatReader->readFrame (frameIndex, dest);

        if (isDefaultAnimation)
            return renderDefaultFrame (frameIndex, dest);

        return false;
    }

    /** Procedurally generates a default animation frame directly into dest. */
    bool renderDefaultFrame (int frameIndex, yup::Image& dest)
    {
        if (dest.getWidth() < defaultAnimSize || dest.getHeight() < defaultAnimSize)
            return false;

        int w = dest.getWidth();
        int h = dest.getHeight();
        float t = static_cast<float> (frameIndex) / static_cast<float> (defaultAnimFrames);

        dest.fill (0xff1a1a2eu);

        float cx = defaultAnimSize * 0.3f + std::cos (t * yup::MathConstants<float>::twoPi) * defaultAnimSize * 0.25f;
        float cy = defaultAnimSize * 0.5f + std::sin (t * yup::MathConstants<float>::twoPi) * defaultAnimSize * 0.25f;
        float radius = defaultAnimSize * 0.15f + std::sin (t * yup::MathConstants<float>::twoPi * 2.0f) * defaultAnimSize * 0.05f;

        auto color = yup::Color::fromHSV (t, 0.8f, 1.0f, 1.0f);
        auto circleColor = color.getARGB();

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                float dx = static_cast<float> (x) - cx;
                float dy = static_cast<float> (y) - cy;
                if (dx * dx + dy * dy <= radius * radius)
                    dest.setPixel (x, y, circleColor);
            }
        }

        return true;
    }

    //==============================================================================
    void updateAnimationControls()
    {
        bool hasAnimation = animationIsAnimated && animationFrameCount > 1;
        playPauseButton->setEnabled (hasAnimation);
        loopButton->setEnabled (hasAnimation);

        if (hasAnimation)
        {
            playPauseButton->setButtonText (animationPlaying ? "Pause" : "Play");
            loopButton->setButtonText (animationLooping ? "Loop: ON" : "Loop: OFF");
        }
        else
        {
            playPauseButton->setButtonText ("Play");
            loopButton->setButtonText ("Loop: OFF");
        }
    }

    static void drawImageFitted (yup::Graphics& g, const yup::Image& img, yup::Rectangle<float> area)
    {
        auto imgW = static_cast<float> (img.getWidth());
        auto imgH = static_cast<float> (img.getHeight());
        auto scale = std::min (area.getWidth() / imgW, area.getHeight() / imgH);

        yup::Rectangle<float> dest (
            area.getCenterX() - imgW * scale * 0.5f,
            area.getCenterY() - imgH * scale * 0.5f,
            imgW * scale,
            imgH * scale);

        g.drawImage (img, dest);
    }

    void updateStatus (const yup::String& text)
    {
        statusText = text;
        statusLabel->setText (statusText, yup::dontSendNotification);
    }

    void updateInfo (const yup::String& text)
    {
        infoLabel->setText (text, yup::dontSendNotification);
    }

    //==============================================================================
    static constexpr int defaultAnimSize = 128;
    static constexpr int defaultAnimFrames = 30;
    static constexpr int defaultAnimDelayMs = 50;

    yup::ImageFormatManager formatManager;

    // For file-loaded images: reader kept alive for on-demand frame decoding.
    std::unique_ptr<yup::ImageFormatReader> imageFormatReader;

    // For the default animation: no pre-rendered frames stored, each frame generated on-demand.
    bool isDefaultAnimation = true;

    // The single image buffer that always holds the currently displayed frame.
    yup::Image currentImage;
    yup::String currentImageLabel;

    std::vector<int> animationFrameDelays;
    int animationFrameCount = 0;
    int animationCurrentFrame = 0;
    int animationElapsedMs = 0;
    bool animationIsAnimated = false;
    bool animationPlaying = false;
    bool animationLooping = false;

    std::unique_ptr<yup::TextButton> loadButton;
    std::unique_ptr<yup::TextButton> saveButton;
    std::unique_ptr<yup::TextButton> playPauseButton;
    std::unique_ptr<yup::TextButton> loopButton;
    std::unique_ptr<yup::Label> statusLabel;
    std::unique_ptr<yup::Label> infoLabel;
    yup::String statusText;
};
