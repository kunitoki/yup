/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "yup_YupRhi_bindings.h"

#include "../utilities/yup_PythonInterop.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

namespace
{

/** Total byte count of a buffer protocol view.

    buffer_info::size counts items, not bytes, so a numpy float32 array of 16
    elements reports 16 here and 64 there - only the latter is what the RHI wants.
*/
size_t byteSizeOf (const py::buffer_info& info) noexcept
{
    return static_cast<size_t> (info.size) * static_cast<size_t> (info.itemsize);
}

/** Unwraps a ResultValue, raising the failure message as a Python exception.

    ResultValue has no py::class_ (and should not get one), so every RHI factory
    returning one has to be unwrapped at the binding boundary.
*/
template <class T>
T unwrapOrRaise (ResultValue<T> result)
{
    if (result.failed())
        py::pybind11_fail (result.getErrorMessage().toRawUTF8());

    return result.getValue();
}

} // namespace

void registerYupRhiBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ GPU enums

    py::enum_<GpuPlatform> (m, "GpuPlatform")
        .value ("Headless", GpuPlatform::Headless)
        .value ("OpenGL", GpuPlatform::OpenGL)
        .value ("OpenGLES", GpuPlatform::OpenGLES)
        .value ("Direct3D", GpuPlatform::Direct3D)
        .value ("Metal", GpuPlatform::Metal)
        .value ("WebGPU", GpuPlatform::WebGPU);

    py::enum_<GpuShaderLanguage> (m, "GpuShaderLanguage")
        .value ("wgsl", GpuShaderLanguage::wgsl)
        .value ("glsl", GpuShaderLanguage::glsl)
        .value ("msl", GpuShaderLanguage::msl)
        .value ("hlsl", GpuShaderLanguage::hlsl);

    py::enum_<GpuVertexFormat> (m, "GpuVertexFormat")
        .value ("float1", GpuVertexFormat::float1)
        .value ("float2", GpuVertexFormat::float2)
        .value ("float3", GpuVertexFormat::float3)
        .value ("float4", GpuVertexFormat::float4)
        .value ("uint8x4", GpuVertexFormat::uint8x4)
        .value ("sint8x4", GpuVertexFormat::sint8x4)
        .value ("snorm8x4", GpuVertexFormat::snorm8x4)
        .value ("unorm8x4", GpuVertexFormat::unorm8x4)
        .value ("uint16x2", GpuVertexFormat::uint16x2)
        .value ("sint16x2", GpuVertexFormat::sint16x2)
        .value ("unorm16x2", GpuVertexFormat::unorm16x2)
        .value ("snorm16x2", GpuVertexFormat::snorm16x2)
        .value ("uint16x4", GpuVertexFormat::uint16x4)
        .value ("sint16x4", GpuVertexFormat::sint16x4)
        .value ("float16x2", GpuVertexFormat::float16x2)
        .value ("float16x4", GpuVertexFormat::float16x4)
        .value ("uint32", GpuVertexFormat::uint32);

    py::enum_<GpuVertexStepMode> (m, "GpuVertexStepMode")
        .value ("vertex", GpuVertexStepMode::vertex)
        .value ("instance", GpuVertexStepMode::instance);

    py::enum_<GpuPrimitiveTopology> (m, "GpuPrimitiveTopology")
        .value ("pointList", GpuPrimitiveTopology::pointList)
        .value ("lineList", GpuPrimitiveTopology::lineList)
        .value ("lineStrip", GpuPrimitiveTopology::lineStrip)
        .value ("triangleList", GpuPrimitiveTopology::triangleList)
        .value ("triangleStrip", GpuPrimitiveTopology::triangleStrip);

    py::enum_<GpuIndexFormat> (m, "GpuIndexFormat")
        .value ("none", GpuIndexFormat::none)
        .value ("uint16", GpuIndexFormat::uint16)
        .value ("uint32", GpuIndexFormat::uint32);

    py::enum_<GpuCullMode> (m, "GpuCullMode")
        .value ("none", GpuCullMode::none)
        .value ("front", GpuCullMode::front)
        .value ("back", GpuCullMode::back);

    py::enum_<GpuFaceWinding> (m, "GpuFaceWinding")
        .value ("clockwise", GpuFaceWinding::clockwise)
        .value ("counterClockwise", GpuFaceWinding::counterClockwise);

    py::enum_<GpuCompareFunction> (m, "GpuCompareFunction")
        .value ("never", GpuCompareFunction::never)
        .value ("less", GpuCompareFunction::less)
        .value ("equal", GpuCompareFunction::equal)
        .value ("lessEqual", GpuCompareFunction::lessEqual)
        .value ("greater", GpuCompareFunction::greater)
        .value ("notEqual", GpuCompareFunction::notEqual)
        .value ("greaterEqual", GpuCompareFunction::greaterEqual)
        .value ("always", GpuCompareFunction::always);

    py::enum_<GpuStencilOp> (m, "GpuStencilOp")
        .value ("keep", GpuStencilOp::keep)
        .value ("zero", GpuStencilOp::zero)
        .value ("replace", GpuStencilOp::replace)
        .value ("incrementClamp", GpuStencilOp::incrementClamp)
        .value ("decrementClamp", GpuStencilOp::decrementClamp)
        .value ("invert", GpuStencilOp::invert)
        .value ("incrementWrap", GpuStencilOp::incrementWrap)
        .value ("decrementWrap", GpuStencilOp::decrementWrap);

    py::enum_<GpuBlendFactor> (m, "GpuBlendFactor")
        .value ("zero", GpuBlendFactor::zero)
        .value ("one", GpuBlendFactor::one)
        .value ("srcColor", GpuBlendFactor::srcColor)
        .value ("oneMinusSrcColor", GpuBlendFactor::oneMinusSrcColor)
        .value ("srcAlpha", GpuBlendFactor::srcAlpha)
        .value ("oneMinusSrcAlpha", GpuBlendFactor::oneMinusSrcAlpha)
        .value ("dstColor", GpuBlendFactor::dstColor)
        .value ("oneMinusDstColor", GpuBlendFactor::oneMinusDstColor)
        .value ("dstAlpha", GpuBlendFactor::dstAlpha)
        .value ("oneMinusDstAlpha", GpuBlendFactor::oneMinusDstAlpha)
        .value ("srcAlphaSaturated", GpuBlendFactor::srcAlphaSaturated)
        .value ("blendColor", GpuBlendFactor::blendColor)
        .value ("oneMinusBlendColor", GpuBlendFactor::oneMinusBlendColor);

    py::enum_<GpuBlendOp> (m, "GpuBlendOp")
        .value ("add", GpuBlendOp::add)
        .value ("subtract", GpuBlendOp::subtract)
        .value ("reverseSubtract", GpuBlendOp::reverseSubtract)
        .value ("min", GpuBlendOp::min)
        .value ("max", GpuBlendOp::max);

    py::enum_<GpuTextureFormat> (m, "GpuTextureFormat")
        .value ("r8unorm", GpuTextureFormat::r8unorm)
        .value ("rg8unorm", GpuTextureFormat::rg8unorm)
        .value ("rgba8unorm", GpuTextureFormat::rgba8unorm)
        .value ("rgba8snorm", GpuTextureFormat::rgba8snorm)
        .value ("bgra8unorm", GpuTextureFormat::bgra8unorm)
        .value ("rgba16float", GpuTextureFormat::rgba16float)
        .value ("rg16float", GpuTextureFormat::rg16float)
        .value ("r16float", GpuTextureFormat::r16float)
        .value ("rgba32float", GpuTextureFormat::rgba32float)
        .value ("rg32float", GpuTextureFormat::rg32float)
        .value ("r32float", GpuTextureFormat::r32float)
        .value ("rgb10a2unorm", GpuTextureFormat::rgb10a2unorm)
        .value ("r11g11b10float", GpuTextureFormat::r11g11b10float)
        .value ("depth16unorm", GpuTextureFormat::depth16unorm)
        .value ("depth24plusStencil8", GpuTextureFormat::depth24plusStencil8)
        .value ("depth32float", GpuTextureFormat::depth32float)
        .value ("depth32floatStencil8", GpuTextureFormat::depth32floatStencil8)
        .value ("bc1unorm", GpuTextureFormat::bc1unorm)
        .value ("bc3unorm", GpuTextureFormat::bc3unorm)
        .value ("bc7unorm", GpuTextureFormat::bc7unorm)
        .value ("etc2rgb8", GpuTextureFormat::etc2rgb8)
        .value ("etc2rgba8", GpuTextureFormat::etc2rgba8)
        .value ("astc4x4", GpuTextureFormat::astc4x4)
        .value ("astc6x6", GpuTextureFormat::astc6x6)
        .value ("astc8x8", GpuTextureFormat::astc8x8);

    m.def ("isDepthStencilFormat", &isDepthStencilFormat, "format"_a,
           "Returns True if the given format is a depth and/or stencil format.");

    // Bit flags. Deliberately not py::arithmetic(): that registers int-returning
    // __or__ / __and__ in the enum_ constructor, which would shadow these and make
    // composing two masks yield an int instead of a mask.
    py::enum_<GpuColorWriteMask> (m, "GpuColorWriteMask")
        .value ("none", GpuColorWriteMask::none)
        .value ("red", GpuColorWriteMask::red)
        .value ("green", GpuColorWriteMask::green)
        .value ("blue", GpuColorWriteMask::blue)
        .value ("alpha", GpuColorWriteMask::alpha)
        .value ("all", GpuColorWriteMask::all)
        .def ("__or__", [] (GpuColorWriteMask a, GpuColorWriteMask b) { return a | b; })
        .def ("__and__", [] (GpuColorWriteMask a, GpuColorWriteMask b) { return a & b; });

    py::enum_<GpuTextureType> (m, "GpuTextureType")
        .value ("texture2D", GpuTextureType::texture2D)
        .value ("cube", GpuTextureType::cube)
        .value ("texture3D", GpuTextureType::texture3D)
        .value ("array2D", GpuTextureType::array2D);

    py::enum_<GpuTextureViewDimension> (m, "GpuTextureViewDimension")
        .value ("texture2D", GpuTextureViewDimension::texture2D)
        .value ("cube", GpuTextureViewDimension::cube)
        .value ("texture3D", GpuTextureViewDimension::texture3D)
        .value ("array2D", GpuTextureViewDimension::array2D)
        .value ("cubeArray", GpuTextureViewDimension::cubeArray);

    py::enum_<GpuTextureAspect> (m, "GpuTextureAspect")
        .value ("all", GpuTextureAspect::all)
        .value ("depthOnly", GpuTextureAspect::depthOnly)
        .value ("stencilOnly", GpuTextureAspect::stencilOnly);

    py::enum_<GpuFilter> (m, "GpuFilter")
        .value ("nearest", GpuFilter::nearest)
        .value ("linear", GpuFilter::linear);

    py::enum_<GpuWrapMode> (m, "GpuWrapMode")
        .value ("repeat", GpuWrapMode::repeat)
        .value ("mirrorRepeat", GpuWrapMode::mirrorRepeat)
        .value ("clampToEdge", GpuWrapMode::clampToEdge);

    py::enum_<GpuBufferType> (m, "GpuBufferType")
        .value ("vertex", GpuBufferType::vertex)
        .value ("index", GpuBufferType::index)
        .value ("uniform", GpuBufferType::uniform)
        .value ("storage", GpuBufferType::storage);

    py::enum_<GpuLoadOp> (m, "GpuLoadOp")
        .value ("clear", GpuLoadOp::clear)
        .value ("load", GpuLoadOp::load)
        .value ("dontCare", GpuLoadOp::dontCare);

    py::enum_<GpuStoreOp> (m, "GpuStoreOp")
        .value ("store", GpuStoreOp::store)
        .value ("discard", GpuStoreOp::discard);

    py::enum_<GpuDitherMode> (m, "GpuDitherMode")
        .value ("none", GpuDitherMode::none)
        .value ("interleavedGradientNoise", GpuDitherMode::interleavedGradientNoise);

    // ============================================================================================ GPU config structs

    py::class_<GpuColor> (m, "GpuColor")
        .def (py::init<>())
        .def (py::init<float, float, float, float>(), "red"_a, "green"_a, "blue"_a, "alpha"_a = 1.0f)
