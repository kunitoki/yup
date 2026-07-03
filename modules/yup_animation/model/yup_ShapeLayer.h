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
/** A Lottie shape layer containing a tree of AnimationGroup content.

    Groups are stored in Lottie draw order (index 0 = bottommost).
    The renderer walks them back-to-front.
*/
class YUP_API ShapeLayer : public AnimationLayer
{
public:
    using Ptr = ReferenceCountedObjectPtr<ShapeLayer>;

    ShapeLayer() = default;

    [[nodiscard]] Type getType() const noexcept override { return Type::Shape; }

    //==============================================================================
    std::vector<AnimationGroup::Ptr> groups;

    //==============================================================================
    /** Appends a new group and returns a raw (non-owning) pointer. */
    AnimationGroup* addGroup (const String& name = {});

    /** Returns the number of top-level groups in this layer. */
    [[nodiscard]] int getNumGroups() const noexcept;

    /** Returns the group at the given index. */
    [[nodiscard]] AnimationGroup* getGroup (int index) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeLayer)
};

} // namespace yup
