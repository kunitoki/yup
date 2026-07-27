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

//==============================================================================
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

static StringArray getAllOptionValues (const ArgumentList& args, StringRef option)
{
    StringArray values;
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i] == option)
        {
            // Check for "--opt=value" form
            if (args[i].isLongOption())
            {
                const String val = args[i].getLongOptionValue();
                if (val.isNotEmpty())
                {
                    values.add (val);
                    continue;
                }
            }

            // Check for "--opt value" form
            if (i + 1 < args.size() && ! args[i + 1].isOption())
            {
                values.add (args[i + 1].text);
                ++i;
            }
        }
    }
    return values;
}

// Collects the remainder of every argument that starts with the given prefix,
// e.g. prefix "-D" over "-DFOO=1" yields "FOO=1" (used for -D and -I flags).
static StringArray getPrefixedValues (const ArgumentList& args, const String& prefix)
{
    StringArray values;
    for (int i = 0; i < args.size(); ++i)
    {
        const String& text = args[i].text;
        if (text.startsWith (prefix) && text.length() > prefix.length())
            values.add (text.substring (prefix.length()));
    }
    return values;
}

//==============================================================================
static String languageToString (ShaderLanguage lang)
{
    switch (lang)
    {
        case ShaderLanguage::glsl:   return "glsl";
        case ShaderLanguage::essl:   return "essl";
        case ShaderLanguage::hlsl:   return "hlsl";
        case ShaderLanguage::msl:    return "msl";
        case ShaderLanguage::spirv:  return "spirv";
        case ShaderLanguage::wgsl:   return "wgsl";
    }
    return "unknown";
}

static std::optional<ShaderLanguage> languageFromString (const String& str)
{
    if (str.equalsIgnoreCase ("glsl"))  return ShaderLanguage::glsl;
    if (str.equalsIgnoreCase ("essl"))  return ShaderLanguage::essl;
    if (str.equalsIgnoreCase ("hlsl"))  return ShaderLanguage::hlsl;
    if (str.equalsIgnoreCase ("msl"))   return ShaderLanguage::msl;
    if (str.equalsIgnoreCase ("spirv")) return ShaderLanguage::spirv;
    if (str.equalsIgnoreCase ("wgsl"))  return ShaderLanguage::wgsl;
    return {};
}

static String stageToString (ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::vertex:       return "vertex";
        case ShaderStage::fragment:     return "fragment";
        case ShaderStage::compute:      return "compute";
        case ShaderStage::geometry:     return "geometry";
        case ShaderStage::tessControl:  return "tessControl";
        case ShaderStage::tessEval:     return "tessEval";
    }
    return "unknown";
}

static std::optional<ShaderStage> stageFromString (const String& str)
{
    if (str.equalsIgnoreCase ("vertex"))      return ShaderStage::vertex;
    if (str.equalsIgnoreCase ("fragment"))    return ShaderStage::fragment;
    if (str.equalsIgnoreCase ("compute"))     return ShaderStage::compute;
    if (str.equalsIgnoreCase ("geometry"))    return ShaderStage::geometry;
    if (str.equalsIgnoreCase ("tesscontrol")) return ShaderStage::tessControl;
    if (str.equalsIgnoreCase ("tesseval"))    return ShaderStage::tessEval;
    return {};
}

//==============================================================================
static void printReflectionInfo (const ShaderInfo& info)
{
    const auto& r = info.reflection;
    Logger* log = Logger::getCurrentLogger();

    log->writeToLog ("Stage: " + stageToString (info.stage)
                   + "  Language: " + languageToString (info.language)
                   + "  Entry: " + info.entryPoint);

    auto printBindings = [&] (const String& title, const std::vector<ShaderReflection::ResourceBinding>& list)
    {
        if (list.empty())
            return;
        log->writeToLog ("  " + title + ":");
        for (const auto& b : list)
            log->writeToLog ("    " + b.name + "  binding=" + String (b.binding)
                           + " set=" + String (b.set)
                           + " location=" + String (b.location)
                           + (b.backendSlot != ~0u ? " backendSlot=" + String (b.backendSlot) : ""));
    };

    printBindings ("UniformBuffers", r.uniformBuffers);
    printBindings ("StorageBuffers", r.storageBuffers);
    printBindings ("StageInputs", r.stageInputs);
    printBindings ("StageOutputs", r.stageOutputs);
    printBindings ("SampledImages", r.sampledImages);
    printBindings ("StorageImages", r.storageImages);
    printBindings ("SubpassInputs", r.subpassInputs);
    printBindings ("AtomicCounters", r.atomicCounters);
    printBindings ("GlPlainUniforms", r.glPlainUniforms);
    printBindings ("PushConstantBuffers", r.pushConstantBuffers);
    printBindings ("AccelerationStructures", r.accelerationStructures);
    printBindings ("SeparateImages", r.separateImages);
    printBindings ("SeparateSamplers", r.separateSamplers);

    if (! r.glCombinedSamplers.empty())
    {
        log->writeToLog ("  GLCombinedSamplers:");
        for (const auto& cs : r.glCombinedSamplers)
            log->writeToLog ("    " + cs.name + "  textureSlot=" + String (cs.textureSlot));
    }

    if (info.stage == ShaderStage::compute)
        log->writeToLog ("  WorkgroupSize: " + String (r.workgroupSize.x)
                       + "x" + String (r.workgroupSize.y)
                       + "x" + String (r.workgroupSize.z));

    log->writeToLog ("");
}

