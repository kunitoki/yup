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

namespace yup
{

//==============================================================================
/** Applies the `cursor`, `effect` and `shadow` declarations a StyleContext computed for one component.

    Each remembers what the application had set before the style, and puts it back when no rule sets the
    declaration any more, unless the application changed it in the meantime.

    @see StyleContext
*/
class StyleEffects
{
public:
    /** Reports a problem with a declaration. */
    using Reporter = std::function<void (const Identifier& declaration, const String& message)>;

    /** Evaluates the color expression of a shadow layer, or returns nothing when it fails (and reports why). */
    using ColorEvaluator = std::function<std::optional<Color> (int layer, const String& source)>;

    StyleEffects() = default;

    /** Sets the cursor named by a `MouseCursor::Type` in camelCase, restoring the original for an empty name. */
    void applyCursor (Component& component, const String& name, const Reporter& report);

    /** Sets a ComponentEffect created by the ComponentEffectRegistry, `none` removes it. */
    void applyEffect (Component& component, const String& name, const Reporter& report);

    /** Parses `x y [blur [spread]] color` layers separated by commas, growing the paint margin to hold them.

        @returns The layers, empty for `none`, an empty text or an error.
    */
    std::vector<StyleState::Shadow> applyShadows (Component& component, const String& text, const ColorEvaluator& evaluateColor, const Reporter& report);

private:
    static std::vector<StyleState::Shadow> parseShadows (const String& text, const ColorEvaluator& evaluateColor, const Reporter& report);
    void applyPaintMargin (Component& component, const std::vector<StyleState::Shadow>& shadows);

    String effectName;
    ComponentEffect::Ptr effect;
    ComponentEffect::Ptr savedEffect;

    std::optional<MouseCursor> cursor;
    MouseCursor savedCursor;

    std::optional<EdgeSpacing<float>> paintMargin;
    EdgeSpacing<float> savedPaintMargin;
};

} // namespace yup
