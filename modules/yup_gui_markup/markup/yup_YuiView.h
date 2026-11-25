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

namespace yup
{

//==============================================================================
/**
    A component showing the interface of a YuiDocument.

    The view owns a document and a loader filling it. After each load, the root of the
    document is a visible child of the view filling its bounds, so the interface follows the
    size, the visibility and the enabled state of the view, and a root replaced by a reload
    takes the place of the previous one. The view paints nothing itself and isn't opaque.

    @code
    YuiView view;
    view.getDocument().setModel ("params", paramsProvider);
    view.getDocument().setAction ("reset", [this] (const Array<var>&) { reset(); });
    view.getLoader().loadFile (sourceFolder.getChildFile ("main.yui"));
    addAndMakeVisible (view);
    @endcode

    Models given to the document must outlive the view.

    @see YuiDocument, YuiLoader

    @tags{UI}
*/
class YUP_API YuiView
    : public Component
    , private YuiDocument::Listener
{
public:
    //==============================================================================
    /** Creates an empty view. */
    explicit YuiView (StringRef componentID = {});

    /** Destroys the view, its document and the interface. */
    ~YuiView() override;

    //==============================================================================
    /** Returns the document whose interface the view shows. */
    YuiDocument& getDocument() noexcept { return document; }

    /** Returns the loader filling the document. */
    YuiLoader& getLoader() noexcept { return loader; }

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    //==============================================================================
    void documentLoaded (YuiDocument& loadedDocument, const Result& result) override;

    YuiDocument document;
    YuiLoader loader { document };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiView)
};

} // namespace yup
