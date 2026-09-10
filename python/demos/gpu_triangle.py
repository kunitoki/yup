#!/usr/bin/env python3
"""
YUP GPU Triangle Demo — "Hello Triangle"

Follows the SpinningCubeDemo renderCube pattern:
  - Compile GLSL 450 shaders via GpuPipeline::compileFromGlsl
  - Render to a GpuTarget via beginRenderPass()
  - Composite to screen with drawTexture()

Fullscreen-triangle vertex shader (no vertex buffer), fragment shader
fills a colored triangle using gl_FragCoord.
"""

import yup_init
import yup


# ---------------------------------------------------------------------------
# GLSL 450 shaders — fullscreen triangle, no vertex buffer
# ---------------------------------------------------------------------------

VERT_GLSL = """#version 450
void main() {
    // Map gl_VertexIndex (0,1,2) to a fullscreen triangle
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
"""

FRAG_GLSL = """#version 450
layout(location=0) out vec4 fragColor;

void main() {
    // Normalised fragment position (0..1)
    vec2 uv = gl_FragCoord.xy / vec2(512.0, 512.0);

    // Draw a colored triangle: red at top-left, green at top-right, blue at bottom-center
    vec3 red   = vec3(1.0, 0.0, 0.0);   // top-left
    vec3 green = vec3(0.0, 1.0, 0.0);   // top-right
    vec3 blue  = vec3(0.0, 0.0, 1.0);   // bottom-center

    // Barycentric-like interpolation based on fragment position
    float w0 = 1.0 - uv.x - uv.y * 0.5;  // red weight
    float w1 = uv.x - uv.y * 0.5;        // green weight
    float w2 = uv.y;                     // blue weight

    // Only draw inside the triangle region
    if (w0 < 0.0 || w1 < 0.0 || w2 < 0.0)
        discard;

    float sum = w0 + w1 + w2;
    vec3 color = (red * w0 + green * w1 + blue * w2) / sum;

    // Add a subtle black-to-transparent border
    float border = 1.0 - smoothstep(0.0, 0.03, min(min(w0, w1), w2));
    fragColor = vec4(mix(color, vec3(0.0), border), 1.0);
}
"""


# ---------------------------------------------------------------------------
class TriangleWindow(yup.DocumentWindow):
    def __init__(self):
        super().__init__()
        self.setTitle("GPU Hello Triangle")
        self.component = TriangleComponent()
        self.addAndMakeVisible(self.component)

    def resized(self):
        self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class TriangleComponent(yup.Component):
    TARGET_SIZE = 512

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        self._ctx = None
        self._device = None
        self._pipeline = None
        self._target = None
        self._gpuTexture = None
        self._initOk = False
        self._didInit = False

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        # The per-frame hook, called on the render thread immediately before
        # painting. Driving repaints from a yup.Timer instead would call
        # repaint() from the message thread while the render thread is inside
        # paint(), which trips Component's isRepainting assertion.
        self.repaint()

    # ------------------------------------------------------------------
    def _ensureInit(self):
        if self._didInit or self._device is None:
            return
        self._didInit = True

        try:
            self._pipeline = yup.GpuPipeline.compileFromGlsl(
                self._device, VERT_GLSL, FRAG_GLSL, yup.GpuPipelineOptions(),
            )
        except RuntimeError as error:
            print(f"pipeline compile failed: {error}")
            return

        self._target = yup.GpuTarget.create(self._device, self.TARGET_SIZE, self.TARGET_SIZE)
        self._initOk = self._target is not None

    # ------------------------------------------------------------------
    def _render(self):
        if not self._initOk:
            return

        # Single frame, single render pass, 3-vertex fullscreen triangle
        frame = yup.GpuFrame.begin(self._device)
        ropts = yup.GpuRenderOptions(True, yup.Colors.black)
        rp = self._target.beginRenderPass(frame, ropts)

        rp.setPipeline(self._pipeline)
        rp.draw(3)
        rp.finish()

        frame.submit()
        self._gpuTexture = self._target.asTexture()

    # ------------------------------------------------------------------
    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgray)
        g.fillAll()

        if self._ctx is None:
            self._ctx = g.getGraphicsContext()
            self._device = self._ctx.getGpuDevice() if self._ctx is not None else None

        if self._ctx is None or not self._ctx.isGpuAvailable():
            return

        self._ensureInit()

        if not self._initOk:
            font = yup.ApplicationTheme.getGlobalTheme().getDefaultFont().withHeight(18.0)
            g.setFillColor(yup.Colors.orange)
            g.fillFittedText(
                "Pipeline compilation failed", font,
                yup.Rectangle[float](0, self.getHeight() / 2 - 20,
                                     self.getWidth(), 40),
                yup.Justification.center,
            )
            return

        self._render()

        tex = self._gpuTexture
        if tex and tex.isValid():
            tw = float(tex.getWidth())
            th = float(tex.getHeight())
            w = float(self.getWidth())
            h = float(self.getHeight())
            scale = min(w / tw, h / th) * 0.9
            dw = tw * scale
            dh = th * scale
            g.drawTexture(
                tex,
                yup.Rectangle[float]((w - dw) / 2, (h - dh) / 2, dw, dh),
            )

        apiNames = {0: "Headless", 1: "OpenGL", 2: "OpenGL ES",
                    3: "Direct3D", 4: "Metal", 5: "WebGPU"}
        api = apiNames.get(int(self._ctx.getPlatform()), "?")
        font = yup.ApplicationTheme.getGlobalTheme().getDefaultFont().withHeight(14.0)
        g.setFillColor(yup.Colors.white.withAlpha(0.7))
        g.fillFittedText(
            f"GPU: {api}  |  GLSL 450  |  Hello Triangle", font,
            yup.Rectangle[float](10, h - 25, 350, 20),
            yup.Justification.left,
        )


class Application(yup.YUPApplication):
    def getApplicationName(self):
        return "GPU Hello Triangle"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        self.window = TriangleWindow()

        def show():
            yup.Process.makeForegroundProcess()
            self.window.setVisible(True)
            self.window.centreWithSize(yup.Size[int](600, 600))

        yup.MessageManager.callAsync(show)

    def shutdown(self):
        del self.window

    def systemRequestedQuit(self):
        self.quit()


if __name__ == "__main__":
    yup.START_YUP_APPLICATION(Application)
