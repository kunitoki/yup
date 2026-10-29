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

#include "SynthPresetPrompt.h"
#include "SynthFieldEditor.h"
#include "SynthTheme.h"

namespace
{
constexpr float tileWidth = 320.0f;
constexpr float buttonWidth = 96.0f;
constexpr float buttonHeight = 26.0f;
constexpr float lineHeight = 22.0f;
} // namespace

//==============================================================================
/** The name field, which turns return and escape into the prompt's answers instead of
    handing the focus on. */
class SynthPresetPrompt::NameField : public SynthFieldEditor
{
public:
    std::function<void()> onReturn;
    std::function<void()> onEscape;

    bool keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override
    {
        if (key.getKey() == yup::KeyPress::enterKey && onReturn != nullptr)
        {
            onReturn();
            return true;
        }

        if (key.getKey() == yup::KeyPress::escapeKey && onEscape != nullptr)
        {
            onEscape();
            return true;
        }

        return SynthFieldEditor::keyDown (key, position);
    }
};

//==============================================================================
void SynthPresetPrompt::askForName (yup::Component& host, const yup::String& title, const yup::String& initialName, std::function<yup::String (const yup::String&)> onConfirm)
{
    auto* prompt = new SynthPresetPrompt (title, {}, "OK", true);
    prompt->onConfirmName = std::move (onConfirm);
    prompt->show (host, initialName);
}

void SynthPresetPrompt::askToConfirm (yup::Component& host, const yup::String& title, const yup::String& message, const yup::String& confirmLabel, std::function<void()> onConfirm)
{
    auto* prompt = new SynthPresetPrompt (title, message, confirmLabel, false);
    prompt->onConfirmAction = std::move (onConfirm);
    prompt->show (host, {});
}

SynthPresetPrompt::SynthPresetPrompt (const yup::String& titleToShow, const yup::String& messageToShow, const yup::String& confirmLabel, bool asksForName)
    : title (titleToShow)
    , message (messageToShow)
{
    captionFont = SynthTheme::captionFont (13.5f);
    valueFont = SynthTheme::valueFont (13.0f);

    setOpaque (false);
    setWantsKeyboardFocus (true);

    if (asksForName)
    {
        field = std::make_unique<NameField>();
        field->setMultiLine (false);
        field->setFont (SynthTheme::valueFont (13.5f));
        field->onReturn = [this] { confirm(); };
        field->onEscape = [this] { close(); };
        addAndMakeVisible (*field);
    }

    cancelButton.setButtonText ("Cancel");
    cancelButton.onClick = [this] { close(); };
    confirmButton.setButtonText (confirmLabel);
    confirmButton.onClick = [this] { confirm(); };

    for (auto* button : { &cancelButton, &confirmButton })
    {
        SynthTheme::styleRaisedButton (*button);
        addAndMakeVisible (*button);
    }
}

SynthPresetPrompt::~SynthPresetPrompt()
{
    if (auto* covered = host.get())
        covered->removeComponentListener (this);
}

void SynthPresetPrompt::show (yup::Component& hostToCover, const yup::String& initialName)
{
    host = &hostToCover;
    hostToCover.addComponentListener (this);
    hostToCover.addAndMakeVisible (*this);
    setBounds (hostToCover.getLocalBounds());

    if (field == nullptr)
    {
        takeKeyboardFocus();
        return;
    }

    field->setText (initialName, yup::dontSendNotification);
    field->setWantsKeyboardFocus (true);
    field->takeKeyboardFocus();
    field->selectAll();
}

void SynthPresetPrompt::confirm()
{
    if (field == nullptr)
    {
        close();

        if (onConfirmAction != nullptr)
            onConfirmAction();

        return;
    }

    error = onConfirmName != nullptr ? onConfirmName (field->getText().trim()) : yup::String();

    if (error.isEmpty())
        close();
    else
        repaint();
}

/** Hands the focus back to the host and deletes the prompt once the click or key that
    closed it is done with it. It keeps listening to the host, so it is deleted with the
    host if that goes first. */
void SynthPresetPrompt::close()
{
    auto* covered = host.get();

    if (covered == nullptr || getParentComponent() != covered)
        return;

    setVisible (false);
    covered->removeChildComponent (this);
    covered->takeKeyboardFocus();

    yup::MessageManager::callAsync ([prompt = yup::WeakReference<yup::Component> (this)]
    {
        delete prompt.get();
    });
}

void SynthPresetPrompt::componentResized (yup::Component& component)
{
    if (getParentComponent() == &component)
        setBounds (component.getLocalBounds());
}

void SynthPresetPrompt::componentBeingDeleted (yup::Component&)
{
    delete this;
}

void SynthPresetPrompt::resized()
{
    const auto bounds = getLocalBounds();
    const auto padding = SynthTheme::panelPadding * 2.0f;
    const auto height = padding * 2.0f + lineHeight * 3.0f + buttonHeight + SynthTheme::controlGap * 3.0f;

    const auto center = bounds.getCenter();
    tile = yup::Rectangle<float> (0.0f, 0.0f, tileWidth, height).withCenter (center.getX(), center.getY());

    auto content = tile.reduced (padding);
    content.removeFromTop (lineHeight + SynthTheme::controlGap);
    fieldArea = content.removeFromTop (lineHeight);

    if (field != nullptr)
        field->setBounds (fieldArea.reduced (4.0f, 1.0f));

    auto buttons = content.removeFromBottom (buttonHeight);
    confirmButton.setBounds (buttons.removeFromRight (buttonWidth));
    buttons.removeFromRight (SynthTheme::controlGap);
    cancelButton.setBounds (buttons.removeFromRight (buttonWidth));
}

void SynthPresetPrompt::paint (yup::Graphics& g)
{
    g.setFillColor (yup::Colors::black.withAlpha (0.45f));
    g.fillRect (getLocalBounds());

    SynthTheme::paintTile (g, *this, tile);

    auto content = tile.reduced (SynthTheme::panelPadding * 2.0f);
    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillFittedText (title, captionFont, content.removeFromTop (lineHeight), yup::Justification::centerLeft);

    if (field != nullptr)
    {
        SynthTheme::paintWell (g, *this, fieldArea, SynthTheme::displayCorner);
        SynthTheme::paintDisplayBorder (g, *this, fieldArea);
    }
    else
    {
        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (message, valueFont, fieldArea, yup::Justification::centerLeft);
    }

    const auto errorArea = fieldArea.withY (fieldArea.getBottom() + SynthTheme::controlGap);
    g.setFillColor (SynthTheme::accent (*this));
    g.fillFittedText (error, valueFont.withHeight (12.0f), errorArea, yup::Justification::centerLeft);
}

bool SynthPresetPrompt::keyDown (const yup::KeyPress& key, const yup::Point<float>&)
{
    if (key.getKey() == yup::KeyPress::escapeKey)
        close();
    else if (key.getKey() == yup::KeyPress::enterKey)
        confirm();

    return true;
}
