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

//==============================================================================
/** A node in the Lottie shape content tree.

    Groups are the primary structural unit of shape layers. They form an arbitrary-depth
    tree. Children are stored in an ordered list matching Lottie draw order:
    shapes preceding a paint define which paths that paint applies to.

    All child objects are reference-counted and owned via Ptr.
*/
class YUP_API AnimationGroup : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationGroup>;

    //==============================================================================
    enum class ChildKind
    {
        Shape,
        Group,
        Fill,
        Stroke,
        Trim,
        Repeater,
        RoundedCorner
    };

    struct ChildItem
    {
        ChildKind kind;
        AnimationShape::Ptr shape;
        AnimationGroup::Ptr group;
        FillPaint::Ptr fill;
        StrokePaint::Ptr stroke;
        AnimationTrim::Ptr trim;
        AnimationRepeater::Ptr repeater;
        AnimationRoundedCorner::Ptr roundedCorner;
    };

    //==============================================================================
    AnimationGroup() = default;

    //==============================================================================
    String name;
    bool hidden = false;
    BlendMode blendMode = BlendMode::SrcOver;
    AnimationTransform transform;

    /** Children in Lottie draw order (index 0 = bottom). */
    std::vector<ChildItem> children;

    //==============================================================================
    /** Adds a shape child and returns a raw (non-owning) pointer to it. */
    template <typename ShapeT>
    ShapeT* addShape()
    {
        auto shape = new ShapeT();
        ShapeT* raw = shape;
        ChildItem item;
        item.kind = ChildKind::Shape;
        item.shape = shape;
        children.push_back (std::move (item));
        return raw;
    }

    /** Adds a nested group and returns a raw pointer to it. */
    AnimationGroup* addGroup();

    /** Adds a FillPaint and returns a raw pointer. */
    FillPaint* addFill();

    /** Adds a StrokePaint and returns a raw pointer. */
    StrokePaint* addStroke();

    /** Adds an AnimationTrim modifier and returns a raw pointer. */
    AnimationTrim* addTrim();

    /** Adds an AnimationRepeater modifier and returns a raw pointer. */
    AnimationRepeater* addRepeater();

    /** Adds an AnimationRoundedCorner modifier and returns a raw pointer. */
    AnimationRoundedCorner* addRoundedCorner();

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationGroup)
};

} // namespace yup
