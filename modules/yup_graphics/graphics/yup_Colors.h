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

inline constexpr Color black = Color (0xff000000);
inline constexpr Color white = Color (0xffffffff);
inline constexpr Color transparentBlack = Color (0x00000000);
inline constexpr Color transparentWhite = Color (0x00ffffff);

//==============================================================================

inline constexpr Color aliceblue = Color (0xfff0f8ff);
inline constexpr Color antiquewhite = Color (0xfffaebd7);
inline constexpr Color aqua = Color (0xff00ffff);
inline constexpr Color aquamarine = Color (0xff7fffd4);
inline constexpr Color azure = Color (0xfff0ffff);
inline constexpr Color beige = Color (0xfff5f5dc);
inline constexpr Color bisque = Color (0xffffe4c4);
inline constexpr Color blanchedalmond = Color (0xffffebcd);
inline constexpr Color blue = Color (0xff0000ff);
inline constexpr Color blueviolet = Color (0xff8a2be2);
inline constexpr Color brown = Color (0xffa52a2a);
inline constexpr Color burlywood = Color (0xffdeb887);
inline constexpr Color cadetblue = Color (0xff5f9ea0);
inline constexpr Color chartreuse = Color (0xff7fff00);
inline constexpr Color chocolate = Color (0xffd2691e);
inline constexpr Color coral = Color (0xffff7f50);
inline constexpr Color cornflowerblue = Color (0xff6495ed);
inline constexpr Color cornsilk = Color (0xfffff8dc);
inline constexpr Color crimson = Color (0xffdc143c);
inline constexpr Color cyan = Color (0xff00ffff);
inline constexpr Color darkblue = Color (0xff00008b);
inline constexpr Color darkcyan = Color (0xff008b8b);
inline constexpr Color darkgoldenrod = Color (0xffb8860b);
inline constexpr Color darkgray = Color (0xffa9a9a9);
inline constexpr Color darkgreen = Color (0xff006400);
inline constexpr Color darkkhaki = Color (0xffbdb76b);
inline constexpr Color darkmagenta = Color (0xff8b008b);
inline constexpr Color darkolivegreen = Color (0xff556b2f);
inline constexpr Color darkorange = Color (0xffff8c00);
inline constexpr Color darkorchid = Color (0xff9932cc);
inline constexpr Color darkred = Color (0xff8b0000);
inline constexpr Color darksalmon = Color (0xffe9967a);
inline constexpr Color darkseagreen = Color (0xff8fbc8f);
inline constexpr Color darkslateblue = Color (0xff483d8b);
inline constexpr Color darkslategray = Color (0xff2f4f4f);
inline constexpr Color darkturquoise = Color (0xff00ced1);
inline constexpr Color darkviolet = Color (0xff9400d3);
inline constexpr Color deeppink = Color (0xffff1493);
inline constexpr Color deepskyblue = Color (0xff00bfff);
inline constexpr Color dimgray = Color (0xff696969);
inline constexpr Color dodgerblue = Color (0xff1e90ff);
inline constexpr Color firebrick = Color (0xffb22222);
inline constexpr Color floralwhite = Color (0xfffffaf0);
inline constexpr Color forestgreen = Color (0xff228b22);
inline constexpr Color fuchsia = Color (0xffff00ff);
inline constexpr Color gainsboro = Color (0xffdcdcdc);
inline constexpr Color ghostwhite = Color (0xfff8f8ff);
inline constexpr Color gold = Color (0xffffd700);
inline constexpr Color goldenrod = Color (0xffdaa520);
inline constexpr Color gray = Color (0xff808080);
inline constexpr Color green = Color (0xff008000);
inline constexpr Color greenyellow = Color (0xffadff2f);
inline constexpr Color honeydew = Color (0xfff0fff0);
inline constexpr Color hotpink = Color (0xffff69b4);
inline constexpr Color indianred = Color (0xffcd5c5c);
inline constexpr Color indigo = Color (0xff4b0082);
inline constexpr Color ivory = Color (0xffffff40);
inline constexpr Color khaki = Color (0xfff0e68c);
inline constexpr Color lavender = Color (0xffe6e6fa);
inline constexpr Color lavenderblush = Color (0xfffff0f5);
inline constexpr Color lawngreen = Color (0xff7cfc00);
inline constexpr Color lemonchiffon = Color (0xfffffacd);
inline constexpr Color lightblue = Color (0xffadd8e6);
inline constexpr Color lightcoral = Color (0xfff08080);
inline constexpr Color lightcyan = Color (0xffe0ffff);
inline constexpr Color lightgoldenrodyellow = Color (0xfffafad2);
inline constexpr Color lightgray = Color (0xffd3d3d3);
inline constexpr Color lightgreen = Color (0xff90ee90);
inline constexpr Color lightpink = Color (0xffffb6c1);
inline constexpr Color lightsalmon = Color (0xffffa07a);
inline constexpr Color lightseagreen = Color (0xff20b2aa);
inline constexpr Color lightskyblue = Color (0xff87cefa);
inline constexpr Color lightslategray = Color (0xff778899);
inline constexpr Color lightsteelblue = Color (0xffb0c4de);
inline constexpr Color lightyellow = Color (0xffffff80);
inline constexpr Color lime = Color (0xff00ff00);
inline constexpr Color limegreen = Color (0xff32cd32);
inline constexpr Color linen = Color (0xfffaf0e6);
inline constexpr Color magenta = Color (0xffff00ff);
inline constexpr Color maroon = Color (0xff800000);
inline constexpr Color mediumaquamarine = Color (0xff66cdaa);
inline constexpr Color mediumblue = Color (0xff0000cd);
inline constexpr Color mediumorchid = Color (0xffba55d3);
inline constexpr Color mediumpurple = Color (0xff9370db);
inline constexpr Color mediumseagreen = Color (0xff3cb371);
inline constexpr Color mediumslateblue = Color (0xff7b68ee);
inline constexpr Color mediumspringgreen = Color (0xff00fa9a);
inline constexpr Color mediumturquoise = Color (0xff48d1cc);
inline constexpr Color mediumvioletred = Color (0xffc71585);
inline constexpr Color midnightblue = Color (0xff191970);
inline constexpr Color mintcream = Color (0xfff5fffa);
inline constexpr Color mistyrose = Color (0xffffe4e1);
inline constexpr Color moccasin = Color (0xffffe4b5);
inline constexpr Color navajowhite = Color (0xffffdead);
inline constexpr Color navy = Color (0xff000080);
inline constexpr Color oldlace = Color (0xfffdf5e6);
inline constexpr Color olive = Color (0xff808000);
inline constexpr Color olivedrab = Color (0xff6b8e23);
inline constexpr Color orange = Color (0xffffa500);
inline constexpr Color orangered = Color (0xffff4500);
inline constexpr Color orchid = Color (0xffda70d6);
inline constexpr Color palegoldenrod = Color (0xffeee8aa);
inline constexpr Color palegreen = Color (0xff98fb98);
inline constexpr Color paleturquoise = Color (0xffafeeee);
inline constexpr Color palevioletred = Color (0xffdb7093);
inline constexpr Color papayawhip = Color (0xffffefd5);
inline constexpr Color peachpuff = Color (0xffffdab9);
inline constexpr Color peru = Color (0xffcd853f);
inline constexpr Color pink = Color (0xffffc0cb);
inline constexpr Color plum = Color (0xffdda0dd);
inline constexpr Color powderblue = Color (0xffb0e0e6);
inline constexpr Color purple = Color (0xff800080);
inline constexpr Color red = Color (0xffff0000);
inline constexpr Color rosybrown = Color (0xffbc8f8f);
inline constexpr Color royalblue = Color (0xff4169e1);
inline constexpr Color saddlebrown = Color (0xff8b4513);
inline constexpr Color salmon = Color (0xfffa8072);
inline constexpr Color sandybrown = Color (0xfff4a460);
inline constexpr Color seagreen = Color (0xff2e8b57);
inline constexpr Color seashell = Color (0xfffff5ee);
inline constexpr Color sienna = Color (0xffa0522d);
inline constexpr Color silver = Color (0xffc0c0c0);
inline constexpr Color skyblue = Color (0xff87ceeb);
inline constexpr Color slateblue = Color (0xff6a5acd);
inline constexpr Color slategray = Color (0xff708090);
inline constexpr Color snow = Color (0xfffffafa);
inline constexpr Color springgreen = Color (0xff00ff7f);
inline constexpr Color steelblue = Color (0xff4682b4);
inline constexpr Color tan = Color (0xffd2b48c);
inline constexpr Color teal = Color (0xff008080);
inline constexpr Color thistle = Color (0xffd8bfd8);
inline constexpr Color tomato = Color (0xffff6347);
inline constexpr Color turquoise = Color (0xff40e0d0);
inline constexpr Color violet = Color (0xffee82ee);
inline constexpr Color wheat = Color (0xfff5deb3);
inline constexpr Color whitesmoke = Color (0xfff5f5f5);
inline constexpr Color yellow = Color (0xffffff00);
inline constexpr Color yellowgreen = Color (0xff9acd32);

//==============================================================================

/** Return one of the named colors. */
std::optional<Color> getNamedColor (StringRef colorName);

} // namespace Colors

} // namespace yup
