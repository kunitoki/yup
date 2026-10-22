#!/usr/bin/env python3

# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright (c) 2026 - kunitoki@gmail.com
#
#  YUP is an open source library subject to open-source licensing.
#
#  The code included in this file is provided under the terms of the ISC license
#  http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#  to use, copy, modify, and/or distribute this software for any purpose with or
#  without fee is hereby granted provided that the above copyright notice and
#  this permission notice appear in all copies.
#
#  YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

"""
Derive the coefficients of yupToneMapAgx in modules/yup_renderer/shaders/lib/color.glsl.

The AgX idea (an inset of the primaries, a per-channel sigmoid in the log domain,
then an outset) is public, but the reference repository at
https://github.com/sobotka/AgX carries no license (checked 2026-10-08), so none
of its values are used. Every number below is our own choice, stated here, and
no Blender or OCIO source or configuration was consulted.

- Inset: the Rec.709 primaries are pushed away from the D65 white point in CIE xy
  by INSET_SCALE and rotated around it by ROTATION_DEGREES. Rec.709 colors
  expressed in those wider primaries are less pure, so the per-channel curve
  rolls bright saturated colors off toward white instead of skewing their hue.
- Outset: the inverse of the same construction with the same rotation and the
  smaller OUTSET_SCALE, which undoes the rotation and restores part of the purity.
  Both matrices keep D65 white, so their rows sum to 1 and greys stay grey.
- Log encoding: log2 of the input relative to mid grey 0.18, mapped from
  [MIN_EV, MAX_EV] stops to [0, 1]. Mid grey sits near the middle of the range,
  so the toe and the shoulder get a similar share of the curve: black at about
  0.0056 and white at about 11.5 in scene-linear units.
- Sigmoid: y = g(k t) / g(k) on each side of mid grey, with the algebraic
  g(u) = u / sqrt(1 + u^2), so both halves join with matching first and second
  derivatives. y is sRGB encoded, and the shader decodes it before the outset.
  It maps mid grey to display-linear MID_GREY_OUTPUT, and its contrast there is
  CONTRAST, defined as the slope dy/dx of the encoded output y against the
  normalized log input x. Over the 11 stops of the log range that is a
  display-linear against scene-linear log-log slope of about 0.975, between
  Reinhard (0.85) and the ACES fit (1.13) used by the other operators.
- The sigmoid is fitted with a 6th order least squares polynomial in the
  normalized log value, which the shader evaluates.

Run with: python3 tools/agx_fit.py (standard library only).
"""

import math

MID_GREY = 0.18
MIN_EV = -5.0
MAX_EV = 6.0

CONTRAST = 1.6
MID_GREY_OUTPUT = 0.18

INSET_SCALE = (0.20, 0.15, 0.20)
OUTSET_SCALE = (0.10, 0.075, 0.10)
ROTATION_DEGREES = (2.0, -1.0, -3.0)

POLYNOMIAL_DEGREE = 6

REC709_PRIMARIES = ((0.64, 0.33), (0.30, 0.60), (0.15, 0.06))
D65_WHITE = (0.3127, 0.3290)


# ==============================================================================
def matmul (a, b):
    return [[sum (a[i][k] * b[k][j] for k in range (len (b))) for j in range (len (b[0]))] for i in range (len (a))]


