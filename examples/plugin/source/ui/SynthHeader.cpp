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
#include "SynthHeader.h"
#include "SynthTheme.h"

namespace
{
/** The RETRACE logo, with the viewBox cropped to the drawing: a signal entering a CRT that
    scans it in five lines. Its #FFFFFF strokes take the palette's text color and the
    scanlines the colors named #SCAN1 to #SCAN5. */
constexpr const char* synthLogoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" width="136" height="100" viewBox="1 0 136 100" fill="none">
  <g stroke="#FFFFFF" stroke-linejoin="round" stroke-linecap="round">
    <path d="M4 61 q3 -7 6 0 t6 0 t6 0 t6 0 t6 0 L40 61" stroke-width="3.2"/>
    <path d="M82 21 L66 5 M82 21 L102 3" stroke-width="2.2"/>
    <path d="M73 26 Q82 15 91 26" stroke-width="3"/>
    <path d="M46 26 L116 26 Q122 26 122 32 L122 86 Q122 92 116 92 L46 92 Q40 92 40 86 L40 32 Q40 26 46 26 Z" stroke-width="4"/>
    <path d="M122 31 L134 37 L134 83 L122 89" stroke-width="2.5"/>
    <path d="M53 33 Q75 30.5 97 33 Q103 33.5 103.5 39.5 Q106 59 103.5 78.5 Q103 84.5 97 85 Q75 87.5 53 85 Q47 84.5 46.5 78.5 Q44 59 46.5 39.5 Q47 33.5 53 33 Z" stroke-width="3"/>
    <path d="M108 40 L117 40 M108 45 L117 45 M108 50 L117 50" stroke-width="1.8"/>
    <circle cx="112.5" cy="67" r="4" stroke-width="2.2"/>
    <circle cx="112.5" cy="80" r="4" stroke-width="2.2"/>
    <path d="M48 92 L51 97 L111 97 L114 92" stroke-width="2.5"/>
    <path d="M96 43 L54 51 M96 51 L54 59 M96 59 L54 67 M96 67 L54 75" stroke-width="2" stroke-opacity="0.5"/>
  </g>
  <circle cx="66" cy="5" r="2.6" fill="#FFFFFF"/>
  <circle cx="102" cy="3" r="2.6" fill="#FFFFFF"/>
  <g stroke-width="3" stroke-linecap="round">
    <path d="M54 43 L96 43" stroke="#SCAN1"/>
    <path d="M54 51 L96 51" stroke="#SCAN2"/>
    <path d="M54 59 L96 59" stroke="#SCAN3"/>
    <path d="M54 67 L96 67" stroke="#SCAN4"/>
    <path d="M54 75 L96 75" stroke="#SCAN5"/>
  </g>
</svg>
)svg";

constexpr float logoAspect = 136.0f / 100.0f;
} // namespace

SynthHeader::SynthHeader (RetraceProcessor& processorToControl)
    : processor (processorToControl)
    , presetBar (processorToControl)
    , volumeKnob ("VOLUME", processorToControl.getSynthParameters().volume, SynthTheme::captionFont (11.0f))
{
    captionFont = SynthTheme::captionFont (11.5f);
    valueFont = SynthTheme::valueFont (13.5f);

    setCachedToTexture (true);

    setOpaque (false);
    SynthTheme::applySection (*this, SynthTheme::Section::header);

    addAndMakeVisible (presetBar);

    presetBar.onBrowserToggled = [this]
    {
        if (onBrowserToggled != nullptr)
            onBrowserToggled();
    };

    saveButton.onClick = [this] { presetBar.savePreset(); };
    loadButton.onClick = [this] { presetBar.loadPreset(); };
    randomButton.onClick = [this]
    {
        if (randomButton.wasRightClicked())
            processor.randomizeSlightly();
        else
            processor.randomize();

        presetBar.syncFromProcessor();
    };
    panicButton.onClick = [this]
    {
        processor.getKeyboardState().allNotesOff (0);
        processor.requestAllNotesOff();
    };

    for (auto* button : { &saveButton, &loadButton, &randomButton, &panicButton })
        addAndMakeVisible (*button);

    addAndMakeVisible (volumeKnob);
    addAndMakeVisible (meter);
}

void SynthHeader::syncFromProcessor()
{
    presetBar.syncFromProcessor();
    volumeKnob.syncFromParameter();
    meter.setLevels (processor.takeOutputPeak (0), processor.takeOutputPeak (1));

    // The readouts change on almost every call, and each change repaints the interface,
    // so they follow at a reading pace.
    if (++readoutCalls < readoutInterval)
        return;

    readoutCalls = 0;

    const auto newVoices = yup::String (processor.getNumActiveVoices()) + " / " + yup::String (SynthExample::voiceCount);
    const auto newLoad = yup::String (processor.getLoadPercentage(), 1) + "%";
    const auto newXruns = yup::String (processor.getXRunCount());

    if (newVoices == voices && newLoad == load && newXruns == xruns)
        return;

    voices = newVoices;
    load = newLoad;
    xruns = newXruns;
    repaint (statsTile);
}

