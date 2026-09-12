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

#version 450

// Fullscreen waterfall shader. A single fullscreen triangle scrolls the
// previous history texture down by numRows rows and writes the numRows new rows
// at the top, mapping each bin's raw magnitude through the color lookup table
// (a lerp between adjacent entries, matching SpectrogramColorMap::map) entirely
// on the GPU.
//
// Bindings:
//   set=0 binding=0 : previous history texture (UV.y = 0 is the top row)
//   set=0 binding=1 : sampler (implicit default, like the matte pipeline)
//   set=0 binding=2 : WaterfallParams UBO
//   set=0 binding=3 : RowData UBO (W * numRows raw magnitudes, oldest row first)
//   set=0 binding=4 : LutData UBO (256 ARGB color entries)

layout(set = 0, binding = 0) uniform texture2D u_prev;
layout(set = 0, binding = 1) uniform sampler   u_samp;
layout(set = 0, binding = 2) uniform WaterfallParams
{
    float numRows;
    float width;
    float height;
    float bins;
    float pad0;
    float pad1;
    float pad2;
    float pad3;
} p;

layout(set = 0, binding = 3) uniform RowData
{
    vec4 mags[512]; // 2048 raw magnitudes packed as vec4s (std140 tight when W is a multiple of 4)
} rows;

layout(set = 0, binding = 4) uniform LutData
{
    uvec4 lut[64]; // 256 ARGB colors packed as uvec4s
} lut;

layout(location = 0) out vec4 fragColor;

float fetchMag(int idx)
{
    return rows.mags[idx >> 2][idx & 3];
}

void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(p.width, p.height);
    if (uv.y >= p.numRows / p.height)
    {
        fragColor = texture(sampler2D(u_prev, u_samp), vec2(uv.x, uv.y - p.numRows / p.height));
    }
    else
    {
        int x = int(gl_FragCoord.x);
        int y = int(gl_FragCoord.y);
        int row = int(p.numRows) - 1 - y;

        float binPos = float(x) * (p.bins / p.width);
        int bin0 = int(binPos);
        int bin1 = min(bin0 + 1, int(p.bins) - 1);
        float fracBin = binPos - float(bin0);
        float mag = mix(fetchMag(row * int(p.bins) + bin0), fetchMag(row * int(p.bins) + bin1), fracBin);

        float t = clamp(mag, 0.0, 1.0) * 255.0;
        int i0 = int(t);
        int i1 = min(i0 + 1, 255);
        float frac = t - float(i0);
        uint c0 = lut.lut[i0 >> 2][i0 & 3];
        uint c1 = lut.lut[i1 >> 2][i1 & 3];
        vec4 col0 = vec4(float((c0 >> 16) & 255u), float((c0 >> 8) & 255u), float(c0 & 255u), float((c0 >> 24) & 255u)) / 255.0;
        vec4 col1 = vec4(float((c1 >> 16) & 255u), float((c1 >> 8) & 255u), float(c1 & 255u), float((c1 >> 24) & 255u)) / 255.0;
        fragColor = mix(col0, col1, frac);
    }
}
