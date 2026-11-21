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

#include <yup_gui/yup_gui.h>

#include <functional>

//==============================================================================
/** The answer of a tool: text for an agent to read, with an image when the tool gives one. */
struct YuiAgentResult
{
    /** What the tool found, as JSON, or why it could not do what it was asked when `isError` is set. */
    yup::String text;

    /** A PNG, empty when the tool gives no image. */
    yup::MemoryBlock image;

    bool isError = false;

    /** Returns the answer of a tool that could not do what it was asked. */
    static YuiAgentResult fail (const yup::String& reason);
};

//==============================================================================
/** What the tools get from the program hosting them. */
struct YuiAgentHost
{
    /** The folder a file is looked for in when its path is relative. */
    yup::File baseFolder;

    /** Draws a component into an image, invalid when there is nothing to draw with. Null in a host that never draws. */
    std::function<yup::Image (yup::Component& component, double scale)> snapshot;

    /** Captures the interface the host shows live. Null in a host that shows none. */
    std::function<YuiAgentResult (const yup::var& arguments)> capturePreview;

    /** True when the tools write the image of a call to the file it names as `output`. A host
        with no disk leaves it false and writes the image of the answer itself.
    */
    bool savesImages = false;
};

//==============================================================================
/** The tools an agent writing `.yui` files checks its work with.

    They are the same in every host: the tool serving them itself over the Model Context Protocol,
    and the module an editor extension calls into.
*/

/** Returns how the tools are meant to be used, for the agent they are given to. */
yup::String getYuiAgentInstructions();

/** Returns the tools, each an object with its `name`, `description` and `inputSchema`.

    @param withPreview True to list the tool capturing what a host shows live.
*/
yup::var getYuiAgentTools (bool withPreview);

/** Returns true for a tool that draws, which a host with nothing to draw with can't run. */
bool doesYuiAgentToolDraw (const yup::String& name);

/** Runs a tool with the arguments it was called with, an object. */
YuiAgentResult callYuiAgentTool (const yup::String& name, const yup::var& arguments, const YuiAgentHost& host);

/** Reads the `scale` a tool that draws was called with, which is 1 when it was given none.

    @param isPart True when the call draws a part of an interface, which may be drawn larger than all of it.
*/
yup::Result readYuiAgentScale (const yup::var& arguments, double& scale, bool isPart = false);
