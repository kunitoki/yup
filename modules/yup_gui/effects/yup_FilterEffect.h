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
    A chain of CSS `filter` post-processing operations applied to a Component subtree.

    Supports the standard CSS filter functions (`blur`, `brightness`, `contrast`,
    `drop-shadow`, `grayscale`, `hue-rotate`, `invert`, `opacity`, `saturate`, `sepia`)
    in declaration order. Stylesheets configure this automatically through `filter:`
    declarations while leaving @ref Component::setComponentEffect and `effect:` free for
    application-defined custom effects.

    @see ComponentEffect, ColorMatrixEffect, BlurEffect, DropShadowEffect

    @tags{UI}
*/
class YUP_API FilterEffect : public ComponentEffect
{
public:
    //==============================================================================
    /** A reference-counted pointer to a FilterEffect. */
    using Ptr = ReferenceCountedObjectPtr<FilterEffect>;

    /** Kind of CSS filter primitive. */
    enum class Type
    {
        blur,
        brightness,
        contrast,
        dropShadow,
        grayscale,
        hueRotate,
        invert,
        opacity,
        saturate,
        sepia
    };

    /** One evaluated step of a filter chain. */
    struct Operation
    {
        Type type = Type::opacity;
        float amount = 1.0f;
        Point<float> offset;
        Color color { 0x80000000u };

        bool operator== (const Operation& other) const = default;

        /** Returns true when this operation leaves pixels unchanged. */
        bool isNoOp() const noexcept;

        static Operation blur (float radiusPoints = 0.0f) noexcept;
        static Operation brightness (float factor = 1.0f) noexcept;
        static Operation contrast (float factor = 1.0f) noexcept;
        static Operation dropShadow (Point<float> shadowOffset,
                                     float radiusPoints = 0.0f,
                                     Color shadowColor = Colors::black) noexcept;
        static Operation grayscale (float amount = 1.0f) noexcept;
        static Operation hueRotate (float angleRadians = 0.0f) noexcept;
        static Operation invert (float amount = 1.0f) noexcept;
        static Operation opacity (float amount = 1.0f) noexcept;
        static Operation saturate (float factor = 1.0f) noexcept;
        static Operation sepia (float amount = 1.0f) noexcept;
    };

    /** A partially parsed filter step whose scalar or color argument may be a `.yui` expression. */
    struct ParsedStep
    {
        Type type = Type::opacity;
        float amount = 1.0f;
        String amountExpression;
        Point<float> offset;
        Color color = Colors::black;
        String colorExpression;
    };

    //==============================================================================
    /** Creates an empty filter chain. */
    FilterEffect() noexcept;

    /** Creates a filter effect with the given chain of operations. */
    explicit FilterEffect (std::vector<Operation> initialOperations);

    /** Destructor. */
    ~FilterEffect() override;

    //==============================================================================
    /** Parses a literal CSS `filter` string into an ordered operation list (`"none"` -> empty). */
    static ResultValue<std::vector<Operation>> parse (StringRef filterText);

    /** Parses a CSS `filter` string, optionally retaining `.yui` expressions for runtime evaluation. */
    static ResultValue<std::vector<ParsedStep>> parseSteps (StringRef filterText, bool allowExpressions);

    /** Clamps a numeric argument to the CSS domain of @a type. */
    static float clampAmount (Type type, float value) noexcept;

    /** Returns the CSS identity (no-op) operation corresponding to @a target. */
    static Operation identityOperationFor (const Operation& target) noexcept;

    /** Interpolates two CSS filter chains according to CSS Filter Effects rules.

        Matching filter primitives are interpolated step-by-step, padding a shorter or empty
        chain with @ref identityOperationFor. Chains with mismatched primitive types switch
        discretely at `progress = 0.5`.
    */
    static std::vector<Operation> interpolate (const std::vector<Operation>& from,
                                               const std::vector<Operation>& to,
                                               float progress);

    /** Creates a FilterEffect from a literal CSS `filter` string. */
    static ResultValue<Ptr> fromString (StringRef filterText);

    //==============================================================================
    /** @internal */
    static bool isOperationsVar (const var& value) noexcept;
    /** @internal */
    static var operationsToVar (const std::vector<Operation>& operations);
    /** @internal */
    static std::vector<Operation> operationsFromVar (const var& value);

    //==============================================================================
    /** Replaces the chain of filter operations. */
    void setOperations (std::vector<Operation> newOperations);

    /** Returns a copy of the current filter operations. */
    std::vector<Operation> getOperations() const;

    /** Returns the paint margin needed around the component bounds: what its blurs and the blur
        of its drop shadows spread over, and the offset of its drop shadows. In a chain each
        operation works on what the ones before it left, so their margins add up.
    */
    EdgeSpacing<float> getRequiredMargin() const noexcept;

    //==============================================================================
    /** @internal */
    void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override;

private:
    std::vector<Operation> operations;
    mutable CriticalSection operationsLock;
    GpuDevice::Ptr blurDevice;
    GpuPipeline::Ptr blurPipeline;
    bool blurCompileAttempted = false;
    GpuDevice::Ptr colorMatrixDevice;
    GpuPipeline::Ptr colorMatrixPipeline;
    bool colorMatrixCompileAttempted = false;
    GpuTarget::Ptr targetA;
    GpuTarget::Ptr targetB;
    GpuTarget::Ptr targetC;
    GpuCanvas::Ptr compositeCanvas;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterEffect)
};

} // namespace yup
