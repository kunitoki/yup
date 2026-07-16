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
    /** Represents a layer in the composition context. */
    struct LayerContext
    {
        String name;
        int id = -1;
        AnimationTransform* transform = nullptr;
    };

    /** Represents the composition context for evaluating expressions. */
    struct CompositionContext
    {
        Size<float> size;
        float frameRate = 60.0f;
        std::vector<LayerContext> layers;
    };

    //==============================================================================
    /** Represents a shape layer's top-level content group. */
    struct EvalResult
    {
        enum class Kind
        {
            Unknown,          ///< Expression empty, failed, or unreachable reference
            StaticValue,      ///< Expression produced a concrete scalar/array value
            LayerPropertyRef, ///< thisComp.layer(X).transform.<property>
            ShapeContentRef   ///< content("G").content("P").path or content("G").transform.rotation
        };

        Kind kind = Kind::Unknown; ///< The kind of evaluation result

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
    /** Constructs a new LottieExpressionEvaluator. */
    LottieExpressionEvaluator();

    /** Registers thisComp with the given composition data.

        @param ctx The composition context to use for evaluating expressions. 

        Must be called before evaluating layer transform expressions.
    */
    void setupCompositionContext (const CompositionContext& ctx);

    /** Registers content() pointing into a ShapeLayer's top-level groups.

        @param layer The shape layer to use for evaluating expressions.

        Call before evaluating expressions inside parseShapeContents.
    */
    void setupShapeContext (const ShapeLayer& layer);

    /** Registers content() pointing into an AnimationGroup's child groups.

        @param group The animation group to use for evaluating expressions.

        Call before evaluating expressions inside parseGroupItems.
    */
    void setupGroupContext (const AnimationGroup& group);

    /** Evaluates an AfterEffects expression string.

        @param expression The expression string to evaluate.

        @returns An EvalResult describing the result of the evaluation, or 
                 Kind::Unknown if the expression is empty or evaluation failed.
    */
    [[nodiscard]] EvalResult evaluate (const String& expression);

    //==============================================================================
    /** @internal */
    void setLastLayerName (const String& s) { lastLayerName_ = s; }

    /** @internal */
    void setLastLayerId (int id) { lastLayerId_ = id; }

    /** @internal */
    void setLastContentGroup (const String& s) { lastContentGroup_ = s; }

    /** @internal */
    void setLastContentItem (const String& s) { lastContentItem_ = s; }

    /** @internal */
    void setLastProperty (const String& s) { lastProperty_ = s; }

    /** @internal */
    String getLastContentGroup() const { return lastContentGroup_; }

    /** @internal */
    String getLastContentItem() const { return lastContentItem_; }

    /** @internal */
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
