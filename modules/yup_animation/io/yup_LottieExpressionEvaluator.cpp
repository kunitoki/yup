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

namespace
{

//==============================================================================
// Records which transform property was accessed and returns the stored value.
struct TransformProxy final : public DynamicObject
{
    TransformProxy (String& lastProperty, const AnimationTransform& t)
        : lastProperty_ (lastProperty)
    {
        const Point<float> pos = t.positionAt (0.0f);
        Array<var> posArr;
        posArr.add (pos.getX());
        posArr.add (pos.getY());
        setProperty ("position", posArr);
        setProperty ("rotation", t.rotation.getValueAt (0.0f));

        const Size<float> sc = t.scale.getValueAt (0.0f);
        Array<var> scArr;
        scArr.add (sc.getWidth());
        scArr.add (sc.getHeight());
        setProperty ("scale", scArr);

        setProperty ("opacity", t.opacity.getValueAt (0.0f));

        const Point<float> a = t.anchor.getValueAt (0.0f);
        Array<var> anchArr;
        anchArr.add (a.getX());
        anchArr.add (a.getY());
        setProperty ("anchorPoint", anchArr);
    }

    const var& getProperty (const Identifier& name) const override
    {
        lastProperty_ = "transform." + name.toString();
        return DynamicObject::getProperty (name);
    }

    String& lastProperty_;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransformProxy)
};

//==============================================================================
// Proxy for a single layer — exposes .name, .index, and .transform.
struct LayerProxy final : public DynamicObject
{
    LayerProxy (const LottieExpressionEvaluator::LayerContext& lc, String& lastProperty)
    {
        setProperty ("name", lc.name);
        setProperty ("index", lc.id);
        if (lc.transform != nullptr)
            setProperty ("transform", new TransformProxy (lastProperty, *lc.transform));
    }

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayerProxy)
};

//==============================================================================
// Proxy for a content item — exposes .path and .transform.rotation side effects.
struct ContentItemProxy final : public DynamicObject
{
    // Records which property was accessed on a content item's transform.
    struct TransformRotationPlaceholder final : public DynamicObject
    {
        explicit TransformRotationPlaceholder (LottieExpressionEvaluator* owner)
            : owner_ (owner)
        {
            setProperty ("rotation", 0.0);
        }

        const var& getProperty (const Identifier& name) const override
        {
            owner_->setLastProperty ("transform." + name.toString());
            return DynamicObject::getProperty (name);
        }

        LottieExpressionEvaluator* owner_;
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransformRotationPlaceholder)
    };

    ContentItemProxy (LottieExpressionEvaluator* owner, String itemName)
        : owner_ (owner)
        , itemName_ (std::move (itemName))
    {
        setMethod ("content", [this] (const var::NativeFunctionArgs& args) -> var
        {
            if (args.numArguments >= 1)
                owner_->setLastContentItem (args.arguments[0].toString());
            return var (new ContentItemProxy (owner_, owner_->getLastContentItem()));
        });

        Array<var> pathPlaceholder;
        setProperty ("path", pathPlaceholder);
        setProperty ("transform", new TransformRotationPlaceholder (owner_));
    }

    const var& getProperty (const Identifier& name) const override
    {
        if (name.toString() == "path")
            owner_->setLastProperty ("path");
        return DynamicObject::getProperty (name);
    }

    LottieExpressionEvaluator* owner_;
    String itemName_;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentItemProxy)
};