//==============================================================================
static void printUsage()
{
    Logger::getCurrentLogger()->writeToLog (
        "Usage: yup_shader_bundler [options]\n"
        "\n"
        "Compile mode:\n"
        "  --stage <stage> <path>  Pipeline stage and source file (repeatable)\n"
        "                          Stages: vertex, fragment, compute, geometry,\n"
        "                          tesscontrol, tesseval\n"
        "  --vert <path>           Shorthand for --stage vertex <path>\n"
        "  --frag <path>           Shorthand for --stage fragment <path>\n"
        "  --output <path>         Output .ysl bundle file path\n"
        "\n"
        "Source options:\n"
        "  --source-lang <lang>    Source language: glsl, hlsl, essl (default: glsl)\n"
        "  --target-langs <list>   Comma-separated target languages\n"
        "                          (default: glsl,essl,hlsl,msl)\n"
        "  --entry <name>          Entry point name (default: main)\n"
        "\n"
        "Compilation options:\n"
        "  --glsl-version <n>      GLSL version (default: 450)\n"
        "  --es                    Emit OpenGL ES-style GLSL\n"
        "  --hlsl-model <n>        HLSL shader model (e.g. 50, 60; default: 50)\n"
        "  -DNAME[=VALUE]          Preprocessor define (repeatable)\n"
        "  -I<dir>                 Include search directory (repeatable)\n"
        "\n"
        "SPIR-V options:\n"
        "  --spirv-opt <mode>      Optimization: none, size, perf (default: none)\n"
        "  --spirv-validate        Run SPIR-V validation after compilation\n"
        "  --spirv-debug           Emit debug info in SPIR-V binary\n"
        "\n"
        "MSL options:\n"
        "  --msl-fbfetch           Enable framebuffer fetch for subpass inputs\n"
        "  --flip-vert-y           Flip vertex Y coordinate\n"
        "\n"
        "Inspect mode:\n"
        "  --inspect <path>        Inspect an existing .ysl bundle file\n"
        "  --print <lang>          Print shader source for the given language\n"
        "                          (glsl, essl, hlsl, msl, spirv, wgsl, all)\n"
        "                          Can be repeated to print multiple languages\n"
        "  --stage <stage>         Filter by pipeline stage (vertex, fragment,\n"
        "                          compute, geometry, tesscontrol, tesseval, all)\n"
        "                          Can be repeated. Default: all stages\n"
        "  --list                  List all variants with source lengths\n"
        "  --info                  Print reflection data for matched variants\n"
        "\n"
        "  --help|-h               Print this help\n");
}

//==============================================================================
static std::optional<SpvOptimizationMode> spirvOptFromString (const String& str)
{
    if (str.equalsIgnoreCase ("none")) return SpvOptimizationMode::none;
    if (str.equalsIgnoreCase ("size")) return SpvOptimizationMode::size;
    if (str.equalsIgnoreCase ("perf")) return SpvOptimizationMode::performance;
    return {};
}

