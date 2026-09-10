#!/usr/bin/env python3
"""
YUP GPU Cube Demo — textured, depth-tested spinning cube

The Python counterpart of the C++ SpinningCubeDemo, and the end-to-end proof
that the geometry half of the RHI is reachable from Python:

  - vertex and index buffers built from Python bytes
  - a GpuVertexBufferLayout owning its attribute list
  - a GpuTextureDesc texture filled with GpuTexture.upload()
  - a GpuSampler for filtering and wrapping
  - a depth attachment plus GpuDepthStencilState, so the far faces are hidden
  - per-frame uniforms uploaded through the buffer protocol

Note that the descriptors own their data: assign whole lists to
GpuVertexBufferLayout.attributes and GpuPipelineOptions.vertexBuffers, since
mutating the returned list in place does not write back.
"""

import math
import struct
import time

import yup_init
import yup


# ---------------------------------------------------------------------------
# GLSL 450 shaders
#
# Bindings, matching the C++ demo:
#   set=0 binding=0  CubeUniforms
#   set=0 binding=1  texture2D
#   set=0 binding=2  sampler
# ---------------------------------------------------------------------------

VERT_GLSL = """#version 450
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_uv;
layout(set = 0, binding = 0) uniform CubeUniforms {
    float angleY; float angleX; float aspect; float pad;
} u;
layout(location = 0) out vec3 v_color;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
void main() {
    float cy = cos(u.angleY), sy = sin(u.angleY);
    float cx = cos(u.angleX), sx = sin(u.angleX);
    vec3 p   = a_pos;
    vec3 ry  = vec3(p.x*cy + p.z*sy,  p.y,  -p.x*sy + p.z*cy);
    vec3 rx  = vec3(ry.x,  ry.y*cx - ry.z*sx,  ry.y*sx + ry.z*cx);
    vec3 n   = a_normal;
    vec3 ryn = vec3(n.x*cy + n.z*sy,  n.y,  -n.x*sy + n.z*cy);
    vec3 rxn = vec3(ryn.x, ryn.y*cx - ryn.z*sx, ryn.y*sx + ryn.z*cx);
    float d   = rx.z + 3.5;
    float fov = 1.7320508;
    gl_Position = vec4(rx.x * fov / u.aspect, rx.y * fov, (d - 0.1) / 99.9 * d, d);
    v_color  = a_color;
    v_normal = rxn;
    v_uv     = a_uv;
}
"""

FRAG_GLSL = """#version 450
layout(location = 0) in vec3 v_color;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(set = 0, binding = 1) uniform texture2D u_tex;
layout(set = 0, binding = 2) uniform sampler   u_samp;
layout(location = 0) out vec4 fragColor;
void main() {
    vec3  light = normalize(vec3(0.503, 0.671, -0.419));
    float ndotl = clamp(dot(normalize(v_normal), light), 0.0, 1.0);
    vec4  tex   = texture(sampler2D(u_tex, u_samp), vec2(v_uv.x, 1.0 - v_uv.y));
    vec3  base  = mix(v_color, tex.rgb, tex.a);
    fragColor   = vec4(base * (0.35 + 0.65 * ndotl), 1.0);
}
"""


# ---------------------------------------------------------------------------
# Geometry — 24 vertices (4 per face) of position, color, normal and uv
# ---------------------------------------------------------------------------

VERTEX_FLOATS = 11                       # 3 pos + 3 color + 3 normal + 2 uv
VERTEX_STRIDE = VERTEX_FLOATS * 4        # bytes

# (normal, color) per face, in the order -Z, +Z, -X, +X, +Y, -Y
FACES = [
    ((0.0, 0.0, -1.0), (1.0, 0.25, 0.25)),
    ((0.0, 0.0, 1.0), (0.25, 1.0, 0.35)),
    ((-1.0, 0.0, 0.0), (0.3, 0.45, 1.0)),
    ((1.0, 0.0, 0.0), (1.0, 0.85, 0.2)),
    ((0.0, 1.0, 0.0), (0.95, 0.4, 0.95)),
    ((0.0, -1.0, 0.0), (0.25, 0.9, 0.95)),
]

