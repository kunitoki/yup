#!/usr/bin/env python3
"""
YUP GPU Effects Demo

Follows the SpinningCubeDemo multi-pass pattern:
  Pass 1 — draw animated shapes to an offscreen GpuCanvas (2D path)
  Pass 2 — fullscreen pass to GpuTarget via GLSL 450 pipeline
Both passes share a single GpuFrame; composite final result to screen
with drawTexture().
"""

import yup_init
import yup
import math
import time


# ---------------------------------------------------------------------------
# GLSL 450 fullscreen triangle + simple color-transform fragment
# ---------------------------------------------------------------------------

VERT_GLSL = """#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
"""

FRAG_GLSL = """#version 450
layout(set=0, binding=0) uniform texture2D u_tex;
layout(set=0, binding=1) uniform sampler u_samp;
layout(location=0) out vec4 fragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(512.0, 512.0);

    // Sample the pass-1 texture
    vec4 col = texture(sampler2D(u_tex, u_samp), uv);

    // Apply a vignette effect
    vec2 v = uv - 0.5;
    float vignette = 1.0 - dot(v, v) * 1.2;

    // Slight warm tint
    col.rgb *= vec3(1.05, 0.95, 0.9);

    fragColor = col * vignette;
}
"""


# ---------------------------------------------------------------------------
class EffectsWindow(yup.DocumentWindow):
    def __init__(self):
        super().__init__()
        self.setTitle("GPU Effects Demo")
        self.component = EffectsComponent()
        self.addAndMakeVisible(self.component)

    def resized(self):
        self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class EffectsComponent(yup.Component):
    SIZE = 512

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        self._ctx = None
        self._canvas2D = None     # pass 1: 2D drawing
        self._target = None       # pass 2: render target
        self._pipeline = None
        self._gpuTexture = None
        self._initOk = False
        self._didInit = False
        self._startTime = time.perf_counter()
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

        self._canvas2D = yup.GpuCanvas.create(self._ctx, self.SIZE, self.SIZE)
        self._target = yup.GpuTarget.create(self._ctx, self.SIZE, self.SIZE)

        if self._canvas2D is None or self._target is None:
            return

        result = yup.GpuPipeline.compileFromGlsl(
            self._ctx, VERT_GLSL, FRAG_GLSL, yup.GpuPipelineOptions(),
        )
        if result:
            self._pipeline = result.getValue()
            self._initOk = True

    # ------------------------------------------------------------------
    def _render(self):
        if not self._initOk:
            return

        # --- Pass 1: 2D drawing to canvas ---
        g2 = self._canvas2D.beginDraw()
        try:
            cw = float(self._canvas2D.getWidth())
            ch = float(self._canvas2D.getHeight())
            t = time.perf_counter() - self._startTime

            g2.setFillColor(yup.Colors.darkblue)
            g2.fillAll()

            for i in range(6):
                r = 40.0 + float(i) * 35.0 + math.sin(t * 2.0 + i) * 10.0
                hue = (float(i) / 6.0 + t * 0.15) % 1.0
                g2.setStrokeColor(yup.Color.fromHSV(hue, 0.8, 1.0, 0.7))
                g2.setStrokeWidth(4.0)
                g2.strokeEllipse(
                    yup.Rectangle[float](cw / 2.0 - r, ch / 2.0 - r, r * 2.0, r * 2.0)
                )

            font = yup.Font(yup.FontOptions(24.0))
            g2.setFillColor(yup.Colors.white)
            g2.fillFittedText(
                "Multi-Pass GPU", font,
                yup.Rectangle[float](0.0, ch - 50.0, cw, 40.0),
                yup.Justification.centred,
            )
        finally:
            self._canvas2D.commit()

        inputTex = self._canvas2D.asTexture()

        # --- Pass 2: fullscreen pipeline with vignette effect ---
        # Both passes share one GpuFrame — GPU serialises them.
        frame = yup.GpuFrame.begin(self._ctx)

        ropts = yup.GpuRenderOptions(True, yup.Colors.transparentBlack)
        rp = self._target.beginRenderPass(frame, ropts)

        rp.setPipeline(self._pipeline)
        rp.setTexture(0, 0, inputTex)
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
            f"GPU: {api}  |  GLSL 450  |  Canvas → Vignette → Screen", font,
            yup.Rectangle[float](10, h - 25, 420, 20),
            yup.Justification.left,
        )


class Application(yup.YUPApplication):
    def getApplicationName(self):
        return "GPU Effects Demo"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        self.window = EffectsWindow()

        def show():
            yup.Process.makeForegroundProcess()
            self.window.setVisible(True)
            self.window.centreWithSize(yup.Size[int](640, 640))

        yup.MessageManager.callAsync(show)

    def shutdown(self):
        del self.window

    def systemRequestedQuit(self):
        self.quit()


if __name__ == "__main__":
    yup.START_YUP_APPLICATION(Application)
