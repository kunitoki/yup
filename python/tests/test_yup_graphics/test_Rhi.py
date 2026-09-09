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


ALL_TEXTURE_FORMATS = [
    "r8unorm", "rg8unorm", "rgba8unorm", "rgba8snorm", "bgra8unorm",
    "rgba16float", "rg16float", "r16float",
    "rgba32float", "rg32float", "r32float",
    "rgb10a2unorm", "r11g11b10float",
    "depth16unorm", "depth24plusStencil8", "depth32float", "depth32floatStencil8",
    "bc1unorm", "bc3unorm", "bc7unorm",
    "etc2rgb8", "etc2rgba8",
    "astc4x4", "astc6x6", "astc8x8",
]


def test_gpu_texture_format_enum_is_complete():
    for name in ALL_TEXTURE_FORMATS:
        assert getattr(yup.GpuTextureFormat, name) is not None, name


@pytest.mark.parametrize("name,expected", [
    ("depth16unorm", True),
    ("depth24plusStencil8", True),
    ("depth32float", True),
    ("depth32floatStencil8", True),
    ("rgba8unorm", False),
    ("rgba16float", False),
])
def test_is_depth_stencil_format(name, expected):
    assert yup.isDepthStencilFormat(getattr(yup.GpuTextureFormat, name)) is expected


def test_gpu_buffer_type_enum():
    assert yup.GpuBufferType.vertex is not None
    assert yup.GpuBufferType.index is not None
    assert yup.GpuBufferType.uniform is not None
    assert yup.GpuBufferType.storage is not None


def test_gpu_color_write_mask_composes():
    mask = yup.GpuColorWriteMask.red | yup.GpuColorWriteMask.green
    assert (mask & yup.GpuColorWriteMask.red) == yup.GpuColorWriteMask.red
    assert (mask & yup.GpuColorWriteMask.blue) == yup.GpuColorWriteMask.none
    assert yup.GpuColorWriteMask.all != yup.GpuColorWriteMask.none


def test_gpu_filter_and_wrap_enums():
    assert yup.GpuFilter.nearest is not None
    assert yup.GpuFilter.linear is not None
    assert yup.GpuWrapMode.repeat is not None
    assert yup.GpuWrapMode.mirrorRepeat is not None
    assert yup.GpuWrapMode.clampToEdge is not None


def test_gpu_texture_type_enums():
    assert yup.GpuTextureType.texture2D is not None
    assert yup.GpuTextureType.cube is not None
    assert yup.GpuTextureType.texture3D is not None
    assert yup.GpuTextureType.array2D is not None
    assert yup.GpuTextureViewDimension.cubeArray is not None
    assert yup.GpuTextureAspect.depthOnly is not None


def test_gpu_platform_enum():
    assert yup.GpuPlatform.Headless is not None
    assert yup.GpuPlatform.OpenGL is not None
    assert yup.GpuPlatform.Metal is not None
    assert yup.GpuPlatform.Direct3D is not None
    assert yup.GpuPlatform.WebGPU is not None


# ==============================================================================
# GPU Config Structs
# ==============================================================================

def test_gpu_shader_source_is_not_exposed():
    # Its code/bindingMap/glFixup fields are borrowed views, so a settable Python
    # attribute would store a pointer into a temporary. Compile through
    # GpuPipeline.compileFromGlsl instead.
    #
    # The positive assertion is deliberate: on its own, the absence check would also
    # pass if the RHI bindings had never been registered at all.
    assert hasattr(yup, "GpuVertexBufferLayout")
    assert not hasattr(yup, "GpuShaderSource")


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
    assert len(layout.attributes) == 0


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
    assert len(opts.colorTargets) == 0
    assert len(opts.vertexBuffers) == 0
    assert opts.sampleCount == 1


def test_gpu_render_options_defaults():
    opts = yup.GpuRenderOptions()
    assert opts.clear is True
    assert opts.clearColor is not None


def test_gpu_render_options_with_args():
    opts = yup.GpuRenderOptions(True, yup.GpuColor.black())
    assert opts.clear is True
    assert opts.clearColor == yup.GpuColor.black()


def test_gpu_device_options_defaults():
    opts = yup.GpuDevice.Options()
    assert opts.retinaDisplay is True
    assert opts.readableFramebuffer is False
    assert opts.synchronousShaderCompilations is False


def test_gpu_texture_desc_defaults():
    desc = yup.GpuTextureDesc()
    assert desc.width == 0
    assert desc.height == 0
    assert desc.depthOrArrayLayers == 1
    assert desc.format == yup.GpuTextureFormat.rgba8unorm
    assert desc.type == yup.GpuTextureType.texture2D
    assert desc.renderTarget is False
    assert desc.mipLevels == 1
    assert desc.sampleCount == 1
    assert desc.label == ""


