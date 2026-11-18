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

#include "yup_YuiToolSupport.h"

#include <algorithm>
#include <iostream>

using namespace yup;

//==============================================================================

bool YuiToolModels::load (const String& path)
{
    const auto file = File::getCurrentWorkingDirectory().getChildFile (path);

    if (! file.existsAsFile())
    {
        std::cerr << "can't read '" << path << "'\n";
        return false;
    }

    const auto fail = [&file] (const String& reason)
    {
        std::cerr << file.getFullPathName() << ": error: " << reason << "\n";
        return false;
    };

    var parsed;

    if (const auto result = JSON::parse (file.loadFileAsString(), parsed); result.failed())
        return fail (result.getErrorMessage());

    const auto* declared = parsed.getDynamicObject();

    if (declared == nullptr)
        return fail ("expected an object with a member for each model");

    for (const auto& model : declared->getProperties())
    {
        const auto* properties = model.value.getDynamicObject();

        if (properties == nullptr)
            return fail ("model '" + model.name.toString() + "' must be an object of properties and sample values");

        DataTree tree ("model");

        {
            auto transaction = tree.beginTransaction();

            for (const auto& property : properties->getProperties())
                transaction.setProperty (property.name, property.value);
        }

        names.add (model.name.toString());
        providers.push_back (std::make_unique<DataTreeBindingProvider> (tree));
    }

    return true;
}

void YuiToolModels::attachTo (YuiDocument& document) const
{
    for (int index = 0; index < names.size(); ++index)
        document.setModel (names[index], *providers[static_cast<std::size_t> (index)]);
}

//==============================================================================

void YuiToolProblem::print() const
{
    std::cerr << file.getFullPathName();

    if (line > 0)
        std::cerr << ":" << line << ":" << column;

    std::cerr << ": " << (isError ? "error" : "warning") << ": " << message << "\n";
}

var YuiToolProblem::toJson() const
{
    auto* record = new DynamicObject();
    record->setProperty ("file", file.getFullPathName());
    record->setProperty ("line", line);
    record->setProperty ("column", column);
    record->setProperty ("severity", isError ? "error" : "warning");
    record->setProperty ("message", message);

    return var (record);
}

std::vector<YuiToolProblem> findYuiToolProblems (const YuiDocument& document, const File& file, const Result& loadResult)
{
    std::vector<YuiToolProblem> problems;

    for (const auto& diagnostic : document.getDiagnostics())
    {
        problems.push_back ({ .file = diagnostic.source.isEmpty() ? file : file.getParentDirectory().getChildFile (diagnostic.source),
                              .line = diagnostic.line,
                              .column = diagnostic.column,
                              .isError = diagnostic.isError(),
                              .message = diagnostic.message });
    }

    const auto hasError = std::any_of (problems.begin(), problems.end(), [] (const YuiToolProblem& problem)
    {
        return problem.isError;
    });

    if (loadResult.failed() && ! hasError)
        problems.push_back ({ .file = file, .message = loadResult.getErrorMessage() });

    return problems;
}

int findYuiToolRootIndex (const DataTree& document)
{
    for (int index = 0; index < document.getNumChildren(); ++index)
    {
        const auto typeName = document.getChild (index).getType().toString();

        if (CharacterFunctions::isUpperCase (static_cast<yup_wchar> (typeName[0])))
            return index;
    }

    return -1;
}

//==============================================================================

