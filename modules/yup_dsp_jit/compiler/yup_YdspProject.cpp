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

namespace yup
{

ResultValue<YdspProject> YdspProject::parse (StringRef text, const File& projectFile, YdspDiagnostics& diagnostics)
{
    const auto source = String (text);
    diagnostics.setSource (source, projectFile.getFullPathName());
    const auto fail = [&] (StringRef message, int line = 1) -> ResultValue<YdspProject>
    {
        diagnostics.addError ({ line, 1, line, 1 }, message);
        return makeResultValueFail (diagnostics.toString());
    };

    YdspProject project;
    project.file = projectFile;
    const auto parsed = YAML::parse (source, project.metadata);
    if (parsed.failed())
    {
        const auto message = parsed.getErrorMessage();
        return fail (message.fromFirstOccurrenceOf (": error: ", false, false), jmax (1, message.getIntValue()));
    }

    const auto* object = project.metadata.getDynamicObject();
    if (object == nullptr)
        return fail ("The project manifest must be a YAML mapping");

    const auto& properties = object->getProperties();
    for (int i = 0; i < properties.size(); ++i)
    {
        const auto key = properties.getName (i).toString();
        const auto& value = properties.getValueAt (i);
        if (key == "formatVersion" || key == "sources" || key == "main")
            continue;
        if (key == "isInstrument")
        {
            if (! value.isBool())
                return fail ("Project property 'isInstrument' must be a boolean");
            continue;
        }
        if (key != "id" && key != "name" && key != "version" && key != "description"
            && key != "manufacturer" && key != "author" && key != "license" && key != "category")
            return fail ("Unknown project property '" + key + "'");
        if (! value.isString())
            return fail ("Project property '" + key + "' must be a string (quote numeric versions)");
    }

    const auto& version = project.metadata["formatVersion"];
    if ((! version.isInt() && ! version.isInt64()) || static_cast<int64> (version) != 1)
        return fail ("Project 'formatVersion' must be the integer 1");

    const auto& main = project.metadata["main"];
    if (! main.isString() || main.toString().trim().isEmpty())
        return fail ("Project 'main' must name a processor or graph");
    project.main = main.toString().trim();

    const auto* sources = project.metadata["sources"].getArray();
    if (sources == nullptr || sources->isEmpty())
        return fail ("Project 'sources' must be a nonempty list of .ydsp file paths");

    StringArray resolvedPaths;
    for (const auto& item : *sources)
    {
        if (! item.isString() || item.toString().trim().isEmpty())
            return fail ("Each project source must be a nonempty .ydsp file path");
        const auto path = item.toString();
        const auto resolved = projectFile.getParentDirectory().getChildFile (path);
        if (! resolved.hasFileExtension (".ydsp"))
            return fail ("Project source must have the .ydsp extension: '" + path + "'");
        if (resolvedPaths.contains (resolved.getFullPathName()))
            return fail ("Duplicate project source: '" + path + "'");
        resolvedPaths.add (resolved.getFullPathName());
        project.sources.add (path);
    }

    return makeResultValueOk (std::move (project));
}

ResultValue<YdspProject> YdspProject::load (const File& projectFile, YdspDiagnostics& diagnostics)
{
    FileInputStream stream (projectFile);
    if (stream.failedToOpen())
    {
        diagnostics.setSource ({}, projectFile.getFullPathName());
        diagnostics.addError ({}, "Cannot read project manifest");
        return makeResultValueFail (diagnostics.toString());
    }
    return parse (stream.readEntireStreamAsString(), projectFile, diagnostics);
}

} // namespace yup