#if YUP_MODULE_AVAILABLE_yup_graphics
        .def (py::init<const Color&>())
#endif
        .def_readwrite ("red", &GpuColor::red)
        .def_readwrite ("green", &GpuColor::green)
        .def_readwrite ("blue", &GpuColor::blue)
        .def_readwrite ("alpha", &GpuColor::alpha)
        .def_static ("black", &GpuColor::black)
        .def_static ("white", &GpuColor::white)
        .def_static ("transparentBlack", &GpuColor::transparentBlack)
        .def ("__eq__", [] (const GpuColor& self, const GpuColor& other)
        {
            return self.red == other.red
                && self.green == other.green
                && self.blue == other.blue
                && self.alpha == other.alpha;
        })
#if YUP_MODULE_AVAILABLE_yup_graphics
        .def ("__eq__", [] (const GpuColor& self, const Color& other)
        {
            return self.red == other.getRedFloat()
                && self.green == other.getGreenFloat()
                && self.blue == other.getBlueFloat()
                && self.alpha == other.getAlphaFloat();
        })
#endif
        .def ("__repr__", [] (const GpuColor& self)
        {
            String repr;
            repr
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << self.red << ", " << self.green << ", " << self.blue << ", " << self.alpha << ")";
            return repr;
        });

#if YUP_MODULE_AVAILABLE_yup_graphics
    py::implicitly_convertible<Color, GpuColor>();
