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

## How to capture

Images: run the baseline test with `YUP_3D_WRITE_BASELINE=1`. On macOS this writes the Metal images (`SceneRendererMetalTests.CapturesBaselineScenes`), on Linux the GL images (`SceneRendererGLTests.CapturesBaselineScenes`).

Timings: open the Scene3D example and press Benchmark. The result is logged and shown in the view status. Measure the WebGL2 emscripten build in a desktop browser, and one Android GLES device.