//==============================================================================
static int runCompileMode (const ArgumentList& args)
{
    args.failIfOptionIsMissing ("--output");

    const auto resolveFile = [] (const String& path) -> File
    {
        if (path.isEmpty())
            return {};
        if (File::isAbsolutePath (path))
            return File (path);
        return File::getCurrentWorkingDirectory().getChildFile (path);
    };

    Logger* log = Logger::getCurrentLogger();

    // -- Collect the requested pipeline stages
    std::vector<std::pair<ShaderStage, File>> stages;

    if (args.containsOption ("--vert"))
        stages.emplace_back (ShaderStage::vertex, resolveFile (getOptionValue (args, "--vert")));

    if (args.containsOption ("--frag"))
        stages.emplace_back (ShaderStage::fragment, resolveFile (getOptionValue (args, "--frag")));

    // General form: --stage <stage> <path>
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i] == "--stage" && i + 2 < args.size()
            && ! args[i + 1].isOption() && ! args[i + 2].isOption())
        {
            auto st = stageFromString (args[i + 1].text);
            if (st.has_value())
                stages.emplace_back (*st, resolveFile (args[i + 2].text));
            else
                log->writeToLog ("Warning: unknown stage '" + args[i + 1].text + "', skipping.");

            i += 2;
        }
    }

    if (stages.empty())
    {
        log->writeToLog ("No shader stages specified. Use --vert, --frag, or --stage <stage> <path>.");
        return 1;
    }

    const auto outputPath = resolveFile (getOptionValue (args, "--output"));

    // -- Source language
    ShaderLanguage sourceLanguage = ShaderLanguage::glsl;
    if (args.containsOption ("--source-lang"))
    {
        const String slVal = getOptionValue (args, "--source-lang");
        auto sl = languageFromString (slVal);
        if (! sl.has_value())
        {
            log->writeToLog ("Unknown source language: " + slVal);
            return 1;
        }
        sourceLanguage = *sl;
    }

    // -- Target languages
    std::vector<ShaderLanguage> targetLanguages;
    if (args.containsOption ("--target-langs"))
    {
        const StringArray parts = StringArray::fromTokens (getOptionValue (args, "--target-langs"), ",", "");
        for (const auto& p : parts)
        {
            const String trimmed = p.trim();
            if (trimmed.isEmpty())
                continue;

            auto tl = languageFromString (trimmed);
            if (tl.has_value())
                targetLanguages.push_back (*tl);
            else
                log->writeToLog ("Warning: unknown target language '" + trimmed + "', skipping.");
        }
    }

    if (targetLanguages.empty())
        targetLanguages = { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl };

    // -- Shared transpile options
    TranspileOptions options;
    options.entryPoint = getOptionValue (args, "--entry", "main");
    options.glslVersion = getOptionValue (args, "--glsl-version", "450").getIntValue();
    options.es = args.containsOption ("--es");
    options.mslUsesFramebufferFetch = args.containsOption ("--msl-fbfetch");
    options.flipVertY = args.containsOption ("--flip-vert-y");
    options.spirvValidate = args.containsOption ("--spirv-validate");
    options.spirvDebugInfo = args.containsOption ("--spirv-debug");

    if (args.containsOption ("--hlsl-model"))
        options.hlslShaderModel = getOptionValue (args, "--hlsl-model").getIntValue();

    if (args.containsOption ("--spirv-opt"))
    {
        const String optVal = getOptionValue (args, "--spirv-opt");
        auto mode = spirvOptFromString (optVal);
        if (! mode.has_value())
        {
            log->writeToLog ("Unknown SPIR-V optimization mode: " + optVal);
            return 1;
        }
        options.spirvOptimization = *mode;
    }

    for (const auto& d : getPrefixedValues (args, "-D"))
    {
        const int eq = d.indexOfChar ('=');
        if (eq >= 0)
            options.defines.set (d.substring (0, eq), d.substring (eq + 1));
        else
            options.defines.set (d, {});
    }

    for (const auto& inc : getPrefixedValues (args, "-I"))
        options.includePaths.push_back (resolveFile (inc).getFullPathName());

    // -- Compile every stage and merge into a single bundle
    ShaderBundleCompiler compiler;
    ShaderBundle bundle;

    for (const auto& [stage, path] : stages)
    {
        if (! path.existsAsFile())
        {
            log->writeToLog (stageToString (stage) + " shader file not found: " + path.getFullPathName());
            return 1;
        }

        const String source = path.loadFileAsString();
        if (source.isEmpty())
        {
            log->writeToLog (stageToString (stage) + " shader file is empty: " + path.getFullPathName());
            return 1;
        }

        ShaderBundleEntry entry;
        entry.stage = stage;
        entry.targetLanguages = targetLanguages;
        entry.options = options;

        ShaderBundleCompileRequest request;
        request.source = source;
        request.sourceLanguage = sourceLanguage;
        request.entries.push_back (entry);

        auto result = compiler.compile (request);
        if (result.failed())
        {
            log->writeToLog (stageToString (stage) + " shader compilation failed: " + result.getErrorMessage());
            return 1;
        }

        for (const auto& info : result.getReference().getShaders())
            bundle.addShader (info);
    }

    // Persist to file
    const auto saveResult = bundle.saveToFile (outputPath);
    if (saveResult.failed())
    {
        log->writeToLog ("Failed to save bundle: " + saveResult.getErrorMessage());
        return 1;
    }

    log->writeToLog ("Shader bundle written to: " + outputPath.getFullPathName());
    return 0;
}

