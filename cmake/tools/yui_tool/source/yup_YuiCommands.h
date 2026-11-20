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

#include <optional>

/** Returns the exit code of a command line that names no command to run, after printing the usage.

    @returns nothing when `argv[1]` names a command, which @ref runYuiToolCommand then runs.
*/
std::optional<int> checkYuiToolCommand (int argc, char** argv);

/** Runs the command `argv[1]` names, once the application is set up as one with a theme is. */
int runYuiToolCommand (int argc, char** argv);

/** Checks `.yui` files: `verify` reports the errors, `lint` the warnings too. */
int runYuiVerifyCommand (int argc, char** argv, bool includeWarnings);

/** Writes the registered component types, their properties and signals as JSON. */
int runYuiSchemaCommand (int argc, char** argv);

/** Writes an SVG as a `.yui` drawing. */
int runYuiImportSvgCommand (int argc, char** argv);

/** Draws a document, or a drawing or a palette it declares, into an image. */
int runYuiRenderCommand (int argc, char** argv);
