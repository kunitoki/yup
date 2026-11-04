#!/usr/bin/env python3
# Copyright (c) 2026 - kunitoki@gmail.com
# ISC license: https://www.isc.org/licenses/
"""Renders, level matches and saves the factory presets into ../data/presets through a running Retrace MCP server.

Usage: python3 generate_presets.py [preset name ...]"""

import os, re, sys
from retrace_mcp import *
from presets import PRESETS

OUT = os.path.join (os.path.dirname (os.path.abspath (__file__)), "..", "data", "presets")
only = set (sys.argv[1:])
names = [p["name"] for p in PRESETS]
assert len (names) == len (set (names)), "duplicate names"


def existing_id (path):
    """The id a preset was saved with before, so favorites and ratings survive a regeneration."""
    if not os.path.exists (path):
        return None
    found = re.search (r'presetId="([^"]+)"', open (path, encoding="utf-8").read())
    return found.group (1) if found else None


def keep_id (path, preset_id):
    text = open (path, encoding="utf-8").read()
    open (path, "w", encoding="utf-8").write (re.sub (r'presetId="[^"]*"', f'presetId="{preset_id}"', text, count=1))


for p in PRESETS:
    if only and p["name"] not in only:
        continue
    patch = build (p)
    a = level_match (patch, p["audition"])
    call ("set_patch", patch=patch, mode="replace")
    got = call ("get_patch")
    nroutes = len ([c for c in got["children"] if c["type"] == "Routes"][0]["children"])
    folder = os.path.join (OUT, p["folder"])
    os.makedirs (folder, exist_ok=True)
    path = os.path.join (folder, p["name"] + ".retrace")
    preset_id = p.get ("id") or existing_id (path)
    call ("save_preset", name=p["name"], path=path, overwrite=True,
          category=p["category"], bank="Factory", author="kunitoki", tags=p["tags"], description=p["description"], notes=p.get ("notes", ""))
    if preset_id:
        keep_id (path, preset_id)
    warn = []
    if nroutes != len (p.get ("routes", [])): warn.append (f"routes {nroutes}/{len (p.get ('routes', []))}")
    if a["clippedSamples"]: warn.append ("CLIP")
    if a["peakDb"] < -6 or a["peakDb"] > -1: warn.append ("LEVEL")
    print (f"{p['folder']:12} {p['name']:22} vol={patch['properties']['global.volume']:6.1f} peak={a['peakDb']:6.1f} rms={a['rmsDb']:6.1f} cent={a['spectralCentroid']:7.0f} corr={a['stereoCorrelation']:5.2f} tail={a['silentTail']:4.2f} {' '.join (warn)}", flush=True)
