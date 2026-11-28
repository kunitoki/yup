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

#include "yup_YuiToolIndex.h"

#include <yup_gui_markup/yup_gui_markup.h>

#include <vector>

//==============================================================================
/** Guides drawn over an interface: the bounds of some of its components, the area each of them
    paints when it has a paint margin, dotted, and its distances to the bounds of what holds it.

    The component is placed over what shows the interface. While it is visible it takes the
    pointer from the interface: a click picks the component under it, and with the shift key
    adds it to those picked, or takes it away. Until one is picked, the component shown is the
    one the caret of an editor is in.

    The guides follow the components they show as these move or change size.
*/
class YuiToolGuides final : public yup::Component
{
public:
    YuiToolGuides();

    /** Sets the document whose interface the guides are over, or nullptr for none.

        It is called again once the document was loaded again, as its components may be others,
        and with nullptr before the document goes.

        @param newDocument The document, loaded.
        @param newInput    The file it was loaded from, which the caret is a place of.
    */
    void setDocument (yup::YuiDocument* newDocument, const yup::File& newInput = {});

    /** Sets what of the interface the guides are over, and how large it is seen.

        @param newOrigin The place of the interface the top left of the guides is at.
        @param zoom      How many times larger than its size the interface is seen: the lines and
                         the texts are drawn that much smaller, so they keep their size on screen.
    */
    void setView (yup::Point<float> newOrigin, float zoom);

    /** Sets the place of the file the caret of an editor is at, whose component is shown until
        one is picked. A caret that moves takes over from what was picked.

        @param newCaret The place, or one of line 0 for no caret.
    */
    void setCaret (YuiToolPlace newCaret);

    /** Sets the components shown, as a click on them does. */
    void setPicked (const std::vector<yup::Component*>& components);

    //==============================================================================
    /** @internal */
    void paint (yup::Graphics& g) override;

    /** @internal */
    void mouseDown (const yup::MouseEvent& event) override;

private:
    using Components = std::vector<yup::WeakReference<yup::Component>>;

    void findAtCaret();
    float getLabelWidth (const yup::String& text) const;
    void paintDistance (yup::Graphics& g, const yup::Font& font, yup::Point<float> from, float length, bool isAcross);
    void paintLabel (yup::Graphics& g, const yup::Font& font, const yup::String& text, yup::Point<float> topLeft);

    yup::YuiDocument* document = nullptr;
    yup::File input;
    yup::Point<float> origin;
    float pixel = 1.0f;
    YuiToolPlace caret;
    Components atCaret;
    Components picked;
    bool hasPicked = false;
    std::vector<yup::Rectangle<float>> painted;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiToolGuides)
};
