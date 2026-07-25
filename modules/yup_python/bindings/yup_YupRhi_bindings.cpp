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
#include "../utilities/yup_PyBind11Includes.h"

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupRhiBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::GraphicsContext enums

    py::enum_<GraphicsContext::Api> (m, "GraphicsApi")
        .value ("Headless", GraphicsContext::Api::Headless)
        .value ("OpenGL", GraphicsContext::Api::OpenGL)
        .value ("OpenGLES", GraphicsContext::Api::OpenGLES)
        .value ("Direct3D", GraphicsContext::Api::Direct3D)
        .value ("Metal", GraphicsContext::Api::Metal)
        .value ("WebGPU", GraphicsContext::Api::WebGPU)
                ;

    // ============================================================================================ yup::GraphicsContext::Options

    py::class_<GraphicsContext::Options> (m, "GraphicsContextOptions")
        .def (py::init<>())
        .def_readwrite ("retinaDisplay", &GraphicsContext::Options::retinaDisplay)
        .def_readwrite ("readableFramebuffer", &GraphicsContext::Options::readableFramebuffer)
        .def_readwrite ("synchronousShaderCompilations", &GraphicsContext::Options::synchronousShaderCompilations)
        .def_readwrite ("enableReadPixels", &GraphicsContext::Options::enableReadPixels)
        .def_readwrite ("disableRasterOrdering", &GraphicsContext::Options::disableRasterOrdering)
        .def_readwrite ("allowHeadlessRendering", &GraphicsContext::Options::allowHeadlessRendering);

    // ============================================================================================ yup::GraphicsContext

    py::class_<GraphicsContext> (m, "GraphicsContext")
        .def_static ("createContext", &GraphicsContext::createContext,
                     "graphicsApi"_a, "options"_a)
        .def ("getApi", &GraphicsContext::getApi)
        .def ("isGpuAvailable", &GraphicsContext::isGpuAvailable);

    // ============================================================================================ GPU enums

    py::enum_<GpuShaderLanguage> (m, "GpuShaderLanguage")
        .value ("wgsl", GpuShaderLanguage::wgsl)
        .value ("glsl", GpuShaderLanguage::glsl)
        .value ("msl", GpuShaderLanguage::msl)
        .value ("hlsl", GpuShaderLanguage::hlsl)
                ;

    py::enum_<GpuVertexFormat> (m, "GpuVertexFormat")
        .value ("float1", GpuVertexFormat::float1)
        .value ("float2", GpuVertexFormat::float2)
        .value ("float3", GpuVertexFormat::float3)
        .value ("float4", GpuVertexFormat::float4)
        .value ("uint8x4", GpuVertexFormat::uint8x4)
        .value ("snorm8x4", GpuVertexFormat::snorm8x4)
        .value ("unorm8x4", GpuVertexFormat::unorm8x4)
                ;

    py::enum_<GpuVertexStepMode> (m, "GpuVertexStepMode")
        .value ("vertex", GpuVertexStepMode::vertex)
        .value ("instance", GpuVertexStepMode::instance)
                ;

    py::enum_<GpuPrimitiveTopology> (m, "GpuPrimitiveTopology")
        .value ("pointList", GpuPrimitiveTopology::pointList)
        .value ("lineList", GpuPrimitiveTopology::lineList)
        .value ("lineStrip", GpuPrimitiveTopology::lineStrip)
        .value ("triangleList", GpuPrimitiveTopology::triangleList)
        .value ("triangleStrip", GpuPrimitiveTopology::triangleStrip)
                ;

    py::enum_<GpuIndexFormat> (m, "GpuIndexFormat")
        .value ("none", GpuIndexFormat::none)
        .value ("uint16", GpuIndexFormat::uint16)
        .value ("uint32", GpuIndexFormat::uint32)
                ;

    py::enum_<GpuCullMode> (m, "GpuCullMode")
        .value ("none", GpuCullMode::none)
        .value ("front", GpuCullMode::front)
        .value ("back", GpuCullMode::back)
                ;

    py::enum_<GpuFaceWinding> (m, "GpuFaceWinding")
        .value ("clockwise", GpuFaceWinding::clockwise)
        .value ("counterClockwise", GpuFaceWinding::counterClockwise)
                ;

    py::enum_<GpuCompareFunction> (m, "GpuCompareFunction")
        .value ("never", GpuCompareFunction::never)
        .value ("less", GpuCompareFunction::less)
        .value ("equal", GpuCompareFunction::equal)
        .value ("lessEqual", GpuCompareFunction::lessEqual)
        .value ("greater", GpuCompareFunction::greater)
        .value ("notEqual", GpuCompareFunction::notEqual)
        .value ("greaterEqual", GpuCompareFunction::greaterEqual)
        .value ("always", GpuCompareFunction::always)
                ;

    py::enum_<GpuStencilOp> (m, "GpuStencilOp")
        .value ("keep", GpuStencilOp::keep)
        .value ("zero", GpuStencilOp::zero)
        .value ("replace", GpuStencilOp::replace)
        .value ("incrementClamp", GpuStencilOp::incrementClamp)
        .value ("decrementClamp", GpuStencilOp::decrementClamp)
        .value ("invert", GpuStencilOp::invert)
        .value ("incrementWrap", GpuStencilOp::incrementWrap)
        .value ("decrementWrap", GpuStencilOp::decrementWrap)
                ;

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
                ;

    py::enum_<GpuBlendOp> (m, "GpuBlendOp")
        .value ("add", GpuBlendOp::add)
        .value ("subtract", GpuBlendOp::subtract)
        .value ("reverseSubtract", GpuBlendOp::reverseSubtract)
        .value ("min", GpuBlendOp::min)
        .value ("max", GpuBlendOp::max)
                ;

    py::enum_<GpuTextureFormat> (m, "GpuTextureFormat")
        .value ("rgba8unorm", GpuTextureFormat::rgba8unorm)
        .value ("bgra8unorm", GpuTextureFormat::bgra8unorm)
        .value ("rgba16float", GpuTextureFormat::rgba16float)
        .value ("depth24plusStencil8", GpuTextureFormat::depth24plusStencil8)
        .value ("depth32float", GpuTextureFormat::depth32float)
                ;

    py::enum_<GpuBufferType> (m, "GpuBufferType")
        .value ("vertex", GpuBufferType::vertex)
        .value ("index", GpuBufferType::index)
        .value ("uniform", GpuBufferType::uniform)
                ;

    // ============================================================================================ GPU config structs

    py::class_<GpuShaderSource> (m, "GpuShaderSource")
        .def (py::init<>())
        .def_readwrite ("language", &GpuShaderSource::language)
        .def_readwrite ("code", &GpuShaderSource::code)
        .def_readwrite ("codeSize", &GpuShaderSource::codeSize)
        .def_readwrite ("entryPoint", &GpuShaderSource::entryPoint);

    py::class_<GpuVertexAttribute> (m, "GpuVertexAttribute")
        .def (py::init<>())
        .def (py::init<GpuVertexFormat, uint32_t, uint32_t>(),
              "format"_a, "offset"_a, "shaderLocation"_a)
        .def_readwrite ("format", &GpuVertexAttribute::format)
        .def_readwrite ("offset", &GpuVertexAttribute::offset)
        .def_readwrite ("shaderLocation", &GpuVertexAttribute::shaderLocation);

    py::class_<GpuVertexBufferLayout> (m, "GpuVertexBufferLayout")
        .def (py::init<>())
        .def_readwrite ("stride", &GpuVertexBufferLayout::stride)
        .def_readwrite ("stepMode", &GpuVertexBufferLayout::stepMode)
        .def_readwrite ("attributeCount", &GpuVertexBufferLayout::attributeCount);

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
        .def_readwrite ("blend", &GpuColorTarget::blend);

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
        .def_readwrite ("topology", &GpuPipelineOptions::topology)
        .def_readwrite ("indexFormat", &GpuPipelineOptions::indexFormat)
        .def_readwrite ("cullMode", &GpuPipelineOptions::cullMode)
        .def_readwrite ("winding", &GpuPipelineOptions::winding)
        .def_readwrite ("colorTargetCount", &GpuPipelineOptions::colorTargetCount)
        .def_readwrite ("depthStencil", &GpuPipelineOptions::depthStencil)
        .def_readwrite ("stencilFront", &GpuPipelineOptions::stencilFront)
        .def_readwrite ("stencilBack", &GpuPipelineOptions::stencilBack)
        .def_readwrite ("stencilReadMask", &GpuPipelineOptions::stencilReadMask)
        .def_readwrite ("stencilWriteMask", &GpuPipelineOptions::stencilWriteMask)
        .def_readwrite ("sampleCount", &GpuPipelineOptions::sampleCount);

    py::class_<GpuRenderOptions> (m, "GpuRenderOptions")
        .def (py::init<>())
        .def (py::init<bool, Color>(), "clear"_a, "clearColor"_a)
        .def_readwrite ("clear", &GpuRenderOptions::clear)
        .def_readwrite ("clearColor", &GpuRenderOptions::clearColor);

    // ============================================================================================ yup::GpuTexture

    py::class_<GpuTexture, ReferenceCountedObjectPtr<GpuTexture>> (m, "GpuTexture")
        .def ("getWidth", &GpuTexture::getWidth)
        .def ("getHeight", &GpuTexture::getHeight)
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

    // ============================================================================================ yup::GpuBuffer

    py::class_<GpuBuffer, ReferenceCountedObjectPtr<GpuBuffer>> (m, "GpuBuffer")
        .def_static ("create", &GpuBuffer::create)
        .def ("getType", &GpuBuffer::getType)
        .def ("getSizeInBytes", &GpuBuffer::getSizeInBytes)
        .def ("isValid", &GpuBuffer::isValid);

    // ============================================================================================ yup::GpuPipeline

    py::class_<GpuPipeline, ReferenceCountedObjectPtr<GpuPipeline>> (m, "GpuPipeline")
        .def_static ("compile", &GpuPipeline::compile)
