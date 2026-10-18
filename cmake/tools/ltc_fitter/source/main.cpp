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

/*
    Fits the GGX lookup tables of yup_renderer's area lights with linearly transformed cosines, after:

    E. Heitz, J. Dupuy, S. Hill and D. Neubelt, "Real-Time Polygonal-Light Shading with Linearly
    Transformed Cosines", ACM Transactions on Graphics 35 (4), SIGGRAPH 2016.

    For every cell of a 64x64 table (rows: roughness, columns: sqrt (1 - cos theta)) the GGX lobe
    f (v, l) cos (theta_l) with height-correlated Smith masking is integrated for its magnitude and
    Schlick Fresnel weight, then a clamped cosine transformed by M = R (psi) [[a, 0, b], [0, c, 0],
    [0, 0, 1]] is fitted to the normalized lobe with Nelder-Mead, minimizing the L3 error estimated
    with multiple importance sampling of both distributions. The first cell starts at the lobe's
    average direction, and every other cell from the better fit of its two neighbours towards that
    cell, turned by the change of the average direction, so the table stays continuous.

    The fit is deterministic: fixed Hammersley points, no random numbers, and every cell depends
    only on its seeding neighbours, never on the thread that fits it.
*/

#include <yup_core/yup_core.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <string>
#include <thread>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================
constexpr int tableSize = 64;
constexpr double minRoughness = 0.04;  // The forward shader's roughness floor
constexpr double minCosTheta = 1.0e-4; // The forward shader's n.v floor
constexpr int lobeSamples = 32768;     // Magnitude, Fresnel and average direction
constexpr int fitSamples = 2048;       // Per distribution, in the error estimate
constexpr double pi = MathConstants<double>::pi;
constexpr double maxHalf = 65504.0;

//==============================================================================
struct Vec3
{
    double x = 0.0, y = 0.0, z = 0.0;

    Vec3 operator+ (const Vec3& other) const noexcept { return { x + other.x, y + other.y, z + other.z }; }

    Vec3 operator- (const Vec3& other) const noexcept { return { x - other.x, y - other.y, z - other.z }; }

    Vec3 operator* (double scale) const noexcept { return { x * scale, y * scale, z * scale }; }

    double dot (const Vec3& other) const noexcept { return x * other.x + y * other.y + z * other.z; }

    double length() const noexcept { return std::sqrt (dot (*this)); }

    Vec3 normalized() const noexcept { return *this * (1.0 / length()); }
};

Vec3 cross (const Vec3& a, const Vec3& b) noexcept
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

/** The i-th of n 2D Hammersley points. */
std::array<double, 2> hammersley (uint32 i, uint32 n) noexcept
{
    uint32 bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xaaaaaaaau) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xccccccccu) >> 2u);
    bits = ((bits & 0x0f0f0f0fu) << 4u) | ((bits & 0xf0f0f0f0u) >> 4u);
    bits = ((bits & 0x00ff00ffu) << 8u) | ((bits & 0xff00ff00u) >> 8u);

    return { (static_cast<double> (i) + 0.5) / static_cast<double> (n), static_cast<double> (bits) * 0x1p-32 };
}

//==============================================================================
/** Isotropic GGX with height-correlated Smith masking, for a view in the xz plane towards +x. */
class Ggx
{
public:
    Ggx (double alphaToUse, double cosTheta) noexcept
        : alpha (alphaToUse)
        , view { std::sqrt (1.0 - cosTheta * cosTheta), 0.0, cosTheta }
        , lambdaView (lambda (cosTheta))
    {
    }

    const Vec3& getView() const noexcept { return view; }

    /** f (v, l) cos (theta_l) without Fresnel. */
    double evaluate (const Vec3& l) const noexcept
    {
        if (l.z <= 0.0)
            return 0.0;

        const auto h = (view + l).normalized();
        return distribution (h.z) / (4.0 * view.z * (1.0 + lambdaView + lambda (l.z)));
    }

    /** The density over l of sample(), on the whole sphere. */
    double pdf (const Vec3& l) const noexcept
    {
        const auto h = (view + l).normalized();
        return distribution (h.z) / (4.0 * view.z * (1.0 + lambdaView));
    }

