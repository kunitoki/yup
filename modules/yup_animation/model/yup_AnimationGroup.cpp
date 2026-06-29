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

AnimationGroup* AnimationGroup::addGroup()
{
    auto g = new AnimationGroup();
    AnimationGroup* raw = g;
    ChildItem item;
    item.kind = ChildKind::Group;
    item.group = g;
    children.push_back (std::move (item));
    return raw;
}

FillPaint* AnimationGroup::addFill()
{
    auto f = new FillPaint();
    FillPaint* raw = f;
    ChildItem item;
    item.kind = ChildKind::Fill;
    item.fill = f;
    children.push_back (std::move (item));
    return raw;
}

StrokePaint* AnimationGroup::addStroke()
{
    auto s = new StrokePaint();
    StrokePaint* raw = s;
    ChildItem item;
    item.kind = ChildKind::Stroke;
    item.stroke = s;
    children.push_back (std::move (item));
    return raw;
}

AnimationTrim* AnimationGroup::addTrim()
{
    auto t = new AnimationTrim();
    AnimationTrim* raw = t;
    ChildItem item;
    item.kind = ChildKind::Trim;
    item.trim = t;
    children.push_back (std::move (item));
    return raw;
}

AnimationRepeater* AnimationGroup::addRepeater()
{
    auto r = new AnimationRepeater();
    AnimationRepeater* raw = r;
    ChildItem item;
    item.kind = ChildKind::Repeater;
    item.repeater = r;
    children.push_back (std::move (item));
    return raw;
}

} // namespace yup
