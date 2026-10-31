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

#include "ParameterLed.h"
#include "SynthTheme.h"

#include <yup_gui/yup_gui.h>

#include <memory>
#include <vector>

//==============================================================================
/** A panel of the instrument with a strip of tabs on top, showing one page at a time.

    The selected tab takes the panel's color and an accent line along its top edge, so
    it reads as part of the panel below; the others stay in the strip. A tab with an
    enable parameter carries an LED left of its name, which toggles the module; clicking
    the name selects the tab. While a module is off its tab is muted and its page dims,
    though it stays editable.

    The right end of the strip holds controls: the ones shared by every tab, then the
    ones of the selected tab. An optional trailing control sits right of that tab's
    other controls. Tabs keep their widths whichever tab is selected; when the selected
    tab's controls don't fit beside them, those controls shrink instead.

    @see ParameterLed
*/
class SynthSection : public yup::Component
{
public:
    /** A control placed in the strip, with the width it takes. */
    struct StripControl
    {
        yup::Component* component = nullptr;
        float width = 0.0f;
    };

    /** One tab and the page it shows. */
    struct Tab
    {
        yup::String name;
        yup::Component* page = nullptr;
        yup::AudioParameter::Ptr enable;          /**< Null for a tab without an LED, unless lockedOn. */
        bool lockedOn = false;                    /**< Shows an LED that is always lit, for a module that is always on. */
        std::vector<StripControl> stripControls;  /**< Shown only while this tab is selected. */
        yup::Component* trailingControl = nullptr; /**< Optional control right of stripControls, shown only while this tab is selected. */
    };

    /** Creates the panel, which wears its section's palette for everything inside it. */
    SynthSection (std::vector<Tab> tabsToShow, SynthTheme::Section section, std::vector<StripControl> sharedStripControls = {});

    /** Shows a tab's page. */
    void selectTab (int index);

    /** Shows the modules' enable states. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseMove (const yup::MouseEvent& event) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    static constexpr float tabPadding = 12.0f;
    static constexpr float minimumTabPadding = 4.0f;
    static constexpr float ledSpace = 16.0f;
    static constexpr float trailingControlWidth = 24.0f;

    yup::Rectangle<float> getTabBounds (int index) const;
    bool isTabOn (int index) const;
    void updatePageLook();

    std::vector<Tab> tabs;
    std::vector<std::unique_ptr<ParameterLed>> leds;
    std::vector<StripControl> sharedControls;
    std::vector<float> tabWidths;
    float tabInset = tabPadding;
    std::vector<bool> shownOn;
    int selected = 0;
    yup::Font tabFont;
};