#endif

    py::class_<GpuTextureDesc> (m, "GpuTextureDesc")
        .def (py::init<>())
        .def (py::init<uint32_t, uint32_t, GpuTextureFormat, bool>(),
              "width"_a, "height"_a, "format"_a, "renderTarget"_a = false)
        .def_readwrite ("width", &GpuTextureDesc::width)
        .def_readwrite ("height", &GpuTextureDesc::height)
        .def_readwrite ("depthOrArrayLayers", &GpuTextureDesc::depthOrArrayLayers)
        .def_readwrite ("format", &GpuTextureDesc::format)
        .def_readwrite ("type", &GpuTextureDesc::type)
        .def_readwrite ("renderTarget", &GpuTextureDesc::renderTarget)
        .def_readwrite ("mipLevels", &GpuTextureDesc::mipLevels)
        .def_readwrite ("sampleCount", &GpuTextureDesc::sampleCount)
        .def_readwrite ("label", &GpuTextureDesc::label)
        .def ("__repr__", [] (const GpuTextureDesc& self)
        {
            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << (int) self.width << ", " << (int) self.height << ")";
            return result;
        });

    // The source pointer is deliberately not exposed: it is a borrowed view, and a
    // settable Python attribute would capture a pointer into a temporary. Pass the
    // pixels to GpuTexture.upload() instead and use this purely to place them.
    py::class_<GpuTextureDataDesc> (m, "GpuTextureDataDesc")
        .def (py::init<>())
        .def_readwrite ("bytesPerRow", &GpuTextureDataDesc::bytesPerRow)
        .def_readwrite ("rowsPerImage", &GpuTextureDataDesc::rowsPerImage)
        .def_readwrite ("mipLevel", &GpuTextureDataDesc::mipLevel)
        .def_readwrite ("layer", &GpuTextureDataDesc::layer)
        .def_readwrite ("x", &GpuTextureDataDesc::x)
        .def_readwrite ("y", &GpuTextureDataDesc::y)
        .def_readwrite ("z", &GpuTextureDataDesc::z)
        .def_readwrite ("width", &GpuTextureDataDesc::width)
        .def_readwrite ("height", &GpuTextureDataDesc::height)
        .def_readwrite ("depth", &GpuTextureDataDesc::depth);

    py::class_<GpuTextureViewDesc> (m, "GpuTextureViewDesc")
        .def (py::init<>())
        .def (py::init<uint32_t, uint32_t>(), "baseMipLevel"_a, "baseLayer"_a)
        .def_readwrite ("dimension", &GpuTextureViewDesc::dimension)
        .def_readwrite ("aspect", &GpuTextureViewDesc::aspect)
        .def_readwrite ("baseMipLevel", &GpuTextureViewDesc::baseMipLevel)
        .def_readwrite ("mipCount", &GpuTextureViewDesc::mipCount)
        .def_readwrite ("baseLayer", &GpuTextureViewDesc::baseLayer)
        .def_readwrite ("layerCount", &GpuTextureViewDesc::layerCount)
        .def ("__eq__", [] (const GpuTextureViewDesc& self, const GpuTextureViewDesc& other)
        {
            return self == other;
        });

    py::class_<GpuSamplerDesc> (m, "GpuSamplerDesc")
        .def (py::init<>())
        .def (py::init<GpuFilter, GpuWrapMode>(), "filter"_a, "wrap"_a)
        .def_readwrite ("minFilter", &GpuSamplerDesc::minFilter)
        .def_readwrite ("magFilter", &GpuSamplerDesc::magFilter)
        .def_readwrite ("mipmapFilter", &GpuSamplerDesc::mipmapFilter)
        .def_readwrite ("wrapU", &GpuSamplerDesc::wrapU)
        .def_readwrite ("wrapV", &GpuSamplerDesc::wrapV)
        .def_readwrite ("wrapW", &GpuSamplerDesc::wrapW)
        .def_readwrite ("compare", &GpuSamplerDesc::compare)
        .def_readwrite ("minLod", &GpuSamplerDesc::minLod)
        .def_readwrite ("maxLod", &GpuSamplerDesc::maxLod)
        .def_readwrite ("maxAnisotropy", &GpuSamplerDesc::maxAnisotropy)
        .def_readwrite ("label", &GpuSamplerDesc::label);

    py::class_<GpuVertexAttribute> (m, "GpuVertexAttribute")
        .def (py::init<>())
        .def (py::init<GpuVertexFormat, uint32_t, uint32_t>(), "format"_a, "offset"_a, "shaderLocation"_a)
        .def_readwrite ("format", &GpuVertexAttribute::format)
        .def_readwrite ("offset", &GpuVertexAttribute::offset)
        .def_readwrite ("shaderLocation", &GpuVertexAttribute::shaderLocation);

    py::class_<GpuVertexBufferLayout> (m, "GpuVertexBufferLayout")
        .def (py::init<>())
        .def (py::init<uint32_t, GpuVertexStepMode, std::vector<GpuVertexAttribute>>(),
              "stride"_a, "stepMode"_a, "attributes"_a)
        .def_readwrite ("stride", &GpuVertexBufferLayout::stride)
        .def_readwrite ("stepMode", &GpuVertexBufferLayout::stepMode)
        // Converted by value: assign a whole list (layout.attributes = [...]).
        // Mutating the returned list in place does not write back.
        .def_readwrite ("attributes", &GpuVertexBufferLayout::attributes);

    py::class_<GpuBlendState> (m, "GpuBlendState")
        .def (py::init<>())
        .def_readwrite ("srcColor", &GpuBlendState::srcColor)
        .def_readwrite ("dstColor", &GpuBlendState::dstColor)
        .def_readwrite ("colorOp", &GpuBlendState::colorOp)
        .def_readwrite ("srcAlpha", &GpuBlendState::srcAlpha)
        .def_readwrite ("dstAlpha", &GpuBlendState::dstAlpha)
        .def_readwrite ("alphaOp", &GpuBlendState::alphaOp);

    py::class_<GpuColorTarget> (m, "GpuColorTarget")
        .def (py::init<>())
        .def_readwrite ("format", &GpuColorTarget::format)
        .def_readwrite ("blendEnabled", &GpuColorTarget::blendEnabled)
        .def_readwrite ("blend", &GpuColorTarget::blend)
        .def_readwrite ("writeMask", &GpuColorTarget::writeMask);

    py::class_<GpuStencilFaceState> (m, "GpuStencilFaceState")
        .def (py::init<>())
        .def_readwrite ("compare", &GpuStencilFaceState::compare)
        .def_readwrite ("failOp", &GpuStencilFaceState::failOp)
        .def_readwrite ("depthFailOp", &GpuStencilFaceState::depthFailOp)
        .def_readwrite ("passOp", &GpuStencilFaceState::passOp);

    py::class_<GpuDepthStencilState> (m, "GpuDepthStencilState")
        .def (py::init<>())
        .def_readwrite ("enabled", &GpuDepthStencilState::enabled)
        .def_readwrite ("format", &GpuDepthStencilState::format)
        .def_readwrite ("depthCompare", &GpuDepthStencilState::depthCompare)
        .def_readwrite ("depthWriteEnabled", &GpuDepthStencilState::depthWriteEnabled);

    py::class_<GpuPipelineOptions> (m, "GpuPipelineOptions")
        .def (py::init<>())
        .def_readwrite ("vertexBuffers", &GpuPipelineOptions::vertexBuffers)
        .def_readwrite ("topology", &GpuPipelineOptions::topology)
        .def_readwrite ("indexFormat", &GpuPipelineOptions::indexFormat)
        .def_readwrite ("cullMode", &GpuPipelineOptions::cullMode)
        .def_readwrite ("winding", &GpuPipelineOptions::winding)
        .def_readwrite ("colorTargets", &GpuPipelineOptions::colorTargets)
        .def_readwrite ("depthStencil", &GpuPipelineOptions::depthStencil)
        .def_readwrite ("stencilFront", &GpuPipelineOptions::stencilFront)
        .def_readwrite ("stencilBack", &GpuPipelineOptions::stencilBack)
        .def_readwrite ("stencilReadMask", &GpuPipelineOptions::stencilReadMask)
        .def_readwrite ("stencilWriteMask", &GpuPipelineOptions::stencilWriteMask)
        .def_readwrite ("sampleCount", &GpuPipelineOptions::sampleCount);

    py::class_<GpuDepthStencilOptions> (m, "GpuDepthStencilOptions")
        .def (py::init<>())
        .def (py::init<float>(), "depthClearValue"_a)
        .def_readwrite ("depthLoadOp", &GpuDepthStencilOptions::depthLoadOp)
        .def_readwrite ("depthStoreOp", &GpuDepthStencilOptions::depthStoreOp)
        .def_readwrite ("depthClearValue", &GpuDepthStencilOptions::depthClearValue)
        .def_readwrite ("stencilLoadOp", &GpuDepthStencilOptions::stencilLoadOp)
        .def_readwrite ("stencilStoreOp", &GpuDepthStencilOptions::stencilStoreOp)
        .def_readwrite ("stencilClearValue", &GpuDepthStencilOptions::stencilClearValue);

    py::class_<GpuWorkgroupSize> (m, "GpuWorkgroupSize")
        .def (py::init<>())
        .def (py::init ([] (uint32_t x, uint32_t y, uint32_t z) { return GpuWorkgroupSize { x, y, z }; }),
              "x"_a = 1, "y"_a = 1, "z"_a = 1)
        .def_readwrite ("x", &GpuWorkgroupSize::x)
        .def_readwrite ("y", &GpuWorkgroupSize::y)
        .def_readwrite ("z", &GpuWorkgroupSize::z);

    py::class_<GpuRenderOptions> (m, "GpuRenderOptions")
        .def (py::init<>())
        .def (py::init<bool, GpuColor>(), "clear"_a, "clearColor"_a)
        .def (py::init<GpuLoadOp, GpuStoreOp, GpuColor>(), "loadOp"_a, "storeOp"_a, "clearColor"_a = GpuColor::transparentBlack())
        .def_readwrite ("loadOp", &GpuRenderOptions::loadOp)
        .def_readwrite ("storeOp", &GpuRenderOptions::storeOp)
        .def_property ("clear",
                       [] (const GpuRenderOptions& self)
                       {
                           return self.loadOp == GpuLoadOp::clear;
                       },
                       [] (GpuRenderOptions& self, bool shouldClear)
                       {
                           self.loadOp = shouldClear ? GpuLoadOp::clear : GpuLoadOp::load;
                       })
        .def_readwrite ("clearColor", &GpuRenderOptions::clearColor);

    py::class_<GpuFrameDescriptor> (m, "GpuFrameDescriptor")
        .def (py::init<>())
        .def_readwrite ("renderTargetWidth", &GpuFrameDescriptor::renderTargetWidth)
        .def_readwrite ("renderTargetHeight", &GpuFrameDescriptor::renderTargetHeight)
        .def_readwrite ("loadOp", &GpuFrameDescriptor::loadOp)
        .def_readwrite ("clearColor", &GpuFrameDescriptor::clearColor)
        .def_readwrite ("msaaSampleCount", &GpuFrameDescriptor::msaaSampleCount)
        .def_readwrite ("disableRasterOrdering", &GpuFrameDescriptor::disableRasterOrdering)
        .def_readwrite ("ditherMode", &GpuFrameDescriptor::ditherMode)
        .def_readwrite ("virtualTileWidth", &GpuFrameDescriptor::virtualTileWidth)
        .def_readwrite ("virtualTileHeight", &GpuFrameDescriptor::virtualTileHeight)
        .def_readwrite ("wireframe", &GpuFrameDescriptor::wireframe)
        .def_readwrite ("fillsDisabled", &GpuFrameDescriptor::fillsDisabled)
        .def_readwrite ("strokesDisabled", &GpuFrameDescriptor::strokesDisabled)
        .def_readwrite ("clockwiseFillOverride", &GpuFrameDescriptor::clockwiseFillOverride)
        .def ("__repr__", [] (const GpuFrameDescriptor& self)
        {
            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << (int) self.renderTargetWidth << ", " << (int) self.renderTargetHeight << ")";
            return result;
        });

    // The code/bindingMap/glFixup blobs own their data, so they are exposed as bytes-in,
    // bytes-out properties rather than def_readwrite (which would default to a list-of-ints
    // caster for std::vector<uint8>).
    py::class_<GpuShaderSource> (m, "GpuShaderSource")
        .def (py::init<>())
        .def_readwrite ("language", &GpuShaderSource::language)
        .def_property ("code",
                       [] (const GpuShaderSource& self)
                       {
                           return py::bytes (reinterpret_cast<const char*> (self.code.data()), self.code.size());
                       },
                       [] (GpuShaderSource& self, py::buffer data)
                       {
                           auto info = data.request();
                           auto* bytePtr = reinterpret_cast<const uint8*> (info.ptr);
                           self.code.assign (bytePtr, bytePtr + byteSizeOf (info));
                       })
        .def_property ("bindingMap",
                       [] (const GpuShaderSource& self)
                       {
                           return py::bytes (reinterpret_cast<const char*> (self.bindingMap.data()), self.bindingMap.size());
                       },
                       [] (GpuShaderSource& self, py::buffer data)
                       {
                           auto info = data.request();
                           auto* bytePtr = reinterpret_cast<const uint8*> (info.ptr);
                           self.bindingMap.assign (bytePtr, bytePtr + byteSizeOf (info));
                       })
        .def_property ("glFixup",
                       [] (const GpuShaderSource& self)
                       {
                           return py::bytes (reinterpret_cast<const char*> (self.glFixup.data()), self.glFixup.size());
                       },
                       [] (GpuShaderSource& self, py::buffer data)
                       {
                           auto info = data.request();
                           auto* bytePtr = reinterpret_cast<const uint8*> (info.ptr);
                           self.glFixup.assign (bytePtr, bytePtr + byteSizeOf (info));
                       })
        .def_readwrite ("entryPoint", &GpuShaderSource::entryPoint);

    // ============================================================================================ yup::GpuDevice

    auto gpuDevice = py::class_<GpuDevice, ReferenceCountedObjectPtr<GpuDevice>>(m, "GpuDevice");

    py::class_<GpuDevice::Options> (gpuDevice, "Options")
        .def (py::init<>())
        .def_readwrite ("retinaDisplay", &GpuDevice::Options::retinaDisplay)
        .def_readwrite ("readableFramebuffer", &GpuDevice::Options::readableFramebuffer)
        .def_readwrite ("synchronousShaderCompilations", &GpuDevice::Options::synchronousShaderCompilations)
        .def_readwrite ("disableRasterOrdering", &GpuDevice::Options::disableRasterOrdering)
        .def_readwrite ("allowHeadlessRendering", &GpuDevice::Options::allowHeadlessRendering)
        .def_readwrite ("vsync", &GpuDevice::Options::vsync)
        /*.def_readwrite ("loaderFunction", &GpuDevice::Options::loaderFunction)*/;

    gpuDevice
        .def_static ("create", &GpuDevice::create, "gpuApi"_a, "options"_a)
        .def ("getPlatform", &GpuDevice::getPlatform)
        .def ("isGpuAvailable", &GpuDevice::isGpuAvailable)
        .def ("isComputeAvailable", &GpuDevice::isComputeAvailable)
        .def ("isFormatSupported", &GpuDevice::isFormatSupported, "format"_a)
        .def ("isFormatRenderable", &GpuDevice::isFormatRenderable, "format"_a)
        .def ("isAnisotropicFilteringAvailable", &GpuDevice::isAnisotropicFilteringAvailable)
        .def ("getMaximumSampleCount", &GpuDevice::getMaximumSampleCount)
        .def ("createBuffer", [] (GpuDevice& self, GpuBufferType type, py::buffer data)
        {
            auto info = data.request();
            return self.createBuffer (type, info.ptr, byteSizeOf (info));
        }, "type"_a, "data"_a)
        .def ("updateBuffer", [] (GpuDevice& self, GpuBuffer::Ptr buffer, py::buffer data)
        {
            auto info = data.request();
            return self.updateBuffer (std::move (buffer), info.ptr, byteSizeOf (info));
        }, "buffer"_a, "data"_a)
        .def ("readBuffer", [] (GpuDevice& self, GpuBuffer::Ptr buffer, py::buffer dst)
        {
            auto info = dst.request (true);
            return self.readBuffer (std::move (buffer), info.ptr, byteSizeOf (info));
        }, "buffer"_a, "dst"_a,
             "Reads a storage buffer back into a writable buffer object. Returns False when no\n"
             "new data is available yet; the destination keeps its previous contents.");

    // ============================================================================================ yup::GpuTexture

    py::class_<GpuTexture, ReferenceCountedObjectPtr<GpuTexture>> (m, "GpuTexture")
        .def_static ("create", &GpuTexture::create, "device"_a, "desc"_a)
        .def ("upload", [] (GpuTexture& self, py::buffer data, const GpuTextureDataDesc& region)
        {
            auto info = data.request();

            auto desc = region;
            desc.data = info.ptr;
            return self.upload (desc);
        }, "data"_a, "region"_a = GpuTextureDataDesc {},
             "Uploads pixels into one mip level of one layer. The region describes where they land;\n"
             "its data field is ignored and taken from the buffer argument instead.")
        .def ("getWidth", &GpuTexture::getWidth)
        .def ("getHeight", &GpuTexture::getHeight)
        .def ("getFormat", &GpuTexture::getFormat)
        .def ("getType", &GpuTexture::getType)
        .def ("getMipLevels", &GpuTexture::getMipLevels)
        .def ("getDepthOrArrayLayers", &GpuTexture::getDepthOrArrayLayers)
        .def ("getSampleCount", &GpuTexture::getSampleCount)
        .def ("isValid", &GpuTexture::isValid)
        .def ("isRenderTarget", &GpuTexture::isRenderTarget)
        .def ("__repr__", [] (const GpuTexture& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " " << self.getWidth() << "x" << self.getHeight() << ">";
            return result;
        });

    // ============================================================================================ yup::GpuSampler

    py::class_<GpuSampler, ReferenceCountedObjectPtr<GpuSampler>> (m, "GpuSampler")
        .def_static ("create", &GpuSampler::create, "device"_a, "desc"_a)
        .def ("isValid", &GpuSampler::isValid)
        .def ("getDescription", &GpuSampler::getDescription);

    // ============================================================================================ yup::GpuBuffer

    py::class_<GpuBuffer, ReferenceCountedObjectPtr<GpuBuffer>> (m, "GpuBuffer")
        .def_static ("create", [] (GpuDevice::Ptr device, GpuBufferType type, py::buffer data)
        {
            auto info = data.request();
            return GpuBuffer::create (std::move (device), type, info.ptr, byteSizeOf (info));
        }, "device"_a, "type"_a, "data"_a)
        .def ("getType", &GpuBuffer::getType)
        .def ("getSizeInBytes", &GpuBuffer::getSizeInBytes)
        .def ("isValid", &GpuBuffer::isValid);

    // ============================================================================================ yup::GpuPipeline

    py::class_<GpuPipeline, ReferenceCountedObjectPtr<GpuPipeline>> (m, "GpuPipeline")
