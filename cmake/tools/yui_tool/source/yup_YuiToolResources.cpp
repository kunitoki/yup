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

#include "yup_YuiToolResources.h"

#if YUP_MODULE_AVAILABLE_yup_renderer
#include <yup_renderer/yup_renderer.h>
#endif

#include <algorithm>
#include <optional>
#include <vector>

using namespace yup;

namespace
{

#if YUP_ENABLE_SHADER_TRANSPILER

// Compiles the source of a shader made from a `fragment` for every graphics API: the styles compile it for the one that
// draws, so what it has wrong for another is told here
void checkFragmentShader (YuiDocument& document, const DataTree& block)
{
    const auto path = block.getProperty ("fragment").toString();

    // A bundle compiled ahead is loaded as it is, with the variants it holds
    if (! path.endsWithIgnoreCase (".ysl") && document.readAsset (path + ".ysl").has_value())
        return;

    const auto source = document.readAsset (path);

    if (! source.has_value() || GpuImageShader::isCompiledBundle (source->getData(), source->getSize()))
        return;

    if (const auto created = GpuImageShader::createFromData (source->getData(), source->getSize()); created.failed())
        document.reportDiagnostic ({ .message = "shader " + block.getProperty ("name").toString().trim() + ": " + created.getErrorMessage(), .element = block, .property = "fragment" });
}

#endif

#if YUP_MODULE_AVAILABLE_yup_renderer

// The frames a scene is given to show its final picture: four times those it accumulates when nothing changes
constexpr int maximumSceneFrames = 64;

// The scenes an interface shows, in the order of its tree
void collectScenes (Component& component, std::vector<SceneComponent*>& scenes)
{
    if (auto* scene = dynamic_cast<SceneComponent*> (&component))
        scenes.push_back (scene);

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        collectScenes (*component.getChildComponent (index), scenes);
}

#if YUP_ENABLE_SHADER_TRANSPILER

// Loads a shader a document makes from a `material`, which only a scene drawing with it loads, and tells what prevents it:
// its file, the errors of its compiler, and the properties its uniform block has no member for
void checkMaterialShader (YuiDocument& document, const DataTree& block)
{
    const auto name = block.getProperty ("name").toString().trim();
    const auto path = block.getProperty ("material").toString();

    // The variants a build compiles the material into are next to it, either or both: without them its source is compiled here
    const auto raster = document.readAsset (path + ".raster.ysl");
    const auto full = document.readAsset (path + ".full.ysl");
    const auto source = raster.has_value() || full.has_value() ? std::optional<MemoryBlock>() : document.readAsset (path);

    if (! raster.has_value() && ! full.has_value() && ! source.has_value())
    {
        document.reportDiagnostic ({ .message = "The shader '" + name + "' can't read '" + path + "' nor its compiled variants '" + path + ".raster.ysl' and '" + path + ".full.ysl'",
                                     .element = block,
                                     .property = "material" });
        return;
    }

    const auto created = source.has_value() ? MaterialShader::createFromSource (source->toString())
                                            : MaterialShader::createFromData (raster.has_value() ? raster->getData() : nullptr,
                                                                              raster.has_value() ? raster->getSize() : size_t (0),
                                                                              full.has_value() ? full->getData() : nullptr,
                                                                              full.has_value() ? full->getSize() : size_t (0));

    if (created.failed())
    {
        document.reportDiagnostic ({ .message = "shader " + name + ": " + created.getErrorMessage(), .element = block, .property = "material" });
        return;
    }

    // A property fills the member of its name, as the one of a shader made from a fragment does
    const auto uniforms = created.getReference()->createUniforms();

    for (const auto& declared : ShaderProperty::readBlock (block, name, &uniforms))
        if (declared.problem.isNotEmpty())
            document.reportDiagnostic ({ .message = declared.problem, .element = declared.element });
}

#endif

#endif

} // namespace

//==============================================================================

void finishLoadingYuiToolScenes ([[maybe_unused]] Component& component)
{
#if YUP_MODULE_AVAILABLE_yup_renderer
    std::vector<SceneComponent*> scenes;
    collectScenes (component, scenes);

    for (auto* scene : scenes)
        scene->finishLoading();
#endif
}

void checkYuiToolResources (YuiDocument& document)
{
    auto* root = document.getRoot();

    if (root == nullptr)
        return;

#if YUP_ENABLE_SHADER_TRANSPILER
    for (const auto& element : document.getTree())
    {
        if (element.getType().toString() != "shader")
            continue;

        // One made from a fragment is loaded by the styles, which tell what prevents it once, however many components use
        // it. They compile it for one graphics API: the others are checked once that one loads
        if (auto* context = root->getStyleContext(); context != nullptr && element.hasProperty ("fragment"))
        {
            if (context->findShader (element.getProperty ("name").toString().trim()) != nullptr)
                checkFragmentShader (document, element);
        }

#if YUP_MODULE_AVAILABLE_yup_renderer
        if (element.hasProperty ("material"))
            checkMaterialShader (document, element);
#endif
    }
#endif

#if YUP_MODULE_AVAILABLE_yup_renderer
    std::vector<SceneComponent*> scenes;
    collectScenes (*root, scenes);

    // The files of a scene are waited for, as no message loop brings them in. What it left to its first frame is then
    // built there, and tells what is wrong with it
    for (auto* scene : scenes)
    {
        scene->finishLoading();
        scene->refreshDisplay (0.0);
    }
#endif
}

ResultValue<Image> drawYuiToolImage (Component& component, const std::function<Image()>& snapshot)
{
#if YUP_MODULE_AVAILABLE_yup_renderer
    std::vector<SceneComponent*> scenes;
    collectScenes (component, scenes);

    Image image;

    for (int frame = 0; frame < maximumSceneFrames; ++frame)
    {
        // The frame a window runs before it paints. No time goes by, so what is animated stands where it starts
        for (auto* scene : scenes)
            scene->refreshDisplay (0.0);

        image = snapshot();

        const auto isFinal = std::all_of (scenes.begin(), scenes.end(), [] (SceneComponent* scene)
        {
            return scene->getRenderer().isConverged();
        });

        if (! image.isValid() || isFinal)
            break;
    }

    for (auto* scene : scenes)
    {
        const auto& renderer = scene->getRenderer();

        // A scene that could not be drawn shows why in its place, which is not the picture asked for
        if (const auto& error = renderer.getLastError(); error.isNotEmpty())
            return makeResultValueFail ("a Scene3D can't be drawn: " + error);

        if (renderer.isConverged())
            continue;

        if (auto* document = YuiDocument::findDocumentFor (scene))
        {
            document->reportDiagnostic ({ .message = "Scene3D: still changing after " + String (maximumSceneFrames) + " frames, the image shows it as it was at the last of them",
                                          .severity = DataTreeDiagnostic::Severity::warning },
                                        scene);
        }
    }

    return makeResultValueOk (image);
#else
    ignoreUnused (component);

    return makeResultValueOk (snapshot());
#endif
}