def inverse3 (m):
    a, b, c = m[0]
    d, e, f = m[1]
    g, h, i = m[2]
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)
    return [[(e * i - f * h) / det, (c * h - b * i) / det, (b * f - c * e) / det],
            [(f * g - d * i) / det, (a * i - c * g) / det, (c * d - a * f) / det],
            [(d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det]]


def xyz_from_rgb (primaries, white):
    """The RGB to XYZ matrix of the given primaries, with RGB (1, 1, 1) mapping to the white point at Y = 1."""
    columns = [[x / y, 1.0, (1.0 - x - y) / y] for x, y in primaries]
    p = [[columns[j][i] for j in range (3)] for i in range (3)]
    w = [white[0] / white[1], 1.0, (1.0 - white[0] - white[1]) / white[1]]
    s = [sum (row[j] * w[j] for j in range (3)) for row in inverse3 (p)]
    return [[p[i][j] * s[j] for j in range (3)] for i in range (3)]


def transformed_primaries (scales, degrees):
    result = []
    for (x, y), scale, angle in zip (REC709_PRIMARIES, scales, degrees):
        dx, dy = x - D65_WHITE[0], y - D65_WHITE[1]
        c, s = math.cos (math.radians (angle)), math.sin (math.radians (angle))
        rx, ry = (dx * c - dy * s) * (1.0 + scale), (dx * s + dy * c) * (1.0 + scale)
        result.append ((D65_WHITE[0] + rx, D65_WHITE[1] + ry))
    return result


def rec709_to (primaries):
    """Converts Rec.709 RGB into RGB of the given primaries."""
    return matmul (inverse3 (xyz_from_rgb (primaries, D65_WHITE)), xyz_from_rgb (REC709_PRIMARIES, D65_WHITE))


# ==============================================================================
def srgb_encode (c):
    return c * 12.92 if c < 0.0031308 else 1.055 * c ** (1.0 / 2.4) - 0.055


def srgb_decode (c):
    return c / 12.92 if c < 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def log_encode (c):
    return min (max ((math.log2 (max (c, 1.0e-10) / MID_GREY) - MIN_EV) / (MAX_EV - MIN_EV), 0.0), 1.0)


def g (u):
    return u / math.sqrt (1.0 + u * u)


def solve_k (target_slope):
    """Finds k so that d/dt [g (k t) / g (k)] at t = 0, which is k / g (k), equals target_slope."""
    lo, hi = 1.0e-6, 1.0e6
    for _ in range (200):
        mid = math.sqrt (lo * hi)
        if mid / g (mid) < target_slope:
            lo = mid
        else:
            hi = mid
    return mid


X0 = (0.0 - MIN_EV) / (MAX_EV - MIN_EV)
Y0 = srgb_encode (MID_GREY_OUTPUT)

K_TOE = solve_k (CONTRAST * X0 / Y0)
K_SHOULDER = solve_k (CONTRAST * (1.0 - X0) / (1.0 - Y0))


def sigmoid (x):
    if x < X0:
        t = (X0 - x) / X0
        return Y0 - Y0 * g (K_TOE * t) / g (K_TOE)

    t = (x - X0) / (1.0 - X0)
    return Y0 + (1.0 - Y0) * g (K_SHOULDER * t) / g (K_SHOULDER)


# ==============================================================================
def fit_polynomial (samples, degree):
    """Least squares through the normal equations, solved with partial pivoting; coefficients from x^0 up."""
    n = degree + 1
    a = [[0.0] * (n + 1) for _ in range (n)]
    for x, y in samples:
        powers = [x ** i for i in range (2 * n)]
        for i in range (n):
            for j in range (n):
                a[i][j] += powers[i + j]
            a[i][n] += y * powers[i]

    for col in range (n):
        pivot = max (range (col, n), key = lambda r: abs (a[r][col]))
        a[col], a[pivot] = a[pivot], a[col]
        for row in range (col + 1, n):
            factor = a[row][col] / a[col][col]
            for j in range (col, n + 1):
                a[row][j] -= factor * a[col][j]

    coefficients = [0.0] * n
    for row in reversed (range (n)):
        coefficients[row] = (a[row][n] - sum (a[row][j] * coefficients[j] for j in range (row + 1, n))) / a[row][row]
    return coefficients


def evaluate (coefficients, x):
    return sum (c * x ** i for i, c in enumerate (coefficients))


def fit_coefficients ():
    samples = [(i / 1024.0, sigmoid (i / 1024.0)) for i in range (1025)]
    return [float ("%.8g" % c) for c in fit_polynomial (samples, POLYNOMIAL_DEGREE)]


def agx (rgb, coefficients, inset, outset):
    """The scene-linear Rec.709 to display-linear mapping, as the shader computes it."""
    shaped = [sum (inset[i][j] * rgb[j] for j in range (3)) for i in range (3)]
    encoded = [min (max (evaluate (coefficients, log_encode (c)), 0.0), 1.0) for c in shaped]
    linear = [srgb_decode (c) for c in encoded]
    return [min (max (sum (outset[i][j] * linear[j] for j in range (3)), 0.0), 1.0) for i in range (3)]


def aces (c):
    return min (max ((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0), 1.0)


def glsl_mat3 (m):
    # GLSL matrices are column major: each vec3 below is a column
    columns = ["vec3(%.8f, %.8f, %.8f)" % (m[0][j], m[1][j], m[2][j]) for j in range (3)]
    return "mat3(" + ",\n     ".join (columns) + ")"


def main ():
    inset = [[float ("%.8f" % v) for v in row] for row in rec709_to (transformed_primaries (INSET_SCALE, ROTATION_DEGREES))]
    outset = [[float ("%.8f" % v) for v in row] for row in inverse3 (rec709_to (transformed_primaries (OUTSET_SCALE, ROTATION_DEGREES)))]
    coefficients = fit_coefficients ()

    print ("// inset (rows sum to %s)" % ", ".join ("%.6f" % sum (row) for row in inset))
    print (glsl_mat3 (inset))
    print ("// outset (rows sum to %s)" % ", ".join ("%.6f" % sum (row) for row in outset))
    print (glsl_mat3 (outset))
    print ("// log range %.4f to %.4f stops around %.2f" % (MIN_EV, MAX_EV, MID_GREY))
    print ("// polynomial, x^0 first")
    print (", ".join ("%.8g" % c for c in coefficients))

    error = max (abs (evaluate (coefficients, i / 4096.0) - sigmoid (i / 4096.0)) for i in range (4097))
    values = [evaluate (coefficients, i / 4096.0) for i in range (4097)]
    monotonic = all (b >= a for a, b in zip (values, values[1:]))
    print ("// max fit error %.6f, monotonic on [0, 1]: %s, ends %.6f %.6f" % (error, monotonic, values[0], values[-1]))
    print ("// k toe %.4f, k shoulder %.4f" % (K_TOE, K_SHOULDER))

    for level in (0.18, 0.25, 0.5, 1.0, 4.0, 16.0):
        grey = agx ([level] * 3, coefficients, inset, outset)[0]
        print ("// %6.2f -> AgX %.6f (8 bit %3d), ACES 8 bit %3d" % (level, grey, round (srgb_encode (grey) * 255.0), round (srgb_encode (aces (level)) * 255.0)))


if __name__ == "__main__":
    main ()