void SynthHeader::setRightGroupWidth (float width)
{
    rightGroupWidth = width;
    resized();
}

void SynthHeader::resized()
{
    const auto gap = SynthTheme::frameGap;
    auto bounds = getLocalBounds();

    logoTile = bounds.removeFromLeft (logoWidth);
    bounds.removeFromLeft (gap);

    auto right = bounds.removeFromRight (rightGroupWidth);
    bounds.removeFromRight (gap);
    presetBar.setBounds (bounds);

    auto icons = right.removeFromLeft (iconTileSize * 2.0f + gap);
    right.removeFromLeft (gap);
    const auto row = (icons.getHeight() - gap) * 0.5f;
    auto top = icons.removeFromTop (row);
    icons.removeFromTop (gap);

    saveButton.setBounds (top.removeFromLeft (iconTileSize));
    loadButton.setBounds (top.removeFromRight (iconTileSize));
    randomButton.setBounds (icons.removeFromLeft (iconTileSize));
    panicButton.setBounds (icons.removeFromRight (iconTileSize));

    volumeTile = right.removeFromRight (volumeWidth);
    right.removeFromRight (gap);
    statsTile = right;

    auto volume = volumeTile.reduced (8.0f, 4.0f);
    meter.setBounds (volume.removeFromRight (14.0f).reduced (0.0f, 6.0f));
    volume.removeFromRight (8.0f);
    volumeKnob.setBounds (volume);

    const auto logoHeight = logoTile.getHeight() - 22.0f;
    logoArea = logoTile.withTrimmedLeft (14.0f).withWidth (logoHeight * logoAspect).withSizeKeepingCenter (logoHeight * logoAspect, logoHeight);
}

void SynthHeader::mouseDown (const yup::MouseEvent& event)
{
    if (event.isRightButtonDown() && logoTile.contains (event.getPosition()) && onSettingsRequested != nullptr)
        onSettingsRequested (event.getPosition());
}

void SynthHeader::styleChanged()
{
    logoNeedsColors = true;
    repaint();
}

/** The scanlines take the colors of the sections they light. */
void SynthHeader::colorLogo()
{
    const auto scan = [this] (SynthTheme::Section section) { return SynthTheme::sectionAccent (*this, section).toString(); };

    logo.parseSVG (yup::String (synthLogoSvg)
                       .replace ("#FFFFFF", SynthTheme::textPrimary (*this).toString())
                       .replace ("#SCAN1", scan (SynthTheme::Section::oscillators))
                       .replace ("#SCAN2", scan (SynthTheme::Section::matrix))
                       .replace ("#SCAN3", scan (SynthTheme::Section::filters))
                       .replace ("#SCAN4", scan (SynthTheme::Section::envelopes))
                       .replace ("#SCAN5", scan (SynthTheme::Section::lfos)));
}

void SynthHeader::paint (yup::Graphics& g)
{
    if (std::exchange (logoNeedsColors, false))
        colorLogo();

    for (const auto& tile : { logoTile, statsTile, volumeTile })
        SynthTheme::paintTile (g, *this, tile);

    logo.paint (g, logoArea);

    auto words = logoTile.withTrimmedLeft (logoArea.getRight() - logoTile.getX() + 10.0f).reduced (0.0f, 14.0f);
    words.removeFromTop (10.0f);
    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillFittedText ("RETRACE", valueFont.withHeight (24.0f), words.removeFromTop (words.getHeight() * 0.62f), yup::Justification::bottomLeft);
    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText ("SPECTRAL FMOD", captionFont.withHeight (13.5f), words, yup::Justification::topLeft);

    auto stats = statsTile.reduced (12.0f, 9.0f);
    const auto rowHeight = stats.getHeight() / 3.0f;
    const std::pair<const char*, yup::String> rows[] = { { "VOICES", voices }, { "CPU", load }, { "XRUNS", xruns } };

    for (const auto& [caption, value] : rows)
    {
        const auto line = stats.removeFromTop (rowHeight);
        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (caption, captionFont, line, yup::Justification::centerLeft);
        g.setFillColor (caption == rows[0].first ? SynthTheme::accent (*this) : SynthTheme::textPrimary (*this));
        g.fillFittedText (value, valueFont, line, yup::Justification::centerRight);
    }
}