# The four corners of each face, counter-clockwise seen from outside.
FACE_CORNERS = [
    [(-1, -1, -1), (-1, 1, -1), (1, 1, -1), (1, -1, -1)],   # -Z
    [(-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)],       # +Z
    [(-1, -1, -1), (-1, -1, 1), (-1, 1, 1), (-1, 1, -1)],   # -X
    [(1, -1, -1), (1, 1, -1), (1, 1, 1), (1, -1, 1)],       # +X
    [(-1, 1, -1), (-1, 1, 1), (1, 1, 1), (1, 1, -1)],       # +Y
    [(-1, -1, -1), (1, -1, -1), (1, -1, 1), (-1, -1, 1)],   # -Y
]

FACE_UVS = [(0.0, 0.0), (0.0, 1.0), (1.0, 1.0), (1.0, 0.0)]


def build_cube_vertices() -> bytes:
    values = []
    for (normal, color), corners in zip(FACES, FACE_CORNERS):
        for corner, uv in zip(corners, FACE_UVS):
            values.extend(corner)
            values.extend(color)
            values.extend(normal)
            values.extend(uv)
    return struct.pack(f"<{len(values)}f", *values)


def build_cube_indices() -> bytes:
    indices = []
    for face in range(6):
        base = face * 4
        indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])
    return struct.pack(f"<{len(indices)}H", *indices)


CHECKER_SIZE = 64


def build_checker_texture() -> bytes:
    """A translucent checkerboard, so the per-vertex face color shows through."""
    pixels = bytearray(CHECKER_SIZE * CHECKER_SIZE * 4)
    for y in range(CHECKER_SIZE):
        for x in range(CHECKER_SIZE):
            light = ((x >> 3) + (y >> 3)) & 1
            value = 235 if light else 40
            offset = (y * CHECKER_SIZE + x) * 4
            pixels[offset + 0] = value
            pixels[offset + 1] = value
            pixels[offset + 2] = value
            pixels[offset + 3] = 160
    return bytes(pixels)


