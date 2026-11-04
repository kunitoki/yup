#!/usr/bin/env python3
# Copyright (c) 2026 - kunitoki@gmail.com
# ISC license: https://www.isc.org/licenses/
"""Minimal JSON-RPC client for the Retrace MCP server plus the patch builder and audition renderer used by the preset tools."""

import copy
import json
import math
import urllib.request

URL = "http://127.0.0.1:7380/mcp"
_id = [0]


def call (tool, **args):
    _id[0] += 1
    body = json.dumps ({"jsonrpc": "2.0", "id": _id[0], "method": "tools/call", "params": {"name": tool, "arguments": args}}).encode()
    request = urllib.request.Request (URL, body, {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"})
    with urllib.request.urlopen (request, timeout=120) as response:
        reply = json.loads (response.read())
    if "error" in reply:
        raise RuntimeError (f"{tool}: {reply['error']}")
    result = reply["result"]
    text = "".join (c.get ("text", "") for c in result.get ("content", []) if c.get ("type") == "text")
    if result.get ("isError"):
        raise RuntimeError (f"{tool}: {text}")
    try:
        return json.loads (text)
    except Exception:
        return text


# Choice indexes
SINE, COSINE, SAW, SQUARE, TRI, PULSE = range (6)
LP12, LP24, HP12, HP24, BP12, BP24, NOTCH, PEAK, ALLPASS, MOOG, KORG35, DIODE, VOWEL, COMB = range (14)
L_SINE, L_TRI, L_SAWUP, L_SQUARE, L_SH, L_SAWDOWN, L_SMOOTH, L_CUSTOM = range (8)
DIV = {"8bar": 0, "4bar": 1, "2bar": 2, "1bar": 3, "1/2": 4, "1/4": 5, "1/8": 6, "1/16": 7, "1/32": 8,
       "1/4T": 9, "1/8T": 10, "1/16T": 11, "1/4D": 12, "1/8D": 13}
POLY, MONO, LEGATO = 0, 1, 2

DEFAULT_ENV = "0.000000 0.000000 0.000000;0.200000 1.000000 0.000000;0.700000 0.300000 -0.400000;1.200000 0.000000 -0.400000"


def adsr (a, d, s, r, curve=-0.4):
    pts = [(0, 0, 0), (a, 1, 0), (a + d, s, curve), (a + d + r, 0, curve)]
    return {"points": pts, "sustain": 2, "loopStart": -1, "loopEnd": -1}


def curve (points, sustain=-1, loop=None):
    return {"points": points, "sustain": sustain, "loopStart": loop[0] if loop else -1, "loopEnd": loop[1] if loop else -1}


def fmt_points (points):
    return ";".join (f"{t:.6f} {l:.6f} {c:.6f}" for t, l, c in points)


def partials_string (values):
    values = list (values) + [0.0] * (128 - len (values))
    return " ".join (f"{v:g}" for v in values[:128])


_base = None


def base_patch():
    global _base
    if _base is None:
        call ("load_preset", factoryIndex=0)
        _base = call ("get_patch")
    return copy.deepcopy (_base)


def build (p):
    patch = base_patch()
    props = patch["properties"]
    props.update ({"preset": -1, "name": p["name"], "bank": "Factory", "author": "kunitoki", "tags": p["tags"],
                   "category": p["category"], "description": p["description"], "notes": p.get ("notes", ""), "presetId": ""})
    for i, m in enumerate (p.get ("macros", [])):
        props[f"macro{i + 1}Name"] = m

    values = dict (p.get ("values", {}))
    for k, v in values.items():
        if k not in props:
            raise KeyError (f"{p['name']}: unknown parameter {k}")
        props[k] = float (v)

    routes = p.get ("routes", [])
    route_children = []
    for slot, (src, dest, depth) in enumerate (routes):
        if dest not in props:
            raise KeyError (f"{p['name']}: unknown route destination {dest}")
        props[f"route.{slot + 1}.depth"] = float (depth)
        route_children.append ({"type": "Route", "properties": {"slot": slot, "source": src, "destination": dest}, "children": []})

    # Enable used modules
    for src, _, _ in routes:
        if src.startswith ("ENV ") and src != "ENV 1":
            props[f"env{src[4:]}.enabled"] = 1.0
        if src.startswith ("LFO "):
            props[f"lfo{src[4:]}.enabled"] = 1.0
    for op in range (2, 5):
        used = props[f"osc{op}.level"] > 0 or any (props[f"fm.{c}{op}"] > 0 for c in range (1, 5) if c != op)
        used = used or any (d in (f"osc{op}.level",) or d.endswith (f"{op}") and d.startswith ("fm.") for _, d, _ in routes)
        if f"osc{op}.enabled" not in values:
            props[f"osc{op}.enabled"] = 1.0 if used else 0.0

    for fx in p.get ("fx", {}):
        props[f"fx.{fx}.enabled"] = 1.0

    children = []
    for child in patch["children"]:
        t = child["type"]
        if t.startswith ("Partials"):
            index = int (t[-1])
            drawn = p.get ("partials", {}).get (index)
            if drawn is not None:
                child["properties"]["custom"] = True
                child["properties"]["sines"] = partials_string (drawn)
                child["properties"]["cosines"] = partials_string ([])
        elif t == "Curves":
            for c in child["children"]:
                index = c["properties"]["index"]
                src = None
                if c["type"] == "Envelope":
                    src = p.get ("envs", {}).get (index)
                elif c["type"] == "LFO":
                    src = p.get ("lfoCurves", {}).get (index)
                elif c["type"] == "Shaper":
                    src = p.get ("shaper")
                if src is not None:
                    c["properties"].update ({"points": fmt_points (src["points"]), "sustain": src["sustain"],
                                             "loopStart": src["loopStart"], "loopEnd": src["loopEnd"]})
        elif t == "Routes":
            child["children"] = route_children
        elif t == "EffectChain" and "fxOrder" in p:
            child["children"] = [{"type": "Effect", "properties": {"name": n}, "children": []} for n in p["fxOrder"]]
        children.append (child)
    patch["children"] = children
    return patch


AUDITIONS = {
    "chord": dict (notes=[{"note": n, "start": 0, "duration": 3.0, "velocity": 0.8} for n in (48, 55, 60, 64)], tail=3.0),
    "keys": dict (notes=[{"note": n, "start": 0, "duration": 1.5, "velocity": 0.9} for n in (48, 60, 64, 67)]
                  + [{"note": n, "start": 1.6, "duration": 1.0, "velocity": 0.5} for n in (53, 57, 60)], tail=2.0),
    "bass": dict (notes=[{"note": n, "start": i * 0.25, "duration": 0.2, "velocity": 0.9} for i, n in enumerate ((36, 36, 43, 36, 41, 36, 48, 36))]
                  + [{"note": 31, "start": 2.0, "duration": 1.0, "velocity": 1.0}], tail=1.0),
    "lead": dict (notes=[{"note": n, "start": i * 0.4, "duration": 0.38, "velocity": 0.9} for i, n in enumerate ((72, 74, 76, 79))]
                  + [{"note": 84, "start": 1.6, "duration": 1.5, "velocity": 1.0}], tail=1.5),
    "pluck": dict (notes=[{"note": n, "start": i * 0.125, "duration": 0.1, "velocity": 0.9} for i, n in enumerate ((60, 64, 67, 72, 76, 72, 67, 64) * 2)]
                   + [{"note": n, "start": 2.0, "duration": 0.5, "velocity": 1.0} for n in (48, 60, 64, 67)], tail=2.0),
    "long": dict (notes=[{"note": 60, "start": 0, "duration": 4.0, "velocity": 1.0}], tail=3.0),
    "hold": dict (notes=[{"note": n, "start": 0, "duration": 4.0, "velocity": 0.9} for n in (48, 60, 67)], tail=2.0),
    "wobble": dict (notes=[{"note": n, "start": i * 1.0, "duration": 0.95, "velocity": v} for i, (n, v) in enumerate (((36, 1.0), (34, 0.8), (41, 0.9), (31, 1.0)))], tail=1.0),
    "perc": dict (notes=[{"note": n, "start": i * 0.25, "duration": 0.1, "velocity": v} for i, (n, v) in enumerate (((36, 1.0), (48, 0.6), (60, 0.9), (48, 0.6)) * 2)], tail=1.5),
}


def render (patch, audition):
    a = AUDITIONS[audition]
    return call ("render_audio", patch=patch, notes=a["notes"], tail=a["tail"], inline=False, path="/tmp/retrace_render.wav")["analysis"]


def level_match (patch, audition, target_peak=-3.0):
    """Renders the patch and sets global.volume so the audition peaks near the target."""
    analysis = render (patch, audition)
    for _ in range (3):
        error = target_peak - analysis["peakDb"]
        if abs (error) < 0.75:
            break
        vol = patch["properties"]["global.volume"] + error
        patch["properties"]["global.volume"] = round (max (-48.0, min (6.0, vol)), 1)
        analysis = render (patch, audition)
    return analysis
