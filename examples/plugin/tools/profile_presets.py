#!/usr/bin/env python3
# Copyright (c) 2026 - kunitoki@gmail.com
# ISC license: https://www.isc.org/licenses/
"""Prints the loudness over time of the named presets, useful to check looping and gated envelopes.

Usage: python3 profile_presets.py "Preset Name" ..."""

import wave, struct, sys, math
from retrace_mcp import *
from presets import PRESETS
def profile (name, step=0.125):
    p = [x for x in PRESETS if x["name"] == name][0]
    patch = build (p)
    a = AUDITIONS[p["audition"]]
    call ("render_audio", patch=patch, notes=a["notes"], tail=a["tail"], inline=False, path="/tmp/r.wav", bitsPerSample=16)
    w = wave.open ("/tmp/r.wav"); n = w.getnframes(); sr = w.getframerate(); data = w.readframes (n)
    s = struct.unpack ("<%dh" % (n * 2), data)
    hop = int (sr * step); out = []
    for i in range (0, n, hop):
        seg = s[2 * i:2 * (i + hop):2]
        r = math.sqrt (sum (x * x for x in seg) / max (1, len (seg))) / 32768
        out.append ("%4d" % (20 * math.log10 (r + 1e-9)))
    print (name, " ".join (out))
for n in sys.argv[1:]: profile (n)
