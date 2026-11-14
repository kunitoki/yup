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

YuiView::YuiView (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
    document.addListener (this);
}

YuiView::~YuiView()
{
    document.removeListener (this);
}

//==============================================================================
void YuiView::resized()
{
    if (auto* root = document.getRoot())
        root->setBounds (getLocalBounds());
}

void YuiView::documentLoaded (YuiDocument&, const Result&)
{
    auto* root = document.getRoot();

    if (root == nullptr)
        return;

    if (root->getParentComponent() != this)
        addAndMakeVisible (*root);

    resized();
}

} // namespace yup