#if YUP_ENABLE_SHADER_TRANSPILER
        .def_static ("compileFromGlsl", &GpuPipeline::compileFromGlsl)
#endif
        .def ("isValid", [](const GpuPipeline& self) { return true; });

    // ============================================================================================ yup::GpuPipelineCache

    py::class_<GpuPipelineCache> (m, "GpuPipelineCache")
        .def (py::init<GraphicsContext&>())
        .def ("getNumEntries", &GpuPipelineCache::getNumEntries)
        .def ("setMaxEntries", &GpuPipelineCache::setMaxEntries)
        .def ("getMaxEntries", &GpuPipelineCache::getMaxEntries)
        .def ("clear", &GpuPipelineCache::clear);

    // ============================================================================================ yup::GpuTarget

    py::class_<GpuTarget, ReferenceCountedObjectPtr<GpuTarget>> (m, "GpuTarget")
        .def_static ("create", &GpuTarget::create)
        .def ("getWidth", &GpuTarget::getWidth)
        .def ("getHeight", &GpuTarget::getHeight)
        .def ("asTexture", &GpuTarget::asTexture)
        .def ("asImage", &GpuTarget::asImage)
        .def ("__repr__", [] (const GpuTarget& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " " << self.getWidth() << "x" << self.getHeight() << ">";
            return result;
        });

    // ============================================================================================ yup::GpuCanvas

    py::class_<GpuCanvas, ReferenceCountedObjectPtr<GpuCanvas>> (m, "GpuCanvas")
        .def_static ("create", &GpuCanvas::create)
        .def ("getWidth", &GpuCanvas::getWidth)
        .def ("getHeight", &GpuCanvas::getHeight)
        .def ("asTexture", &GpuCanvas::asTexture)
        .def ("asImage", &GpuCanvas::asImage)
        .def ("getTarget", &GpuCanvas::getTarget)
        .def ("__repr__", [] (const GpuCanvas& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " " << self.getWidth() << "x" << self.getHeight() << ">";
            return result;
        });

    // ============================================================================================ yup::GpuFrame (move-only, context manager)

    py::class_<GpuFrame> (m, "GpuFrame")
        .def_static ("begin", &GpuFrame::begin)
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
        .def ("setPipeline", &GpuRenderPass::setPipeline)
        .def ("setTexture", [] (GpuRenderPass& self, int group, int binding, GpuTexture::Ptr texture)
        {
            self.setTexture (group, binding, std::move (texture));
        })
        .def ("setUniformBuffer", [] (GpuRenderPass& self, int group, int binding,
                                       py::bytes data)
        {
            self.setUniformBuffer (group, binding,
                                   data.cast<std::string_view>().data(),
                                   data.cast<std::string_view>().size());
        })
        .def ("setVertexBuffer", &GpuRenderPass::setVertexBuffer)
        .def ("setIndexBuffer", &GpuRenderPass::setIndexBuffer)
        .def ("draw", &GpuRenderPass::draw)
        .def ("drawIndexed", &GpuRenderPass::drawIndexed)
        .def ("finish", &GpuRenderPass::finish)
        .def ("__enter__", [] (GpuRenderPass& self) -> GpuRenderPass& { return self; })
        .def ("__exit__", [] (GpuRenderPass&, const std::optional<py::type>&,
                               const std::optional<py::object>&,
                               const std::optional<py::object>&) { /* auto-finish on destructor */ });

    // clang-format on
}

} // namespace yup::Bindings
