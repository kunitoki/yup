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

/** Evaluates After Effects-style Lottie expression strings using JavascriptEngine.

    Supports two independent expression contexts:
    - Composition context: `thisComp.layer(name|index).transform.<property>`
    - Shape content context: `content("GroupName").content("PathName").path`
      or `content("GroupName").transform.rotation`

    Call setupCompositionContext() or setupShapeContext() before evaluate().
*/
class LottieExpressionEvaluator
{
public:
    //==============================================================================
    struct LayerContext
    {
        String name;
        int id = -1;
        AnimationTransform* transform = nullptr;
    };

    struct CompositionContext
    {
        Size<float> size;
        float frameRate = 60.0f;
        std::vector<LayerContext> layers;
    };

    //==============================================================================
    struct EvalResult
    {
        enum class Kind
        {
            Unknown,          ///< Expression empty, failed, or unreachable reference
            StaticValue,      ///< Expression produced a concrete scalar/array value
            LayerPropertyRef, ///< thisComp.layer(X).transform.<property>
            ShapeContentRef   ///< content("G").content("P").path or content("G").transform.rotation
        };

        Kind kind = Kind::Unknown;

        var value; ///< Computed frame-0 value (set for StaticValue and as a snapshot for ref kinds)

        // LayerPropertyRef fields
        String referencedLayerName; ///< Non-empty when referenced by name
        int referencedLayerId = -1; ///< >=0 when referenced by index
        String referencedProperty;  ///< Dot-path, e.g. "transform.position"

        // ShapeContentRef fields
        String contentGroupName; ///< First content() argument
        String contentItemName;  ///< Second content() argument (path refs only)
        String contentProperty;  ///< "path" or "transform.rotation"
    };

    //==============================================================================
    LottieExpressionEvaluator();

    /** Registers thisComp with the given composition data.
        Must be called before evaluating layer transform expressions. */
    void setupCompositionContext (const CompositionContext& ctx);

    /** Registers content() pointing into a ShapeLayer's top-level groups.
        Call before evaluating expressions inside parseShapeContents. */
    void setupShapeContext (const ShapeLayer& layer);

    /** Registers content() pointing into an AnimationGroup's child groups.
        Call before evaluating expressions inside parseGroupItems. */
    void setupGroupContext (const AnimationGroup& group);

    /** Evaluates an AE expression string.
        Returns Kind::Unknown for empty input or evaluation failures. */
    [[nodiscard]] EvalResult evaluate (const String& expression);

    //==============================================================================
    // Internal state setters used by proxy objects during evaluation.
    void setLastLayerName (const String& s) { lastLayerName_ = s; }

    void setLastLayerId (int id) { lastLayerId_ = id; }

    void setLastContentGroup (const String& s) { lastContentGroup_ = s; }

    void setLastContentItem (const String& s) { lastContentItem_ = s; }

    void setLastProperty (const String& s) { lastProperty_ = s; }

    String getLastContentGroup() const { return lastContentGroup_; }

    String getLastContentItem() const { return lastContentItem_; }

    String& getLastPropertyRef() { return lastProperty_; }

private:
    //==============================================================================
    void registerContentRoot();

    JavascriptEngine engine;

    // Written by proxy objects during evaluate(); cleared before each call.
    String lastLayerName_;
    int lastLayerId_ = -1;
    String lastContentGroup_;
    String lastContentItem_;
    String lastProperty_;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LottieExpressionEvaluator)
};

} // namespace yup
