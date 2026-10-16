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

"""Regenerates the shader bundles of a module.

Each entry of a bundle set is `Name` or `Name:DEF1+DEF2`. It bundles `yup_<Name>.vert` (or, for a
fragment-only pass, the shared `yup_Fullscreen.vert`), `yup_<Name>.frag` and `yup_<Name>.comp` of the
set's shader folder, whichever exist, with the defines, through `yup_shader_bundler`. The result goes
to `yup_<Name>[_<defs>].inc`, as `xxd -i` bytes under a generated-file header. A define names its
suffix without its `YUP_3D_TIER_` (or else `YUP_3D_`) prefix, in lowercase.

Exits non-zero when the bundler cannot be built or fails on an entry.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BUNDLER = REPO_ROOT / "cmake" / "tools" / "shader_bundler" / "build" / "yup_shader_bundler"
TARGET_LANGS = "glsl,essl,hlsl,msl,spirv,wgsl"

# The bundle sets, named as `just update_shaders` takes them: their shader folder and their entries, in order
BUNDLE_SETS = {
    "yup_3d": {
        "directory": "modules/yup_3d/shaders",
        "entries": [
            "ForwardPass",
            "ForwardPass:YUP_3D_AREA_LIGHTS",
            "ForwardPass:YUP_3D_SOFT_SHADOWS",
            "ForwardPass:YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS",
            "ForwardPass:YUP_3D_LOCAL_INDIRECT",
            "ForwardPass:YUP_3D_AREA_LIGHTS+YUP_3D_LOCAL_INDIRECT",
            "ForwardPass:YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT",
            "ForwardPass:YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT",
            "ForwardPass:YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "ForwardPass:YUP_3D_AREA_LIGHTS+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "ForwardPass:YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "ForwardPass:YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "ForwardPass:YUP_3D_TIER_FULL",
            "ForwardPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS",
            "ForwardPass:YUP_3D_TIER_FULL+YUP_3D_SOFT_SHADOWS",
            "ForwardPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS",
            "Background",
            "GBufferPass:YUP_3D_TIER_FULL",
            "GBufferBackground",
            "DepthNormalPrepass",
            "LightingPass:YUP_3D_TIER_FULL",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_SOFT_SHADOWS",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_LOCAL_INDIRECT",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS+YUP_3D_LOCAL_INDIRECT",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "LightingPass:YUP_3D_TIER_FULL+YUP_3D_AREA_LIGHTS+YUP_3D_SOFT_SHADOWS+YUP_3D_LOCAL_INDIRECT+YUP_3D_LAYERS",
            "ShadowPass",
            "ShadowPass:YUP_3D_MASKED",
            "TemporalAA",
            "TemporalAA:YUP_3D_UPSCALE",
            "ToneMapping",
            "BloomDownsample",
            "BloomUpsample",
            "LightClusters",
            "ProbeEquirectToCube",
            "ProbeCubeDownsample",
            "ProbePrefilter",
            "ProbeCaptureResolve",
            "ProbeCaptureDistance",
            "ProbeShProjection",
            "ProbeVisibility",
            "GtaoTrace",
            "GtaoTrace:YUP_3D_SCREEN_SPACE_GI",
            "GtaoDenoise",
            "GtaoDenoise:YUP_3D_SCREEN_SPACE_GI",
            "SsgiComposite",
            "HiZCopy",
            "HiZDownsample",
            "SsrTrace",
            "SsrResolve",
            "DebugView",
            "DofSetup",
            "DofTiles",
            "DofGather",
            "DofComposite",
            "ToneMapping:YUP_3D_RADIANCE_COMPOSITE",
            "MotionTileMax",
            "MotionNeighborMax",
            "MotionReconstruct",
            "FogApply",
        ],
    },
    "yup_rhi": {
        "directory": "modules/yup_rhi/shaders",
        "entries": [
            "GpuMipDownsample",
        ],
    },
}


def bundle_name_and_defines(entry: str) -> tuple[str, str, list[str]]:
    """Returns the shader name of an entry, its `yup_<Name>[_<defs>]` base name and its -D arguments."""
    name, _, defines = entry.partition(":")
    suffix = ""
    arguments = []

    for define in defines.split("+") if defines else []:
        arguments.append(f"-D{define}")
        part = define.removeprefix("YUP_3D_TIER_")
        if part == define:
            part = define.removeprefix("YUP_3D_")
        suffix += f"_{part.lower()}"

    return name, f"yup_{name}{suffix}", arguments


def stage_arguments(shader_dir: Path, name: str) -> list[str]:
    """Returns the bundler's stage arguments for the sources of @p name that exist."""
    vertex = shader_dir / f"yup_{name}.vert"
    fragment = shader_dir / f"yup_{name}.frag"
    compute = shader_dir / f"yup_{name}.comp"
    fullscreen = shader_dir / "yup_Fullscreen.vert"
    stages = []

    if vertex.is_file():
        stages += ["--vert", str(vertex)]
    elif fragment.is_file() and fullscreen.is_file():
        stages += ["--vert", str(fullscreen)]

    if fragment.is_file():
        stages += ["--frag", str(fragment)]

    if compute.is_file():
        stages += ["--stage", "compute", str(compute)]

    return stages


def bundle_body(ysl: Path) -> str:
    """Returns the bytes of @p ysl as `xxd -i` prints them, without its declaration, its closing brace and its length."""
    dump = subprocess.run(["xxd", "-i", str(ysl)], check=True, capture_output=True, text=True).stdout
    lines = dump.splitlines()[1:]
    kept = [line for line in lines if not line.startswith("};") and "_len =" not in line and line.strip() != ""]
    return "".join(f"{line}\n" for line in kept)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("set", choices=sorted(BUNDLE_SETS), help="the bundle set to regenerate")
    parser.add_argument("--just", default="just", help="the just executable that builds the bundler")
    args = parser.parse_args()

    bundle_set = BUNDLE_SETS[args.set]
    shader_dir = REPO_ROOT / bundle_set["directory"]

    # Builds the bundler, through its own recipe
    if subprocess.run([args.just, "shader_bundler", "--help"], cwd=REPO_ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
        print("could not build yup_shader_bundler", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as scratch:
        for entry in bundle_set["entries"]:
            name, base, defines = bundle_name_and_defines(entry)
            ysl = Path(scratch) / f"{base}.ysl"

            command = [str(BUNDLER), *stage_arguments(shader_dir, name), *defines, f"-I{shader_dir}",
                       "--output", str(ysl), "--target-langs", TARGET_LANGS]
            if subprocess.run(command, cwd=REPO_ROOT).returncode != 0:
                print(f"yup_shader_bundler failed on {entry}", file=sys.stderr)
                return 1

            output = shader_dir / f"{base}.inc"
            header = f"// Generated shader bundle ({base}.ysl) - do not edit by hand.\n// Regenerate with: just update_shaders {args.set}\n"
            output.write_text(header + bundle_body(ysl), newline="\n")
            print(f"wrote {output}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