    /** Reflects the view about a visible normal, l may be under the horizon. After E. Heitz,
        "Sampling the GGX Distribution of Visible Normals", JCGT 7 (4), 2018.
    */
    Vec3 sample (double u1, double u2) const noexcept
    {
        const auto stretched = Vec3 { alpha * view.x, alpha * view.y, view.z }.normalized();
        const auto lengthSquared = stretched.x * stretched.x + stretched.y * stretched.y;
        const auto t1 = lengthSquared > 0.0 ? Vec3 { -stretched.y, stretched.x, 0.0 } * (1.0 / std::sqrt (lengthSquared)) : Vec3 { 1.0, 0.0, 0.0 };
        const auto t2 = cross (stretched, t1);

        const auto radius = std::sqrt (u1);
        const auto phi = 2.0 * pi * u2;
        const auto p1 = radius * std::cos (phi);
        const auto s = 0.5 * (1.0 + stretched.z);
        const auto p2 = (1.0 - s) * std::sqrt (1.0 - p1 * p1) + s * radius * std::sin (phi);

        const auto hemisphere = t1 * p1 + t2 * p2 + stretched * std::sqrt (std::max (0.0, 1.0 - p1 * p1 - p2 * p2));
        const auto h = Vec3 { alpha * hemisphere.x, alpha * hemisphere.y, std::max (0.0, hemisphere.z) }.normalized();

        return h * (2.0 * view.dot (h)) - view;
    }

    /** The weight evaluate (l) / pdf (l) of a sample over the horizon. */
    double sampleWeight (const Vec3& l) const noexcept
    {
        return l.z > 0.0 ? (1.0 + lambdaView) / (1.0 + lambdaView + lambda (l.z)) : 0.0;
    }

private:
    double distribution (double cosTheta) const noexcept
    {
        if (cosTheta <= 0.0)
            return 0.0;

        const auto alpha2 = alpha * alpha;
        const auto d = cosTheta * cosTheta * (alpha2 - 1.0) + 1.0;
        return alpha2 / (pi * d * d);
    }

    double lambda (double cosTheta) const noexcept
    {
        const auto cos2 = cosTheta * cosTheta;
        const auto tan2 = (1.0 - cos2) / cos2;
        return 0.5 * (std::sqrt (1.0 + alpha * alpha * tan2) - 1.0);
    }

    double alpha;
    Vec3 view;
    double lambdaView;
};

//==============================================================================
/** A clamped cosine transformed by M = R (psi) [[a, 0, b], [0, c, 0], [0, 0, 1]], psi being the
    rotation of +z towards +x.
*/
class Ltc
{
public:
    using Parameters = std::array<double, 4>; // psi, log (a / alpha), b, log (c / alpha)

    Ltc (const Parameters& parameters, double alpha) noexcept
    {
        const auto psi = parameters[0];
        const auto a = alpha * std::exp (parameters[1]);
        const auto b = parameters[2];
        const auto c = alpha * std::exp (parameters[3]);
        const auto cosPsi = std::cos (psi);
        const auto sinPsi = std::sin (psi);

        m00 = a * cosPsi;
        m02 = b * cosPsi + sinPsi;
        m20 = -a * sinPsi;
        m22 = cosPsi - b * sinPsi;
        m11 = c;

        // The xz block has determinant a, the rotation having none
        i00 = m22 / a;
        i02 = -m02 / a;
        i20 = -m20 / a;
        i22 = m00 / a;
        i11 = 1.0 / c;
        inverseDeterminant = 1.0 / (a * c);
    }

    Vec3 transform (const Vec3& w) const noexcept
    {
        return Vec3 { m00 * w.x + m02 * w.z, m11 * w.y, m20 * w.x + m22 * w.z }.normalized();
    }

    double evaluate (const Vec3& l) const noexcept
    {
        const Vec3 w { i00 * l.x + i02 * l.z, i11 * l.y, i20 * l.x + i22 * l.z };
        const auto length = w.length();
        const auto cosine = w.z / length;

        return cosine > 0.0 ? cosine / pi * inverseDeterminant / (length * length * length) : 0.0;
    }

