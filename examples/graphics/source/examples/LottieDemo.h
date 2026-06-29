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
    Demonstrates loading and interactive playback of Lottie animations.

    Loads `.json` or `.lottie` (ZIP) files via FileChooser and plays them back
    through AnimationPlayer. Controls include play/pause, stop, loop toggle,
    direction (forward / reverse / ping-pong), speed, and a scrub slider.
*/
class LottieDemo : public yup::Component
{
public:
    LottieDemo()
        : yup::Component ("LottieDemo")
    {
        loadButton = std::make_unique<yup::TextButton> ("Load...");
        loadButton->onClick = [this]
        {
            browseForFile();
        };
        addAndMakeVisible (loadButton.get());

        playPauseButton = std::make_unique<yup::TextButton> ("Play");
        playPauseButton->onClick = [this]
        {
            togglePlayPause();
        };
        playPauseButton->setEnabled (false);
        addAndMakeVisible (playPauseButton.get());

        stopButton = std::make_unique<yup::TextButton> ("Stop");
        stopButton->onClick = [this]
        {
            player.stop();
            playPauseButton->setButtonText ("Play");
            updateScrubber();
            repaint();
        };
        stopButton->setEnabled (false);
        addAndMakeVisible (stopButton.get());

        loopButton = std::make_unique<yup::TextButton> ("Loop: ON");
        loopButton->onClick = [this]
        {
            looping = ! looping;
            player.setLooping (looping);
            loopButton->setButtonText (looping ? "Loop: ON" : "Loop: OFF");
        };
        loopButton->setEnabled (false);
        addAndMakeVisible (loopButton.get());

        fwdButton = std::make_unique<yup::TextButton> (">");
        fwdButton->onClick = [this]
        {
            direction = yup::AnimationPlayer::Direction::Forward;
            player.setDirection (direction);
            updateDirectionButtons();
        };
        fwdButton->setEnabled (false);
        addAndMakeVisible (fwdButton.get());

        revButton = std::make_unique<yup::TextButton> ("<");
        revButton->onClick = [this]
        {
            direction = yup::AnimationPlayer::Direction::Reverse;
            player.setDirection (direction);
            updateDirectionButtons();
        };
        revButton->setEnabled (false);
        addAndMakeVisible (revButton.get());

        pingPongButton = std::make_unique<yup::TextButton> ("<>");
        pingPongButton->onClick = [this]
        {
            direction = yup::AnimationPlayer::Direction::PingPong;
            player.setDirection (direction);
            updateDirectionButtons();
        };
        pingPongButton->setEnabled (false);
        addAndMakeVisible (pingPongButton.get());

        speedSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        speedSlider->setRange (0.1, 4.0);
        speedSlider->setValue (1.0);
        speedSlider->onValueChanged = [this] (double value)
        {
            player.setSpeed (static_cast<float> (value));
            speedLabel->setText ("Speed: " + yup::String (value, 2) + "x", yup::dontSendNotification);
        };
        speedSlider->setEnabled (false);
        addAndMakeVisible (speedSlider.get());

        speedLabel = std::make_unique<yup::Label> ("speedLabel");
        speedLabel->setText ("Speed: 1.00x", yup::dontSendNotification);
        addAndMakeVisible (speedLabel.get());

        scrubSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        scrubSlider->setRange (0.0, 1.0);
        scrubSlider->setValue (0.0);
        scrubSlider->onValueChanged = [this] (double value)
        {
            if (! scrubberDriven)
                return;
            player.seekToProgress (static_cast<float> (value));
            repaint();
        };
        scrubSlider->onDragStart = [this] (const yup::MouseEvent&)
        {
            scrubberDriven = true;
        };
        scrubSlider->onDragEnd = [this] (const yup::MouseEvent&)
        {
            scrubberDriven = false;
        };
        scrubSlider->setEnabled (false);
        addAndMakeVisible (scrubSlider.get());

        scrubLabel = std::make_unique<yup::Label> ("scrubLabel");
        scrubLabel->setText ("Frame: --/--", yup::dontSendNotification);
        addAndMakeVisible (scrubLabel.get());

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        statusLabel->setText ("Load a .json or .lottie file to begin.", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());

        player.setLooping (looping);
        player.setDirection (direction);

        player.onFrameChanged = [this] (float /*frameNo*/)
        {
            updateScrubber();
        };

        player.onLoopCompleted = [this]
        {
            // nothing extra needed — loop keeps playing
        };

        player.onPlaybackEnded = [this]
        {
            playPauseButton->setButtonText ("Play");
        };
    }

    ~LottieDemo() override
    {
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        if (! player.isPlaying())
            return;

        if (player.advanceTime (lastFrameTimeSeconds))
            repaint();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromTop (controlAreaHeight);

        if (player.getAnimation().isValid())
        {
            g.setFillColor (yup::Colors::white);
            g.fillRoundedRect (bounds, 6.0f);

            player.render (g, bounds.reduced (4.0f), true);
        }
        else
        {
            g.setFillColor (yup::Colors::black.withAlpha (0.15f));
            g.fillRoundedRect (bounds, 6.0f);

            g.setFillColor (yup::Colors::white.withAlpha (0.3f));
            g.fillFittedText ("No animation loaded",
                              yup::Font().withHeight (18.0f),
                              bounds,
                              yup::Justification::center);
        }
    }

