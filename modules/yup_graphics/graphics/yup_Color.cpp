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

namespace
{

//==============================================================================

int hexCharToInt (yup_wchar c) noexcept
{
    return CharacterFunctions::getHexDigitValue (c);
}

int parseNextInt (String::CharPointerType& data)
{
    int result = 0;
    bool isNegative = false;

    while (*data != '\0' && (*data == ' ' || *data == ','))
        ++data;

    if (*data == '-')
    {
        isNegative = true;
        ++data;
    }

    while (*data >= '0' && *data <= '9')
    {
        result = result * 10 + (*data - '0');
        ++data;
    }

    while (*data != '\0' && (*data == ' ' || *data == ',' || *data == ')'))
        ++data;

    return isNegative ? -result : result;
}

float parseNextFloat (String::CharPointerType& data)
{
    float result = 0;
    bool isNegative = false;

    while (*data != '\0' && (*data == ' ' || *data == ','))
        ++data;

    if (*data == '-')
    {
        isNegative = true;
        ++data;
    }

    while (*data >= '0' && *data <= '9')
    {
        result = result * 10 + (*data - '0');
        ++data;
    }

    if (*data == '.')
    {
        ++data;
        float decimalFactor = 10.0f;

        while (*data >= '0' && *data <= '9')
        {
            result += (*data - '0') / decimalFactor;
            decimalFactor *= 10.0f;
            ++data;
        }
    }

    if (*data == '%')
    {
        result /= 100.0f;
        ++data;
    }

    return isNegative ? -result : result;
}

Color parseHexColor (const String& hexString)
{
    const int length = hexString.length();
    auto data = hexString.getCharPointer();

    if (length == 4) // #RGB
    {
        uint8 red = static_cast<uint8> (hexCharToInt (data[1]) * 16 + hexCharToInt (data[1]));
        uint8 green = static_cast<uint8> (hexCharToInt (data[2]) * 16 + hexCharToInt (data[2]));
        uint8 blue = static_cast<uint8> (hexCharToInt (data[3]) * 16 + hexCharToInt (data[3]));

        return { red, green, blue };
    }
    else if (length == 7) // #RRGGBB
    {
        uint8 red = static_cast<uint8> (hexCharToInt (data[1]) * 16 + hexCharToInt (data[2]));
        uint8 green = static_cast<uint8> (hexCharToInt (data[3]) * 16 + hexCharToInt (data[4]));
        uint8 blue = static_cast<uint8> (hexCharToInt (data[5]) * 16 + hexCharToInt (data[6]));

        return { red, green, blue };
    }
    else if (length == 9) // #RRGGBBAA
    {
        uint8 red = static_cast<uint8> (hexCharToInt (data[1]) * 16 + hexCharToInt (data[2]));
        uint8 green = static_cast<uint8> (hexCharToInt (data[3]) * 16 + hexCharToInt (data[4]));
        uint8 blue = static_cast<uint8> (hexCharToInt (data[5]) * 16 + hexCharToInt (data[6]));
        uint8 alpha = static_cast<uint8> (hexCharToInt (data[7]) * 16 + hexCharToInt (data[8]));

        return { alpha, red, green, blue };
    }
    else
    {
        return Colors::transparentBlack;
    }
}

Color parseRGBColor (const String& rgbString)
{
    int r = 0, g = 0, b = 0, a = 255;

    auto data = rgbString.getCharPointer();
    bool isRGBA = rgbString.startsWithIgnoreCase ("rgba(");
    bool isRGB = rgbString.startsWithIgnoreCase ("rgb(");

    if (! isRGBA && ! isRGB)
        return Colors::transparentBlack;

    while (*data != '(' && *data != '\0')
        ++data;

    if (*data == '(')
        ++data;

    r = parseNextInt (data);
    g = parseNextInt (data);
    b = parseNextInt (data);

    if (isRGBA)
        a = parseNextInt (data);

    return Color::fromRGBA (static_cast<uint8> (r), static_cast<uint8> (g), static_cast<uint8> (b), static_cast<uint8> (a));
}

Color parseHSLColor (const String& hslString)
{
    auto data = hslString.getCharPointer();
    bool isHSL = hslString.startsWithIgnoreCase ("hsl(");
    bool isHSLA = hslString.startsWithIgnoreCase ("hsla(");

    if (! isHSL && ! isHSLA)
        return Colors::transparentBlack;

    while (*data != '(' && *data != '\0')
        ++data;

    if (*data == '(')
        ++data;

    float h = 0, s = 0, l = 0, a = 1;
    h = parseNextFloat (data);
    s = parseNextFloat (data);
    l = parseNextFloat (data);

    if (isHSLA)
        a = parseNextFloat (data);

    return Color::fromHSL (h, s, l, a);
}

Color parseNamedColor (const String& name)
{
    if (auto color = Colors::getNamedColor (name))
        return *color;

    return Colors::transparentBlack;
}

//==============================================================================

using DoubleRgb = std::array<double, 3>;
using IntRgb = std::array<int, 3>;

struct HsluvBounds
{
    double a;
    double b;
};

constexpr double hsluvRefU = 0.19783000664283680764;
constexpr double hsluvRefV = 0.46831999493879100370;
constexpr double hsluvKappa = 903.29629629629629629630;
constexpr double hsluvEpsilon = 0.00885645167903563082;
constexpr double hsluvZero = 0.00000001;

const std::array<DoubleRgb, 3> hsluvMatrixRgb = { { { 3.24096994190452134377, -1.53738317757009345794, -0.49861076029300328366 },
                                                    { -0.96924363628087982613, 1.87596750150772066772, 0.04155505740717561247 },
                                                    { 0.05563007969699360846, -0.20397695888897656435, 1.05697151424287856072 } } };

const std::array<DoubleRgb, 3> hsluvMatrixXyz = { { { 0.41239079926595948129, 0.35758433938387796373, 0.18048078840183428751 },
                                                    { 0.21263900587151035754, 0.71516867876775592746, 0.07219231536073371500 },
                                                    { 0.01933081871559185069, 0.11919477979462598791, 0.95053215224966058086 } } };

double hsluvDotProduct (const DoubleRgb& t1, const DoubleRgb& t2) noexcept
{
    return (t1[0] * t2[0] + t1[1] * t2[1] + t1[2] * t2[2]);
}

double hsluvFromLinear (double value) noexcept
{
    if (value <= 0.0031308)
        return 12.92 * value;

    return 1.055 * std::pow (value, 1.0 / 2.4) - 0.055;
}

double hsluvToLinear (double value) noexcept
{
    if (value > 0.04045)
        return std::pow ((value + 0.055) / 1.055, 2.4);

    return value / 12.92;
}

void hsluvXyzToRgb (DoubleRgb& value) noexcept
{
    const double r = hsluvFromLinear (hsluvDotProduct (hsluvMatrixRgb[0], value));
    const double g = hsluvFromLinear (hsluvDotProduct (hsluvMatrixRgb[1], value));
    const double b = hsluvFromLinear (hsluvDotProduct (hsluvMatrixRgb[2], value));

    value[0] = r;
    value[1] = g;
    value[2] = b;
}

void hsluvRgbToXyz (DoubleRgb& value) noexcept
{
    const DoubleRgb linear = { hsluvToLinear (value[0]), hsluvToLinear (value[1]), hsluvToLinear (value[2]) };

    value[0] = hsluvDotProduct (hsluvMatrixXyz[0], linear);
    value[1] = hsluvDotProduct (hsluvMatrixXyz[1], linear);
    value[2] = hsluvDotProduct (hsluvMatrixXyz[2], linear);
}

double hsluvYToL (double value) noexcept
{
    if (value <= hsluvEpsilon)
        return value * hsluvKappa;

    return 116.0 * std::cbrt (value) - 16.0;
}

double hsluvLToY (double value) noexcept
{
    if (value <= 8.0)
        return value / hsluvKappa;

    const double x = (value + 16.0) / 116.0;
    return x * x * x;
}

void hsluvXyzToLuv (DoubleRgb& value) noexcept
{
    const double varU = (4.0 * value[0]) / (value[0] + (15.0 * value[1]) + (3.0 * value[2]));
    const double varV = (9.0 * value[1]) / (value[0] + (15.0 * value[1]) + (3.0 * value[2]));
    const double l = hsluvYToL (value[1]);
    const double u = 13.0 * l * (varU - hsluvRefU);
    const double v = 13.0 * l * (varV - hsluvRefV);

    value[0] = l;
    if (l < hsluvZero)
    {
        value[1] = 0.0;
        value[2] = 0.0;
    }
    else
    {
        value[1] = u;
        value[2] = v;
    }
}

void hsluvLuvToXyz (DoubleRgb& value) noexcept
{
    if (value[0] <= hsluvZero)
    {
        value[0] = 0.0;
        value[1] = 0.0;
        value[2] = 0.0;
        return;
    }

    const double varU = value[1] / (13.0 * value[0]) + hsluvRefU;
    const double varV = value[2] / (13.0 * value[0]) + hsluvRefV;
    const double y = hsluvLToY (value[0]);
    const double x = -(9.0 * y * varU) / ((varU - 4.0) * varV - varU * varV);
    const double z = (9.0 * y - (15.0 * varV * y) - (varV * x)) / (3.0 * varV);

    value[0] = x;
    value[1] = y;
    value[2] = z;
}

void hsluvLuvToLch (DoubleRgb& value) noexcept
{
    const double l = value[0];
    const double u = value[1];
    const double v = value[2];
    const double c = std::sqrt (u * u + v * v);

    double h = 0.0;
    if (c >= hsluvZero)
    {
        h = std::atan2 (v, u) * 57.29577951308232087680;
        if (h < 0.0)
            h += 360.0;
    }

    value[0] = l;
    value[1] = c;
    value[2] = h;
}

void hsluvLchToLuv (DoubleRgb& value) noexcept
{
    const double hrad = value[2] * 0.01745329251994329577;
    const double u = std::cos (hrad) * value[1];
    const double v = std::sin (hrad) * value[1];

    value[1] = u;
    value[2] = v;
}

void hsluvGetBounds (double l, std::array<HsluvBounds, 6>& bounds) noexcept
{
    const double tl = l + 16.0;
    const double sub1 = (tl * tl * tl) / 1560896.0;
    const double sub2 = sub1 > hsluvEpsilon ? sub1 : (l / hsluvKappa);

    for (int channel = 0; channel < 3; ++channel)
    {
        const auto& matrix = hsluvMatrixRgb[channel];

        const double m1 = matrix[0];
        const double m2 = matrix[1];
        const double m3 = matrix[2];

        for (int t = 0; t < 2; ++t)
        {
            const double top1 = (284517.0 * m1 - 94839.0 * m3) * sub2;
            const double top2 = (838422.0 * m3 + 769860.0 * m2 + 731718.0 * m1) * l * sub2 - 769860.0 * t * l;
            const double bottom = (632260.0 * m3 - 126452.0 * m2) * sub2 + 126452.0 * t;

            bounds[channel * 2 + t] = { top1 / bottom, top2 / bottom };
        }
    }
}

double hsluvRayLengthUntilIntersect (double theta, const HsluvBounds& line) noexcept
{
    return line.b / (std::sin (theta) - line.a * std::cos (theta));
}

double hsluvMaxChromaForLH (double l, double h) noexcept
{
    double minLen = std::numeric_limits<double>::max();
    const double hrad = h * 0.01745329251994329577;
    std::array<HsluvBounds, 6> bounds;

    hsluvGetBounds (l, bounds);
    for (const auto& bound : bounds)
    {
        const double len = hsluvRayLengthUntilIntersect (hrad, bound);

        if (len >= 0.0 && len < minLen)
            minLen = len;
    }

    return minLen;
}

void hsluvToLch (DoubleRgb& value) noexcept
{
    double h = value[0];
    const double s = value[1];
    const double l = value[2];
    double c = 0.0;

    if (l > 99.9999999 || l < hsluvZero)
        c = 0.0;
    else
        c = hsluvMaxChromaForLH (l, h) / 100.0 * s;

    if (s < hsluvZero)
        h = 0.0;

    value[0] = l;
    value[1] = c;
    value[2] = h;
}

void hsluvFromLch (DoubleRgb& value) noexcept
{
    const double l = value[0];
    const double c = value[1];
    double h = value[2];
    double s = 0.0;

    if (l > 99.9999999 || l < hsluvZero)
        s = 0.0;
    else
        s = c / hsluvMaxChromaForLH (l, h) * 100.0;

    if (c < hsluvZero)
        h = 0.0;

    value[0] = h;
    value[1] = s;
    value[2] = l;
}

DoubleRgb hsluvToRgb (double h, double s, double l) noexcept
{
    DoubleRgb tmp = { h, s, l };

    hsluvToLch (tmp);
    hsluvLchToLuv (tmp);
    hsluvLuvToXyz (tmp);
    hsluvXyzToRgb (tmp);

    tmp[0] = jlimit (0.0, 1.0, tmp[0]);
    tmp[1] = jlimit (0.0, 1.0, tmp[1]);
    tmp[2] = jlimit (0.0, 1.0, tmp[2]);

    return tmp;
}

DoubleRgb hsluvFromRgb (double r, double g, double b) noexcept
{
    DoubleRgb tmp = { r, g, b };

    hsluvRgbToXyz (tmp);
    hsluvXyzToLuv (tmp);
    hsluvLuvToLch (tmp);
    hsluvFromLch (tmp);

    tmp[0] = jlimit (0.0, 360.0, tmp[0]);
    tmp[1] = jlimit (0.0, 100.0, tmp[1]);
    tmp[2] = jlimit (0.0, 100.0, tmp[2]);

    return tmp;
}

//==============================================================================

constexpr int spectralSampleCount = 38;
constexpr double spectralGamma = 2.4;
constexpr double spectralEpsilon = 0.00000001;

using SpectralSamples = std::array<double, spectralSampleCount>;

constexpr SpectralSamples spdC = { 0.96853629, 0.96855103, 0.96859338, 0.96877345, 0.96942204, 0.97143709, 0.97541862, 0.98074186, 0.98580992, 0.98971194, 0.99238027, 0.99409844, 0.995172, 0.99576545, 0.99593552, 0.99564041, 0.99464769, 0.99229579, 0.98638762, 0.96829712, 0.89228016, 0.53740239, 0.15360445, 0.05705719, 0.03126539, 0.02205445, 0.01802271, 0.0161346, 0.01520947, 0.01475977, 0.01454263, 0.01444459, 0.01439897, 0.0143762, 0.01436343, 0.01435687, 0.0143537, 0.01435408 };
constexpr SpectralSamples spdM = { 0.51567122, 0.5401552, 0.62645502, 0.75595012, 0.92826996, 0.97223624, 0.98616174, 0.98955255, 0.98676237, 0.97312575, 0.91944277, 0.32564851, 0.13820628, 0.05015143, 0.02912336, 0.02421691, 0.02660696, 0.03407586, 0.04835936, 0.0001172, 0.00008554, 0.85267882, 0.93188793, 0.94810268, 0.94200977, 0.91478045, 0.87065445, 0.78827548, 0.65738359, 0.59909403, 0.56817268, 0.54031997, 0.52110241, 0.51041094, 0.50526577, 0.5025508, 0.50126452, 0.50083021 };
constexpr SpectralSamples spdY = { 0.02055257, 0.02059936, 0.02062723, 0.02073387, 0.02114202, 0.02233154, 0.02556857, 0.03330189, 0.05185294, 0.10087639, 0.24000413, 0.53589066, 0.79874659, 0.91186529, 0.95399623, 0.97137099, 0.97939505, 0.98345207, 0.98553736, 0.98648905, 0.98674535, 0.98657555, 0.98611877, 0.98559942, 0.98507063, 0.98460039, 0.98425301, 0.98403909, 0.98388535, 0.98376116, 0.98368246, 0.98365023, 0.98361309, 0.98357259, 0.98353856, 0.98351247, 0.98350101, 0.98350852 };
constexpr SpectralSamples spdR = { 0.03147571, 0.03146636, 0.03140624, 0.03119611, 0.03053888, 0.02856855, 0.02459485, 0.0192952, 0.01423112, 0.01033111, 0.00765876, 0.00593693, 0.00485616, 0.00426186, 0.00409039, 0.00438375, 0.00537525, 0.00772962, 0.0136612, 0.03181352, 0.10791525, 0.46249516, 0.84604333, 0.94275572, 0.96860996, 0.97783966, 0.98187757, 0.98377315, 0.98470202, 0.98515481, 0.98537114, 0.98546685, 0.98550011, 0.98551031, 0.98550741, 0.98551323, 0.98551563, 0.98551547 };
constexpr SpectralSamples spdG = { 0.49108579, 0.46944057, 0.4016578, 0.2449042, 0.0682688, 0.02732883, 0.013606, 0.01000187, 0.01284127, 0.02636635, 0.07058713, 0.70421692, 0.85473994, 0.95081565, 0.9717037, 0.97651888, 0.97429245, 0.97012917, 0.9425863, 0.99989207, 0.99989891, 0.13823139, 0.06968113, 0.05628787, 0.06111561, 0.08987709, 0.13656016, 0.22169624, 0.32176956, 0.36157329, 0.4836192, 0.46488579, 0.47440306, 0.4857699, 0.49267971, 0.49625685, 0.49807754, 0.49889859 };
constexpr SpectralSamples spdB = { 0.97901834, 0.97901649, 0.97901118, 0.97892146, 0.97858555, 0.97743705, 0.97428075, 0.96663223, 0.94822893, 0.89937713, 0.76070164, 0.4642044, 0.20123039, 0.08808402, 0.04592894, 0.02860373, 0.02060067, 0.01656701, 0.01451549, 0.01357964, 0.01331243, 0.01347661, 0.01387181, 0.01435472, 0.01479836, 0.0151525, 0.01540513, 0.01557233, 0.0156571, 0.01571025, 0.01571916, 0.01572133, 0.01572502, 0.01571717, 0.01571905, 0.01571059, 0.01569728, 0.0157002 };
constexpr SpectralSamples cieCmfX = { 0.00006469, 0.00021941, 0.00112057, 0.00376661, 0.01188055, 0.02328644, 0.03455942, 0.03722379, 0.03241838, 0.02123321, 0.01049099, 0.00329584, 0.00050704, 0.00094867, 0.00627372, 0.01686462, 0.02868965, 0.04267481, 0.05625475, 0.0694704, 0.08305315, 0.0861261, 0.09046614, 0.08500387, 0.07090667, 0.05062889, 0.03547396, 0.02146821, 0.01251646, 0.00680458, 0.00346457, 0.00149761, 0.0007697, 0.00040737, 0.00016901, 0.00009522, 0.00004903, 0.00002 };
constexpr SpectralSamples cieCmfY = { 0.00000184, 0.00000621, 0.00003101, 0.00010475, 0.00035364, 0.00095147, 0.00228226, 0.00420733, 0.0066888, 0.0098884, 0.01524945, 0.02141831, 0.03342293, 0.05131001, 0.07040208, 0.08783871, 0.09424905, 0.09795667, 0.09415219, 0.08678102, 0.07885653, 0.0635267, 0.05374142, 0.04264606, 0.03161735, 0.02088521, 0.01386011, 0.00810264, 0.0046301, 0.00249138, 0.0012593, 0.00054165, 0.00027795, 0.00014711, 0.00006103, 0.00003439, 0.00001771, 0.00000722 };
constexpr SpectralSamples cieCmfZ = { 0.00030502, 0.00103681, 0.00531314, 0.01795439, 0.05707758, 0.11365162, 0.17335873, 0.19620658, 0.18608237, 0.13995048, 0.08917453, 0.04789621, 0.02814563, 0.01613766, 0.0077591, 0.00429615, 0.00200551, 0.00086147, 0.00036904, 0.00019143, 0.00014956, 0.00009231, 0.00006813, 0.00002883, 0.00001577, 0.00000394, 0.00000158, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
constexpr std::array<DoubleRgb, 3> xyzRgb = { { { 3.24306333, -1.53837619, -0.49893282 }, { -0.96896309, 1.87542451, 0.04154303 }, { 0.05568392, -0.20417438, 1.05799454 } } };

double dotProduct (const SpectralSamples& a, const SpectralSamples& b) noexcept
{
    return std::inner_product (a.begin(), a.end(), b.begin(), 0.0);
}

double dotProduct (const DoubleRgb& a, const DoubleRgb& b) noexcept
{
    return std::inner_product (a.begin(), a.end(), b.begin(), 0.0);
}

double compand (double value) noexcept
{
    if (value < 0.0031308)
        return value * 12.92;

    return 1.055 * std::pow (value, 1.0 / spectralGamma) - 0.055;
}

double uncompand (double value) noexcept
{
    if (value < 0.04045)
        return value / 12.92;

    return std::pow ((value + 0.055) / 1.055, spectralGamma);
}

IntRgb linearToSrgb (const DoubleRgb& lrgb) noexcept
{
    const double r = compand (lrgb[0]);
    const double g = compand (lrgb[1]);
    const double b = compand (lrgb[2]);

    return {
        static_cast<int> (jlimit (0.0, 1.0, r) * 255.0),
        static_cast<int> (jlimit (0.0, 1.0, g) * 255.0),
        static_cast<int> (jlimit (0.0, 1.0, b) * 255.0)
    };
}

IntRgb xyzToSrgb (const DoubleRgb& xyz) noexcept
{
    const double r = dotProduct (xyzRgb[0], xyz);
    const double g = dotProduct (xyzRgb[1], xyz);
    const double b = dotProduct (xyzRgb[2], xyz);

    return linearToSrgb ({ r, g, b });
}

DoubleRgb reflectanceToXyz (const SpectralSamples& reflectance) noexcept
{
    return {
        dotProduct (reflectance, cieCmfX),
        dotProduct (reflectance, cieCmfY),
        dotProduct (reflectance, cieCmfZ)
    };
}

std::array<double, 7> spectralUpsampling (const DoubleRgb& lrgb) noexcept
{
    double w = jmin (lrgb[0], lrgb[1], lrgb[2]);

    const DoubleRgb normalized = { lrgb[0] - w, lrgb[1] - w, lrgb[2] - w };

    const double c = jmin (normalized[1], normalized[2]);
    const double m = jmin (normalized[0], normalized[2]);
    const double y = jmin (normalized[0], normalized[1]);
    const double r = jmax (0.0, jmin (normalized[0] - normalized[2], normalized[0] - normalized[1]));
    const double g = jmax (0.0, jmin (normalized[1] - normalized[2], normalized[1] - normalized[0]));
    const double b = jmax (0.0, jmin (normalized[2] - normalized[1], normalized[2] - normalized[0]));

    return { w, c, m, y, r, g, b };
}

SpectralSamples linearToReflectance (const DoubleRgb& lrgb) noexcept
{
    const auto weights = spectralUpsampling (lrgb);
    SpectralSamples reflectance = {};

    for (int i = 0; i < spectralSampleCount; ++i)
    {
        reflectance[i] = jmax (
            spectralEpsilon,
            weights[0] + weights[1] * spdC[i] + weights[2] * spdM[i] + weights[3] * spdY[i] + weights[4] * spdR[i] + weights[5] * spdG[i] + weights[6] * spdB[i]);
    }

    return reflectance;
}

DoubleRgb srgbToLinear (Color color) noexcept
{
    const double r = uncompand (color.getRedFloat());
    const double g = uncompand (color.getGreenFloat());
    const double b = uncompand (color.getBlueFloat());

    return { r, g, b };
}

double linearToConcentration (double l1, double l2, double t) noexcept
{
    const double t1 = l1 * std::pow (1.0 - t, 2.0);
    const double t2 = l2 * std::pow (t, 2.0);

    return t2 / (t1 + t2);
}

IntRgb spectralMix (const DoubleRgb& lrgb1, const DoubleRgb& lrgb2, double t) noexcept
{
    const SpectralSamples reflectance1 = linearToReflectance (lrgb1);
    const SpectralSamples reflectance2 = linearToReflectance (lrgb2);

    const double l1 = dotProduct (reflectance1, cieCmfY);
    const double l2 = dotProduct (reflectance2, cieCmfY);

    t = linearToConcentration (l1, l2, t);

    SpectralSamples reflectance = {};

    for (int i = 0; i < spectralSampleCount; ++i)
    {
        const double ks1 = (1.0 - t) * (std::pow (1.0 - reflectance1[i], 2.0) / (2.0 * reflectance1[i]));
        const double ks2 = t * (std::pow (1.0 - reflectance2[i], 2.0) / (2.0 * reflectance2[i]));
        const double ks = ks1 + ks2;
        const double km = 1.0 + ks - std::sqrt (std::pow (ks, 2.0) + 2.0 * ks);

        reflectance[i] = km;
    }

    return xyzToSrgb (reflectanceToXyz (reflectance));
}

//==============================================================================

struct DoubleHsl
{
    double h = 0.0;
    double s = 0.0;
    double l = 0.0;
};

DoubleHsl rgbToHsl (const DoubleRgb& rgb) noexcept
{
    const double maxValue = jmax (rgb[0], rgb[1], rgb[2]);
    const double minValue = jmin (rgb[0], rgb[1], rgb[2]);
    const double l = (maxValue + minValue) * 0.5;

    if (maxValue == minValue)
        return { 0.0, 0.0, l };

    const double d = maxValue - minValue;
    const double s = l > 0.5 ? d / (2.0 - maxValue - minValue) : d / (maxValue + minValue);
    double h = 0.0;

    if (maxValue == rgb[0])
        h = (rgb[1] - rgb[2]) / d + (rgb[1] < rgb[2] ? 6.0 : 0.0);
    else if (maxValue == rgb[1])
        h = (rgb[2] - rgb[0]) / d + 2.0;
    else
        h = (rgb[0] - rgb[1]) / d + 4.0;

    h /= 6.0;
    return { h, s, l };
}

DoubleRgb hslToRgb (const DoubleHsl& hsl) noexcept
{
    auto hueToRgb = [] (double p, double q, double t)
    {
        if (t < 0.0)
            t += 1.0;
        if (t > 1.0)
            t -= 1.0;
        if (t < 1.0 / 6.0)
            return p + (q - p) * 6.0 * t;
        if (t < 1.0 / 2.0)
            return q;
        if (t < 2.0 / 3.0)
            return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
        return p;
    };

    double r = hsl.l;
    double g = hsl.l;
    double b = hsl.l;

    if (hsl.s != 0.0)
    {
        const double q = hsl.l < 0.5 ? hsl.l * (1.0 + hsl.s) : hsl.l + hsl.s - hsl.l * hsl.s;
        const double p = 2.0 * hsl.l - q;
        r = hueToRgb (p, q, hsl.h + 1.0 / 3.0);
        g = hueToRgb (p, q, hsl.h);
        b = hueToRgb (p, q, hsl.h - 1.0 / 3.0);
    }

    return { r, g, b };
}

double blendChannel (BlendMode mode, double backdrop, double source) noexcept
{
    switch (mode)
    {
        case BlendMode::SrcOver:
            return source;
        case BlendMode::Screen:
            return 1.0 - (1.0 - backdrop) * (1.0 - source);
        case BlendMode::Overlay:
            return backdrop <= 0.5 ? 2.0 * backdrop * source : 1.0 - 2.0 * (1.0 - backdrop) * (1.0 - source);
        case BlendMode::Darken:
            return jmin (backdrop, source);
        case BlendMode::Lighten:
            return jmax (backdrop, source);
        case BlendMode::ColorDodge:
            return source >= 1.0 ? 1.0 : jmin (1.0, backdrop / (1.0 - source));
        case BlendMode::ColorBurn:
            return source <= 0.0 ? 0.0 : 1.0 - jmin (1.0, (1.0 - backdrop) / source);
        case BlendMode::HardLight:
            return source <= 0.5 ? 2.0 * backdrop * source : 1.0 - 2.0 * (1.0 - backdrop) * (1.0 - source);
        case BlendMode::SoftLight:
        {
            if (source <= 0.5)
                return backdrop - (1.0 - 2.0 * source) * backdrop * (1.0 - backdrop);

            const double d = backdrop <= 0.25 ? ((16.0 * backdrop - 12.0) * backdrop + 4.0) * backdrop : std::sqrt (backdrop);
            return backdrop + (2.0 * source - 1.0) * (d - backdrop);
        }
        case BlendMode::Difference:
            return std::abs (backdrop - source);
        case BlendMode::Exclusion:
            return backdrop + source - 2.0 * backdrop * source;
        case BlendMode::Multiply:
            return backdrop * source;
        case BlendMode::Hue:
        case BlendMode::Saturation:
        case BlendMode::Color:
        case BlendMode::Luminosity:
            return source;
        default:
            return source;
    }
}

DoubleRgb blendRgb (BlendMode mode, const DoubleRgb& backdrop, const DoubleRgb& source) noexcept
{
    if (mode == BlendMode::Hue || mode == BlendMode::Saturation || mode == BlendMode::Color || mode == BlendMode::Luminosity)
    {
        const auto hslBackdrop = rgbToHsl (backdrop);
        const auto hslSource = rgbToHsl (source);
        DoubleHsl blended = hslBackdrop;

        if (mode == BlendMode::Hue)
            blended.h = hslSource.h;
        else if (mode == BlendMode::Saturation)
            blended.s = hslSource.s;
        else if (mode == BlendMode::Color)
        {
            blended.h = hslSource.h;
            blended.s = hslSource.s;
        }
        else if (mode == BlendMode::Luminosity)
            blended.l = hslSource.l;

        return hslToRgb (blended);
    }

    return {
        blendChannel (mode, backdrop[0], source[0]),
        blendChannel (mode, backdrop[1], source[1]),
        blendChannel (mode, backdrop[2], source[2])
    };
}
} // namespace

//==============================================================================

Color& Color::mixWith (Color other, float amount, ColorSpace space) noexcept
{
    const float clamped = jlimit (0.0f, 1.0f, amount);

    if (space == ColorSpace::RGB)
    {
        if (clamped <= 0.0f)
            return *this;

        if (clamped >= 1.0f)
        {
            *this = other;
            return *this;
        }

        return blendWith (other.withMultipliedAlpha (clamped), BlendMode::SrcOver);
    }

    if (space == ColorSpace::SRGB)
    {
        const double ra = getRedFloat();
        const double ga = getGreenFloat();
        const double ba = getBlueFloat();
        const double rb = other.getRedFloat();
        const double gb = other.getGreenFloat();
        const double bb = other.getBlueFloat();

        const float outA = getAlphaFloat() + (other.getAlphaFloat() - getAlphaFloat()) * clamped;
        const float outR = static_cast<float> (ra + (rb - ra) * clamped);
        const float outG = static_cast<float> (ga + (gb - ga) * clamped);
        const float outB = static_cast<float> (ba + (bb - ba) * clamped);

        a = normalizedToComponent (outA);
        r = normalizedToComponent (outR);
        g = normalizedToComponent (outG);
        b = normalizedToComponent (outB);
        return *this;
    }

    const auto mixed = spectralMix (srgbToLinear (*this), srgbToLinear (other), static_cast<double> (clamped));
    const float alpha = getAlphaFloat() + (other.getAlphaFloat() - getAlphaFloat()) * clamped;

    a = normalizedToComponent (alpha);
    r = static_cast<uint8> (mixed[0]);
    g = static_cast<uint8> (mixed[1]);
    b = static_cast<uint8> (mixed[2]);
    return *this;
}

Color Color::mixedWith (Color other, float amount, ColorSpace space) const noexcept
{
    Color result (*this);
    result.mixWith (other, amount, space);
    return result;
}

//==============================================================================

Color& Color::blendWith (Color src, BlendMode mode) noexcept
{
    const double srcAlpha = src.getAlphaFloat();
    const double destAlpha = getAlphaFloat();

    if (destAlpha <= 0.0)
    {
        *this = src;
        return *this;
    }

    if (srcAlpha <= 0.0)
        return *this;

    const DoubleRgb dest = { getRedFloat(), getGreenFloat(), getBlueFloat() };
    const DoubleRgb source = { src.getRedFloat(), src.getGreenFloat(), src.getBlueFloat() };
    const DoubleRgb blended = blendRgb (mode, dest, source);

    const double outAlpha = srcAlpha + destAlpha - srcAlpha * destAlpha;
    if (outAlpha <= 0.0)
    {
        a = 0;
        r = 0;
        g = 0;
        b = 0;
        return *this;
    }

    auto compositeChannel = [srcAlpha, destAlpha, outAlpha] (double cb, double cs, double bb)
    {
        const double co = (1.0 - srcAlpha) * cb * destAlpha
                        + (1.0 - destAlpha) * cs * srcAlpha
                        + destAlpha * srcAlpha * bb;
        return jlimit (0.0, 1.0, co / outAlpha);
    };

    const float outR = static_cast<float> (compositeChannel (dest[0], source[0], blended[0]));
    const float outG = static_cast<float> (compositeChannel (dest[1], source[1], blended[1]));
    const float outB = static_cast<float> (compositeChannel (dest[2], source[2], blended[2]));
    const float outA = static_cast<float> (jlimit (0.0, 1.0, outAlpha));

    r = normalizedToComponent (outR);
    g = normalizedToComponent (outG);
    b = normalizedToComponent (outB);
    a = normalizedToComponent (outA);
    return *this;
}

Color Color::blendedWith (Color src, BlendMode mode) const noexcept
{
    Color result (*this);
    result.blendWith (src, mode);
    return result;
}

//==============================================================================

std::tuple<float, float, float> Color::toHSLuv() const noexcept
{
    const auto hsluv = hsluvFromRgb (getRedFloat(), getGreenFloat(), getBlueFloat());
    const float h = static_cast<float> (modulo (hsluv[0] / 360.0, 1.0));
    const float s = static_cast<float> (jlimit (0.0, 1.0, hsluv[1] / 100.0));
    const float l = static_cast<float> (jlimit (0.0, 1.0, hsluv[2] / 100.0));
    return std::make_tuple (h, s, l);
}

Color Color::fromHSLuv (float h, float s, float l, float a) noexcept
{
    const float normalizedHue = modulo (h, 1.0f);
    const float normalizedSaturation = jlimit (0.0f, 1.0f, s);
    const float normalizedLuminance = jlimit (0.0f, 1.0f, l);
    const auto rgb = hsluvToRgb (normalizedHue * 360.0, normalizedSaturation * 100.0, normalizedLuminance * 100.0);

    return {
        normalizedToComponent (jlimit (0.0f, 1.0f, a)),
        normalizedToComponent (static_cast<float> (rgb[0])),
        normalizedToComponent (static_cast<float> (rgb[1])),
        normalizedToComponent (static_cast<float> (rgb[2]))
    };
}

//==============================================================================

String Color::toString() const
{
    String result;
    result << "#"
           << String::toHexString (r).paddedLeft ('0', 2)
           << String::toHexString (g).paddedLeft ('0', 2)
           << String::toHexString (b).paddedLeft ('0', 2)
           << String::toHexString (a).paddedLeft ('0', 2);
    return result;
}

String Color::toStringRGB (bool withAlpha) const
{
    String result;
    result << "rgb(" << String (r) << "," << String (g);

    if (withAlpha)
        result << "," << String (b);

    result << ")";

    return result;
}

//==============================================================================

Color Color::fromString (const String& colorString)
{
    if (colorString.startsWith ("#"))
        return parseHexColor (colorString);

    else if (colorString.startsWithIgnoreCase ("rgb"))
        return parseRGBColor (colorString);

    else if (colorString.startsWithIgnoreCase ("hsl"))
        return parseHSLColor (colorString);

    else
        return parseNamedColor (colorString);
}

} // namespace yup