    /** M^-1 scaled to m22 = 1, as m00, m02, m11, m20. */
    std::array<double, 4> getNormalizedInverse() const noexcept
    {
        return { i00 / i22, i02 / i22, i11 / i22, i20 / i22 };
    }

private:
    double m00, m02, m20, m22, m11;
    double i00, i02, i20, i22, i11;
    double inverseDeterminant;
};

//==============================================================================
template <typename Function>
Ltc::Parameters minimize (Function&& function, const Ltc::Parameters& start, const Ltc::Parameters& steps, int maxIterations)
{
    constexpr int numPoints = 5;

    std::array<Ltc::Parameters, numPoints> points;
    std::array<double, numPoints> values;

    for (int i = 0; i < numPoints; ++i)
    {
        points[i] = start;
        if (i > 0)
            points[i][i - 1] += steps[i - 1];

        values[i] = function (points[i]);
    }

    const auto blend = [] (const Ltc::Parameters& from, const Ltc::Parameters& to, double t)
    {
        Ltc::Parameters result;
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = from[i] + (to[i] - from[i]) * t;
        return result;
    };

    for (int iteration = 0; iteration < maxIterations; ++iteration)
    {
        std::array<int, numPoints> order { 0, 1, 2, 3, 4 };
        std::sort (order.begin(), order.end(), [&] (int a, int b)
        {
            return values[a] < values[b] || (values[a] == values[b] && a < b);
        });

        const auto best = order.front();
        const auto worst = order.back();
        const auto secondWorst = order[numPoints - 2];

        if (values[worst] - values[best] <= 1.0e-9 * std::abs (values[best]))
            break;

        Ltc::Parameters centroid {};
        for (int i = 0; i < numPoints - 1; ++i)
            for (size_t j = 0; j < centroid.size(); ++j)
                centroid[j] += points[order[i]][j] / (numPoints - 1);

        const auto reflected = blend (centroid, points[worst], -1.0);
        const auto reflectedValue = function (reflected);

        if (reflectedValue < values[best])
        {
            const auto expanded = blend (centroid, points[worst], -2.0);
            const auto expandedValue = function (expanded);

            points[worst] = expandedValue < reflectedValue ? expanded : reflected;
            values[worst] = std::min (expandedValue, reflectedValue);
            continue;
        }

        if (reflectedValue < values[secondWorst])
        {
            points[worst] = reflected;
            values[worst] = reflectedValue;
            continue;
        }

        const auto outside = reflectedValue < values[worst];
        const auto contracted = blend (centroid, outside ? reflected : points[worst], 0.5);
        const auto contractedValue = function (contracted);

        if (contractedValue < (outside ? reflectedValue : values[worst]))
        {
            points[worst] = contracted;
            values[worst] = contractedValue;
            continue;
        }

        for (int i = 0; i < numPoints; ++i)
        {
            if (i == best)
                continue;

            points[i] = blend (points[best], points[i], 0.5);
            values[i] = function (points[i]);
        }
    }

    return points[static_cast<size_t> (std::min_element (values.begin(), values.end()) - values.begin())];
}

//==============================================================================
struct Cell
{
    Ltc::Parameters parameters {};
    double averageAngle = 0.0;
    std::array<double, 4> inverse {};
    double magnitude = 0.0;
    double fresnel = 0.0;
};

double getAlpha (int roughnessIndex) noexcept
{
    const auto roughness = std::max (static_cast<double> (roughnessIndex) / (tableSize - 1), minRoughness);
    return roughness * roughness;
}

double getCosTheta (int angleIndex) noexcept
{
    const auto u = static_cast<double> (angleIndex) / (tableSize - 1);
    return std::max (1.0 - u * u, minCosTheta);
}