namespace
{

// The names of the drawings or of the palettes a document declares, in a style block too
StringArray findDeclared (const DataTree& document, const Identifier& type)
{
    static const Identifier name ("name");
    static const Identifier style ("style");

    StringArray names;

    for (const auto& element : document)
    {
        if (element.getType() == type)
            names.add (element.getProperty (name).toString());

        if (element.getType() != style)
            continue;

        for (const auto& nested : element)
            if (nested.getType() == type)
                names.add (nested.getProperty (name).toString());
    }

    return names;
}

// A few widgets and a swatch of each role over the background, in the order of ThemePalette::Role
String makePaletteSample (const String& palette)
{
    String swatches;

    for (const auto& role : ThemePalette::getRoleNames())
        if (role != "background")
            swatches << "            Window { backgroundColor: palette." << role << "; flexGrow: 1; }\n";

    return String (R"yui(
Window { palette: NAME; backgroundColor: palette.background;
    Column { padding: 16; gap: 10;
        Label { text: "NAME"; preferredHeight: 24; }
        Row { gap: 10; preferredHeight: 32;
            TextButton { text: "Button"; flexGrow: 1; }
            ToggleButton { text: "Toggle"; checked: true; flexGrow: 1; }
        }
        Slider { value: 0.6; preferredHeight: 28; }
        ProgressBar { progress: 0.4; preferredHeight: 16; }
        TextEditor { text: "Text"; preferredHeight: 32; }
        Row { gap: 6; flexGrow: 1;
SWATCHES        }
    }
})yui")
        .replace ("NAME", palette)
        .replace ("SWATCHES", swatches);
}

// Gives a parsed document another root component, the one hosting what is previewed
DataTree withRoot (const DataTree& document, const DataTree& root)
{
    auto tree = document.clone();
    const auto previousRoot = findYuiToolRootIndex (tree);

    {
        auto transaction = tree.beginTransaction();

        if (previousRoot >= 0)
            transaction.removeChild (previousRoot);

        transaction.addChild (root.clone());
    }

    return tree;
}

} // namespace

YuiToolShown loadYuiToolTarget (YuiDocument& document, YuiLoader& loader, const File& input, YuiToolTarget target)
{
    YuiToolShown shown;
    shown.result = loader.loadFile (input);

    const auto parsed = YuiParser::parse (loader.getLoadedText (input.getFileName()).value_or (String()), [&loader] (const String& path)
    {
        return loader.getLoadedText (path);
    });

    const auto nothing = [&shown, &target] (const String& reason)
    {
        shown.status = YuiToolShown::Status::nothing;
        shown.message = reason;
        shown.target = target;
        return shown;
    };

    if (parsed.wasOk())
    {
        const auto hasRoot = findYuiToolRootIndex (parsed.document) >= 0;
        const auto isPreview = target.drawing.isNotEmpty() || target.palette.isNotEmpty() || target.hostType.isNotEmpty();
        const auto drawings = findDeclared (parsed.document, "drawing");
        const auto palettes = findDeclared (parsed.document, "palette");

        if (target.drawing.isNotEmpty() && ! drawings.contains (target.drawing))
            return nothing ("there is no drawing '" + target.drawing + "'");

        if (target.palette.isNotEmpty() && ! palettes.contains (target.palette))
            return nothing ("there is no palette '" + target.palette + "'");

        // A file of declarations shows the first thing it declares that can be drawn
        if (! hasRoot && ! isPreview)
        {
            target.drawing = drawings[0];

            if (target.drawing.isEmpty())
                target.palette = palettes[0];
        }

        String hostText;

        if (target.palette.isNotEmpty())
        {
            hostText = makePaletteSample (target.palette);
            shown.defaultSize = { 360.0f, 300.0f };
        }
        else if (target.drawing.isNotEmpty() || target.hostType.isNotEmpty())
        {
            hostText = (target.hostType.isNotEmpty() ? target.hostType : String ("Component")) + " { " + (target.drawing.isNotEmpty() ? "drawing: " + target.drawing + "; " : String()) + "}";
            shown.defaultSize = { 200.0f, 200.0f };
        }
        else if (! hasRoot)
        {
            return nothing ("the file has no component, drawing or palette");
        }

        if (hostText.isNotEmpty())
        {
            const auto host = YuiParser::parse (hostText);

            if (! host.wasOk() || host.document.getNumChildren() != 1)
            {
                shown.status = YuiToolShown::Status::invalidName;
                shown.target = target;
                return shown;
            }

            // The positions of the text only hold while its elements keep their place
            shown.result = document.load (withRoot (parsed.document, host.document.getChild (0)), hasRoot ? YuiSourceMap() : parsed.sourceMap);
        }
    }

    shown.target = target;
    return shown;
}