//==============================================================================
static int runInspectMode (const ArgumentList& args)
{
    const auto resolveFile = [] (const String& path) -> File
    {
        if (path.isEmpty())
            return {};
        if (File::isAbsolutePath (path))
            return File (path);
        return File::getCurrentWorkingDirectory().getChildFile (path);
    };

    const auto inspectPath = resolveFile (getOptionValue (args, "--inspect"));

    if (inspectPath == File() || ! inspectPath.existsAsFile())
    {
        const String msg = "Bundle file not found: " + inspectPath.getFullPathName();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    auto loaded = ShaderBundle::loadFromFile (inspectPath);
    if (loaded.failed())
    {
        const String msg = "Failed to load bundle: " + loaded.getErrorMessage();
        Logger::getCurrentLogger()->writeToLog (msg);
        return 1;
    }

    const ShaderBundle& bundle = loaded.getReference();
    Logger* log = Logger::getCurrentLogger();

    // Parse --print filters
    const StringArray printValues = getAllOptionValues (args, "--print");
    std::optional<std::vector<ShaderLanguage>> printLanguages;
    bool printAll = false;

    if (! printValues.isEmpty())
    {
        printLanguages.emplace();
        for (const auto& v : printValues)
        {
            if (v.equalsIgnoreCase ("all"))
            {
                printAll = true;
                break;
            }
            auto lang = languageFromString (v);
            if (lang.has_value())
                printLanguages->push_back (*lang);
            else
                log->writeToLog ("Warning: unknown language '" + v + "', skipping.");
        }
    }

    // Parse --stage filters
    const StringArray stageValues = getAllOptionValues (args, "--stage");
    std::optional<std::vector<ShaderStage>> stageFilters;
    bool stageAll = false;

    if (! stageValues.isEmpty())
    {
        stageFilters.emplace();
        for (const auto& v : stageValues)
        {
            if (v.equalsIgnoreCase ("all"))
            {
                stageAll = true;
                break;
            }
            auto stage = stageFromString (v);
            if (stage.has_value())
                stageFilters->push_back (*stage);
            else
                log->writeToLog ("Warning: unknown stage '" + v + "', skipping.");
        }
    }

    const bool doList = args.containsOption ("--list");
    const bool doInfo = args.containsOption ("--info");

    // If no action specified, default to listing
    if (printValues.isEmpty() && ! doList && ! doInfo)
    {
        log->writeToLog ("Bundle: " + inspectPath.getFullPathName());
        log->writeToLog ("Variants: " + String ((int) bundle.getShaders().size()));
        log->writeToLog ("");

        for (const auto& shader : bundle.getShaders())
        {
            log->writeToLog ("  " + stageToString (shader.stage)
                           + " :: " + languageToString (shader.language)
                           + "  entry=" + shader.entryPoint
                           + "  source=" + String ((int) shader.source.length()) + " bytes");
        }
        return 0;
    }

    const bool shouldPrint = ! printValues.isEmpty();

    // Helper: check if a shader matches filters
    auto matches = [&] (const ShaderInfo& shader) -> bool
    {
        if (stageFilters.has_value() && ! stageAll)
        {
            bool found = false;
            for (auto s : *stageFilters)
            {
                if (shader.stage == s)
                {
                    found = true;
                    break;
                }
            }
            if (! found)
                return false;
        }

        if (printLanguages.has_value() && ! printAll && shouldPrint)
        {
            bool found = false;
            for (auto l : *printLanguages)
            {
                if (shader.language == l)
                {
                    found = true;
                    break;
                }
            }
            if (! found)
                return false;
        }

        return true;
    };

    for (const auto& shader : bundle.getShaders())
    {
        if (! matches (shader))
            continue;

        if (doList)
        {
            log->writeToLog (stageToString (shader.stage)
                           + " :: " + languageToString (shader.language)
                           + "  entry=" + shader.entryPoint
                           + "  source=" + String ((int) shader.source.length()) + " bytes");
        }

        if (doInfo)
            printReflectionInfo (shader);

        if (shouldPrint)
        {
            log->writeToLog ("// ==== " + stageToString (shader.stage)
                           + " :: " + languageToString (shader.language)
                           + "  entry=" + shader.entryPoint + " ====");
            log->writeToLog (shader.source);
            log->writeToLog ("// ==== END ====");
            log->writeToLog ("");
        }
    }

    return 0;
}

//==============================================================================
int main (int argc, char* argv[])
{
    ArgumentList args (argc, argv);

    if (args.containsOption ("--help|-h"))
    {
        printUsage();
        return 0;
    }

    if (args.containsOption ("--inspect"))
        return runInspectMode (args);

    if (args.containsOption ("--vert") || args.containsOption ("--frag")
        || args.containsOption ("--stage") || args.containsOption ("--output"))
        return runCompileMode (args);

    printUsage();
    return 1;
}