    void resized() override
    {
        constexpr float margin = 8.0f;
        constexpr float buttonH = 30.0f;
        constexpr float rowGap = 6.0f;

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        // Row 1: Load / Play / Stop / Loop
        auto row1 = bounds.removeFromTop (buttonH);
        loadButton->setBounds (row1.removeFromLeft (90.0f).reduced (0.0f, 2.0f));
        row1.removeFromLeft (margin);
        playPauseButton->setBounds (row1.removeFromLeft (70.0f).reduced (0.0f, 2.0f));
        row1.removeFromLeft (margin);
        stopButton->setBounds (row1.removeFromLeft (60.0f).reduced (0.0f, 2.0f));
        row1.removeFromLeft (margin);
        loopButton->setBounds (row1.removeFromLeft (90.0f).reduced (0.0f, 2.0f));

        bounds.removeFromTop (rowGap);

        // Row 2: Direction buttons
        auto row2 = bounds.removeFromTop (buttonH);
        fwdButton->setBounds (row2.removeFromLeft (50.0f).reduced (0.0f, 2.0f));
        row2.removeFromLeft (margin);
        revButton->setBounds (row2.removeFromLeft (50.0f).reduced (0.0f, 2.0f));
        row2.removeFromLeft (margin);
        pingPongButton->setBounds (row2.removeFromLeft (50.0f).reduced (0.0f, 2.0f));

        bounds.removeFromTop (rowGap);

        // Row 3: Speed slider + label
        auto row3 = bounds.removeFromTop (buttonH);
        speedLabel->setBounds (row3.removeFromLeft (120.0f));
        row3.removeFromLeft (margin);
        speedSlider->setBounds (row3);

        bounds.removeFromTop (rowGap);

        // Row 4: Scrub slider + label
        auto row4 = bounds.removeFromTop (buttonH);
        scrubLabel->setBounds (row4.removeFromLeft (120.0f));
        row4.removeFromLeft (margin);
        scrubSlider->setBounds (row4);

        bounds.removeFromTop (rowGap);

        // Row 5: Status
        statusLabel->setBounds (bounds.removeFromTop (20.0f));

        bounds.removeFromTop (rowGap);
    }

private:
    void browseForFile()
    {
        auto chooser = yup::FileChooser::create ("Open Lottie Animation",
                                                 yup::File::getCurrentWorkingDirectory(),
                                                 "*.json;*.lottie");
        chooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (success && ! results.isEmpty())
                loadFile (results[0]);
        });
    }

    void loadFile (const yup::File& file)
    {
        auto anim = yup::Animation::loadFromFile (file);
        if (! anim.isValid())
        {
            statusLabel->setText ("Failed to load: " + file.getFileName(), yup::dontSendNotification);
            return;
        }

        player.stop();
        player.setAnimation (std::move (anim));
        player.setLooping (looping);
        player.setDirection (direction);
        player.play();

        const auto& loaded = player.getAnimation();
        statusLabel->setText (
            file.getFileName()
                + "  |  " + yup::String (loaded.totalFrames(), 0) + " frames"
                + "  @  " + yup::String (loaded.frameRate(), 0) + " fps"
                + "  (" + yup::String (loaded.duration(), 2) + "s)"
                + "  " + yup::String ((int) loaded.size().getWidth()) + "x" + yup::String ((int) loaded.size().getHeight()),
            yup::dontSendNotification);

        playPauseButton->setButtonText ("Pause");
        setControlsEnabled (true);
        updateScrubber();
        repaint();
    }

    void togglePlayPause()
    {
        if (player.isPlaying())
        {
            player.pause();
            playPauseButton->setButtonText ("Play");
        }
        else
        {
            player.play();
            playPauseButton->setButtonText ("Pause");
        }
    }

    void setControlsEnabled (bool enabled)
    {
        playPauseButton->setEnabled (enabled);
        stopButton->setEnabled (enabled);
        loopButton->setEnabled (enabled);
        fwdButton->setEnabled (enabled);
        revButton->setEnabled (enabled);
        pingPongButton->setEnabled (enabled);
        speedSlider->setEnabled (enabled);
        scrubSlider->setEnabled (enabled);
    }

    void updateDirectionButtons()
    {
        // No visual selection state on TextButton, so we just update button text to hint active choice.
        fwdButton->setButtonText (direction == yup::AnimationPlayer::Direction::Forward ? "[>]" : ">");
        revButton->setButtonText (direction == yup::AnimationPlayer::Direction::Reverse ? "[<]" : "<");
        pingPongButton->setButtonText (direction == yup::AnimationPlayer::Direction::PingPong ? "[<>]" : "<>");
    }

    void updateScrubber()
    {
        if (scrubberDriven)
            return;

        const float progress = player.currentProgress();
        const float frame = player.currentFrame();
        const float total = player.getAnimation().isValid() ? player.getAnimation().totalFrames() : 0.0f;

        scrubSlider->setValue (static_cast<double> (progress), yup::dontSendNotification);
        scrubLabel->setText ("Frame: " + yup::String ((int) frame) + "/" + yup::String ((int) total),
                             yup::dontSendNotification);
    }

    //==============================================================================
    static constexpr float controlAreaHeight = 5 * 36.0f + 20.0f + 12.0f;

    yup::AnimationPlayer player;

    bool looping = true;
    bool scrubberDriven = false;
    yup::AnimationPlayer::Direction direction = yup::AnimationPlayer::Direction::Forward;

    std::unique_ptr<yup::TextButton> loadButton;
    std::unique_ptr<yup::TextButton> playPauseButton;
    std::unique_ptr<yup::TextButton> stopButton;
    std::unique_ptr<yup::TextButton> loopButton;
    std::unique_ptr<yup::TextButton> fwdButton;
    std::unique_ptr<yup::TextButton> revButton;
    std::unique_ptr<yup::TextButton> pingPongButton;
    std::unique_ptr<yup::Slider> speedSlider;
    std::unique_ptr<yup::Label> speedLabel;
    std::unique_ptr<yup::Slider> scrubSlider;
    std::unique_ptr<yup::Label> scrubLabel;
    std::unique_ptr<yup::Label> statusLabel;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LottieDemo)
};
