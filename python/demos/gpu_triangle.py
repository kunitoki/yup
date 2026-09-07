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
        self._pipeline = None
        self._target = None
        self._gpuTexture = None
        self._initOk = False
        self._didInit = False
        self.timer = yup.Timer(self.onTimer)
        self.timer.startTimerHz(30)

    def onTimer(self):
        self.repaint()

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        pass

    # ------------------------------------------------------------------
    def _ensureInit(self):
        if self._didInit or self._ctx is None:
            return
        self._didInit = True

        result = yup.GpuPipeline.compileFromGlsl(
            self._ctx, VERT_GLSL, FRAG_GLSL, yup.GpuPipelineOptions(),
        )
        if result:
            self._pipeline = result.getValue()
            self._target = yup.GpuTarget.create(self._ctx, self.TARGET_SIZE, self.TARGET_SIZE)
            self._initOk = self._target is not None

    # ------------------------------------------------------------------
    def _render(self):
        if not self._initOk:
            return

        # Single frame, single render pass, 3-vertex fullscreen triangle
        frame = yup.GpuFrame.begin(self._ctx)
        ropts = yup.GpuRenderOptions(True, yup.Colors.black)
        rp = self._target.beginRenderPass(frame, ropts)

        rp.setPipeline(self._pipeline)
        rp.draw(3)
        rp.finish()

        frame.submit()
        self._gpuTexture = self._target.asTexture()

    # ------------------------------------------------------------------
    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgrey)
        g.fillAll()

        if self._ctx is None:
            self._ctx = g.getGraphicsContext()

        if self._ctx is None or not self._ctx.isGpuAvailable():
            return

        self._ensureInit()

        if not self._initOk:
            font = yup.Font(yup.FontOptions(18.0))
            g.setFillColor(yup.Colors.orange)
            g.fillFittedText(
                "Pipeline compilation failed", font,
                yup.Rectangle[float](0, self.getHeight() / 2 - 20,
                                     self.getWidth(), 40),
                yup.Justification.centred,
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
        api = apiNames.get(self._ctx.getApi(), "?")
        font = yup.Font(yup.FontOptions(14.0))
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