#if YUP_ENABLE_SHADER_TRANSPILER
        .def_static ("compileFromGlsl", [] (GpuDevice::Ptr device,
                                            const String& vertexGlsl,
                                            const String& fragmentGlsl,
                                            const GpuPipelineOptions& options)
        {
            return unwrapOrRaise (GpuPipeline::compileFromGlsl (std::move (device), vertexGlsl, fragmentGlsl, options));
        }, "device"_a, "vertexGlsl"_a, "fragmentGlsl"_a, "options"_a = GpuPipelineOptions {},
             "Compiles a pipeline from GLSL 450 sources. Raises RuntimeError with the compiler\n"
             "diagnostics when compilation fails.")
#endif
        .def_static ("compile", [] (GpuDevice::Ptr device,
                                    const GpuShaderSource& vertexShader,
                                    const GpuShaderSource& fragmentShader,
                                    const GpuPipelineOptions& options)
        {
            return unwrapOrRaise (GpuPipeline::compile (std::move (device), vertexShader, fragmentShader, options));
        }, "device"_a, "vertexShader"_a, "fragmentShader"_a, "options"_a = GpuPipelineOptions {},
             "Compiles a pipeline from native shader sources. Raises RuntimeError with a\n"
             "human-readable description when compilation fails.")
        .def ("isValid", [] (const GpuPipeline&) { return true; });

    // ============================================================================================ yup::GpuComputePipeline

    py::class_<GpuComputePipeline, ReferenceCountedObjectPtr<GpuComputePipeline>> (m, "GpuComputePipeline")
