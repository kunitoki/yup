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

#include <yup_core/yup_core.h>
#include <yup_shading/yup_shading.h>

using namespace yup;

static String getOptionValue (const ArgumentList& args, StringRef option, const String& defaultValue = {})
{
    const int index = args.indexOfOption (option);
    if (index < 0)
        return defaultValue;

    // Long option assignment form: "--opt=value"
    const String assigned = args.getValueForOption (option);
    if (assigned.isNotEmpty())
        return assigned;

    // Space-separated form: "--opt value"
    if (index + 1 < args.size() && ! args[index + 1].isOption())
        return args[index + 1].text;

    return defaultValue;
}

static void printUsage()
{
    Logger::getCurrentLogger()->writeToLog (
        "Usage: yup_shader_bundler [options]\n"
        "\n"
        "Options:\n"
        "  --vert <path>         Path to vertex shader (.vert) file\n"
        "  --frag <path>         Path to fragment shader (.frag) file\n"
        "  --output <path>       Output .ysl bundle file path\n"
        "  --entry <name>        Shader entry point name (default: main)\n"
        "  --glsl-version <n>    GLSL version to target (default: 450)\n"
        "  --help|-h             Print this help\n"
        "\n"
        "Compiles a vertex and fragment GLSL shader pair into a portable\n"
        ".ysl bundle containing transpiled variants for all target languages\n"
        "(GLSL, ESSL, HLSL, MSL).\n");
}

int main (int argc, char* argv[])
{
    ArgumentList args (argc, argv);

    if (args.containsOption ("--help|-h"))
    {
        printUsage();
        return 0;
    }

    args.failIfOptionIsMissing ("--vert");
    args.failIfOptionIsMissing ("--frag");
    args.failIfOptionIsMissing ("--output");

    const auto resolveFile = [] (const String& path) -> File
    {
        if (path.isEmpty())
            return {};
        if (File::isAbsolutePath (path))
            return File (path);
        return File::getCurrentWorkingDirectory().getChildFile (path);
    };

    const auto vertPath = resolveFile (getOptionValue (args, "--vert"));
    const auto fragPath = resolveFile (getOptionValue (args, "--frag"));
    const auto outputPath = resolveFile (getOptionValue (args, "--output"));

    const auto entryPoint = getOptionValue (args, "--entry", "main");

    const String glslVersionVal = getOptionValue (args, "--glsl-version", "450");
    const int glslVersion = glslVersionVal.getIntValue();

    // -- Validate input files
    if (! vertPath.existsAsFile())
    {
        const String msg = "Vertex shader file not found: " + vertPath.getFullPathName();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    if (! fragPath.existsAsFile())
    {
        const String msg = "Fragment shader file not found: " + fragPath.getFullPathName();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    const String vertSource = vertPath.loadFileAsString();
    const String fragSource = fragPath.loadFileAsString();

    if (vertSource.isEmpty())
    {
        const String msg = "Vertex shader file is empty: " + vertPath.getFullPathName();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    if (fragSource.isEmpty())
    {
        const String msg = "Fragment shader file is empty: " + fragPath.getFullPathName();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    // -- Target all supported shading languages
    const std::vector<ShaderLanguage> targetLanguages = {
        ShaderLanguage::glsl,
        ShaderLanguage::essl,
        ShaderLanguage::hlsl,
        ShaderLanguage::msl
    };

    auto makeEntry = [&] (ShaderStage stage)
    {
        ShaderBundleEntry entry;
        entry.stage = stage;
        entry.targetLanguages = targetLanguages;
        entry.options.entryPoint = entryPoint;
        entry.options.glslVersion = glslVersion;
        return entry;
    };

    ShaderBundleCompiler compiler;

    // Compile vertex stage
    ShaderBundleCompileRequest vertRequest;
    vertRequest.source = vertSource;
    vertRequest.sourceLanguage = ShaderLanguage::glsl;
    vertRequest.entries.push_back (makeEntry (ShaderStage::vertex));

    auto vsBundle = compiler.compile (vertRequest);
    if (vsBundle.failed())
    {
        const String msg = "Vertex shader compilation failed: " + vsBundle.getErrorMessage();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    // Compile fragment stage
    ShaderBundleCompileRequest fragRequest;
    fragRequest.source = fragSource;
    fragRequest.sourceLanguage = ShaderLanguage::glsl;
    fragRequest.entries.push_back (makeEntry (ShaderStage::fragment));

    auto fsBundle = compiler.compile (fragRequest);
    if (fsBundle.failed())
    {
        const String msg = "Fragment shader compilation failed: " + fsBundle.getErrorMessage();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    // Merge both stages into a single bundle
    ShaderBundle bundle;
    for (const auto& info : vsBundle.getReference().getShaders())
        bundle.addShader (info);
    for (const auto& info : fsBundle.getReference().getShaders())
        bundle.addShader (info);

    // Persist to file
    const auto saveResult = bundle.saveToFile (outputPath);
    if (saveResult.failed())
    {
        const String msg = "Failed to save bundle: " + saveResult.getErrorMessage();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    Logger::getCurrentLogger()->writeToLog ("Shader bundle written to: " + outputPath.getFullPathName());
    return 0;
}