# ---------------------------------------------------------------------------
class CubeWindow(yup.DocumentWindow):
    def __init__(self):
        super().__init__()
        self.setTitle("GPU Spinning Cube")
        self.component = CubeComponent()
        self.addAndMakeVisible(self.component)

    def resized(self):
        self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class CubeComponent(yup.Component):
    TARGET_SIZE = 512

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        self._ctx = None
        self._device = None
        self._pipeline = None
        self._target = None
        self._depthTexture = None
        self._vertexBuffer = None
        self._indexBuffer = None
        self._texture = None
        self._sampler = None
        self._gpuTexture = None
        self._indexCount = 0
        self._initOk = False
        self._didInit = False
        self._failure = None
        self._startTime = time.perf_counter()

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        # The per-frame hook, called on the render thread immediately before
        # painting. Driving repaints from a yup.Timer instead would call
        # repaint() from the message thread while the render thread is inside
        # paint(), which trips Component's isRepainting assertion.
        self.repaint()

    # ------------------------------------------------------------------
    @staticmethod
    def _pipelineOptions():
        options = yup.GpuPipelineOptions()

        # Whole-list assignment: the layout owns its attributes now.
        options.vertexBuffers = [
            yup.GpuVertexBufferLayout(
                VERTEX_STRIDE,
                yup.GpuVertexStepMode.vertex,
                [
                    yup.GpuVertexAttribute(yup.GpuVertexFormat.float3, 0, 0),   # a_pos
                    yup.GpuVertexAttribute(yup.GpuVertexFormat.float3, 12, 1),  # a_color
                    yup.GpuVertexAttribute(yup.GpuVertexFormat.float3, 24, 2),  # a_normal
                    yup.GpuVertexAttribute(yup.GpuVertexFormat.float2, 36, 3),  # a_uv
                ],
            ),
        ]

        options.topology = yup.GpuPrimitiveTopology.triangleList
        options.indexFormat = yup.GpuIndexFormat.uint16
        options.cullMode = yup.GpuCullMode.back
        options.winding = yup.GpuFaceWinding.counterClockwise

        target = yup.GpuColorTarget()
        target.format = yup.GpuTextureFormat.rgba8unorm
        target.blendEnabled = False
        options.colorTargets = [target]

        options.depthStencil.enabled = True
        options.depthStencil.format = yup.GpuTextureFormat.depth24plusStencil8
        options.depthStencil.depthCompare = yup.GpuCompareFunction.less
        options.depthStencil.depthWriteEnabled = True
        return options

    def _ensureInit(self):
        if self._didInit or self._device is None:
            return
        self._didInit = True

        try:
            self._pipeline = yup.GpuPipeline.compileFromGlsl(
                self._device, VERT_GLSL, FRAG_GLSL, self._pipelineOptions(),
            )
        except RuntimeError as error:
            self._failure = f"Pipeline compilation failed: {error}"
            return

        vertexData = build_cube_vertices()
        indexData = build_cube_indices()
        self._indexCount = len(indexData) // 2

        self._vertexBuffer = yup.GpuBuffer.create(
            self._device, yup.GpuBufferType.vertex, vertexData)
        self._indexBuffer = yup.GpuBuffer.create(
            self._device, yup.GpuBufferType.index, indexData)

        self._target = yup.GpuTarget.create(self._device, self.TARGET_SIZE, self.TARGET_SIZE)

        depthDesc = yup.GpuTextureDesc(
            self.TARGET_SIZE, self.TARGET_SIZE,
            yup.GpuTextureFormat.depth24plusStencil8, True)
        depthDesc.label = "gpu_cube depth"
        self._depthTexture = yup.GpuTexture.create(self._device, depthDesc)

        textureDesc = yup.GpuTextureDesc(
            CHECKER_SIZE, CHECKER_SIZE, yup.GpuTextureFormat.rgba8unorm)
        textureDesc.label = "gpu_cube checker"
        self._texture = yup.GpuTexture.create(self._device, textureDesc)

        if self._texture is not None:
            region = yup.GpuTextureDataDesc()
            region.width = CHECKER_SIZE
            region.height = CHECKER_SIZE
            self._texture.upload(build_checker_texture(), region)

        samplerDesc = yup.GpuSamplerDesc(
            yup.GpuFilter.linear, yup.GpuWrapMode.clampToEdge)
        samplerDesc.label = "gpu_cube bilinear"
        self._sampler = yup.GpuSampler.create(self._device, samplerDesc)

        missing = [
            name for name, value in (
                ("vertex buffer", self._vertexBuffer),
                ("index buffer", self._indexBuffer),
                ("render target", self._target),
                ("depth texture", self._depthTexture),
                ("texture", self._texture),
                ("sampler", self._sampler),
            ) if value is None
        ]

        if missing:
            self._failure = "Could not create: " + ", ".join(missing)
            return

        self._initOk = True

    # ------------------------------------------------------------------
    def _render(self):
        if not self._initOk:
            return

        elapsed = time.perf_counter() - self._startTime
        uniforms = struct.pack(
            "<4f",
            elapsed * 0.9,                       # angleY
            math.sin(elapsed * 0.6) * 0.55,      # angleX
            1.0,                                 # aspect (square target)
            0.0,                                 # pad
        )

        with yup.GpuFrame.begin(self._device) as frame:
            options = yup.GpuRenderOptions(True, yup.GpuColor(0.06, 0.07, 0.10, 1.0))
            with self._target.beginRenderPass(frame, options) as rp:
                rp.setDepthStencilAttachment(self._depthTexture,
                                             yup.GpuDepthStencilOptions(1.0))
                rp.setPipeline(self._pipeline)
                rp.setUniformBuffer(0, 0, uniforms)
                rp.setTexture(0, 1, self._texture)
                rp.setSampler(0, 2, self._sampler)
                rp.setVertexBuffer(0, self._vertexBuffer)
                rp.setIndexBuffer(yup.GpuIndexFormat.uint16, self._indexBuffer)
                rp.drawIndexed(self._indexCount)

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

        w = float(self.getWidth())
        h = float(self.getHeight())

        if not self._initOk:
            font = yup.ApplicationTheme.getGlobalTheme().getDefaultFont().withHeight(16.0)
            g.setFillColor(yup.Colors.orange)
            g.fillFittedText(
                self._failure or "GPU initialisation failed", font,
                yup.Rectangle[float](10, h / 2 - 30, w - 20, 60),
                yup.Justification.center,
            )
            return

        self._render()

        tex = self._gpuTexture
        if tex and tex.isValid():
            tw = float(tex.getWidth())
            th = float(tex.getHeight())
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
            f"GPU: {api}  |  {self._indexCount} indices  |  depth-tested, textured",
            font,
            yup.Rectangle[float](10, h - 25, 450, 20),
            yup.Justification.left,
        )


class Application(yup.YUPApplication):
    def getApplicationName(self):
        return "GPU Spinning Cube"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        self.window = CubeWindow()

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
