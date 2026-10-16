# Scene3D renderer baseline

Reference images and timings of the renderer before the EEVEE-class work. Phase 1 compares against them.

## Images

Files are named `<platform>_<scene>.png`, with platform `metal` or `gl` and scene `shadows`, `materials` or `edges`. They are 128x128.

## Timings

Average ms/frame from the Benchmark button of the Scene3D demo (300 frames at 1920x1080).

| Device | Backend | Browser / OS | Build config | ms/frame | Date |
|--------|---------|--------------|--------------|----------|------|
|        | WebGL2  |              | Release      |          |      |
|        | GLES    | Android      | Release      |          |      |

### Phase 3

The raster gate: Phase 3 default settings (2 sun cascades on raster, hard shadows) at most 5% slower than the baseline row of the same device above. If it fails, `RenderCapabilities::defaultRasterCascades` drops to 1 and the device is measured again.

| Device | Backend | Browser / OS | Build config | Raster cascades | ms/frame | vs baseline | Date |
|--------|---------|--------------|--------------|-----------------|----------|-------------|------|
|        | WebGL2  |              | Release      | 2               |          |             |      |
|        | GLES    | Android      | Release      | 2               |          |             |      |

### Phase 4

The raster gate again, at Phase 4 default settings (no probes, volumes or ambient occlusion on raster). Measure the MPC scene once without probes (the gate row) and once with the demo's box probe and irradiance volume, to record what they cost.

| Device | Backend | Browser / OS | Build config | Scene | ms/frame | vs baseline | Date |
|--------|---------|--------------|--------------|-------|----------|-------------|------|
|        | WebGL2  |              | Release      | no probes |      |             |      |
|        | WebGL2  |              | Release      | demo probes |    |             |      |
|        | GLES    | Android      | Release      | no probes |      |             |      |
|        | GLES    | Android      | Release      | demo probes |    |             |      |

## Phase 4 raster gate

`SceneRendererMetalTests.MatchesPhaseZeroBaselineOnRaster` and its GL twin render the three scenes on raster at the default settings and compare them with the images above. Metal results (2026-10-15):

| Scene | Check | Phase 4 | Baseline |
|-------|-------|---------|----------|
| `materials` | pixels more than 2 levels off | 0 | must be 0 |
| `edges` | pixels more than 2 levels off | 0 | must be 0 |
| `edges` | MSAA coverage: intermediate pixels on the silhouettes | 170 | 170 |
| `shadows` | lit minus shadowed margin (red levels) | 14 | 14 |
| `shadows` | shadow pixels | 126 | 126 |
| `shadows` | pixels off outside the plates | 0 | must be 0 |
| `shadows` | plates' mean luma | 226.54 | 226.61 (-0.03%) |
| all | new pixels with a channel at 0 or 255 (a non-finite value) | 0 | must be 0 |

The gate still compares against the Phase 0 images, as it did through Phase 3. No pixel of the `shadows` scene is more than 2 levels from its image. The plates have no material, so they take the default white dielectric (roughness 0.5): with an f0 of 0.04, neither the split-sum table nor the multiple-scattering compensation shows there. The look change of Phase 4 is on metals, where a rough white metal now reflects all of a uniform environment instead of 45% of it (`RoughWhiteMetalPassesTheWhiteFurnaceTest`).

`metal_shadows.png` was recaptured on 2026-10-17, from the current frame. The Metal test device now compiles Rive's shaders synchronously, so every readback goes through the specialized shader, and the Phase 0 image had been read back through the fallback shader of a fresh context. The new image is within 2 levels of the old one (1259 pixels changed). `metal_materials.png` and `metal_edges.png` remain the Phase 0 images.

There are no `gl_*.png` images yet, so the GL twin skips until they are captured on Linux (see below). Captured now, they record the Phase 4 frame, and the GL gate checks later phases against it.

`SceneRendererTierTests.DefaultSettingsAddNoOptInPass` (and its GL twin) checks the default frame on both tiers, with no probe or volume in the scene. Raster creates no depth and normal prepass, ambient occlusion, screen-space reflections, Hi-Z pyramid, screen-space GI or probe baker, and shades with the default variant, which declares no probe block. The full tier traces ambient occlusion and screen-space reflections (both on by default) and no screen-space GI.

Phase 4 adds no per-frame allocation on raster. Each renderer creates two objects once: a 64x64x2 rgba16f lookup table, which replaces the two LTC textures of Phase 2, and a 1x1 black cube placeholder. A scene with an environment also builds its prefiltered cube once per environment, in a frame of its own.

### Default raster shader bundles

The bundles a default raster frame draws with, decoded with `yup_shader_bundler --inspect --print all --info` (every transpiled language and the reflection), plus a hash of each SPIR-V, binary and source chunk. They are compared with the last commit before Phase 4, 588510d1. Two Phase 3 changes were not committed there yet: the soft-shadow filter floor, which only the soft-shadow variants compile, and the RHI dynamic uniform offsets, a pipeline option that changes no shader.

| Bundle | Change since Phase 3 | Cause |
|--------|----------------------|-------|
| `ShadowPass` | none, byte-identical | |
| `ShadowPass_masked` | WGSL only: unused library functions, which the WGSL output does not strip. SPIR-V, GLSL, ESSL, HLSL, MSL and reflection are identical. | The split-sum and multiple-scattering helpers in `bsdf.glsl` and the RGBM helpers in `color.glsl`. Nothing runs differently. |
| `Background` | fragment stage, all languages | The environment is a GPU cube, sampled through the shared trilinear `linearClamp`, instead of an equirect image with its own sampler. RGBM is decoded through a uniform flag, and `LightingBlock` gains `probeInfo`. |
| `ForwardPass`, `ForwardPass_area_lights` | fragment stage, all languages; vertex stage identical | The split-sum lookup table replaces the analytic environment BRDF fit, plus the multiple-scattering compensation: one table fetch per pixel, only with an environment in `ForwardPass`. In `ForwardPass_area_lights` the LTC matrix and amplitude move from two 2D textures into the two layers of the same array, and the area lights share that fetch. The environment is the same GPU cube with the RGBM flag. The ambient and environment term becomes `lightsIndirect`, evaluated before the light loop with the material occlusion. |
| `GpuMipDownsample` (`yup_rhi`) | none, byte-identical | |

Ambient occlusion, probes, volumes, clearcoat and sheen compile only into the opt-in variants, so they add no code to these bundles. From the end of the environment work (402ceebf) to now, the five `yup_3d` bundles decode identically. The `ForwardPass` files changed only in the embedded `.frag` source text, which gained the `#ifdef` blocks of those variants.

A raster frame with an environment also runs `ProbeEquirectToCube`, `ProbeCubeDownsample` and `ProbePrefilter` (new in Phase 4) once per environment, to build its cube.

## How to capture

Images: run the baseline test with `YUP_3D_WRITE_BASELINE=1`. On macOS this writes the Metal images (`SceneRendererMetalTests.CapturesBaselineScenes`), on Linux the GL images (`SceneRendererGLTests.CapturesBaselineScenes`).

Timings: open the Scene3D example and press Benchmark. The result is logged and shown in the view status. Measure the WebGL2 emscripten build in a desktop browser, and one Android GLES device.
