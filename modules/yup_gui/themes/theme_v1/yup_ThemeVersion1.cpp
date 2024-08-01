/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================

extern const uint8_t RobotoRegularFont_data[];
extern const std::size_t RobotoRegularFont_size;

//==============================================================================

ApplicationTheme::Ptr createThemeVersion1()
{
    auto theme = new ApplicationTheme;

    Font font;
    if (auto result = font.loadFromData (MemoryBlock (&RobotoRegularFont_data[0], RobotoRegularFont_size)); result.failed())
        yup::Logger::outputDebugString (result.getErrorMessage());

    theme->setDefaultFont (std::move (font));

    theme->setComponentTheme<Slider::Theme> ({ [theme] (Graphics& g, const Slider& s)
                                               {
                                                   auto bounds = s.getLocalBounds().reduced (s.proportionOfWidth (0.1f));
                                                   const auto center = bounds.getCenter();

                                                   constexpr auto fromRadians = degreesToRadians (135.0f);
                                                   constexpr auto toRadians = fromRadians + degreesToRadians (270.0f);

                                                   Path backgroundPath;
                                                   backgroundPath.addEllipse (bounds.reduced (s.proportionOfWidth (0.105f)));

                                                   g.setFillColor (Color (0xff3d3d3d));
                                                   g.fillPath (backgroundPath);

                                                   g.setStrokeColor (Color (0xff2b2b2b));
                                                   g.setStrokeWidth (s.proportionOfWidth (0.0175f));
                                                   g.strokePath (backgroundPath);

                                                   Path backgroundArc;
                                                   backgroundArc.addCenteredArc (center,
                                                                                 bounds.getWidth() / 2.0f,
                                                                                 bounds.getHeight() / 2.0f,
                                                                                 0.0f,
                                                                                 fromRadians,
                                                                                 toRadians,
                                                                                 true);

                                                   g.setStrokeCap (StrokeCap::Round);
                                                   g.setStrokeColor (Color (0xff636363));
                                                   g.setStrokeWidth (s.proportionOfWidth (0.075f));
                                                   g.strokePath (backgroundArc);

                                                   const auto toCurrentRadians = fromRadians + degreesToRadians (270.0f) * s.getValue();

                                                   Path foregroundArc;
                                                   foregroundArc.addCenteredArc (center,
                                                                                 bounds.getWidth() / 2.0f,
                                                                                 bounds.getHeight() / 2.0f,
                                                                                 0.0f,
                                                                                 fromRadians,
                                                                                 toCurrentRadians,
                                                                                 true);

                                                   g.setStrokeCap (StrokeCap::Round);
                                                   g.setStrokeColor (s.isMouseOver() ? Color (0xff4ebfff).brighter (0.3f) : Color (0xff4ebfff));
                                                   g.setStrokeWidth (s.proportionOfWidth (0.075f));
                                                   g.strokePath (foregroundArc);

                                                   const auto reducedBounds = bounds.reduced (s.proportionOfWidth (0.175f));
                                                   const auto pos = center.getPointOnCircumference (
                                                       reducedBounds.getWidth() / 2.0f,
                                                       reducedBounds.getHeight() / 2.0f,
                                                       toCurrentRadians);

                                                   Path foregroundLine;
                                                   foregroundLine.addLine (Line<float> (pos, center).keepOnlyStart (0.25f));

                                                   g.setStrokeCap (StrokeCap::Round);
                                                   g.setStrokeColor (Color (0xffffffff));
                                                   g.setStrokeWidth (s.proportionOfWidth (0.03f));
                                                   g.strokePath (foregroundLine);

                                                   StyledText text;
                                                   text.appendText (theme->getDefaultFont(), s.proportionOfHeight (0.1f), s.proportionOfHeight (0.1f), String (s.getValue(), 3).toRawUTF8());
                                                   text.layout (s.getLocalBounds().reduced (5).removeFromBottom (s.proportionOfWidth (0.1f)), StyledText::center);

                                                   g.setStrokeColor (Color (0xffffffff));
                                                   g.strokeFittedText (text, s.getLocalBounds().reduced (5).removeFromBottom (s.proportionOfWidth (0.1f)));

                                                   if (s.hasFocus())
                                                   {
                                                       g.setStrokeColor (Color (0xffff5f2b));
                                                       g.setStrokeWidth (s.proportionOfWidth (0.0175f));
                                                       g.strokeRect (s.getLocalBounds());
                                                   }
                                               } });

    return theme;
}

} // namespace yup
