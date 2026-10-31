/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of this software for any purpose is hereby granted, provided that the
   above copyright notice and this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  ==============================================================================
*/

#pragma once

#include "../dsp/SynthParameters.h"
#include "../dsp/SynthRoutes.h"

#include <yup_gui/yup_gui.h>

#include <array>
#include <memory>

//==============================================================================
/** The modulation routes as a scrollable editable list.

    Shows every used route and one empty row to add the next. When they do not all fit,
    the viewport scrolls by whole rows and adding a route brings the new empty row into view.
    A reset button, placed by the owner, clears every route as one undo step.

    @see SynthRoutes, SynthModulationRow
*/
class SynthModulationPage : public yup::Component
{
public:
    SynthModulationPage (const SynthParameters& parametersToEdit,
                         SynthRoutes& routesToEdit,
                         yup::Span<const yup::AudioParameter::Ptr> processorParameters);
    ~SynthModulationPage() override;

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    /** Shows the macros under the names the patch gives them, in every row. */
    void setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames);

    /** Returns the button that clears every route and depth, for the owner to place. */
    yup::Component& getResetButton() noexcept { return resetButton; }

    void resized() override;

private:
    class Content;

    void resetRoutes();

    std::unique_ptr<Content> content;
    yup::Viewport viewport;
    yup::TextButton resetButton { "Reset" };
};
