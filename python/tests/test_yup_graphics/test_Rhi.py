import pytest
import yup


# ==============================================================================
# GPU Enums
# ==============================================================================

def test_gpu_shader_language_enum():
    assert yup.GpuShaderLanguage.wgsl is not None
    assert yup.GpuShaderLanguage.glsl is not None
    assert yup.GpuShaderLanguage.msl is not None
    assert yup.GpuShaderLanguage.hlsl is not None


def test_gpu_vertex_format_enum():
    assert yup.GpuVertexFormat.float1 is not None
    assert yup.GpuVertexFormat.float2 is not None
    assert yup.GpuVertexFormat.float3 is not None
    assert yup.GpuVertexFormat.float4 is not None


def test_gpu_vertex_step_mode_enum():
    assert yup.GpuVertexStepMode.vertex is not None
    assert yup.GpuVertexStepMode.instance is not None


def test_gpu_primitive_topology_enum():
    assert yup.GpuPrimitiveTopology.pointList is not None
    assert yup.GpuPrimitiveTopology.lineList is not None
    assert yup.GpuPrimitiveTopology.triangleList is not None
    assert yup.GpuPrimitiveTopology.triangleStrip is not None


def test_gpu_index_format_enum():
    assert yup.GpuIndexFormat.none is not None
    assert yup.GpuIndexFormat.uint16 is not None
    assert yup.GpuIndexFormat.uint32 is not None


def test_gpu_cull_mode_enum():
    assert yup.GpuCullMode.none is not None
    assert yup.GpuCullMode.front is not None
    assert yup.GpuCullMode.back is not None


def test_gpu_face_winding_enum():
    assert yup.GpuFaceWinding.clockwise is not None
    assert yup.GpuFaceWinding.counterClockwise is not None


def test_gpu_compare_function_enum():
    assert yup.GpuCompareFunction.never is not None
    assert yup.GpuCompareFunction.less is not None
    assert yup.GpuCompareFunction.equal is not None
    assert yup.GpuCompareFunction.always is not None


def test_gpu_stencil_op_enum():
    assert yup.GpuStencilOp.keep is not None
    assert yup.GpuStencilOp.zero is not None
    assert yup.GpuStencilOp.replace is not None


def test_gpu_blend_factor_enum():
    assert yup.GpuBlendFactor.zero is not None
    assert yup.GpuBlendFactor.one is not None
    assert yup.GpuBlendFactor.srcAlpha is not None
    assert yup.GpuBlendFactor.oneMinusSrcAlpha is not None


def test_gpu_blend_op_enum():
    assert yup.GpuBlendOp.add is not None
    assert yup.GpuBlendOp.subtract is not None
    assert yup.GpuBlendOp.min is not None
    assert yup.GpuBlendOp.max is not None


def test_gpu_texture_format_enum():
    assert yup.GpuTextureFormat.rgba8unorm is not None
    assert yup.GpuTextureFormat.bgra8unorm is not None
    assert yup.GpuTextureFormat.rgba16float is not None


def test_gpu_buffer_type_enum():
    assert yup.GpuBufferType.vertex is not None
    assert yup.GpuBufferType.index is not None
    assert yup.GpuBufferType.uniform is not None


def test_graphics_api_enum():
    assert yup.GraphicsApi.Headless is not None
    assert yup.GraphicsApi.OpenGL is not None
    assert yup.GraphicsApi.Metal is not None
    assert yup.GraphicsApi.Direct3D is not None
    assert yup.GraphicsApi.WebGPU is not None


# ==============================================================================
# GPU Config Structs
# ==============================================================================

def test_gpu_shader_source_defaults():
    src = yup.GpuShaderSource()
    assert src.language == yup.GpuShaderLanguage.wgsl
    assert src.entryPoint is None


def test_gpu_shader_source_set_fields():
    src = yup.GpuShaderSource()
    src.language = yup.GpuShaderLanguage.glsl
    src.codeSize = 1024
    assert src.language == yup.GpuShaderLanguage.glsl
    assert src.codeSize == 1024


def test_gpu_vertex_attribute_construction():
    attr = yup.GpuVertexAttribute()
    assert attr.format == yup.GpuVertexFormat.float4
    assert attr.offset == 0
    assert attr.shaderLocation == 0


def test_gpu_vertex_attribute_with_args():
    attr = yup.GpuVertexAttribute(
        yup.GpuVertexFormat.float3, 12, 0
    )
    assert attr.format == yup.GpuVertexFormat.float3
    assert attr.offset == 12
    assert attr.shaderLocation == 0


def test_gpu_vertex_buffer_layout_defaults():
    layout = yup.GpuVertexBufferLayout()
    assert layout.stride == 0
    assert layout.stepMode == yup.GpuVertexStepMode.vertex
    assert layout.attributeCount == 0


def test_gpu_blend_state_defaults():
    bs = yup.GpuBlendState()
    assert bs.srcColor == yup.GpuBlendFactor.srcAlpha
    assert bs.dstColor == yup.GpuBlendFactor.oneMinusSrcAlpha
    assert bs.colorOp == yup.GpuBlendOp.add


def test_gpu_color_target_defaults():
    ct = yup.GpuColorTarget()
    assert ct.format == yup.GpuTextureFormat.rgba8unorm
    assert ct.blendEnabled is True


def test_gpu_stencil_face_state_defaults():
    sfs = yup.GpuStencilFaceState()
    assert sfs.compare == yup.GpuCompareFunction.always
    assert sfs.failOp == yup.GpuStencilOp.keep
    assert sfs.depthFailOp == yup.GpuStencilOp.keep
    assert sfs.passOp == yup.GpuStencilOp.keep


def test_gpu_depth_stencil_state_defaults():
    dss = yup.GpuDepthStencilState()
    assert dss.enabled is False
    assert dss.depthWriteEnabled is True
    assert dss.depthCompare == yup.GpuCompareFunction.less


def test_gpu_pipeline_options_defaults():
    opts = yup.GpuPipelineOptions()
    assert opts.topology == yup.GpuPrimitiveTopology.triangleList
    assert opts.indexFormat == yup.GpuIndexFormat.none
    assert opts.cullMode == yup.GpuCullMode.none
    assert opts.colorTargetCount == 0
    assert opts.sampleCount == 1


def test_gpu_render_options_defaults():
    opts = yup.GpuRenderOptions()
    assert opts.clear is True
    assert opts.clearColor is not None


def test_gpu_render_options_with_args():
    opts = yup.GpuRenderOptions(True, yup.Colors.black)
    assert opts.clear is True
    assert opts.clearColor == yup.Colors.black


def test_graphics_context_options_defaults():
    opts = yup.GraphicsContextOptions()
    assert opts.retinaDisplay is True
    assert opts.readableFramebuffer is False
    assert opts.synchronousShaderCompilations is False