#if YUP_ENABLE_SHADER_TRANSPILER
        .def_static ("compileFromGlsl", [] (GpuDevice::Ptr device,
                                            const String& glsl,
                                            const GpuWorkgroupSize& workgroupSize)
        {
            return unwrapOrRaise (GpuComputePipeline::compileFromGlsl (std::move (device), glsl, workgroupSize));
        }, "device"_a, "glsl"_a, "workgroupSize"_a = GpuWorkgroupSize {},
             "Compiles a compute pipeline from GLSL 450 source. Raises RuntimeError with the\n"
             "compiler diagnostics when compilation fails.")
#endif
        .def_static ("compile", [] (GpuDevice::Ptr device,
                                    const GpuShaderSource& source,
                                    const GpuWorkgroupSize& workgroupSize)
        {
            return unwrapOrRaise (GpuComputePipeline::compile (std::move (device), source, workgroupSize));
        }, "device"_a, "source"_a, "workgroupSize"_a = GpuWorkgroupSize {},
             "Compiles a compute pipeline from a native shader source. Raises RuntimeError with a\n"
             "human-readable description when compilation fails.")
        .def ("getWorkgroupSize", &GpuComputePipeline::getWorkgroupSize);

    // ============================================================================================ yup::GpuPipelineCache

    py::class_<GpuPipelineCache> (m, "GpuPipelineCache")
        .def (py::init<GpuDevice::Ptr>())
        .def ("getNumEntries", &GpuPipelineCache::getNumEntries)
        .def ("setMaxEntries", &GpuPipelineCache::setMaxEntries)
        .def ("getMaxEntries", &GpuPipelineCache::getMaxEntries)
        .def ("clear", &GpuPipelineCache::clear);

    // ============================================================================================ yup::GpuTarget

    py::class_<GpuTarget, ReferenceCountedObjectPtr<GpuTarget>> (m, "GpuTarget")
        .def_static ("create", py::overload_cast<GpuDevice::Ptr, int, int> (&GpuTarget::create), "device"_a, "width"_a, "height"_a)
        .def_static ("create", py::overload_cast<GpuDevice::Ptr, const GpuTextureDesc&> (&GpuTarget::create), "device"_a, "desc"_a)
        .def_static ("createFromTexture", &GpuTarget::createFromTexture,
                     "device"_a, "texture"_a, "view"_a = GpuTextureViewDesc {})
        .def ("getWidth", &GpuTarget::getWidth)
        .def ("getHeight", &GpuTarget::getHeight)
        // A pass borrows both the target it draws into and the frame it records into,
        // so both have to outlive it.
        .def ("beginRenderPass", &GpuTarget::beginRenderPass, "frame"_a, "options"_a = GpuRenderOptions {},
              py::keep_alive<0, 1>(), py::keep_alive<0, 2>())
        .def ("asTexture", &GpuTarget::asTexture)
        .def ("readPixels", [] (GpuTarget& self) -> py::object
        {
            std::vector<uint8> pixels ((size_t) self.getWidth() * (size_t) self.getHeight() * 4);
            if (pixels.empty() || ! self.readPixels (pixels.data(), pixels.size()))
                return py::none();

            return py::bytes (reinterpret_cast<const char*> (pixels.data()), pixels.size());
        }, "Reads the rendered result back as width*height*4 RGBA bytes, or None when readback is\n"
           "unavailable. Only targets created by the width/height overload can be read back; one\n"
           "backed by a directly allocated texture always returns None, because the backend layer\n"
           "has no texture readback path.")
        .def ("__repr__", [] (const GpuTarget& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " " << self.getWidth() << "x" << self.getHeight() << ">";
            return result;
        });

    // ============================================================================================ yup::GpuFrame (move-only, context manager)

    py::class_<GpuFrame> (m, "GpuFrame")
        // The frame holds the device alive itself, but keeping the Python device object
        // alive too stops it being collected mid-frame.
        .def_static ("begin", &GpuFrame::begin, "device"_a, py::keep_alive<0, 1>())
        .def ("isValid", &GpuFrame::isValid)
        .def ("submit", &GpuFrame::submit)
        .def ("waitForGPU", &GpuFrame::waitForGPU)
        .def ("__enter__", [] (GpuFrame& self) -> GpuFrame& { return self; })
        .def ("__exit__", [] (GpuFrame&, const std::optional<py::type>&,
                               const std::optional<py::object>&,
                               const std::optional<py::object>&) { /* auto-submit on destructor */ });

    // ============================================================================================ yup::GpuRenderPass (move-only, context manager)

    py::class_<GpuRenderPass> (m, "GpuRenderPass")
        .def ("isValid", &GpuRenderPass::isValid)
        .def ("setPipeline", &GpuRenderPass::setPipeline, "pipeline"_a)
        .def ("setTexture", &GpuRenderPass::setTexture, "group"_a, "binding"_a, "texture"_a)
        .def ("setSampler", &GpuRenderPass::setSampler, "group"_a, "binding"_a, "sampler"_a)
        .def ("setUniformBuffer", [] (GpuRenderPass& self, int group, int binding, py::buffer data)
        {
            auto info = data.request();
            self.setUniformBuffer (group, binding, info.ptr, byteSizeOf (info));
        }, "group"_a, "binding"_a, "data"_a)
        .def ("setVertexBuffer", &GpuRenderPass::setVertexBuffer, "slot"_a, "buffer"_a)
        .def ("setIndexBuffer", &GpuRenderPass::setIndexBuffer, "format"_a, "buffer"_a)
        .def ("setColorAttachment", &GpuRenderPass::setColorAttachment,
              "index"_a, "texture"_a, "options"_a = GpuRenderOptions {}, "view"_a = GpuTextureViewDesc {})
        .def ("setDepthStencilAttachment", &GpuRenderPass::setDepthStencilAttachment,
              "texture"_a, "options"_a = GpuDepthStencilOptions {}, "view"_a = GpuTextureViewDesc {})
        .def ("setResolveTarget", &GpuRenderPass::setResolveTarget,
              "index"_a, "texture"_a, "view"_a = GpuTextureViewDesc {})
        .def ("setViewport", &GpuRenderPass::setViewport,
              "x"_a, "y"_a, "width"_a, "height"_a, "minDepth"_a = 0.0f, "maxDepth"_a = 1.0f)
        .def ("setScissorRect", &GpuRenderPass::setScissorRect, "x"_a, "y"_a, "width"_a, "height"_a)
        .def ("setStencilReference", &GpuRenderPass::setStencilReference, "reference"_a)
        .def ("setBlendColor", &GpuRenderPass::setBlendColor, "color"_a)
        .def ("draw", &GpuRenderPass::draw,
              "vertexCount"_a, "instanceCount"_a = 1, "firstVertex"_a = 0, "firstInstance"_a = 0)
        .def ("drawIndexed", &GpuRenderPass::drawIndexed,
              "indexCount"_a, "instanceCount"_a = 1, "firstIndex"_a = 0, "baseVertex"_a = 0, "firstInstance"_a = 0)
        .def ("finish", &GpuRenderPass::finish)
        .def ("__enter__", [] (GpuRenderPass& self) -> GpuRenderPass& { return self; })
        .def ("__exit__", [] (GpuRenderPass&, const std::optional<py::type>&,
                               const std::optional<py::object>&,
                               const std::optional<py::object>&) { /* auto-finish on destructor */ });

    // ============================================================================================ yup::GpuComputePass (move-only, context manager)

    py::class_<GpuComputePass> (m, "GpuComputePass")
        .def_static ("begin", &GpuComputePass::begin, "device"_a, py::keep_alive<0, 1>())
        .def ("isValid", &GpuComputePass::isValid)
        .def ("setPipeline", &GpuComputePass::setPipeline, "pipeline"_a)
        .def ("setStorageBuffer", &GpuComputePass::setStorageBuffer, "group"_a, "binding"_a, "buffer"_a)
        .def ("setTexture", &GpuComputePass::setTexture, "group"_a, "binding"_a, "texture"_a)
        .def ("setUniformBuffer", [] (GpuComputePass& self, int group, int binding, py::buffer data)
        {
            auto info = data.request();
            self.setUniformBuffer (group, binding, info.ptr, byteSizeOf (info));
        }, "group"_a, "binding"_a, "data"_a)
        .def ("dispatch", &GpuComputePass::dispatch, "groupsX"_a, "groupsY"_a = 1, "groupsZ"_a = 1)
        .def ("finish", &GpuComputePass::finish)
        .def ("__enter__", [] (GpuComputePass& self) -> GpuComputePass& { return self; })
        .def ("__exit__", [] (GpuComputePass&, const std::optional<py::type>&,
                               const std::optional<py::object>&,
                               const std::optional<py::object>&) { /* auto-finish on destructor */ });

    // clang-format on
}

} // namespace yup::Bindings