//==============================================================================
// Root object for thisComp — exposes .width, .height, .frameRate, and .layer().
struct ThisCompObject final : public DynamicObject
{
    ThisCompObject (LottieExpressionEvaluator* owner,
                    const LottieExpressionEvaluator::CompositionContext& ctx)
        : owner_ (owner)
        , ctx_ (ctx)
    {
        setProperty ("width", ctx.size.getWidth());
        setProperty ("height", ctx.size.getHeight());
        setProperty ("frameRate", ctx.frameRate);

        setMethod ("layer", [this] (const var::NativeFunctionArgs& args) -> var
        {
            if (args.numArguments < 1)
                return var::undefined();

            if (args.arguments[0].isString())
            {
                const String layerName = args.arguments[0].toString();
                for (const auto& lc : ctx_.layers)
                {
                    if (lc.name == layerName)
                    {
                        owner_->setLastLayerName (layerName);
                        owner_->setLastLayerId (-1);
                        return var (new LayerProxy (lc, owner_->getLastPropertyRef()));
                    }
                }
            }
            else
            {
                const int idx = static_cast<int> (args.arguments[0]);
                for (const auto& lc : ctx_.layers)
                {
                    if (lc.id == idx)
                    {
                        owner_->setLastLayerName ({});
                        owner_->setLastLayerId (idx);
                        return var (new LayerProxy (lc, owner_->getLastPropertyRef()));
                    }
                }
            }

            return var::undefined();
        });
    }

    LottieExpressionEvaluator* owner_;
    LottieExpressionEvaluator::CompositionContext ctx_; // value copy for lifetime

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThisCompObject)
};

//==============================================================================
// Root object for content() global function — records the group name.
struct ContentRootObject final : public DynamicObject
{
    explicit ContentRootObject (LottieExpressionEvaluator* owner)
        : owner_ (owner)
    {
        setMethod ("__content__", [this] (const var::NativeFunctionArgs& args) -> var
        {
            if (args.numArguments >= 1)
            {
                owner_->setLastContentGroup (args.arguments[0].toString());
                owner_->setLastContentItem ({});
            }
            return var (new ContentItemProxy (owner_, owner_->getLastContentGroup()));
        });
    }

    LottieExpressionEvaluator* owner_;
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentRootObject)
};

//==============================================================================
// Helpers used by AEGlobalsObject methods.
namespace AEMath
{

// Element-wise binary op on two array-or-scalar vars.
template <typename Op>
static var vecOp (const var& a, const var& b, Op op)
{
    const auto* aa = a.getArray();
    const auto* ba = b.getArray();
    if (aa != nullptr && ba != nullptr)
    {
        Array<var> r;
        for (int i = 0; i < aa->size(); ++i)
        {
            const double x = static_cast<double> ((*aa)[i]);
            const double y = i < ba->size() ? static_cast<double> ((*ba)[i]) : 0.0;
            r.add (op (x, y));
        }
        return r;
    }
    return op (static_cast<double> (a), static_cast<double> (b));
}

static var lerp (const var& a, const var& b, double t)
{
    return vecOp (a, b, [t] (double x, double y)
    {
        return x + (y - x) * t;
    });
}

static double magnitude (const Array<var>& arr)
{
    double s = 0.0;
    for (const auto& v : arr)
        s += static_cast<double> (v) * static_cast<double> (v);
    return std::sqrt (s);
}

} // namespace AEMath

//==============================================================================
// Stub for `thisProperty` — allows numKeys guards and loop-modifier calls without throwing.
struct ThisPropertyObject final : public DynamicObject
{
    ThisPropertyObject()
    {
        setProperty ("numKeys", 0);
        auto noop = [] (const var::NativeFunctionArgs&) -> var
        {
            return var::undefined();
        };
        setMethod ("loopOut", noop);
        setMethod ("loopIn", noop);
    }

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThisPropertyObject)
};

//==============================================================================
// Stub for `thisLayer` — allows .effect('name')('param') chains without throwing.
struct ThisLayerObject final : public DynamicObject
{
    ThisLayerObject()
    {
        setMethod ("effect", [] (const var::NativeFunctionArgs&) -> var
        {
            return var::NativeFunction ([] (const var::NativeFunctionArgs&) -> var
            {
                return var::undefined();
            });
        });

        setMethod ("toComp", [] (const var::NativeFunctionArgs& a) -> var
        {
            return a.numArguments > 0 ? a.arguments[0] : var {};
        });
    }

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThisLayerObject)
};

} // namespace