def test_gpu_texture_desc_with_args():
    desc = yup.GpuTextureDesc(64, 32, yup.GpuTextureFormat.rgba16float, True)
    assert desc.width == 64
    assert desc.height == 32
    assert desc.format == yup.GpuTextureFormat.rgba16float
    assert desc.renderTarget is True


def test_gpu_texture_desc_label_round_trips():
    # The label used to be a raw const char*, so a temporary left the descriptor
    # holding a dangling pointer. It is a String now and owns its bytes.
    desc = yup.GpuTextureDesc()
    desc.label = "my texture"
    assert desc.label == "my texture"


def test_gpu_texture_view_desc_defaults_and_equality():
    view = yup.GpuTextureViewDesc()
    assert view.dimension == yup.GpuTextureViewDimension.texture2D
    assert view.aspect == yup.GpuTextureAspect.all
    assert view.baseMipLevel == 0
    assert view.mipCount == 1
    assert view.baseLayer == 0
    assert view.layerCount == 1
    assert view == yup.GpuTextureViewDesc()
    assert not (view == yup.GpuTextureViewDesc(2, 3))


def test_gpu_texture_data_desc_defaults():
    region = yup.GpuTextureDataDesc()
    assert region.mipLevel == 0
    assert region.layer == 0
    assert region.bytesPerRow == 0
    assert region.width == 0
    assert region.height == 0
    assert region.depth == 1
    # The source pointer is deliberately not exposed.
    assert not hasattr(region, "data")


def test_gpu_sampler_desc_defaults():
    desc = yup.GpuSamplerDesc()
    assert desc.minFilter == yup.GpuFilter.nearest
    assert desc.magFilter == yup.GpuFilter.nearest
    assert desc.wrapU == yup.GpuWrapMode.clampToEdge
    assert desc.compare is None
    assert desc.minLod == 0.0
    assert desc.maxLod == 32.0
    assert desc.maxAnisotropy == 1
    assert desc.label == ""


def test_gpu_sampler_desc_with_args_and_compare():
    desc = yup.GpuSamplerDesc(yup.GpuFilter.linear, yup.GpuWrapMode.repeat)
    assert desc.minFilter == yup.GpuFilter.linear
    assert desc.magFilter == yup.GpuFilter.linear
    assert desc.wrapU == yup.GpuWrapMode.repeat
    assert desc.wrapW == yup.GpuWrapMode.repeat

    desc.compare = yup.GpuCompareFunction.lessEqual
    assert desc.compare == yup.GpuCompareFunction.lessEqual
    desc.compare = None
    assert desc.compare is None


def test_gpu_color_target_write_mask():
    ct = yup.GpuColorTarget()
    assert ct.writeMask == yup.GpuColorWriteMask.all
    ct.writeMask = yup.GpuColorWriteMask.red | yup.GpuColorWriteMask.alpha
    assert (ct.writeMask & yup.GpuColorWriteMask.green) == yup.GpuColorWriteMask.none
    assert (ct.writeMask & yup.GpuColorWriteMask.alpha) == yup.GpuColorWriteMask.alpha


def test_gpu_depth_stencil_options_defaults():
    opts = yup.GpuDepthStencilOptions()
    assert opts.depthLoadOp == yup.GpuLoadOp.clear
    assert opts.depthStoreOp == yup.GpuStoreOp.store
    assert opts.depthClearValue == 1.0
    assert opts.stencilStoreOp == yup.GpuStoreOp.discard
    assert opts.stencilClearValue == 0

    assert yup.GpuDepthStencilOptions(0.0).depthClearValue == 0.0


def test_gpu_workgroup_size():
    wgs = yup.GpuWorkgroupSize()
    assert (wgs.x, wgs.y, wgs.z) == (1, 1, 1)
    assert (yup.GpuWorkgroupSize(64, 2, 1).x) == 64


# ==============================================================================
# Owning descriptors: whole-list assignment round-trips
# ==============================================================================

def test_vertex_buffer_layout_owns_its_attributes():
    layout = yup.GpuVertexBufferLayout()
    layout.stride = 32
    layout.attributes = [
        yup.GpuVertexAttribute(yup.GpuVertexFormat.float3, 0, 0),
        yup.GpuVertexAttribute(yup.GpuVertexFormat.float2, 12, 1),
    ]

    assert len(layout.attributes) == 2
    assert layout.attributes[0].format == yup.GpuVertexFormat.float3
    assert layout.attributes[1].offset == 12


