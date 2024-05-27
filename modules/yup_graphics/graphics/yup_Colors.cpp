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

namespace Colors
{

//==============================================================================

std::optional<Color> getNamedColor (StringRef colorName)
{
    static const std::unordered_map<String, Color> namedColors =
    {
        { "aliceblue", aliceblue },
        { "antiquewhite", antiquewhite },
        { "aqua", aqua },
        { "aquamarine", aquamarine },
        { "azure", azure },
        { "beige", beige },
        { "bisque", bisque },
        { "black", black },
        { "blanchedalmond", blanchedalmond },
        { "blue", blue },
        { "blueviolet", blueviolet },
        { "brown", brown },
        { "burlywood", burlywood },
        { "cadetblue", cadetblue },
        { "chartreuse", chartreuse },
        { "chocolate", chocolate },
        { "coral", coral },
        { "cornflowerblue", cornflowerblue },
        { "cornsilk", cornsilk },
        { "crimson", crimson },
        { "cyan", cyan },
        { "darkblue", darkblue },
        { "darkcyan", darkcyan },
        { "darkgoldenrod", darkgoldenrod },
        { "darkgray", darkgray },
        { "darkgrey", darkgray },
        { "darkgreen", darkgreen },
        { "darkkhaki", darkkhaki },
        { "darkmagenta", darkmagenta },
        { "darkolivegreen", darkolivegreen },
        { "darkorange", darkorange },
        { "darkorchid", darkorchid },
        { "darkred", darkred },
        { "darksalmon", darksalmon },
        { "darkseagreen", darkseagreen },
        { "darkslateblue", darkslateblue },
        { "darkslategray", darkslategray },
        { "darkslategrey", darkslategray },
        { "darkturquoise", darkturquoise },
        { "darkviolet", darkviolet },
        { "deeppink", deeppink },
        { "deepskyblue", deepskyblue },
        { "dimgray", dimgray },
        { "dimgrey", dimgray },
        { "dodgerblue", dodgerblue },
        { "firebrick", firebrick },
        { "floralwhite", floralwhite },
        { "forestgreen", forestgreen },
        { "fuchsia", fuchsia },
        { "gainsboro", gainsboro },
        { "ghostwhite", ghostwhite },
        { "gold", gold },
        { "goldenrod", goldenrod },
        { "gray", gray },
        { "grey", gray },
        { "green", green },
        { "greenyellow", greenyellow },
        { "honeydew", honeydew },
        { "hotpink", hotpink },
        { "indianred", indianred },
        { "indigo", indigo },
        { "ivory", ivory },
        { "khaki", khaki },
        { "lavender", lavender },
        { "lavenderblush", lavenderblush },
        { "lawngreen", lawngreen },
        { "lemonchiffon", lemonchiffon },
        { "lightblue", lightblue },
        { "lightcoral", lightcoral },
        { "lightcyan", lightcyan },
        { "lightgoldenrodyellow", lightgoldenrodyellow },
        { "lightgray", lightgray },
        { "lightgrey", lightgray },
        { "lightgreen", lightgreen },
        { "lightpink", lightpink },
        { "lightsalmon", lightsalmon },
        { "lightseagreen", lightseagreen },
        { "lightskyblue", lightskyblue },
        { "lightslategray", lightslategray },
        { "lightslategrey", lightslategray },
        { "lightsteelblue", lightsteelblue },
        { "lightyellow", lightyellow },
        { "lime", lime },
        { "limegreen", limegreen },
        { "linen", linen },
        { "magenta", magenta },
        { "maroon", maroon },
        { "mediumaquamarine", mediumaquamarine },
        { "mediumblue", mediumblue },
        { "mediumorchid", mediumorchid },
        { "mediumpurple", mediumpurple },
        { "mediumseagreen", mediumseagreen },
        { "mediumslateblue", mediumslateblue },
        { "mediumspringgreen", mediumspringgreen },
        { "mediumturquoise", mediumturquoise },
        { "mediumvioletred", mediumvioletred },
        { "midnightblue", midnightblue },
        { "mintcream", mintcream },
        { "mistyrose", mistyrose },
        { "moccasin", moccasin },
        { "navajowhite", navajowhite },
        { "navy", navy },
        { "oldlace", oldlace },
        { "olive", olive },
        { "olivedrab", olivedrab },
        { "orange", orange },
        { "orangered", orangered },
        { "orchid", orchid },
        { "palegoldenrod", palegoldenrod },
        { "palegreen", palegreen },
        { "paleturquoise", paleturquoise },
        { "palevioletred", palevioletred },
        { "papayawhip", papayawhip },
        { "peachpuff", peachpuff },
        { "peru", peru },
        { "pink", pink },
        { "plum", plum },
        { "powderblue", powderblue },
        { "purple", purple },
        { "red", red },
        { "rosybrown", rosybrown },
        { "royalblue", royalblue },
        { "saddlebrown", saddlebrown },
        { "salmon", salmon },
        { "sandybrown", sandybrown },
        { "seagreen", seagreen },
        { "seashell", seashell },
        { "sienna", sienna },
        { "silver", silver },
        { "skyblue", skyblue },
        { "slateblue", slateblue },
        { "slategray", slategray },
        { "slategrey", slategray },
        { "snow", snow },
        { "springgreen", springgreen },
        { "steelblue", steelblue },
        { "tan", tan },
        { "teal", teal },
        { "thistle", thistle },
        { "tomato", tomato },
        { "turquoise", turquoise },
        { "violet", violet },
        { "wheat", wheat },
        { "white", white },
        { "whitesmoke", whitesmoke },
        { "yellow", yellow },
        { "yellowgreen", yellowgreen },
    };

    auto it = namedColors.find (String (colorName).toLowerCase());
    if (it != namedColors.end())
        return it->second;

    return std::nullopt;
}

} // namespace Colors

} // namespace yup