/** Fits a cell from its fitted neighbours, keeping the better fit, or from a clamped cosine. */
Cell fitCell (int roughnessIndex, int angleIndex, std::initializer_list<const Cell*> neighbours)
{
    const auto alpha = getAlpha (roughnessIndex);
    const Ggx ggx (alpha, getCosTheta (angleIndex));
    const auto& view = ggx.getView();

    Cell cell;
    Vec3 average;

    for (uint32 i = 0; i < static_cast<uint32> (lobeSamples); ++i)
    {
        const auto [u1, u2] = hammersley (i, static_cast<uint32> (lobeSamples));
        const auto l = ggx.sample (u1, u2);
        const auto weight = ggx.sampleWeight (l);
        const auto h = (view + l).normalized();

        cell.magnitude += weight;
        cell.fresnel += weight * std::pow (1.0 - view.dot (h), 5.0);
        average = average + l * weight;
    }

    cell.magnitude /= lobeSamples;
    cell.fresnel /= lobeSamples;
    cell.averageAngle = std::atan2 (average.x, average.z);

    struct LobeSample
    {
        Vec3 direction;
        double value, pdf;
    };

    std::vector<LobeSample> lobe;
    std::vector<Vec3> cosine;
    lobe.reserve (fitSamples);
    cosine.reserve (fitSamples);

    for (uint32 i = 0; i < static_cast<uint32> (fitSamples); ++i)
    {
        const auto [u1, u2] = hammersley (i, static_cast<uint32> (fitSamples));
        const auto l = ggx.sample (u1, u2);
        lobe.push_back ({ l, ggx.evaluate (l) / cell.magnitude, ggx.pdf (l) });

        const auto radius = std::sqrt (u1);
        cosine.push_back ({ radius * std::cos (2.0 * pi * u2), radius * std::sin (2.0 * pi * u2), std::sqrt (1.0 - u1) });
    }

    // The L3 error between the transformed cosine and the normalized lobe, estimated with
    // the balance heuristic over samples of both distributions
    const auto error = [&] (const Ltc::Parameters& parameters)
    {
        // Beyond a quarter turn m22 changes sign, and the matrix cannot be scaled to m22 = 1
        if (std::abs (parameters[0]) >= 0.5 * pi - 1.0e-6)
            return std::numeric_limits<double>::max();

        const Ltc ltc (parameters, alpha);
        auto sum = 0.0;

        const auto accumulate = [&sum] (double ltcValue, double lobeValue, double lobePdf)
        {
            const auto difference = std::abs (ltcValue - lobeValue);
            const auto pdf = ltcValue + lobePdf;

            if (pdf > 0.0)
                sum += difference * difference * difference / pdf;
        };

        for (const auto& sample : lobe)
            accumulate (ltc.evaluate (sample.direction), sample.value, sample.pdf);

        for (const auto& w : cosine)
        {
            const auto l = ltc.transform (w);
            accumulate (ltc.evaluate (l), ggx.evaluate (l) / cell.magnitude, ggx.pdf (l));
        }

        return sum / fitSamples;
    };

    const auto step = 0.25 * std::max (alpha, 0.01);
    const Ltc::Parameters steps { step, 0.25, step, 0.25 };

    // Restarting once lets the simplex escape a premature collapse
    const auto fitFrom = [&] (const Ltc::Parameters& start)
    {
        return minimize (error, minimize (error, start, steps, 400), steps, 400);
    };

    std::vector<Ltc::Parameters> starts;

    for (const auto* neighbour : neighbours)
    {
        if (neighbour == nullptr)
            continue;

        // The neighbour's fit, turned by the change of the lobe's average direction
        auto start = neighbour->parameters;
        start[0] += cell.averageAngle - neighbour->averageAngle;
        starts.push_back (start);
    }

    if (starts.empty())
        starts.push_back ({ cell.averageAngle, 0.0, 0.0, 0.0 });

    auto bestError = std::numeric_limits<double>::max();

    for (const auto& start : starts)
    {
        const auto parameters = fitFrom (start);
        const auto parametersError = error (parameters);

        if (parametersError < bestError)
        {
            cell.parameters = parameters;
            bestError = parametersError;
        }
    }

    cell.inverse = Ltc (cell.parameters, alpha).getNormalizedInverse();

    return cell;
}

//==============================================================================
std::string formatFloat (double value)
{
    char buffer[32];
    std::snprintf (buffer, sizeof (buffer), "%.8ef", static_cast<double> (static_cast<float> (value)));
    return buffer;
}