def test_vertex_buffer_layout_constructor_takes_attributes():
    layout = yup.GpuVertexBufferLayout(
        20,
        yup.GpuVertexStepMode.instance,
        [yup.GpuVertexAttribute(yup.GpuVertexFormat.float4, 0, 3)],
    )
    assert layout.stride == 20
    assert layout.stepMode == yup.GpuVertexStepMode.instance
    assert len(layout.attributes) == 1


def test_vertex_buffer_layout_attributes_are_converted_by_value():
    # stl.h converts std::vector by value, so in-place mutation of the returned
    # list does not write back. Assign the whole list instead.
    layout = yup.GpuVertexBufferLayout()
    layout.attributes.append(yup.GpuVertexAttribute())
    assert len(layout.attributes) == 0


def test_pipeline_options_owns_its_vertex_buffers():
    opts = yup.GpuPipelineOptions()
    opts.vertexBuffers = [
        yup.GpuVertexBufferLayout(
            32,
            yup.GpuVertexStepMode.vertex,
            [yup.GpuVertexAttribute(yup.GpuVertexFormat.float3, 0, 0)],
        ),
    ]

    assert len(opts.vertexBuffers) == 1
    assert opts.vertexBuffers[0].stride == 32
    assert len(opts.vertexBuffers[0].attributes) == 1


def test_pipeline_options_owns_its_color_targets():
    target = yup.GpuColorTarget()
    target.format = yup.GpuTextureFormat.bgra8unorm
    target.blendEnabled = False

    opts = yup.GpuPipelineOptions()
    opts.colorTargets = [target]

    assert len(opts.colorTargets) == 1
    assert opts.colorTargets[0].format == yup.GpuTextureFormat.bgra8unorm
    assert opts.colorTargets[0].blendEnabled is False


# ==============================================================================
# GPU device — everything below needs a real device and is skipped without one
# ==============================================================================

@pytest.fixture(scope="module")
def headless_device():
    """A headless GpuDevice, or None when this machine cannot provide one.

    GpuDevice.create returns None rather than raising when the backend is
    unavailable, which is the case on the GPU-less Linux CI runners.
    """
    return yup.GpuDevice.create(yup.GpuPlatform.Headless, yup.GpuDevice.Options())


def test_gpu_device_create_headless_returns_device_or_none(headless_device):
    assert headless_device is None or isinstance(headless_device, yup.GpuDevice)


def test_gpu_device_capability_probes_are_bound(headless_device):
    if headless_device is None:
        pytest.skip("no headless GPU device available")

    assert isinstance(headless_device.isGpuAvailable(), bool)
    assert isinstance(headless_device.isComputeAvailable(), bool)
    assert isinstance(headless_device.isAnisotropicFilteringAvailable(), bool)
    assert isinstance(
        headless_device.isFormatSupported(yup.GpuTextureFormat.rgba8unorm), bool
    )
    assert isinstance(
        headless_device.isFormatRenderable(yup.GpuTextureFormat.rgba8unorm), bool
    )
    assert headless_device.getMaximumSampleCount() >= 1
    assert headless_device.getPlatform() == yup.GpuPlatform.Headless


# ==============================================================================
# Regressions: the two paths that could not be called from Python at all
# ==============================================================================

def test_gpu_target_begin_render_pass_is_bound():
    # Was entirely unbound, so both python/demos GPU scripts died on this call.
    assert hasattr(yup.GpuTarget, "beginRenderPass")


def test_gpu_pipeline_compile_from_glsl_raises_on_bad_source(headless_device):
    # compileFromGlsl returns ResultValue<Ptr>, which has no py::class_ and used to
    # blow up on return conversion. It must raise a plain Python exception instead.
    if headless_device is None:
        pytest.skip("no headless GPU device available")

    if not hasattr(yup.GpuPipeline, "compileFromGlsl"):
        pytest.skip("shader transpiler not compiled in")

    with pytest.raises(RuntimeError):
        yup.GpuPipeline.compileFromGlsl(
            headless_device, "not glsl at all", "nor is this", yup.GpuPipelineOptions()
        )


def test_gpu_buffer_create_accepts_any_buffer_protocol_object(headless_device):
    if headless_device is None or not headless_device.isGpuAvailable():
        pytest.skip("no GPU context available")

    data = bytearray(64)
    buffer = yup.GpuBuffer.create(headless_device, yup.GpuBufferType.vertex, data)
    if buffer is not None:
        assert buffer.getSizeInBytes() == 64
        assert buffer.getType() == yup.GpuBufferType.vertex
