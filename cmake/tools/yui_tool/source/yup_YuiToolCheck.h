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

#pragma once

#include "yup_YuiToolModels.h"

#include <yup_gui_markup/yup_gui_markup.h>

#include <optional>
#include <vector>

//==============================================================================
/** A problem of a document, in the file it was found in. */
struct YuiToolProblem
{
    yup::File file;
    int line = 0;
    int column = 0;
    bool isError = true;
    yup::String message;

    /** Prints the problem as `path:line:column: error: message`. */
    void print() const;

    /** Returns the problem as an object with its file, line, column, severity and message. */
    yup::var toJson() const;
};

/** Returns the problems of a document loaded from a file, the failure of the load included. */
std::vector<YuiToolProblem> findYuiToolProblems (const yup::YuiDocument& document, const yup::File& file, const yup::Result& loadResult);

#if YUP_WASM
/** Notes the files a document names that the tool was not given, in the tool built for the web.

    That tool is a module reading the files its host gave it in memory. It calls this with each
    document it loaded from one of them, and the host then asks for the files that document reads
    beside its texts and that are not there, to give those it has. It is defined with the entry
    points of the module, see web/main.cpp.
*/
void noteYuiToolMissingFiles (yup::YuiDocument& document);
#endif

/** The files a build embeds for the documents an application loads with YuiLoader::loadResource. */
struct YuiToolEmbedded
{
    /** The folder the paths of the files are relative to. */
    yup::File base;

    /** The paths of the files from that folder, separated by slashes. A shader is listed as the
        source its document names, which stands for the bundles a build compiles from it.
    */
    yup::StringArray paths;
};

/** Returns the problems of a file, checked as `verify` and `lint` check it.

    What a file with an interface needs is checked whether it is used or not: every shader it
    declares is loaded, from its source where the tool compiles shaders, and its scenes run the
    frame a window showing them starts with, where they build what they left to it. A file of
    declarations is checked without them: what it needs is read from the folder of the file
    importing it, so it is checked with that file.

    @param unsavedText The text to check in place of the one of the file, when given.
    @param embedded    The files a build embeds, to check the file as an application loads it from
                       them: as the resource of its path, reading nothing but the files listed.
                       What it reads that is in their folder without being listed is a problem,
                       and so is a file that is not one of them. Null to read the folder of the file.
*/
std::vector<YuiToolProblem> findYuiToolFileProblems (const yup::File& file,
                                                     const std::optional<yup::String>& unsavedText,
                                                     const YuiToolModels& models,
                                                     const YuiToolEmbedded* embedded = nullptr);

/** Loads a file into a document as it is checked, from its folder: what only declares as an empty interface, and
    with what an interface needs checked whether it is used or not. The problems are then those of the document.
*/
yup::Result loadYuiToolFileForChecking (yup::YuiDocument& document, yup::YuiLoader& loader, const yup::File& file);

/** Returns the index of the root component of a parsed document, or -1 when it only holds declarations. */
int findYuiToolRootIndex (const yup::DataTree& document);