//==============================================================================
LottieExpressionEvaluator::LottieExpressionEvaluator()
{
    engine.execute ("var time = 0; var value = undefined; var $bm_rt = undefined;");

    engine.registerNativeObject ("thisProperty", new ThisPropertyObject());
    engine.registerNativeObject ("thisLayer", new ThisLayerObject());

    engine.registerNativeFunction ("add", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 2)
            return {};
        return AEMath::vecOp (a.arguments[0], a.arguments[1], [] (double x, double y)
        {
            return x + y;
        });
    });

    engine.registerNativeFunction ("sub", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 2)
            return {};
        return AEMath::vecOp (a.arguments[0], a.arguments[1], [] (double x, double y)
        {
            return x - y;
        });
    });

    engine.registerNativeFunction ("mul", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 2)
            return {};
        const double scalar = static_cast<double> (a.arguments[1]);
        if (const auto* arr = a.arguments[0].getArray())
        {
            Array<var> r;
            for (const auto& v : *arr)
                r.add (static_cast<double> (v) * scalar);
            return r;
        }
        return static_cast<double> (a.arguments[0]) * scalar;
    });

    engine.registerNativeFunction ("div", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 2)
            return {};
        const double divisor = static_cast<double> (a.arguments[1]);
        if (divisor == 0.0)
            return 0.0;
        if (const auto* arr = a.arguments[0].getArray())
        {
            Array<var> r;
            for (const auto& v : *arr)
                r.add (static_cast<double> (v) / divisor);
            return r;
        }
        return static_cast<double> (a.arguments[0]) / divisor;
    });

    engine.registerNativeFunction ("clamp", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 3)
            return {};
        const double v = static_cast<double> (a.arguments[0]);
        const double mn = static_cast<double> (a.arguments[1]);
        const double mx = static_cast<double> (a.arguments[2]);
        return std::clamp (v, mn, mx);
    });

    auto lerpImpl = [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 2)
            return {};
        if (a.numArguments < 3)
            return a.arguments[1];
        return AEMath::lerp (a.arguments[1], a.arguments[2], static_cast<double> (a.arguments[0]));
    };
    engine.registerNativeFunction ("linear", lerpImpl);
    engine.registerNativeFunction ("ease", lerpImpl);
    engine.registerNativeFunction ("easeIn", lerpImpl);
    engine.registerNativeFunction ("easeOut", lerpImpl);

    engine.registerNativeFunction ("length", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments == 0)
            return 0.0;
        if (a.numArguments == 1)
        {
            if (const auto* arr = a.arguments[0].getArray())
                return AEMath::magnitude (*arr);
            return std::abs (static_cast<double> (a.arguments[0]));
        }
        const auto* aa = a.arguments[0].getArray();
        const auto* ba = a.arguments[1].getArray();
        if (aa != nullptr && ba != nullptr)
        {
            double s = 0.0;
            for (int i = 0; i < aa->size(); ++i)
            {
                const double d = static_cast<double> ((*aa)[i])
                               - (i < ba->size() ? static_cast<double> ((*ba)[i]) : 0.0);
                s += d * d;
            }
            return std::sqrt (s);
        }
        return std::abs (static_cast<double> (a.arguments[0]) - static_cast<double> (a.arguments[1]));
    });

    engine.registerNativeFunction ("normalize", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments == 0)
            return {};
        if (const auto* arr = a.arguments[0].getArray())
        {
            const double len = AEMath::magnitude (*arr);
            if (len == 0.0)
                return a.arguments[0];
            Array<var> r;
            for (const auto& v : *arr)
                r.add (static_cast<double> (v) / len);
            return r;
        }
        return a.arguments[0];
    });

    engine.registerNativeFunction ("dot", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments < 2)
            return 0.0;
        const auto* aa = a.arguments[0].getArray();
        const auto* ba = a.arguments[1].getArray();
        if (aa != nullptr && ba != nullptr)
        {
            double s = 0.0;
            for (int i = 0; i < aa->size(); ++i)
                s += static_cast<double> ((*aa)[i])
                   * (i < ba->size() ? static_cast<double> ((*ba)[i]) : 0.0);
            return s;
        }
        return static_cast<double> (a.arguments[0]) * static_cast<double> (a.arguments[1]);
    });

    engine.registerNativeFunction ("degreesToRadians", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments == 0)
            return 0.0;
        return degreesToRadians (static_cast<double> (a.arguments[0]));
    });

    engine.registerNativeFunction ("radiansToDegrees", [] (const var::NativeFunctionArgs& a) -> var
    {
        if (a.numArguments == 0)
            return 0.0;
        return radiansToDegrees (static_cast<double> (a.arguments[0]));
    });

    // random/wiggle are non-deterministic by design — always return 0 / undefined
    // at parse time so expressions degrade gracefully to Unknown.
    engine.registerNativeFunction ("random", [] (const var::NativeFunctionArgs&) -> var
    {
        return 0.0;
    });
    engine.registerNativeFunction ("wiggle", [] (const var::NativeFunctionArgs&) -> var
    {
        return var::undefined();
    });

    // Loop modifiers — returning undefined causes evaluate() to yield Kind::Unknown,
    // so the parsed keyframe data is preserved unchanged.
    auto noopUndefined = [] (const var::NativeFunctionArgs&) -> var
    {
        return var::undefined();
    };
    engine.registerNativeFunction ("loopOut", noopUndefined);
    engine.registerNativeFunction ("loopIn", noopUndefined);
    engine.registerNativeFunction ("loopOutDuration", noopUndefined);
    engine.registerNativeFunction ("loopInDuration", noopUndefined);
}

