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
/** The PRISM logo, with the viewBox cropped to the drawing. Its #FFFFFF strokes take the
    palette's text color and the five rays the colors named #RAY1 to #RAY5. */
constexpr const char* synthLogoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" width="236" height="122" viewBox="10 66 236 122" fill="none">
  <g stroke="#FFFFFF" stroke-linejoin="miter" stroke-linecap="square" transform="translate(0 37)">
    <path d="M14 100 q4 -7 8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 L91 100" stroke-width="3.2" stroke-linecap="round"/>
    <path d="M128 36 L191.5 146 L64.5 146 Z" stroke-width="4.5"/>
    <path d="M91 100 L160.3 92" stroke-width="2.5" stroke-opacity="0.5"/>
    <g stroke-linecap="butt">
      <path d="M160.3 92 L242 66"  stroke="#RAY1" stroke-width="3.5"/>
      <path d="M160.3 92 L242 80"  stroke="#RAY2" stroke-width="3.2"/>
      <path d="M160.3 92 L242 94"  stroke="#RAY3" stroke-width="3"/>
      <path d="M160.3 92 L242 108" stroke="#RAY4" stroke-width="2.7"/>
      <path d="M160.3 92 L242 122" stroke="#RAY5" stroke-width="2.5"/>
    </g>
  </g>
</svg>
)svg";

constexpr float logoAspect = 236.0f / 122.0f;
} // namespace

SynthHeader::SynthHeader (PrismProcessor& processorToControl)
    : processor (processorToControl)
    , presetBar (processorToControl)
    , volumeKnob ("VOLUME", processorToControl.getSynthParameters().volume, SynthTheme::captionFont (11.0f))
{
    setOpaque (false);
    SynthTheme::applySection (*this, SynthTheme::Section::header);

    // The rays leave the prism in the colors of the sections they light.
    const auto ray = [] (SynthTheme::Section section) { return SynthTheme::sectionRole (section, yup::ThemePalette::Role::accent).toString(); };

    logo.parseSVG (yup::String (synthLogoSvg)
                       .replace ("#FFFFFF", SynthTheme::textPrimary (*this).toString())
                       .replace ("#RAY1", ray (SynthTheme::Section::oscillators))
                       .replace ("#RAY2", ray (SynthTheme::Section::matrix))
                       .replace ("#RAY3", ray (SynthTheme::Section::filters))
                       .replace ("#RAY4", ray (SynthTheme::Section::envelopes))
                       .replace ("#RAY5", ray (SynthTheme::Section::lfos)));

    presetBar.onEditingFinished = [this]
    {
        if (onEditingFinished != nullptr)
            onEditingFinished();
    };
    addAndMakeVisible (presetBar);

    saveButton.onClick = [this] { presetBar.savePreset(); };
    loadButton.onClick = [this] { presetBar.loadPreset(); };
    randomButton.onClick = [this]
    {
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
    meter.setLevels (processor.getOutputPeak (0), processor.getOutputPeak (1));

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
        onSettingsRequested();
}

void SynthHeader::paint (yup::Graphics& g)
{
    for (const auto& tile : { logoTile, statsTile, volumeTile })
        SynthTheme::paintTile (g, *this, tile);

    logo.paint (g, logoArea);

    auto words = logoTile.withTrimmedLeft (logoArea.getRight() - logoTile.getX() + 10.0f).reduced (0.0f, 14.0f);
    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillFittedText ("PRISM", SynthTheme::valueFont (24.0f).withAxisValue ("wdth", 100.0f), words.removeFromTop (words.getHeight() * 0.62f), yup::Justification::bottomLeft);
    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText ("SPECTRAL FMOD", SynthTheme::captionFont (9.5f), words, yup::Justification::topLeft);

    auto stats = statsTile.reduced (12.0f, 9.0f);
    const auto rowHeight = stats.getHeight() / 3.0f;
    const auto captionFont = SynthTheme::captionFont (11.5f);
    const auto valueFont = SynthTheme::valueFont (13.5f);
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