String writeTables (const std::vector<Cell>& cells)
{
    std::string text;
    text += "// Generated by cmake/tools/ltc_fitter - do not edit by hand.\n";
    text += "// Regenerate with: just ltc_tables\n";
    text += "//\n";
    text += "// GGX fitted with linearly transformed cosines, after E. Heitz, J. Dupuy, S. Hill and D. Neubelt,\n";
    text += "// \"Real-Time Polygonal-Light Shading with Linearly Transformed Cosines\", SIGGRAPH 2016.\n";
    text += "// Rows: roughness, columns: sqrt (1 - cos theta), see LtcTables.\n";
    text += "\n";

    text += "constexpr float ltcInverseMatrices[] = {\n";
    for (const auto& cell : cells)
        text += "    " + formatFloat (cell.inverse[0]) + ", " + formatFloat (cell.inverse[1]) + ", " + formatFloat (cell.inverse[2]) + ", " + formatFloat (cell.inverse[3]) + ",\n";
    text += "};\n\n";

    text += "constexpr float ltcAmplitudes[] = {\n";
    for (const auto& cell : cells)
        text += "    " + formatFloat (cell.magnitude) + ", " + formatFloat (cell.fresnel) + ",\n";
    text += "};\n";

    return String (text);
}

void printUsage()
{
    std::printf ("Usage: yup_ltc_fitter --output <yup_LtcTables.inc>\n");
}

} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    ArgumentList args (argc, argv);

    if (args.containsOption ("--help|-h"))
    {
        printUsage();
        return 0;
    }

    const auto outputIndex = args.indexOfOption ("--output");
    if (outputIndex < 0 || outputIndex + 1 >= args.size())
    {
        printUsage();
        return 1;
    }

    const auto output = args[outputIndex + 1].resolveAsFile();
    const auto startTime = Time::getMillisecondCounterHiRes();

    std::vector<Cell> cells (static_cast<size_t> (tableSize * tableSize));
    const auto at = [&cells] (int roughnessIndex, int angleIndex) -> Cell&
    {
        return cells[static_cast<size_t> (roughnessIndex * tableSize + angleIndex)];
    };

    // A wavefront from the rough cell at normal incidence: every cell starts from the fits of
    // its smoother-angle and rougher neighbours, and the cells of a diagonal are independent
    const auto numThreads = static_cast<int> (std::max (1u, std::thread::hardware_concurrency()));

    for (int diagonal = 0; diagonal < 2 * tableSize - 1; ++diagonal)
    {
        const auto firstAngle = std::max (0, diagonal - (tableSize - 1));
        const auto lastAngle = std::min (diagonal, tableSize - 1);

        std::atomic<int> nextAngle { firstAngle };
        std::vector<std::thread> workers;

        for (int i = 0; i < std::min (numThreads, lastAngle - firstAngle + 1); ++i)
        {
            workers.emplace_back ([&]
            {
                for (int a = nextAngle++; a <= lastAngle; a = nextAngle++)
                {
                    const auto r = tableSize - 1 - (diagonal - a);
                    at (r, a) = fitCell (r, a, { a > 0 ? &at (r, a - 1) : nullptr, r < tableSize - 1 ? &at (r + 1, a) : nullptr });
                }
            });
        }

        for (auto& worker : workers)
            worker.join();
    }

    auto maxEntry = 0.0;

    for (const auto& cell : cells)
    {
        for (const auto value : { cell.inverse[0], cell.inverse[1], cell.inverse[2], cell.inverse[3], cell.magnitude, cell.fresnel })
        {
            if (! std::isfinite (value))
            {
                std::fprintf (stderr, "Non-finite table entry, nothing written\n");
                return 1;
            }

            maxEntry = std::max (maxEntry, std::abs (value));
        }
    }

    std::printf ("Fitted %d cells in %.1f s, largest entry %g\n",
                 tableSize * tableSize,
                 (Time::getMillisecondCounterHiRes() - startTime) / 1000.0,
                 maxEntry);

    if (maxEntry > maxHalf)
    {
        std::fprintf (stderr, "A matrix entry exceeds the half float range, nothing written\n");
        return 1;
    }

    if (! output.replaceWithText (writeTables (cells), false, false, "\n"))
    {
        std::fprintf (stderr, "Cannot write %s\n", output.getFullPathName().toRawUTF8());
        return 1;
    }

    std::printf ("Wrote %s\n", output.getFullPathName().toRawUTF8());
    return 0;
}