void LottieExpressionEvaluator::setupCompositionContext (const CompositionContext& ctx)
{
    engine.registerNativeObject ("thisComp", new ThisCompObject (this, ctx));
}

void LottieExpressionEvaluator::setupShapeContext (const ShapeLayer& /*layer*/)
{
    registerContentRoot();
}

void LottieExpressionEvaluator::setupGroupContext (const AnimationGroup& /*group*/)
{
    registerContentRoot();
}

void LottieExpressionEvaluator::registerContentRoot()
{
    if (engine.getRootObjectProperties().contains (Identifier ("__cr__")))
        return;

    engine.registerNativeObject ("__cr__", new ContentRootObject (this));
    engine.execute ("function content(n) { return __cr__.__content__(n); }");
}

//==============================================================================
LottieExpressionEvaluator::EvalResult LottieExpressionEvaluator::evaluate (const String& expression)
{
    if (expression.isEmpty())
        return {};

    lastLayerName_ = {};
    lastLayerId_ = -1;
    lastContentGroup_ = {};
    lastContentItem_ = {};
    lastProperty_ = {};

    engine.execute ("$bm_rt = undefined; value = undefined;");

    const auto statementResult = engine.executeWithResult (expression);

    if (statementResult.failed())
        return {};

    auto result = statementResult.getValue();
    if (result.isVoid() || result.isUndefined())
        return {};

    EvalResult out;

    if (lastContentGroup_.isNotEmpty())
    {
        out.kind = EvalResult::Kind::ShapeContentRef;
        out.contentGroupName = lastContentGroup_;
        out.contentItemName = lastContentItem_;
        out.contentProperty = lastProperty_;
        out.value = result;
    }
    else if (lastLayerName_.isNotEmpty() || lastLayerId_ >= 0)
    {
        out.kind = EvalResult::Kind::LayerPropertyRef;
        out.referencedLayerName = lastLayerName_;
        out.referencedLayerId = lastLayerId_;
        out.referencedProperty = lastProperty_;
        out.value = result;
    }
    else
    {
        out.kind = EvalResult::Kind::StaticValue;
        out.value = result;
    }

    return out;
}

} // namespace yup
