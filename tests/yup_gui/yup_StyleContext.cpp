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

#include <yup_gui/yup_gui.h>
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace yup;

class StyleContextTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    static DataTree declaration (const String& name, const String& value)
    {
        return DataTree ("declaration", { { "name", name }, { "value", value } });
    }

    static DataTree rule (const String& selector, std::initializer_list<DataTree> declarations)
    {
        return DataTree ("rule", { { "selector", selector } }, declarations);
    }

    static StyleSheet::Ptr build (const DataTree& style)
    {
        auto sheet = StyleSheet::fromTree (style);
        EXPECT_FALSE (sheet.failed()) << sheet.getErrorMessage();
        return sheet.wasOk() ? sheet.getReference() : nullptr;
    }

    static void restyle (Component& component)
    {
        auto* context = component.getStyleContext();
        ASSERT_NE (context, nullptr);
        context->restyleNow();
    }

    static void attach (Component& root, const String& text)
    {
        auto sheet = YuiParser::parseStyleSheet (text);
        ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
        root.setStyleSheet (sheet.getReference());
        restyle (root);
    }

    static void advance (Component& root, double elapsedSeconds)
    {
        root.getStyleContext()->refresh (elapsedSeconds);
    }

    /** Puts a component in a window of its own, and returns the native that sends its frames. */
    static StubComponentNative& makeWindow (Component& component)
    {
        auto* native = new StubComponentNative (component, ComponentNative::defaultFlags);
        yup::ComponentTestHelper<Component>::attachNative (component, native);
        return *native;
    }

    static double glow (const Component& component)
    {
        return static_cast<double> (component.findProperty (Identifier ("glow")));
    }

    static void expectDiagnostic (const Component& root, const String& prefix)
    {
        const auto& diagnostics = root.getStyleContext()->getDiagnostics();

        for (const auto& diagnostic : diagnostics)
            if (diagnostic.message.startsWith (prefix))
                return;

        ADD_FAILURE() << "No diagnostic starting with '" << prefix << "' in: " << DataTreeDiagnostic::join (diagnostics);
    }

    class StyleTestEffect : public ComponentEffect
    {
    public:
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}
    };

    /** Makes a document read its assets from the memory of the test, which counts the reads of each path. */
    void serveAssets (YuiDocument& document)
    {
        document.setAssetReader ([this] (const String& path) -> std::optional<MemoryBlock>
        {
            ++assetReads[path];

            const auto found = assets.find (path);
            return found != assets.end() ? std::optional<MemoryBlock> (found->second) : std::nullopt;
        });
    }

    void setAsset (const String& path, const String& text)
    {
        assets[path] = MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    /** Restyles everything once more, and returns how many problems that reported. */
    static size_t countProblemsOfARestyle (Component& root)
    {
        root.getStyleContext()->markDirty (root);
        restyle (root);
        return root.getStyleContext()->getDiagnostics().size();
    }

    static ShaderEffect* getShaderEffect (const Component* component)
    {
        return component != nullptr ? dynamic_cast<ShaderEffect*> (component->getComponentEffect().get()) : nullptr;
    }

    static DrawingStyle* getDrawingStyle (const Component* component)
    {
        return component != nullptr ? dynamic_cast<DrawingStyle*> (component->getStyle().get()) : nullptr;
    }

    /** Puts the root of a document in a window keeping what it is asked to repaint, with one of its components showing. */
    static StubComponentNative& showInAWindow (Component& root, Component& shown)
    {
        auto* native = new StubComponentNative (root, ComponentNative::defaultFlags);
        yup::ComponentTestHelper<Component>::attachNative (root, native);

        root.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
        root.setVisible (true);
        shown.setBounds (0.0f, 0.0f, 32.0f, 16.0f);
        shown.setVisible (true);

        return *native;
    }

    /** Returns what the drawing of a component gives the first shader it runs, empty when it runs none. */
    static ShaderUniforms getShaderUniforms (const Component* component)
    {
        if (auto* drawing = getDrawingStyle (component))
        {
            for (const auto& element : drawing->measureElements (*component))
                if (element.type == "use" && ! element.uniforms.isEmpty())
                    return element.uniforms;
        }

        return {};
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Tints the picture it reads: the members `shader Crt` of the tests declares as properties, and one texture. */
    static constexpr const char* crtFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;
    float intensity;
} params;
layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
    color = mix (pixel, pixel * params.tint, params.intensity);
})";

    /** The same members, and no texture: what tells it from crtFragment once loaded. */
    static constexpr const char* proceduralFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;
    float intensity;
} params;
layout(location = 0) out vec4 color;
void main() { color = params.tint * params.intensity; })";

    /** The members of proceduralFragment the other way around: the same shader to a `shader` block, laid out differently. */
    static constexpr const char* reorderedFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    float intensity;
    vec4 tint;
} params;
layout(location = 0) out vec4 color;
void main() { color = params.tint * params.intensity; })";

    /** Reads two textures, one more than an effect has. */
    static constexpr const char* twoInputsFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D base;
layout(set = 0, binding = 1) uniform texture2D overlay;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (texture (sampler2D (base, linearClamp), uv).r, texture (sampler2D (overlay, linearClamp), uv).g, 0.0, 1.0);
})";

    /** Does not compile. */
    static constexpr const char* brokenFragment = R"(#version 450
layout(location = 0) out vec4 color;
void main() { color = vec4 (missingValue); })";

    /** A member of its own named as what a shader is given: two numbers where it is given the pixels of a unit. */
    static constexpr const char* ownScaleFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { vec2 scale; } params;
layout(location = 0) out vec4 color;
void main() { color = vec4 (params.scale, 0.0, 1.0); })";

    /** The bytes of the bundle a build makes of a fragment shader: compiled behind the vertex stage of an image shader. */
    static MemoryBlock compileBundle (const char* fragment)
    {
        static constexpr const char* vertex = R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    vec2 p = vec2 (float ((gl_VertexIndex << 1) & 2), float (gl_VertexIndex & 2));
    uv = vec2 (p.x, 1.0 - p.y);
    gl_Position = vec4 (p * 2.0 - 1.0, 0.0, 1.0);
})";

        ShaderBundleCompileRequest request;

        for (const auto& [stage, source] : { std::pair (ShaderStage::vertex, vertex), std::pair (ShaderStage::fragment, fragment) })
        {
            ShaderBundleEntry entry;
            entry.stage = stage;
            entry.source = source;
            entry.targetLanguages = { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl };
            request.entries.push_back (entry);
        }

        const auto compiled = ShaderBundleCompiler().compile (request);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

        MemoryBlock bytes;

        if (compiled.wasOk())
            EXPECT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());

        return bytes;
    }

    /** The files of an interface drawn through a shader, as the embedded resources a YuiLoader finds: its text and the
        source of the shader, with no compiled bundle next to it.
    */
    static const uint8* findShaderResource (const char* path, std::size_t& size)
    {
        static constexpr const char* panel = "shader Crt { fragment: \"fx/crt.frag\"; property intensity: number = 0.5; }\n"
                                             "style { #screen { effect: Crt; } }\n"
                                             "Component { Component { id: screen; } }";

        const String key (path);
        const char* text = key == "ui.yui" ? panel : (key == "fx/crt.frag" ? crtFragment : nullptr);

        size = text != nullptr ? std::strlen (text) : 0;
        return reinterpret_cast<const uint8*> (text);
    }
#endif

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    std::map<String, MemoryBlock> assets;
    std::map<String, int> assetReads;
};

TEST_F (StyleContextTests, ComputesCustomPropertiesForMatchedComponents)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    child.addStyleClass (Identifier ("knob"));

    root.setStyleSheet (build (DataTree ("style", { rule (".knob", { declaration ("trackWidth", "4 + 2") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (Identifier ("trackWidth"))), 6.0);
    EXPECT_FALSE (root.getStyleState() != nullptr && root.getStyleState()->getComputedValues().contains (Identifier ("trackWidth")));
}

TEST_F (StyleContextTests, ComputesColorValues)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("thumbColor", "#ff8800") }) })));
    restyle (root);

    const auto color = root.findColor (Identifier ("thumbColor"));
    ASSERT_TRUE (color.has_value());
    EXPECT_EQ (*color, Color (0xffff8800));
}

TEST_F (StyleContextTests, ExplicitValuesWinOverComputedOnes)
{
    Component root ("root");
    root.setProperty (Identifier ("trackWidth"), 12.0);

    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("trackWidth", "4") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 12.0);
}

TEST_F (StyleContextTests, HigherSpecificityWins)
{
    Component root ("root");
    root.addStyleClass (Identifier ("panel"));

    root.setStyleSheet (build (DataTree ("style",
                                         { rule ("#root", { declaration ("trackWidth", "1") }),
                                           rule (".panel", { declaration ("trackWidth", "2") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 1.0);
}

TEST_F (StyleContextTests, LaterRuleWinsAtEqualSpecificity)
{
    Component root ("root");
    root.addStyleClass (Identifier ("panel"));

    root.setStyleSheet (build (DataTree ("style",
                                         { rule (".panel", { declaration ("trackWidth", "1") }),
                                           rule (".panel", { declaration ("trackWidth", "2") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 2.0);
}

TEST_F (StyleContextTests, QualifiedDeclarationsCascadeToDescendants)
{
    Component root ("root");
    Slider slider (Slider::LinearHorizontal, "slider");
    Component other ("other");

    root.addChildComponent (slider);
    root.addChildComponent (other);
    root.addStyleClass (Identifier ("panel"));

    root.setStyleSheet (build (DataTree ("style", { rule (".panel", { declaration ("Slider.trackWidth", "7") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (slider.findProperty (Identifier ("trackWidth"))), 7.0);
    EXPECT_TRUE (other.getStyleState() == nullptr || other.getStyleState()->getComputedValues().isEmpty());
}

TEST_F (StyleContextTests, RestylingLaysAContainerOutOnce)
{
    struct CountingColumn : ColumnComponent
    {
        int numLayouts = 0;

        void performLayout (Rectangle<float> area) override
        {
            ++numLayouts;
            ColumnComponent::performLayout (area);
        }
    };

    CountingColumn column;
    std::vector<std::unique_ptr<Component>> items;

    column.setBounds (0.0f, 0.0f, 100.0f, 400.0f);

    for (int index = 0; index < 50; ++index)
    {
        auto& item = *items.emplace_back (std::make_unique<Component>());
        item.addStyleClass (Identifier ("item"));
        column.addAndMakeVisible (item);
    }

    const auto before = column.numLayouts;
    attach (column, "style { .item { preferredHeight: 4; } }");

    // Not at each of the items the rule gives a size to
    EXPECT_LE (column.numLayouts - before, 4);
    EXPECT_EQ (items.back()->getBounds(), Rectangle<float> (0.0f, 196.0f, 100.0f, 4.0f));
}

TEST_F (StyleContextTests, ChangingStyleClassesRestyles)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    root.setStyleSheet (build (DataTree ("style", { rule (".knob", { declaration ("trackWidth", "9") }) })));
    restyle (root);

    EXPECT_FALSE (child.findProperty (Identifier ("trackWidth")).isDouble());

    child.addStyleClass (Identifier ("knob"));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (Identifier ("trackWidth"))), 9.0);

    child.removeStyleClass (Identifier ("knob"));
    restyle (root);

    EXPECT_TRUE (child.getStyleState() == nullptr || child.getStyleState()->getComputedValues().isEmpty());
}

TEST_F (StyleContextTests, StateBackedPropertiesAreAppliedAndRestored)
{
    Component root ("root");
    root.setOpacity (1.0f);

    root.setStyleSheet (build (DataTree ("style", { rule (".faded", { declaration ("opacity", "0.5") }) })));
    restyle (root);

    EXPECT_FLOAT_EQ (root.getOpacity(), 1.0f);

    root.addStyleClass (Identifier ("faded"));
    restyle (root);

    EXPECT_NEAR (root.getOpacity(), 0.5f, 0.01f);

    root.removeStyleClass (Identifier ("faded"));
    restyle (root);

    EXPECT_FLOAT_EQ (root.getOpacity(), 1.0f);
}

TEST_F (StyleContextTests, AStyledEnabledStateHoldsWhenADisabledParentIsEnabledAgain)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);
    root.setEnabled (false);

    root.setStyleSheet (build (DataTree ("style", { rule (".off", { declaration ("enabled", "false") }) })));
    child.addStyleClass (Identifier ("off"));
    restyle (root);

    root.setEnabled (true);
    restyle (root);

    EXPECT_FALSE (child.isEnabled());

    child.removeStyleClass (Identifier ("off"));
    restyle (root);

    EXPECT_TRUE (child.isEnabled());
}

TEST_F (StyleContextTests, ScopesPaletteDeclarationsToTheSubtree)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    root.setStyleSheet (build (DataTree ("style",
                                         { DataTree ("palette", { { "name", "Ember" }, { "colors", "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f" }, { "accent", "#ff9a1f" } }),
                                           rule ("#root", { declaration ("palette", "Ember") }),
                                           rule ("#child", { declaration ("thumbColor", "palette.accent") }) })));
    restyle (root);

    ASSERT_NE (root.getStyleState(), nullptr);
    ASSERT_NE (root.getStyleState()->getStylePalette(), nullptr);
    EXPECT_EQ (child.findPalette(), root.getStyleState()->getStylePalette());

    const auto color = child.findColor (Identifier ("thumbColor"));
    ASSERT_TRUE (color.has_value());
    EXPECT_EQ (*color, Color (0xffff9a1f));
}

TEST_F (StyleContextTests, DetachingClearsComputedValues)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("trackWidth", "3") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 3.0);

    root.setStyleSheet (nullptr);

    EXPECT_EQ (root.getStyleContext(), nullptr);
    EXPECT_TRUE (root.getStyleState() == nullptr || root.getStyleState()->getComputedValues().isEmpty());
}

TEST_F (StyleContextTests, ReportsDiagnosticsForBadValues)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("trackWidth", "notAKnownSymbol * 2") }) })));
    restyle (root);

    EXPECT_FALSE (root.getStyleContext()->getDiagnostics().empty());
}

TEST_F (StyleContextTests, InstallsAndRestoresDrawings)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    const auto previousStyle = child.getStyle();

    auto sheet = YuiParser::parseStyleSheet (R"yui(
        drawing Dot { circle { center: bounds.center; radius: 2; fill: #ff0000; } }
        style { .dot { drawing: Dot; } })yui");

    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());

    child.addStyleClass (Identifier ("dot"));
    restyle (root);

    const auto installed = child.getStyle();
    EXPECT_NE (installed, previousStyle);
    EXPECT_NE (dynamic_cast<DrawingStyle*> (installed.get()), nullptr);

    restyle (root);
    EXPECT_EQ (child.getStyle(), installed);

    child.removeStyleClass (Identifier ("dot"));
    restyle (root);
    EXPECT_EQ (child.getStyle(), previousStyle);
}

TEST_F (StyleContextTests, ReportsUnknownDrawings)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("drawing", "Missing") }) })));
    restyle (root);

    expectDiagnostic (root, "Unknown drawing 'Missing'");
}

TEST_F (StyleContextTests, TypesTheDeclarationsOfDrawingProperties)
{
    Component root ("root");
    Component first ("first");
    Component second ("second");
    root.addChildComponent (first);
    root.addChildComponent (second);

    auto sheet = YuiParser::parseStyleSheet (R"yui(
        drawing Dot {
            property dotRadius: number = 2;
            circle { center: bounds.center; radius: dotRadius; fill: #ff0000; }
        }
        style {
            #first { drawing: Dot; dotRadius: 1; }
            #second { drawing: Dot; dotRadius: #ff0000; }
        })yui");

    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());
    restyle (root);

    const auto radius = first.findProperty (Identifier ("dotRadius"));
    EXPECT_TRUE (radius.isDouble());
    EXPECT_DOUBLE_EQ (static_cast<double> (radius), 1.0);

    const auto diagnostics = root.getStyleContext()->getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("dotRadius: ")) << diagnostics[0].message;
}

TEST_F (StyleContextTests, TransitionsFollowStyleChanges)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");
    EXPECT_DOUBLE_EQ (glow (root), 0.0);

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 0.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    advance (root, 0.6);
    EXPECT_DOUBLE_EQ (glow (root), 10.0);
    EXPECT_EQ (root.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);
}

TEST_F (StyleContextTests, AsksForFramesOnlyWhileItHasStylesToApplyOrToAnimate)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    window.setVisible (true);

    Component root ("root");
    Component knob ("knob");
    knob.addStyleClass (Identifier ("knob"));
    root.addAndMakeVisible (knob);
    window.addAndMakeVisible (root);

    auto sheet = YuiParser::parseStyleSheet ("style { .knob { glow: 2; transition: glow 1s; } .lit { glow: 12; } }");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());

    // The frame that follows applies the styles, and with nothing left to do the window is free to idle
    EXPECT_TRUE (native.hasFrameListeners());
    native.callFrameListeners (0.0);
    EXPECT_DOUBLE_EQ (glow (knob), 2.0);
    EXPECT_FALSE (native.hasFrameListeners());

    // A change is applied by the frame that follows it, in which its transition starts
    knob.addStyleClass (Identifier ("lit"));
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.0);
    EXPECT_DOUBLE_EQ (glow (knob), 2.0);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.5);
    EXPECT_DOUBLE_EQ (glow (knob), 7.0);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.6);
    EXPECT_DOUBLE_EQ (glow (knob), 12.0);
    EXPECT_FALSE (native.hasFrameListeners());

    // So is a transition started by a restyle asked for between two frames
    knob.removeStyleClass (Identifier ("lit"));
    restyle (root);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.5);
    EXPECT_NEAR (glow (knob), 7.0, 1.0e-9);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.6);
    EXPECT_DOUBLE_EQ (glow (knob), 2.0);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (StyleContextTests, AnIdleContextFollowsAChangeOfTheThemeInTheNextFrame)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    window.setVisible (true);

    Component root ("root");
    window.addAndMakeVisible (root);

    auto sheet = YuiParser::parseStyleSheet ("style { #root { glow: 2; } }");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());

    int themeChanges = 0;
    root.getStyleContext()->onPaletteChanged = [&themeChanges]
    {
        ++themeChanges;
    };

    native.callFrameListeners (0.0);
    ASSERT_FALSE (native.hasFrameListeners());
    ASSERT_FALSE (native.hasThemeChanged());

    // The window is what notices: it has a frame to run, in which the context looks at the theme
    theme->setPalette (ThemePalette ({}, ThemePalette::Mode::light));
    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_TRUE (native.hasThemeChanged());

    native.callFrameListeners (0.0);
    EXPECT_EQ (themeChanges, 1);
    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_FALSE (native.hasThemeChanged());

    // Another theme becoming the global one is followed the same way
    const ApplicationTheme::Ptr otherTheme (new ApplicationTheme());
    ApplicationTheme::setGlobalTheme (otherTheme);
    EXPECT_TRUE (native.hasThemeChanged());

    native.callFrameListeners (0.0);
    EXPECT_EQ (themeChanges, 2);
    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_FALSE (native.hasThemeChanged());

    ApplicationTheme::setGlobalTheme (theme);
}

TEST_F (StyleContextTests, AWindowTellsOnlyTheContextsOfItsOwnComponentsThatAThemeChanged)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    window.setVisible (true);

    Component otherWindow ("otherWindow");
    auto& otherNative = makeWindow (otherWindow);
    otherWindow.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    otherWindow.setVisible (true);

    auto sheet = YuiParser::parseStyleSheet ("style { #root { glow: 2; } }");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();

    Component root ("root");
    window.addAndMakeVisible (root);
    root.setStyleSheet (sheet.getReference());

    Component otherRoot ("root");
    otherWindow.addAndMakeVisible (otherRoot);
    otherRoot.setStyleSheet (sheet.getReference());

    int themeChanges = 0;
    int otherThemeChanges = 0;
    root.getStyleContext()->onPaletteChanged = [&themeChanges] { ++themeChanges; };
    otherRoot.getStyleContext()->onPaletteChanged = [&otherThemeChanges] { ++otherThemeChanges; };

    native.callFrameListeners (0.0);
    otherNative.callFrameListeners (0.0);

    theme->setPalette (ThemePalette ({}, ThemePalette::Mode::light));

    // Each window follows the theme in a frame of its own: one running its frame reaches no context of the other
    native.callFrameListeners (0.0);
    EXPECT_EQ (themeChanges, 1);
    EXPECT_EQ (otherThemeChanges, 0);
    EXPECT_TRUE (otherNative.hasThemeChanged());

    otherNative.callFrameListeners (0.0);
    EXPECT_EQ (otherThemeChanges, 1);
    EXPECT_FALSE (otherNative.hasThemeChanged());
}

TEST_F (StyleContextTests, TheFirstStyleIsNotTransitioned)
{
    Component root ("root");
    attach (root, "style { #root { glow: 10; transition: glow 1s; } }");

    EXPECT_DOUBLE_EQ (glow (root), 10.0);
    EXPECT_EQ (root.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);
}

TEST_F (StyleContextTests, TransitionsStateBackedPropertiesBothWays)
{
    Component root ("root");
    root.setOpacity (1.0f);
    root.addStyleClass (Identifier ("fade"));

    attach (root, "style { .fade { transition: opacity 1s; } .faded { opacity: 0.5; } }");

    root.addStyleClass (Identifier ("faded"));
    advance (root, 0.0);
    EXPECT_NEAR (root.getOpacity(), 1.0f, 0.01f);

    advance (root, 0.5);
    EXPECT_NEAR (root.getOpacity(), 0.75f, 0.01f);

    advance (root, 0.6);
    EXPECT_NEAR (root.getOpacity(), 0.5f, 0.01f);

    root.removeStyleClass (Identifier ("faded"));
    advance (root, 0.0);
    EXPECT_NEAR (root.getOpacity(), 0.5f, 0.01f);

    advance (root, 0.5);
    EXPECT_NEAR (root.getOpacity(), 0.75f, 0.01f);

    advance (root, 0.6);
    EXPECT_NEAR (root.getOpacity(), 1.0f, 0.01f);

    root.removeStyleClass (Identifier ("fade"));
    advance (root, 0.0);
    EXPECT_NEAR (root.getOpacity(), 1.0f, 0.01f);
}

TEST_F (StyleContextTests, RetargetsARunningTransitionFromWhereItIs)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    root.removeStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 2.5);

    advance (root, 0.6);
    EXPECT_DOUBLE_EQ (glow (root), 0.0);
}

TEST_F (StyleContextTests, TransitionsTheNewStyleDoesNotDeclareStop)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } .instant { transition: none; } }");

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    root.addStyleClass (Identifier ("instant"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 10.0);
    EXPECT_EQ (root.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);
}

TEST_F (StyleContextTests, ExplicitValuesWinOverAnimatedOnes)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");
    root.setProperty (Identifier ("glow"), 3.0);

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 3.0);
}

TEST_F (StyleContextTests, PlaysKeyframeAnimations)
{
    Component root ("root");
    attach (root, "style { .pulsing { animation: pulse 1s cubicBezier (0, 0, 1, 1); } @keyframes pulse { from { glow: 0; } to { glow: 10; } } }");

    root.addStyleClass (Identifier ("pulsing"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 0.0);

    advance (root, 0.25);
    EXPECT_NEAR (glow (root), 2.5, 1.0e-4);

    advance (root, 1.0);
    EXPECT_TRUE (root.findProperty (Identifier ("glow")).isVoid());
}

TEST_F (StyleContextTests, AnimationsFillingForwardsHoldUntilRemoved)
{
    Component root ("root");
    attach (root, "style { .pulsing { animation: pulse 1s forwards; } @keyframes pulse { from { glow: 0; } to { glow: 10; } } }");

    root.addStyleClass (Identifier ("pulsing"));
    advance (root, 0.0);
    advance (root, 1.5);
    EXPECT_DOUBLE_EQ (glow (root), 10.0);

    root.removeStyleClass (Identifier ("pulsing"));
    advance (root, 0.0);
    EXPECT_TRUE (root.findProperty (Identifier ("glow")).isVoid());
}

TEST_F (StyleContextTests, MissingKeyframeEndsUseTheStyledValue)
{
    Component root ("root");
    attach (root, "style { .flashing { glow: 4; animation: flash 1s; } @keyframes flash { 50% { glow: 10; } } }");

    root.addStyleClass (Identifier ("flashing"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 4.0);

    advance (root, 0.25);
    EXPECT_DOUBLE_EQ (glow (root), 7.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 7.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 4.0);
}

TEST_F (StyleContextTests, ReportsAnimationDiagnostics)
{
    Component root ("root");
    attach (root, "style { #root { transition: glow 1; animation: missing 1s; } }");

    expectDiagnostic (root, "transition: Unexpected '1'");
    expectDiagnostic (root, "Unknown keyframes 'missing'");
}

TEST_F (StyleContextTests, ReportsTheDeclarationOfEachDiagnostic)
{
    const auto parsed = YuiParser::parse (R"yui(drawing Broken { circle { radius: missingSymbol; } }
style {
    #root {
        palette: Missing;
        transition: glow 1;
        text: "x";
        glow: notAKnownSymbol * 2;
        cursor: finger;
        effect: missingEffect;
        shadow: 1 red;
        animation: missing 1s;
    }
    #a { drawing: Missing; shadow: 0 0 2 notAColor; animation: 1s; }
    #b { drawing: Broken; animation: bad 1s; }
    @keyframes bad { from { text: "y"; } to { glow: notAKnownSymbol; } }
})yui");

    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    TextButton root ("root");
    Component a ("a");
    TextButton b ("b");
    root.addChildComponent (a);
    root.addChildComponent (b);
    root.setStyleSheet (build (parsed.document));

    StringArray reported;
    root.getStyleContext()->onDiagnostic = [&] (const DataTreeDiagnostic& diagnostic)
    {
        reported.add (parsed.sourceMap.locate (diagnostic).toString());
    };

    restyle (root);

    EXPECT_EQ (reported.size(), static_cast<int> (root.getStyleContext()->getDiagnostics().size()));

    for (const auto* expected : { "4:9: Unknown palette 'Missing'",
                                  "5:9: transition: ",
                                  "6:9: The property 'text' can't be styled",
                                  "7:9: glow: ",
                                  "8:9: Unknown cursor 'finger'",
                                  "9:9: Unknown effect 'missingEffect'",
                                  "10:9: shadow: Expected",
                                  "11:9: Unknown keyframes 'missing'",
                                  "13:10: Unknown drawing 'Missing'",
                                  "13:28: shadow[0]: ",
                                  "13:53: animation: ",
                                  "1:18: drawing Broken: ",
                                  "15:29: The property 'text' can't be animated",
                                  "15:47: bad glow: " })
    {
        EXPECT_TRUE (std::any_of (reported.begin(), reported.end(), [&] (const String& line)
        {
            return line.startsWith (expected);
        })) << "No diagnostic starting with '" << expected << "' in:\n"
            << reported.joinIntoString ("\n");
    }
}

TEST_F (StyleContextTests, ReportsDiagnosticsOfRulesMatchingLater)
{
    Component root ("root");
    attach (root, "style { #root.late { glow: notAKnownSymbol; } }");

    std::vector<DataTreeDiagnostic> reported;
    root.getStyleContext()->onDiagnostic = [&] (const DataTreeDiagnostic& diagnostic)
    {
        reported.push_back (diagnostic);
    };

    root.addStyleClass (Identifier ("late"));
    restyle (root);

    ASSERT_EQ (reported.size(), 1u);
    EXPECT_TRUE (reported[0].message.startsWith ("glow: ")) << reported[0].message;
    EXPECT_EQ (reported[0].element.getProperty ("name").toString(), "glow");

    root.removeStyleClass (Identifier ("late"));
    restyle (root);

    EXPECT_TRUE (root.getStyleContext()->getDiagnostics().empty());
    EXPECT_EQ (reported.size(), 1u);
}

TEST_F (StyleContextTests, StylesAndRestoresTheMouseCursor)
{
    Component root ("root");
    root.setMouseCursor (MouseCursor::Crosshair);

    attach (root, "style { .link { cursor: hand; } }");
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Crosshair);

    root.addStyleClass (Identifier ("link"));
    restyle (root);
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Hand);

    root.removeStyleClass (Identifier ("link"));
    restyle (root);
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Crosshair);
}

TEST_F (StyleContextTests, ReportsUnknownCursors)
{
    Component root ("root");
    attach (root, "style { #root { cursor: finger; } }");

    expectDiagnostic (root, "Unknown cursor 'finger'");
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Default);
}

TEST_F (StyleContextTests, InstallsRegisteredEffectsAndRestoresTheOriginal)
{
    const Identifier effectName ("styleTestEffect");
    ComponentEffectRegistry::getInstance().registerEffect (effectName, []
    {
        return ComponentEffect::Ptr (new StyleTestEffect());
    });

    Component root ("root");
    const ComponentEffect::Ptr explicitEffect (new StyleTestEffect());
    root.setComponentEffect (explicitEffect);

    attach (root, "style { .neon { effect: styleTestEffect; } .flat { effect: none; } }");
    EXPECT_EQ (root.getComponentEffect(), explicitEffect);

    root.addStyleClass (Identifier ("neon"));
    restyle (root);

    const auto installed = root.getComponentEffect();
    EXPECT_NE (installed, explicitEffect);
    EXPECT_NE (dynamic_cast<StyleTestEffect*> (installed.get()), nullptr);

    root.getStyleContext()->markDirty (root);
    restyle (root);
    EXPECT_EQ (root.getComponentEffect(), installed);

    root.addStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_EQ (root.getComponentEffect(), nullptr);

    root.removeStyleClass (Identifier ("neon"));
    root.removeStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_EQ (root.getComponentEffect(), explicitEffect);

    ComponentEffectRegistry::getInstance().unregisterEffect (effectName);
}

TEST_F (StyleContextTests, ReportsUnknownEffects)
{
    Component root ("root");
    attach (root, "style { #root { effect: missingEffect; } }");

    expectDiagnostic (root, "Unknown effect 'missingEffect'");
    EXPECT_EQ (root.getComponentEffect(), nullptr);
}

TEST_F (StyleContextTests, ParsesShadowLayersAndWidensThePaintMargin)
{
    Component root ("root");
    attach (root, "style { #root { shadow: 0 4 12 #00000080, -2 0 3 1 #ff0000; } }");

    ASSERT_NE (root.getStyleState(), nullptr);
    const auto& shadows = root.getStyleState()->getShadows();
    ASSERT_EQ (shadows.size(), 2u);

    EXPECT_EQ (shadows[0].offset, Point<float> (0.0f, 4.0f));
    EXPECT_FLOAT_EQ (shadows[0].blur, 12.0f);
    EXPECT_FLOAT_EQ (shadows[0].spread, 0.0f);
    EXPECT_EQ (shadows[0].color, Color (0x80000000));

    EXPECT_EQ (shadows[1].offset, Point<float> (-2.0f, 0.0f));
    EXPECT_FLOAT_EQ (shadows[1].blur, 3.0f);
    EXPECT_FLOAT_EQ (shadows[1].spread, 1.0f);
    EXPECT_EQ (shadows[1].color, Color (0xffff0000));

    const auto margin = root.getPaintMargin();
    EXPECT_FLOAT_EQ (margin.getLeft(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getTop(), 8.0f);
    EXPECT_FLOAT_EQ (margin.getRight(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getBottom(), 16.0f);
}

TEST_F (StyleContextTests, ShadowColorsReadThePalette)
{
    Component root ("root");
    attach (root, R"yui(
        palette Ember { colors: "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f"; accent: #ff9a1f; }
        palette Ice   { colors: "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f"; accent: #22aaff; }
        style {
            #root { palette: Ember; shadow: 0 0 8 palette.accent.withAlpha (0.5); }
            #root.cool { palette: Ice; }
        })yui");

    ASSERT_NE (root.getStyleState(), nullptr);
    EXPECT_EQ (root.getStyleState()->getShadows().at (0).color, Color (0xffff9a1f).withAlpha (0.5f));

    root.addStyleClass (Identifier ("cool"));
    restyle (root);
    EXPECT_EQ (root.getStyleState()->getShadows().at (0).color, Color (0xff22aaff).withAlpha (0.5f));
}

TEST_F (StyleContextTests, RemovingShadowsRestoresThePaintMargin)
{
    Component root ("root");
    root.setPaintMargin (2.0f);

    attach (root, "style { .card { shadow: 0 0 6 #000000; } .flat { shadow: none; } }");

    root.addStyleClass (Identifier ("card"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 6.0f);

    root.addStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_TRUE (root.getStyleState()->getShadows().empty());
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 2.0f);

    root.removeStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 6.0f);

    root.removeStyleClass (Identifier ("card"));
    restyle (root);
    EXPECT_TRUE (root.getStyleState()->getShadows().empty());
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 2.0f);
}

TEST_F (StyleContextTests, ARuleSetsThePaintMarginAndRestoresItWhenItStopsApplying)
{
    Component root ("root");
    root.setPaintMargin (2.0f);

    attach (root, R"yui(style { .glowing { paintMargin: 20; } .wide { paintMargin: "4 8"; } })yui");
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 2.0f);

    root.addStyleClass (Identifier ("glowing"));
    restyle (root);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float> (20.0f));

    // One, two or four values, in the order margin takes them: top and bottom, then left and right
    root.addStyleClass (Identifier ("wide"));
    restyle (root);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float> (8.0f, 4.0f, 8.0f, 4.0f));

    root.removeStyleClass (Identifier ("wide"));
    root.removeStyleClass (Identifier ("glowing"));
    restyle (root);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float> (2.0f));
}

TEST_F (StyleContextTests, ABlurFilterWidensThePaintMarginByWhatItSpreadsOver)
{
    Component root ("root");

    attach (root, "style { .soft { filter: blur(4px); } }");
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float>());

    root.addStyleClass (Identifier ("soft"));
    restyle (root);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float> (12.0f));

    root.removeStyleClass (Identifier ("soft"));
    restyle (root);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float>());
}

TEST_F (StyleContextTests, AFilterSpreadsTheShadowsAndWhatIsInThePaintMargin)
{
    Component root ("root");
    attach (root, "style { #root { paintMargin: 10; filter: blur(2px); } .floating { shadow: 0 0 30 #000000; } }");

    // The filter runs over what is painted in the margin: a blur of 2 spreads it 6 further
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float> (16.0f));

    root.addStyleClass (Identifier ("floating"));
    restyle (root);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float> (36.0f));
}

TEST_F (StyleContextTests, AShadowWidensThePaintMarginARuleSets)
{
    Component root ("root");
    attach (root, "style { #root { paintMargin: 10; } .lifted { shadow: 0 0 6 #000000; } .floating { shadow: 0 0 30 #000000; } }");
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 10.0f);

    // The margin is the larger of the one set and the reach of the shadows
    root.addStyleClass (Identifier ("lifted"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 10.0f);

    root.addStyleClass (Identifier ("floating"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 30.0f);

    root.removeStyleClass (Identifier ("floating"));
    root.removeStyleClass (Identifier ("lifted"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 10.0f);
}

TEST_F (StyleContextTests, ReportsMalformedShadows)
{
    Component root ("root");
    attach (root, "style { #root { shadow: 0 0 4 #000000, 4 #000000; } }");

    expectDiagnostic (root, "shadow: Expected 'x y [blur [spread]] color'");
    EXPECT_TRUE (root.getStyleState()->getShadows().empty());
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 0.0f);
}

TEST_F (StyleContextTests, ParsesGlowLayersComposesWithShadowsAndClearsOnNone)
{
    Component root ("root");
    attach (root, R"yui(
        palette Ember { colors: "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f"; accent: #ff9a1f; }
        style {
            #root {
                palette: Ember;
                shadow: 0 4 6 #00000080;
                glow: 10 2 palette.accent.withAlpha (0.5), 4 #ffffff40;
            }
            #root.dim { glow: none; }
        })yui");

    ASSERT_NE (root.getStyleState(), nullptr);
    const auto& shadows = root.getStyleState()->getShadows();
    const auto& glows = root.getStyleState()->getGlows();
    ASSERT_EQ (shadows.size(), 1u);
    ASSERT_EQ (glows.size(), 2u);

    EXPECT_EQ (glows[0].offset, Point<float> (0.0f, 0.0f));
    EXPECT_FLOAT_EQ (glows[0].blur, 10.0f);
    EXPECT_FLOAT_EQ (glows[0].spread, 2.0f);
    EXPECT_EQ (glows[0].color, Color (0xffff9a1f).withAlpha (0.5f));

    EXPECT_EQ (glows[1].offset, Point<float> (0.0f, 0.0f));
    EXPECT_FLOAT_EQ (glows[1].blur, 4.0f);
    EXPECT_FLOAT_EQ (glows[1].spread, 0.0f);
    EXPECT_EQ (glows[1].color, Color (0x40ffffff));

    const auto margin = root.getPaintMargin();
    EXPECT_FLOAT_EQ (margin.getLeft(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getTop(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getRight(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getBottom(), 12.0f);

    root.addStyleClass (Identifier ("dim"));
    restyle (root);
    EXPECT_TRUE (root.getStyleState()->getGlows().empty());
    EXPECT_EQ (root.getStyleState()->getShadows().size(), 1u);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getBottom(), 10.0f);
}

TEST_F (StyleContextTests, ReportsMalformedGlowAndInvalidGlowColor)
{
    Component root ("root");
    attach (root, "style { #root { glow: 8 2 4 #ff0000; } }");
    expectDiagnostic (root, "glow: Expected 'blur [spread] color'");

    Component second ("second");
    attach (second, "style { #second { glow: 8 notAColor; } }");
    expectDiagnostic (second, "glow[0]: ");
}

TEST_F (StyleContextTests, AppliesCssFilterDeclarations)
{
    Component root ("root");
    attach (root, "style { .blurred { filter: blur(4px); } .gray { filter: grayscale(100%); } }");

    root.addStyleClass (Identifier ("blurred"));
    restyle (root);
    ASSERT_NE (root.getStyleState(), nullptr);
    ASSERT_NE (root.getStyleState()->getFilterEffect(), nullptr);
    ASSERT_EQ (root.getStyleState()->getFilterEffect()->getOperations().size(), 1u);
    EXPECT_EQ (root.getStyleState()->getFilterEffect()->getOperations()[0].type, FilterEffect::Type::blur);
    EXPECT_EQ (root.getComponentEffect(), nullptr);

    root.removeStyleClass (Identifier ("blurred"));
    root.addStyleClass (Identifier ("gray"));
    restyle (root);
    ASSERT_NE (root.getStyleState()->getFilterEffect(), nullptr);
    ASSERT_EQ (root.getStyleState()->getFilterEffect()->getOperations().size(), 1u);
    EXPECT_EQ (root.getStyleState()->getFilterEffect()->getOperations()[0].type, FilterEffect::Type::grayscale);
}

namespace yup
{

template <>
class ComponentTestHelper<StyleContext>
{
public:
    static void triggerMouseEnter (Component& component, const MouseEvent& event) { component.internalMouseEnter (event); }
    static void triggerMouseExit (Component& component, const MouseEvent& event) { component.internalMouseExit (event); }
    static void triggerMouseDown (Component& component, const MouseEvent& event) { component.internalMouseDown (event); }
    static void triggerMouseUp (Component& component, const MouseEvent& event) { component.internalMouseUp (event); }
    static void triggerFocusGained (Component& component) { component.internalFocusChanged (true); }
    static void triggerFocusLost (Component& component) { component.internalFocusChanged (false); }
};

} // namespace yup

TEST_F (StyleContextTests, AncestorStateClassAndIdChangesRestyleCombinatorDescendants)
{
    Component root ("root");
    Component row ("row");
    Component icon ("icon");
    icon.addStyleClass (Identifier ("icon"));

    root.addAndMakeVisible (row);
    row.addAndMakeVisible (icon);

    attach (root, R"yui(
        style {
            .icon { glow: 1; }
            #row:hover .icon { glow: 2; }
            #row:pressed > .icon { glow: 3; }
            #row:focused .icon { glow: 4; }
            .active .icon { glow: 5; }
            #featured > .icon { glow: 6; }
            #row:disabled .icon { glow: 7; }
        })yui");

    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    ComponentTestHelper<StyleContext>::triggerMouseEnter (row, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 2.0);

    ComponentTestHelper<StyleContext>::triggerMouseDown (row, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 3.0);

    ComponentTestHelper<StyleContext>::triggerMouseUp (row, MouseEvent {});
    ComponentTestHelper<StyleContext>::triggerMouseExit (row, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    ComponentTestHelper<StyleContext>::triggerFocusGained (row);
    restyle (root);
    EXPECT_TRUE (row.isFocused());
    EXPECT_TRUE (static_cast<bool> (row.getProperty (Identifier ("focused"))));
    EXPECT_DOUBLE_EQ (glow (icon), 4.0);

    ComponentTestHelper<StyleContext>::triggerFocusLost (row);
    restyle (root);
    EXPECT_FALSE (row.isFocused());
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    row.addStyleClass (Identifier ("active"));
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 5.0);

    row.removeStyleClass (Identifier ("active"));
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    row.setEnabled (false);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 7.0);

    row.setEnabled (true);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    row.setComponentID ("featured");
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 6.0);
}

TEST_F (StyleContextTests, HoverAndPressedSetWithoutAPointerMatchTheirPseudoClasses)
{
    Component root ("root");
    Component row ("row");
    Component icon ("icon");
    icon.addStyleClass (Identifier ("icon"));

    root.addAndMakeVisible (row);
    row.addAndMakeVisible (icon);

    attach (root, R"yui(
        style {
            .icon { glow: 1; }
            #row:hover .icon { glow: 2; }
            #row:pressed > .icon { glow: 3; }
        })yui");

    icon.setHovered (true);
    restyle (root);
    EXPECT_TRUE (row.isHovered());
    EXPECT_TRUE (root.isHovered());
    EXPECT_DOUBLE_EQ (glow (icon), 2.0);

    icon.setPressed (true);
    restyle (root);
    EXPECT_TRUE (row.isPressed());
    EXPECT_DOUBLE_EQ (glow (icon), 3.0);

    icon.setPressed (false);
    icon.setHovered (false);
    restyle (root);
    EXPECT_FALSE (row.isHovered());
    EXPECT_FALSE (row.isPressed());
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);
}

TEST_F (StyleContextTests, FocusWithinAndActiveRestyleAncestorsAndSiblings)
{
    Component root ("root");
    Component row ("row");
    Component input ("input");
    Component icon ("icon");
    icon.addStyleClass (Identifier ("icon"));

    root.addAndMakeVisible (row);
    row.addAndMakeVisible (input);
    row.addAndMakeVisible (icon);

    attach (root, R"yui(
        style {
            #row { glow: 1; }
            #row:focus-within { glow: 10; }
            .icon { glow: 2; }
            #row:focus-within .icon { glow: 20; }
            #input:active { glow: 30; }
        })yui");

    EXPECT_DOUBLE_EQ (glow (row), 1.0);
    EXPECT_DOUBLE_EQ (glow (icon), 2.0);

    ComponentTestHelper<StyleContext>::triggerFocusGained (input);
    restyle (root);
    EXPECT_TRUE (input.hasFocusWithin());
    EXPECT_TRUE (row.hasFocusWithin());
    EXPECT_TRUE (root.hasFocusWithin());
    EXPECT_DOUBLE_EQ (glow (row), 10.0);
    EXPECT_DOUBLE_EQ (glow (icon), 20.0);

    ComponentTestHelper<StyleContext>::triggerFocusLost (input);
    restyle (root);
    EXPECT_FALSE (row.hasFocusWithin());
    EXPECT_DOUBLE_EQ (glow (row), 1.0);
    EXPECT_DOUBLE_EQ (glow (icon), 2.0);

    ComponentTestHelper<StyleContext>::triggerMouseDown (input, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (input), 30.0);

    ComponentTestHelper<StyleContext>::triggerMouseUp (input, MouseEvent {});
    restyle (root);
    EXPECT_TRUE (input.getProperty (Identifier ("glow")).isVoid());
    EXPECT_DOUBLE_EQ (glow (input), 1.0);
}

TEST_F (StyleContextTests, DetachingAChildStopsItsAnimationsAndClearsCachedState)
{
    Component root ("root");
    Component knob ("knob");
    knob.addStyleClass (Identifier ("knob"));
    root.addAndMakeVisible (knob);

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");
    EXPECT_DOUBLE_EQ (glow (knob), 0.0);

    knob.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.25);
    EXPECT_DOUBLE_EQ (glow (knob), 2.5);
    ASSERT_NE (knob.getStyleState(), nullptr);
    EXPECT_NE (knob.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);

    const auto detachedGlow = glow (knob);
    root.removeChildComponent (&knob);

    EXPECT_EQ (knob.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (knob), detachedGlow);

    alignas (Component) unsigned char slot[sizeof (Component)];
    auto* first = new (slot) Component ("slot");
    first->addStyleClass (Identifier ("lit"));
    root.addAndMakeVisible (first);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (*first), 10.0);
    first->~Component();

    auto* second = new (slot) Component ("slot");
    root.addAndMakeVisible (second);
    restyle (root);
    EXPECT_TRUE (second->findProperty (Identifier ("glow")).isVoid());
    root.removeChildComponent (second);
    second->~Component();
}

TEST_F (StyleContextTests, ComponentSetPaletteMarksSubtreeDirtyAndRefreshesPaletteRules)
{
    Component root ("root");
    Component section ("section");
    Slider child (Slider::LinearHorizontal);
    child.addStyleClass (Identifier ("themed"));
    section.addAndMakeVisible (child);
    root.addAndMakeVisible (section);

    attach (root, "style { .themed { thumbColor: palette.accent; } }");

    ThemePalette custom;
    custom.setColor (ThemePalette::Role::accent, Colors::magenta);
    section.setPalette (custom);

    restyle (root);
    EXPECT_EQ (VariantConverter<Color>::fromVar (child.findProperty (Identifier ("thumbColor"))), Colors::magenta);
}

TEST_F (StyleContextTests, NestedStylesheetReleasesOuterComputedValuesAndRestoresOnRemoval)
{
    Component root ("root");
    Component inner ("inner");
    Slider slider (Slider::LinearHorizontal);
    inner.addAndMakeVisible (slider);
    root.addAndMakeVisible (inner);

    attach (root, "style { Slider { trackWidth: 14; opacity: 0.5; } }");
    EXPECT_DOUBLE_EQ (static_cast<double> (slider.findProperty (Identifier ("trackWidth"))), 14.0);
    EXPECT_NEAR (slider.getOpacity(), 0.5f, 1.0f / 255.0f);

    attach (inner, "style { Slider { thumbColor: #ff0000; } }");
    EXPECT_EQ (VariantConverter<Color>::fromVar (slider.findProperty (Identifier ("thumbColor"))), Color (0xffff0000));
    EXPECT_NE (static_cast<double> (slider.findProperty (Identifier ("trackWidth"))), 14.0);
    EXPECT_FLOAT_EQ (slider.getOpacity(), 1.0f);

    inner.setStyleSheet (nullptr);
    restyle (root);
    EXPECT_DOUBLE_EQ (static_cast<double> (slider.findProperty (Identifier ("trackWidth"))), 14.0);
    EXPECT_NEAR (slider.getOpacity(), 0.5f, 1.0f / 255.0f);
}

TEST_F (StyleContextTests, UnwatchedStateChangesDoNotTriggerRestyle)
{
    struct CountingComponent : public Component
    {
        using Component::Component;

        void styleChanged() override { ++styleChanges; }

        int styleChanges = 0;
    };

    Component root ("root");
    CountingComponent button ("btn");
    button.addStyleClass (Identifier ("knob"));
    root.addAndMakeVisible (button);

    attach (root, "style { .knob { glow: 4; } }");
    button.styleChanges = 0;

    ComponentTestHelper<StyleContext>::triggerMouseEnter (button, MouseEvent {});
    ComponentTestHelper<StyleContext>::triggerMouseDown (button, MouseEvent {});
    ComponentTestHelper<StyleContext>::triggerMouseUp (button, MouseEvent {});
    ComponentTestHelper<StyleContext>::triggerMouseExit (button, MouseEvent {});
    ComponentTestHelper<StyleContext>::triggerFocusGained (button);
    ComponentTestHelper<StyleContext>::triggerFocusLost (button);
    button.setEnabled (false);
    button.setEnabled (true);
    restyle (root);

    EXPECT_EQ (button.styleChanges, 0);
    EXPECT_DOUBLE_EQ (glow (button), 4.0);
}

TEST_F (StyleContextTests, DeclarationValuesFollowTokensParentsSiblingsAndModels)
{
    DataTree modelState ("state");
    modelState.beginTransaction().setProperty ("scale", 2.0);
    DataTreeBindingProvider provider (modelState);
    BindingModel model (provider);

    const ComponentTypeInfo rootTokens (Identifier ("RootTokens"),
                                        &Component::getClassTypeInfo(),
                                        { ComponentProperty { .name = Identifier ("unit"),
                                                              .type = ComponentProperty::Type::number,
                                                              .defaultValue = 1.0,
                                                              .isInherited = true } });

    Component root ("root");
    root.setTypeInfo (&rootTokens);
    root.setProperty (Identifier ("unit"), 4.0);

    Component panel ("panel");
    panel.setBounds (10.0f, 0.0f, 100.0f, 50.0f);

    Slider source (Slider::LinearHorizontal);
    source.setComponentID ("source");
    source.setRange (0.0, 10.0, 0.0);
    source.setValue (3.0, dontSendNotification);

    Component target ("target");

    panel.addAndMakeVisible (source);
    panel.addAndMakeVisible (target);
    root.addAndMakeVisible (panel);

    auto sheet = YuiParser::parseStyleSheet ("style { #target { glow: unit + parent.x + source.value * params.scale; } }");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());
    root.getStyleContext()->setComponentResolver ([&] (const String& name) -> Component*
    {
        if (name == "source")
            return &source;
        if (name == "params")
            return &model;

        return nullptr;
    });
    restyle (root);

    EXPECT_DOUBLE_EQ (glow (target), 4.0 + 10.0 + 3.0 * 2.0);

    root.setProperty (Identifier ("unit"), 8.0);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (target), 8.0 + 10.0 + 3.0 * 2.0);

    panel.setPosition ({ 25.0f, 0.0f });
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (target), 8.0 + 25.0 + 3.0 * 2.0);

    source.setValue (5.0, dontSendNotification);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (target), 8.0 + 25.0 + 5.0 * 2.0);

    modelState.beginTransaction().setProperty ("scale", 3.0);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (target), 8.0 + 25.0 + 5.0 * 3.0);
}

TEST_F (StyleContextTests, StylesAndAnimatesTransformInSeparateLayerComposedWithExplicitTransform)
{
    Component root ("root");
    root.setBounds (0.0f, 0.0f, 400.0f, 400.0f);
    root.setVisible (true);

    Component card ("card");
    card.setBounds (10.0f, 20.0f, 100.0f, 50.0f);
    card.setTransform (AffineTransform::translation (30.0f, 0.0f));
    card.addStyleClass (Identifier ("card"));
    root.addAndMakeVisible (card);

    attach (root, R"yui(
        style {
            .card { transition: transform 1s linear; }
            .shifted { transform: transform.translation (0, 40); }
        }
    )yui");

    EXPECT_EQ (card.getTransform(), AffineTransform::translation (30.0f, 0.0f));
    EXPECT_EQ (card.getCombinedTransform(), AffineTransform::translation (30.0f, 0.0f));

    card.addStyleClass (Identifier ("shifted"));
    advance (root, 0.0);
    advance (root, 0.5);

    // Explicit C++ transform stays untouched while the style transform animates and composes with it.
    EXPECT_EQ (card.getTransform(), AffineTransform::translation (30.0f, 0.0f));
    EXPECT_NEAR (card.getCombinedTransform().getTranslateX(), 30.0f, 1.0e-3f);
    EXPECT_NEAR (card.getCombinedTransform().getTranslateY(), 20.0f, 1.0e-3f);
    EXPECT_NE (card.getStyleState(), nullptr);
    EXPECT_NEAR (card.getStyleState()->getStyleTransform().getTranslateY(), 20.0f, 1.0e-3f);

    advance (root, 0.5);
    EXPECT_NEAR (card.getCombinedTransform().getTranslateX(), 30.0f, 1.0e-3f);
    EXPECT_NEAR (card.getCombinedTransform().getTranslateY(), 40.0f, 1.0e-3f);

    // Coordinate mapping and hit-testing follow the composed transform: card local (5, 5) maps to parent (10 + 30 + 5, 20 + 40 + 5) = (45, 65).
    EXPECT_EQ (root.findComponentAt ({ 45.0f, 65.0f }), &card);
    EXPECT_EQ (root.findComponentAt ({ 15.0f, 25.0f }), &root);

    // Removing the class transitions the style transform back to identity while preserving the explicit C++ transform.
    card.removeStyleClass (Identifier ("shifted"));
    advance (root, 0.0);
    advance (root, 1.0);
    EXPECT_EQ (card.getTransform(), AffineTransform::translation (30.0f, 0.0f));
    EXPECT_NEAR (card.getCombinedTransform().getTranslateX(), 30.0f, 1.0e-3f);
    EXPECT_NEAR (card.getCombinedTransform().getTranslateY(), 0.0f, 1.0e-3f);
}

TEST_F (StyleContextTests, EmitsWarningDiagnosticsForUndeclaredCustomPropertiesUnlessDeclaredInRuleOrDrawing)
{
    const auto parsed = YuiParser::parse (R"yui(
        drawing Ring {
            property ringThickness: number = 2;
            circle { center: bounds.center; radius: ringThickness; }
        }
        style {
            #undeclared { mysteryValue: 42; cornerRadius: 8; }
            #declaredWithDefault { property declaredProp: number = 1; declaredProp: 7; }
            #declaredWithoutDefault { property bareProp: number; bareProp: 9; }
            #withDrawing { drawing: Ring; ringThickness: 5; }
        }
    )yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    Component root ("root");
    Component undeclared ("undeclared");
    Component declaredWithDefault ("declaredWithDefault");
    Component declaredWithoutDefault ("declaredWithoutDefault");
    Component withDrawing ("withDrawing");
    root.addAndMakeVisible (undeclared);
    root.addAndMakeVisible (declaredWithDefault);
    root.addAndMakeVisible (declaredWithoutDefault);
    root.addAndMakeVisible (withDrawing);

    root.setStyleSheet (build (parsed.document));
    restyle (root);

    // Undeclared property still evaluates and writes into the style store, but emits a Severity::warning diagnostic.
    EXPECT_DOUBLE_EQ (static_cast<double> (undeclared.findProperty (Identifier ("mysteryValue"))), 42.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (declaredWithDefault.findProperty (Identifier ("declaredProp"))), 7.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (declaredWithoutDefault.findProperty (Identifier ("bareProp"))), 9.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (withDrawing.findProperty (Identifier ("ringThickness"))), 5.0);

    const auto& diagnostics = root.getStyleContext()->getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_EQ (diagnostics[0].severity, DataTreeDiagnostic::Severity::warning);
    EXPECT_TRUE (diagnostics[0].isWarning());
    EXPECT_FALSE (diagnostics[0].isError());
    EXPECT_TRUE (diagnostics[0].message.contains ("mysteryValue")) << diagnostics[0].message;

    const auto located = parsed.sourceMap.locate (diagnostics[0]);
    EXPECT_TRUE (located.toString().contains ("warning: ")) << located.toString();
}

TEST_F (StyleContextTests, RulesSetThePropertiesAShaderDeclaresWithTheirType)
{
    const auto parsed = YuiParser::parse (R"yui(
        shader Crt {
            fragment: "fx/crt.frag";
            property intensity: number = 0.5;
            property steps: integer = 4;
        }
        style {
            #screen { intensity: 0.25 + 0.5; steps: 2.7; sharpness: 3; }
        }
    )yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    Component root ("root");
    Component screen ("screen");
    root.addAndMakeVisible (screen);

    root.setStyleSheet (build (parsed.document));
    restyle (root);

    // A declared property has its type: the number given to an integer is cut to a whole one
    EXPECT_DOUBLE_EQ (static_cast<double> (screen.findProperty (Identifier ("intensity"))), 0.75);
    EXPECT_DOUBLE_EQ (static_cast<double> (screen.findProperty (Identifier ("steps"))), 2.0);

    // Only the property nothing declares is reported
    const auto& diagnostics = root.getStyleContext()->getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].isWarning());
    EXPECT_TRUE (diagnostics[0].message.contains ("sharpness")) << diagnostics[0].message;
}

TEST_F (StyleContextTests, ExplicitInstanceDrawingStyleBeatsStylesheetDrawingAndExposesProperties)
{
    const auto parsed = YuiParser::parse (R"yui(
        drawing DefaultDial {
            property dialScale: number = 1;
            circle { center: bounds.center; radius: 10 * dialScale; }
        }
        drawing CustomDial {
            property customThickness: number = 2;
            circle { center: bounds.center; radius: 10 + customThickness; }
        }
        style {
            Slider {
                drawing: DefaultDial;
                customThickness: 6;
            }
        }
    )yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    auto sheet = build (parsed.document);

    Component root ("root");
    Slider defaultSlider (Slider::RotaryVerticalDrag);
    Slider customSlider (Slider::RotaryVerticalDrag);

    auto explicitStyle = DrawingStyle::create (customSlider, sheet->findDrawing ("CustomDial"));
    ASSERT_TRUE (explicitStyle.wasOk()) << explicitStyle.getErrorMessage();
    customSlider.setStyle (explicitStyle.getReference());

    root.addAndMakeVisible (defaultSlider);
    root.addAndMakeVisible (customSlider);
    root.setStyleSheet (sheet);
    restyle (root);

    // defaultSlider gets DefaultDial from the stylesheet; customSlider keeps its explicit CustomDial
    // and still has customThickness typed and styled without an undeclared-property warning on customSlider.
    EXPECT_NE (defaultSlider.getStyle().get(), explicitStyle.getReference().get());
    EXPECT_EQ (customSlider.getStyle().get(), explicitStyle.getReference().get());
    EXPECT_DOUBLE_EQ (static_cast<double> (customSlider.findProperty (Identifier ("customThickness"))), 6.0);
}

TEST_F (StyleContextTests, DrawingPropertiesReachStyleRulesWhenMoreSpecificSelectorOverridesDrawing)
{
    const auto parsed = YuiParser::parse (R"yui(
        drawing PorcelainKnob {
            property bezelSize: number = 0.72;
            circle { center: bounds.center; radius: 16 * bezelSize; }
        }
        drawing SynthKnob {
            circle { center: bounds.center; radius: 16; }
        }
        style {
            .knob {
                drawing: PorcelainKnob;
                bezelSize: 0.8;
            }
            .synth .knob {
                drawing: SynthKnob;
            }
        }
    )yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    Component root ("root");
    Component synthSection ("synth");
    synthSection.addStyleClass (Identifier ("synth"));

    Slider porcelain (Slider::RotaryVerticalDrag);
    porcelain.addStyleClass (Identifier ("knob"));

    Slider synth (Slider::RotaryVerticalDrag);
    synth.addStyleClass (Identifier ("knob"));

    synthSection.addAndMakeVisible (synth);
    root.addAndMakeVisible (porcelain);
    root.addAndMakeVisible (synthSection);

    root.setStyleSheet (build (parsed.document));
    restyle (root);

    EXPECT_TRUE (root.getStyleContext()->getDiagnostics().empty())
        << DataTreeDiagnostic::join (root.getStyleContext()->getDiagnostics());
    EXPECT_FLOAT_EQ (static_cast<float> (porcelain.findProperty (Identifier ("bezelSize"))), 0.8f);
    EXPECT_FLOAT_EQ (static_cast<float> (synth.findProperty (Identifier ("bezelSize"))), 0.8f);
}

TEST_F (StyleContextTests, DrawingCompilationFailureLocatesChildPropertyLineInSourceMap)
{
    const auto parsed = YuiParser::parse (
        "drawing BrokenDial {\n"
        "    circle {\n"
        "        center: bounds.center;\n"
        "        radius: missingVar;\n"
        "    }\n"
        "}\n"
        "style {\n"
        "    Slider { drawing: BrokenDial; }\n"
        "}\n");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    Component root ("root");
    Slider slider (Slider::RotaryVerticalDrag);
    root.addAndMakeVisible (slider);
    root.setStyleSheet (build (parsed.document));
    restyle (root);

    const auto& diagnostics = root.getStyleContext()->getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u);
    const auto located = parsed.sourceMap.locate (diagnostics[0]);
    EXPECT_EQ (located.line, 4);
    EXPECT_EQ (diagnostics[0].property, Identifier ("radius"));
}

//==============================================================================
// The shaders of the sheet an `effect:` declaration selects

TEST_F (StyleContextTests, ReportsAMaterialShaderSelectedAsAnEffect)
{
    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Hologram { material: \"fx/hologram.glsl\"; }\n"
                                                 "style { #screen { effect: Hologram; } }\n"
                                                 "Component { Component { id: screen; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    // One problem, where the effect is selected: the name is not looked for among the registered effects
    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("The shader 'Hologram' is a material")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].line, 2);
    EXPECT_EQ (diagnostics[0].element.getProperty ("name").toString(), "effect");

    // A material is no shader to draw a picture with, and its file is not read
    EXPECT_EQ (document.findById ("screen")->getComponentEffect(), nullptr);
    EXPECT_EQ (document.getRoot()->getStyleContext()->findShader ("Hologram"), nullptr);
    EXPECT_TRUE (assetReads.empty());
}

TEST_F (StyleContextTests, ReportsOnceAShaderWhoseFileIsMissing)
{
    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Crt { fragment: \"fx/missing.frag\"; property intensity: number = 0.5; }\n"
                                                 "style { .crt { effect: Crt; } }\n"
                                                 "Component { Component { id: screen; class: crt; } Component { class: crt; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");

    // Two components select the shader: it is read once, and reported once where its file is named
    EXPECT_EQ (root->getStyleContext()->getDiagnostics().size(), 1u);

    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_EQ (diagnostics[0].message, "The shader 'Crt' can't read 'fx/missing.frag' nor its compiled bundle 'fx/missing.frag.ysl'");
    EXPECT_EQ (diagnostics[0].line, 1);
    EXPECT_EQ (diagnostics[0].property, Identifier ("fragment"));
    EXPECT_EQ (assetReads["fx/missing.frag.ysl"], 1);
    EXPECT_EQ (assetReads["fx/missing.frag"], 1);

    // No shader, so no effect and none of its defaults
    EXPECT_EQ (screen->getComponentEffect(), nullptr);
    EXPECT_TRUE (screen->findProperty (Identifier ("intensity")).isVoid());
    EXPECT_EQ (root->getStyleContext()->findShader ("Crt"), nullptr);

    // The next restyle does not read it nor report it again
    EXPECT_EQ (countProblemsOfARestyle (*root), 0u);
    EXPECT_EQ (assetReads["fx/missing.frag"], 1);
}

TEST_F (StyleContextTests, ReportsAShaderADrawingCantPaintWhereItIsUsedAndOneThatDoesNotLoadWhereItIsDeclared)
{
    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Hologram { material: \"fx/hologram.glsl\"; }\n"
                                                 "shader Missing { fragment: \"fx/missing.frag\"; }\n"
                                                 "drawing Lit { use Hologram { } }\n"
                                                 "drawing Lost {\n"
                                                 "    rect { id: back; bounds: bounds; fill: #202020; }\n"
                                                 "    use Missing { }\n"
                                                 "}\n"
                                                 "style { #lit { drawing: Lit; } #lost { drawing: Lost; } }\n"
                                                 "Component { Component { id: lit; } Component { id: lost; } Component { id: placed; drawing: Lost; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto& diagnostics = document.getDiagnostics();

    const auto lineOf = [&diagnostics] (const String& message)
    {
        for (const auto& diagnostic : diagnostics)
            if (diagnostic.message == message)
                return diagnostic.line;

        return 0;
    };

    // A material is told where it is used. A shader whose file is missing is told once, where the file is named, whatever
    // selects the drawings using it: a rule, or the element of a component
    EXPECT_EQ (diagnostics.size(), 2u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_EQ (lineOf ("drawing Lit: Lit/use[0]: the shader 'Hologram' is a material, a drawing takes a shader with a 'fragment'"), 3);
    EXPECT_EQ (lineOf ("The shader 'Missing' can't read 'fx/missing.frag' nor its compiled bundle 'fx/missing.frag.ysl'"), 2);
    EXPECT_EQ (assetReads.count ("fx/hologram.glsl"), 0u);
    EXPECT_EQ (assetReads["fx/missing.frag"], 1);

    // A drawing that can't be compiled is not installed. One whose shader is not loaded is, and paints the rest
    EXPECT_EQ (getDrawingStyle (document.findById ("lit")), nullptr);

    for (const auto* id : { "lost", "placed" })
    {
        auto* drawing = getDrawingStyle (document.findById (id));
        ASSERT_NE (drawing, nullptr) << id;

        const auto shaders = drawing->getShaders();
        ASSERT_EQ (shaders.size(), 1u) << id;
        EXPECT_EQ (shaders.front().shader, nullptr) << id;
        EXPECT_EQ (drawing->measureElements (*document.findById (id)).front().id, String ("back")) << id;
    }

    // The next restyle does not read the missing file again, nor tell about it
    document.getRoot()->getStyleContext()->markDirty (*document.getRoot());
    restyle (*document.getRoot());

    EXPECT_EQ (assetReads["fx/missing.frag"], 1);

    const auto& restyled = document.getRoot()->getStyleContext()->getDiagnostics();
    ASSERT_EQ (restyled.size(), 1u) << DataTreeDiagnostic::join (restyled);
    EXPECT_TRUE (restyled.front().message.startsWith ("drawing Lit: ")) << restyled.front().message;
}

TEST_F (StyleContextTests, ARuleNamesTheDrawingOrTheShaderOfAPropertyTakingAName)
{
    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Hologram { material: \"fx/hologram.glsl\"; }\n"
                                                 "drawing Dial { }\n"
                                                 "component Tile : Component { property look: drawing; }\n"
                                                 "style {\n"
                                                 "    #bare { look: Hologram; }\n"
                                                 "    #drawn { look: Dial; }\n"
                                                 "    #quoted { look: \"Hologram\"; }\n"
                                                 "    #unknown { look: Missing; }\n"
                                                 "}\n"
                                                 "Component { Tile { id: bare; } Tile { id: drawn; } Tile { id: quoted; } Tile { id: unknown; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto lookOf = [&document] (const char* id)
    {
        return document.findById (id)->findProperty (Identifier ("look")).toString();
    };

    // A name the sheet declares is written bare, a shader as a drawing and whatever the shader is for, or as a text
    EXPECT_EQ (lookOf ("bare"), String ("Hologram"));
    EXPECT_EQ (lookOf ("drawn"), String ("Dial"));
    EXPECT_EQ (lookOf ("quoted"), String ("Hologram"));
    EXPECT_TRUE (assetReads.empty());

    // A bare name it does not declare names nothing
    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_EQ (diagnostics[0].message, "look: unknown drawing or shader 'Missing'");
    EXPECT_EQ (diagnostics[0].line, 8);
    EXPECT_EQ (lookOf ("unknown"), String());
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (StyleContextTests, EffectInstallsAShaderOfTheSheetAndRestoresTheOriginal)
{
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Crt {
            fragment: "fx/crt.frag";
            property intensity: number = 0.5;
            property tint: color = #ff8800;
        }

        style {
            .crt  { effect: Crt; }
            .flat { effect: none; }
        }

        Component {
            Component { id: screen; }
            Component { id: other; class: crt; }
        })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");
    ASSERT_NE (screen, nullptr);

    const ComponentEffect::Ptr explicitEffect (new StyleTestEffect());
    screen->setComponentEffect (explicitEffect);

    screen->addStyleClass (Identifier ("crt"));
    restyle (*root);

    const auto installed = screen->getComponentEffect();
    auto* effect = dynamic_cast<ShaderEffect*> (installed.get());
    ASSERT_NE (effect, nullptr);
    ASSERT_NE (effect->getShader(), nullptr);
    EXPECT_EQ (effect->getShader()->getNumInputTextures(), 1);

    // The context gives the shader it loaded, with the properties its block declares
    const auto* shader = root->getStyleContext()->findShader ("Crt");
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->shader, effect->getShader());
    EXPECT_EQ (shader->block.getProperty ("name").toString(), "Crt");
    ASSERT_EQ (shader->properties.size(), 2u);
    EXPECT_EQ (shader->properties[0].name, Identifier ("intensity"));
    EXPECT_EQ (shader->properties[0].type, ComponentProperty::Type::number);
    EXPECT_EQ (shader->properties[1].name, Identifier ("tint"));
    EXPECT_EQ (shader->properties[1].type, ComponentProperty::Type::color);
    EXPECT_EQ (root->getStyleContext()->findShader ("Glow"), nullptr);

    // Each component has an effect of its own, running the shader read once for all of them
    auto* otherEffect = getShaderEffect (document.findById ("other"));
    ASSERT_NE (otherEffect, nullptr);
    EXPECT_NE (otherEffect, effect);
    EXPECT_EQ (otherEffect->getShader(), effect->getShader());
    EXPECT_EQ (assetReads["fx/crt.frag"], 1);

    // A restyle keeps the effect
    EXPECT_EQ (countProblemsOfARestyle (*root), 0u);
    EXPECT_EQ (screen->getComponentEffect(), installed);

    screen->addStyleClass (Identifier ("flat"));
    restyle (*root);
    EXPECT_EQ (screen->getComponentEffect(), nullptr);

    screen->removeStyleClass (Identifier ("crt"));
    screen->removeStyleClass (Identifier ("flat"));
    restyle (*root);
    EXPECT_EQ (screen->getComponentEffect(), explicitEffect);
}

TEST_F (StyleContextTests, AShaderOfTheSheetComesBeforeARegisteredEffectOfItsName)
{
    const auto makeEffect = []
    {
        return ComponentEffect::Ptr (new StyleTestEffect());
    };

    auto& registry = ComponentEffectRegistry::getInstance();
    registry.registerEffect (Identifier ("Crt"), makeEffect);
    registry.registerEffect (Identifier ("styleTestEffect"), makeEffect);

    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Crt { fragment: "fx/crt.frag"; }
        style { #screen { effect: Crt; } #plain { effect: styleTestEffect; } }
        Component { Component { id: screen; } Component { id: plain; } })yui");

    registry.unregisterEffect (Identifier ("Crt"));
    registry.unregisterEffect (Identifier ("styleTestEffect"));

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    EXPECT_NE (getShaderEffect (document.findById ("screen")), nullptr);
    EXPECT_NE (dynamic_cast<StyleTestEffect*> (document.findById ("plain")->getComponentEffect().get()), nullptr);
}

TEST_F (StyleContextTests, ReportsOnceWhatPreventsAShaderFromLoading)
{
    setAsset ("fx/crt.frag", crtFragment);
    setAsset ("fx/broken.frag", brokenFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Broken { fragment: \"fx/broken.frag\"; }\n"
                                                 "shader NoMember { fragment: \"fx/crt.frag\"; property gain: number = 1; }\n"
                                                 "shader NoType { fragment: \"fx/crt.frag\"; property tint: string = \"high\"; }\n"
                                                 "shader TooSmall { fragment: \"fx/crt.frag\"; property intensity: color = #ff0000; }\n"
                                                 "style { #a { effect: Broken; } #b { effect: NoMember; } #c { effect: NoType; } #d { effect: TooSmall; } }\n"
                                                 "Component { Component { id: a; } Component { id: b; } Component { id: c; } Component { id: d; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 4u) << DataTreeDiagnostic::join (diagnostics);

    // The message of the compiler, where the file is named
    EXPECT_TRUE (diagnostics[0].message.startsWith ("shader Broken: ")) << diagnostics[0].message;
    EXPECT_TRUE (diagnostics[0].message.contains ("missingValue")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].line, 1);
    EXPECT_EQ (diagnostics[0].property, Identifier ("fragment"));

    // What a property can't fill, where it is declared
    EXPECT_EQ (diagnostics[1].message, "The uniform block of the shader 'NoMember' has no member 'gain'");
    EXPECT_EQ (diagnostics[1].line, 2);
    EXPECT_EQ (diagnostics[1].element.getProperty ("name").toString(), "gain");

    EXPECT_TRUE (diagnostics[2].message.startsWith ("The property 'tint' of the shader 'NoType' can't be of type 'string'")) << diagnostics[2].message;
    EXPECT_EQ (diagnostics[2].line, 3);

    EXPECT_EQ (diagnostics[3].message, "The member 'intensity' of the uniform block of the shader 'TooSmall' is too small for a color");
    EXPECT_EQ (diagnostics[3].line, 4);

    for (const auto* id : { "a", "b", "c", "d" })
        EXPECT_EQ (document.findById (id)->getComponentEffect(), nullptr) << id;

    // None is read nor reported again by the next restyle
    const auto reads = assetReads["fx/crt.frag"];
    EXPECT_EQ (countProblemsOfARestyle (*document.getRoot()), 0u);
    EXPECT_EQ (assetReads["fx/crt.frag"], reads);
}

TEST_F (StyleContextTests, ReportsAMemberAShaderIsGivenThatItsBlockDeclaresAsSomethingElse)
{
    setAsset ("fx/scaled.frag", ownScaleFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Scaled { fragment: \"fx/scaled.frag\"; }\n"
                                                 "shader Placed { fragment: \"fx/scaled.frag\"; property scale: point; }\n"
                                                 "style { #a { effect: Scaled; } #b { effect: Placed; } }\n"
                                                 "Component { Component { id: a; } Component { id: b; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    // The pixels of a unit would be written over the two numbers of the shader: told where the shader is declared
    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_EQ (diagnostics[0].message, "The member 'scale' of the uniform block of the shader 'Scaled' is not a float, which is what a shader is given there: declare a property 'scale' to set it");
    EXPECT_EQ (diagnostics[0].line, 1);
    EXPECT_EQ (document.findById ("a")->getComponentEffect(), nullptr);

    // A property of that name fills the member, which the shader is then not given
    EXPECT_NE (getShaderEffect (document.findById ("b")), nullptr);
}

TEST_F (StyleContextTests, ReportsAShaderReadingTwoTexturesSelectedAsAnEffect)
{
    setAsset ("fx/mix.frag", twoInputsFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Mix { fragment: \"fx/mix.frag\"; }\n"
                                                 "style { #screen { effect: Mix; } }\n"
                                                 "Component { Component { id: screen; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");

    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_EQ (diagnostics[0].message, "The shader 'Mix' reads two textures, an effect gives it one");
    EXPECT_EQ (diagnostics[0].line, 2);
    EXPECT_EQ (screen->getComponentEffect(), nullptr);

    // The shader itself is loaded, for what has two textures to give it
    const auto* shader = root->getStyleContext()->findShader ("Mix");
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->shader->getNumInputTextures(), 2);

    // Once its file reads one texture, it is the effect
    setAsset ("fx/mix.frag", crtFragment);
    document.notifyAssetChanged ("fx/mix.frag");
    restyle (*root);

    EXPECT_NE (getShaderEffect (screen), nullptr);
    EXPECT_TRUE (root->getStyleContext()->getDiagnostics().empty());

    // And once it reads two again, the effect goes and the component is told why
    setAsset ("fx/mix.frag", twoInputsFragment);
    document.notifyAssetChanged ("fx/mix.frag");
    restyle (*root);

    EXPECT_EQ (screen->getComponentEffect(), nullptr);
    ASSERT_EQ (root->getStyleContext()->getDiagnostics().size(), 1u);
    EXPECT_EQ (root->getStyleContext()->getDiagnostics()[0].message, "The shader 'Mix' reads two textures, an effect gives it one");
}

TEST_F (StyleContextTests, AShaderSamplingASeriesReadsOnePictureAsAnEffect)
{
    setAsset ("fx/mix.frag", twoInputsFragment);

    YuiDocument document;
    serveAssets (document);

    // The second texture is filled with the numbers of a property, which leaves the shader one picture to read
    const auto loaded = document.loadFromString ("shader Mix { fragment: \"fx/mix.frag\"; property overlay: number[]; }\n"
                                                 "style { #screen { effect: Mix; } }\n"
                                                 "Component { Component { id: screen; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_NE (getShaderEffect (document.findById ("screen")), nullptr);

    const auto* shader = document.getRoot()->getStyleContext()->findShader ("Mix");
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (ShaderEffect::getNumPictures (*shader->shader, shader->properties), 1);
}

TEST_F (StyleContextTests, TheDefaultsOfAShaderAreStyledValuesOfTheComponentItIsTheEffectOf)
{
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Crt {
            fragment: "fx/crt.frag";
            property intensity: number = 0.5;
            property tint: color = palette.accent;
        }

        style {
            #screen     { effect: Crt; }
            #screen.hot { intensity: 0.75; }
        }

        Component {
            Component { id: screen; }
            Component { id: plain; }
        })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");
    const Identifier intensity ("intensity");

    const auto readIntensity = [&]
    {
        return static_cast<double> (screen->findProperty (intensity));
    };

    const auto readTint = [&]
    {
        return VariantConverter<Color>::fromVar (screen->findProperty (Identifier ("tint")));
    };

    // A constant, and a color read from the palette
    EXPECT_DOUBLE_EQ (readIntensity(), 0.5);
    EXPECT_EQ (readTint(), theme->getPalette().getColor (ThemePalette::Role::accent));

    // They are values of the component the effect is on, and of no other
    EXPECT_TRUE (document.findById ("plain")->findProperty (intensity).isVoid());
    EXPECT_TRUE (root->findProperty (intensity).isVoid());

    // A rule comes before the default, and a value set on the component before both
    screen->addStyleClass (Identifier ("hot"));
    restyle (*root);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.75);

    screen->setProperty (intensity, 0.25);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.25);

    screen->removeProperty (intensity);
    screen->removeStyleClass (Identifier ("hot"));
    restyle (*root);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.5);

    // The default follows the palette it reads
    ThemePalette custom;
    custom.setColor (ThemePalette::Role::accent, Colors::magenta);
    root->setPalette (custom);
    restyle (*root);
    EXPECT_EQ (readTint(), Colors::magenta);
}

TEST_F (StyleContextTests, TransitionsAPropertyOfAShaderFromItsDefaultAndBack)
{
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Crt { fragment: "fx/crt.frag"; property intensity: number = 0.5; }

        style {
            #screen     { effect: Crt; transition: intensity 1s; }
            #screen.hot { intensity: 1; }
        }

        Component { Component { id: screen; } })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");

    const auto readIntensity = [&]
    {
        return static_cast<double> (screen->findProperty (Identifier ("intensity")));
    };

    EXPECT_DOUBLE_EQ (readIntensity(), 0.5);

    screen->addStyleClass (Identifier ("hot"));
    advance (*root, 0.0);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.5);

    advance (*root, 0.5);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.75);

    advance (*root, 0.6);
    EXPECT_DOUBLE_EQ (readIntensity(), 1.0);

    // The default is where the property goes back to once no rule sets it
    screen->removeStyleClass (Identifier ("hot"));
    advance (*root, 0.0);
    EXPECT_DOUBLE_EQ (readIntensity(), 1.0);

    advance (*root, 0.5);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.75);

    advance (*root, 0.6);
    EXPECT_DOUBLE_EQ (readIntensity(), 0.5);
}

TEST_F (StyleContextTests, TheCompiledBundleNextToAShaderIsLoadedInsteadOfItsSource)
{
    // The source makes its picture alone and the bundle reads one, which tells them apart
    setAsset ("fx/crt.frag", proceduralFragment);
    assets["fx/crt.frag.ysl"] = compileBundle (crtFragment);
    ASSERT_GT (assets["fx/crt.frag.ysl"].getSize(), 0u);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Crt    { fragment: "fx/crt.frag"; }
        shader Bundle { fragment: "fx/crt.frag.ysl"; }
        style { #screen { effect: Crt; } #other { effect: Bundle; } }
        Component { Component { id: screen; } Component { id: other; } })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* effect = getShaderEffect (document.findById ("screen"));
    ASSERT_NE (effect, nullptr);
    EXPECT_EQ (effect->getShader()->getNumInputTextures(), 1);
    EXPECT_EQ (assetReads["fx/crt.frag"], 0);

    // A path that is a bundle is read as it is
    auto* bundleEffect = getShaderEffect (document.findById ("other"));
    ASSERT_NE (bundleEffect, nullptr);
    EXPECT_EQ (bundleEffect->getShader()->getNumInputTextures(), 1);
    EXPECT_EQ (assetReads["fx/crt.frag.ysl.ysl"], 0);
}

TEST_F (StyleContextTests, AnAssetThatChangesLoadsItsShaderAgainAndOneThatFailsKeepsTheShaderRunning)
{
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const String text ("shader Crt { fragment: \"fx/crt.frag\"; property intensity: number = 0.5; }\n"
                       "style { #screen { effect: Crt; } }\n"
                       "Component { Component { id: screen; } }");

    const auto loaded = document.loadFromString (text);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");
    auto* effect = getShaderEffect (screen);
    ASSERT_NE (effect, nullptr);

    const auto first = effect->getShader();
    ASSERT_NE (first, nullptr);
    EXPECT_EQ (first->getNumInputTextures(), 1);

    // The shader is loaded again by the restyle that follows, not when its file is told about
    setAsset ("fx/crt.frag", proceduralFragment);
    document.notifyAssetChanged ("fx/crt.frag");
    EXPECT_EQ (effect->getShader(), first);

    // The effect the component has then runs the shader of the new source
    restyle (*root);
    EXPECT_EQ (getShaderEffect (screen), effect);

    const auto second = effect->getShader();
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    EXPECT_EQ (second->getNumInputTextures(), 0);
    EXPECT_EQ (root->getStyleContext()->findShader ("Crt")->shader, second);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // Another file changing is nothing to it
    const auto reads = assetReads["fx/crt.frag"];
    document.notifyAssetChanged ("fx/glow.frag");
    EXPECT_EQ (countProblemsOfARestyle (*root), 0u);
    EXPECT_EQ (assetReads["fx/crt.frag"], reads);
    EXPECT_EQ (effect->getShader(), second);

    // A source that fails is reported where the file is named, and leaves the shader running
    setAsset ("fx/crt.frag", brokenFragment);
    document.notifyAssetChanged ("fx/crt.frag");
    restyle (*root);

    EXPECT_EQ (effect->getShader(), second);
    EXPECT_EQ (root->getStyleContext()->getDiagnostics().size(), 1u);

    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("shader Crt: ")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].line, 1);
    EXPECT_EQ (diagnostics[0].property, Identifier ("fragment"));

    // It is not read nor reported again by the restyles that follow
    EXPECT_EQ (countProblemsOfARestyle (*root), 0u);
    EXPECT_EQ (assetReads["fx/crt.frag"], reads + 1);

    // Loading the document again clears its problems: the shader is still one of them, and still runs
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    screen = document.findById ("screen");

    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("shader Crt: ")) << diagnostics[0].message;
    EXPECT_EQ (assetReads["fx/crt.frag"], reads + 1);
    ASSERT_EQ (getShaderEffect (screen), effect);
    EXPECT_EQ (effect->getShader(), second);

    // The bundle of the shader appearing next to it is a change of the shader too
    assets["fx/crt.frag.ysl"] = compileBundle (crtFragment);
    document.notifyAssetChanged ("fx/crt.frag.ysl");
    restyle (*document.getRoot());

    EXPECT_EQ (getShaderEffect (screen), effect);
    EXPECT_NE (effect->getShader(), second);
    EXPECT_EQ (effect->getShader()->getNumInputTextures(), 1);
    EXPECT_TRUE (document.getRoot()->getStyleContext()->getDiagnostics().empty());
}

TEST_F (StyleContextTests, AShaderThatCouldNotBeLoadedIsInstalledOnceItsFileIsThere)
{
    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Crt { fragment: \"fx/crt.frag\"; property intensity: number = 0.5; }\n"
                                                 "style { #screen { effect: Crt; } }\n"
                                                 "Component { Component { id: screen; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* screen = document.findById ("screen");
    EXPECT_EQ (screen->getComponentEffect(), nullptr);
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    setAsset ("fx/crt.frag", crtFragment);
    document.notifyAssetChanged ("fx/crt.frag");
    restyle (*document.getRoot());

    EXPECT_NE (getShaderEffect (screen), nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (screen->findProperty (Identifier ("intensity"))), 0.5);
}

TEST_F (StyleContextTests, AShaderBrokenByAnOverrideIsReportedByEachReloadAndKeepsRunning)
{
    YuiDocument document;
    YuiLoader loader (document);

    const auto loaded = loader.loadResource ("ui.yui", findShaderResource);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* effect = getShaderEffect (document.findById ("screen"));
    ASSERT_NE (effect, nullptr);

    const auto first = effect->getShader();
    ASSERT_NE (first, nullptr);

    // What an editor does with an unsaved text. The reload tells about the file before it loads the document, which
    // clears its problems: the shader is loaded by the restyle of that load, so what prevents it is one of them
    loader.setOverride ("fx/crt.frag", brokenFragment);
    ASSERT_TRUE (loader.reload().wasOk());

    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("shader Crt: ")) << diagnostics[0].message;
    EXPECT_TRUE (diagnostics[0].message.contains ("missingValue")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].line, 1);
    EXPECT_EQ (diagnostics[0].property, Identifier ("fragment"));
    EXPECT_EQ (getShaderEffect (document.findById ("screen")), effect);
    EXPECT_EQ (effect->getShader(), first);

    // It stands for as long as the source is broken, whatever is loaded again meanwhile
    ASSERT_TRUE (loader.reload().wasOk());

    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("shader Crt: ")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].property, Identifier ("fragment"));
    EXPECT_EQ (effect->getShader(), first);

    // Fixed, it is gone and the effect runs the new shader
    loader.setOverride ("fx/crt.frag", proceduralFragment);
    ASSERT_TRUE (loader.reload().wasOk());

    EXPECT_TRUE (diagnostics.empty()) << DataTreeDiagnostic::join (diagnostics);
    ASSERT_EQ (getShaderEffect (document.findById ("screen")), effect);
    ASSERT_NE (effect->getShader(), nullptr);
    EXPECT_NE (effect->getShader(), first);
    EXPECT_EQ (effect->getShader()->getNumInputTextures(), 0);
}

TEST_F (StyleContextTests, AShaderIsLoadedAgainOnceHoweverManyOfItsFilesAreToldAbout)
{
    int sourceReads = 0;

    const auto find = [&sourceReads] (const char* path, std::size_t& size)
    {
        if (String (path) == "fx/crt.frag")
            ++sourceReads;

        return findShaderResource (path, size);
    };

    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadResource ("ui.yui", find).wasOk());

    auto* effect = getShaderEffect (document.findById ("screen"));
    ASSERT_NE (effect, nullptr);

    const auto first = effect->getShader();
    EXPECT_EQ (sourceReads, 1);

    // Loading a source tells about every file read from the one before: for the shader its bundle, which was looked for
    // and not found, and its source
    ASSERT_TRUE (loader.loadResource ("ui.yui", find).wasOk());

    EXPECT_EQ (sourceReads, 2);
    ASSERT_EQ (getShaderEffect (document.findById ("screen")), effect);

    const auto second = effect->getShader();
    EXPECT_NE (second, first);

    // Told any number of times, the shader is loaded by the next restyle and not before
    auto* root = document.getRoot();
    auto* context = root->getStyleContext();

    context->assetChanged ("fx/crt.frag");
    context->assetChanged ("fx/crt.frag.ysl");
    context->assetChanged ("fx/crt.frag");

    EXPECT_EQ (sourceReads, 2);
    EXPECT_EQ (context->findShader ("Crt")->shader, second);

    restyle (*root);
    EXPECT_EQ (sourceReads, 3);
    EXPECT_NE (effect->getShader(), second);

    // And not by the ones that follow
    EXPECT_EQ (countProblemsOfARestyle (*root), 0u);
    EXPECT_EQ (sourceReads, 3);
}

TEST_F (StyleContextTests, ReplacingTheStyleSheetDropsTheShadersItLoaded)
{
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Crt { fragment: \"fx/crt.frag\"; }\n"
                                                 "style { #screen { effect: Crt; } }\n"
                                                 "Component { Component { id: screen; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* root = document.getRoot();
    auto* screen = document.findById ("screen");
    const auto sheet = root->getStyleSheet();

    ASSERT_NE (getShaderEffect (screen), nullptr);
    EXPECT_EQ (assetReads["fx/crt.frag"], 1);

    root->setStyleSheet (build (DataTree ("style")));
    restyle (*root);

    EXPECT_EQ (screen->getComponentEffect(), nullptr);
    EXPECT_EQ (root->getStyleContext()->findShader ("Crt"), nullptr);

    // The sheet coming back reads its shader again
    root->setStyleSheet (sheet);
    restyle (*root);

    EXPECT_NE (getShaderEffect (screen), nullptr);
    EXPECT_EQ (assetReads["fx/crt.frag"], 2);
}

TEST_F (StyleContextTests, ADrawingPaintsAShaderOfTheSheetWithTheValuesItsUseSets)
{
    setAsset ("fx/plasma.frag", proceduralFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Plasma {
            fragment: "fx/plasma.frag";
            property intensity: number = 0.5;
            property tint: color = #ff8800;
        }

        drawing Screen {
            property level: number = 0.25;
            use Plasma { intensity: level * 2; }
        }

        style {
            #first { drawing: Screen; }
            #second { drawing: Screen; level: 0.75; }
        }

        Component { Component { id: first; } Component { id: second; } }
    )yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* first = document.findById ("first");
    auto* second = document.findById ("second");
    ASSERT_NE (getDrawingStyle (first), nullptr);
    ASSERT_NE (getDrawingStyle (second), nullptr);

    // The shader is the one of the sheet, loaded once for the two drawings painting it
    const auto* shader = document.getRoot()->getStyleContext()->findShader ("Plasma");
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (assetReads["fx/plasma.frag"], 1);

    // It is given what the use sets, read from the properties of the drawing a rule styles, and the default of its block
    auto expected = shader->shader->createUniforms();
    EXPECT_TRUE (expected.set ("intensity", 0.5f));
    EXPECT_TRUE (ShaderEffect::setUniform (expected, { "tint", ComponentProperty::Type::color }, VariantConverter<Color>::toVar (Color (0xffff8800))));
    EXPECT_TRUE (getShaderUniforms (first) == expected);

    EXPECT_TRUE (expected.set ("intensity", 1.5f));
    EXPECT_TRUE (getShaderUniforms (second) == expected);
}

TEST_F (StyleContextTests, AnAssetThatChangesMakesTheDrawingsPaintingItsShaderAgain)
{
    setAsset ("fx/plasma.frag", proceduralFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Plasma { fragment: \"fx/plasma.frag\"; property intensity: number = 0.5; }\n"
                                                 "drawing Screen { use Plasma { } }\n"
                                                 "drawing Plain { rect { bounds: bounds; fill: #ff0000; } }\n"
                                                 "style { #screen { drawing: Screen; } #plain { drawing: Plain; } }\n"
                                                 "Component { Component { id: screen; } Component { id: plain; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* root = document.getRoot();
    auto* context = root->getStyleContext();
    auto* screen = document.findById ("screen");
    auto* plain = document.findById ("plain");

    auto& native = showInAWindow (*root, *screen);

    const auto screenStyle = screen->getStyle();
    const auto plainStyle = plain->getStyle();
    const auto first = context->findShader ("Plasma")->shader;
    ASSERT_NE (getDrawingStyle (screen), nullptr);
    ASSERT_NE (first, nullptr);

    // The drawing keeps the shader it was made with until the restyle that loads the file again
    setAsset ("fx/plasma.frag", reorderedFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    EXPECT_EQ (screen->getStyle(), screenStyle);

    native.repaintAreas.clearQuick();
    restyle (*root);

    // That restyle makes the drawing painting the shader again, which repaints its component, and leaves the others
    const auto second = context->findShader ("Plasma")->shader;
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    EXPECT_NE (screen->getStyle(), screenStyle);
    EXPECT_EQ (plain->getStyle(), plainStyle);
    EXPECT_GT (native.getRepaintAreas().getNumRectangles(), 0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // The new drawing fills the uniforms as the new shader lays them out
    auto expected = second->createUniforms();
    EXPECT_TRUE (expected.set ("intensity", 0.5f));
    EXPECT_TRUE (getShaderUniforms (screen) == expected);
    EXPECT_FALSE (first->createUniforms() == second->createUniforms());

    // A source that fails is reported, and leaves the drawing with the shader it has
    const auto kept = screen->getStyle();
    setAsset ("fx/plasma.frag", brokenFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*root);

    EXPECT_EQ (screen->getStyle(), kept);
    EXPECT_TRUE (getShaderUniforms (screen) == expected);
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics()[0].message.startsWith ("shader Plasma: ")) << document.getDiagnostics()[0].message;

    // A source reading a picture is no shader for a drawing to paint: the component is left without its drawing. The
    // document lists what it was told since its load, so the source that failed is still in front
    setAsset ("fx/plasma.frag", crtFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*root);

    EXPECT_EQ (getDrawingStyle (screen), nullptr);
    ASSERT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics()[1].message, "drawing Screen: Screen/use[0]: the shader 'Plasma' reads a picture, run it in the 'filter' of a layer or as an 'effect:'");
    EXPECT_EQ (document.getDiagnostics()[1].line, 2);

    // And the drawing is made again once the source is one it can paint, which tells nothing more
    setAsset ("fx/plasma.frag", proceduralFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*root);

    EXPECT_NE (getDrawingStyle (screen), nullptr);
    EXPECT_TRUE (context->getDiagnostics().empty()) << DataTreeDiagnostic::join (context->getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (StyleContextTests, AFilterOfADrawingRunsAShaderOfTheSheet)
{
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString (R"yui(
        shader Crt {
            fragment: "fx/crt.frag";
            property intensity: number = 0.5;
            property tint: color = #ff8800;
        }

        drawing Screen {
            layer {
                filter { blur { deviation: 2; } use Crt { intensity: 0.8; } }
                circle { center: bounds.center; radius: 20; fill: palette.accent; }
            }
        }

        style { #screen { drawing: Screen; } }

        Component { Component { id: screen; } }
    )yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* screen = document.findById ("screen");
    ASSERT_NE (getDrawingStyle (screen), nullptr);

    // The shader reading a picture is the one of the sheet, given what the use sets and the default of its block
    const auto* shader = document.getRoot()->getStyleContext()->findShader ("Crt");
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->shader->getNumInputTextures(), 1);

    auto expected = shader->shader->createUniforms();
    EXPECT_TRUE (expected.set ("intensity", 0.8f));
    EXPECT_TRUE (ShaderEffect::setUniform (expected, { "tint", ComponentProperty::Type::color }, VariantConverter<Color>::toVar (Color (0xffff8800))));
    EXPECT_TRUE (getShaderUniforms (screen) == expected);
}

TEST_F (StyleContextTests, ADrawingSelectedOnAnElementRunsTheShadersOfTheSheet)
{
    setAsset ("fx/plasma.frag", proceduralFragment);
    setAsset ("fx/crt.frag", crtFragment);

    YuiDocument document;
    serveAssets (document);

    // The drawings are selected on elements, one of them inside a composite whose instance its values read
    const String declarations (R"yui(
        shader Plasma {
            fragment: "fx/plasma.frag";
            property intensity: number = 0.5;
            property tint: color = #ff8800;
        }

        shader Crt { fragment: "fx/crt.frag"; property intensity: number = 0.5; }

        drawing Screen { use Plasma { intensity: root.level * 2; } }

        drawing Filtered {
            layer {
                filter { use Crt { intensity: 0.8; } }
                rect { bounds: bounds; fill: #ffffff; }
            }
        }

        component Meter : Component {
            property level: number = 0.25;
            Component { drawing: Screen; }
        }
    )yui");

    const String elements (R"yui(
        Component {
            Meter { id: low; }
            Meter { id: high; level: 0.75; }
            Component { id: filtered; drawing: Filtered; }
        }
    )yui");

    const auto expectShaders = [&]
    {
        EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

        auto* context = document.getRoot()->getStyleContext();
        const auto* plasma = context->findShader ("Plasma");
        const auto* crt = context->findShader ("Crt");
        ASSERT_NE (plasma, nullptr);
        ASSERT_NE (crt, nullptr);

        // The element holding the drawing is the child of each instance of the composite
        auto* lowMeter = document.findById ("low");
        auto* highMeter = document.findById ("high");
        auto* filtered = document.findById ("filtered");
        ASSERT_TRUE (lowMeter != nullptr && lowMeter->getNumChildComponents() == 1);
        ASSERT_TRUE (highMeter != nullptr && highMeter->getNumChildComponents() == 1);

        // Each drawing runs the shader the sheet loaded, painted or in its filter
        auto* low = lowMeter->getChildComponent (0);
        auto* high = highMeter->getChildComponent (0);

        for (auto* painted : { low, high })
        {
            ASSERT_NE (getDrawingStyle (painted), nullptr);

            const auto shaders = getDrawingStyle (painted)->getShaders();
            ASSERT_EQ (shaders.size(), 1u);
            EXPECT_EQ (shaders.front().shader, plasma->shader);
        }

        ASSERT_NE (getDrawingStyle (filtered), nullptr);
        ASSERT_EQ (getDrawingStyle (filtered)->getShaders().size(), 1u);
        EXPECT_EQ (getDrawingStyle (filtered)->getShaders().front().shader, crt->shader);

        // And gives it what its use sets, read where the element is: from the instance of the composite it is in
        auto expected = plasma->shader->createUniforms();
        EXPECT_TRUE (expected.set ("intensity", 0.5f));
        EXPECT_TRUE (ShaderEffect::setUniform (expected, { "tint", ComponentProperty::Type::color }, VariantConverter<Color>::toVar (Color (0xffff8800))));
        EXPECT_TRUE (getShaderUniforms (low) == expected);

        EXPECT_TRUE (expected.set ("intensity", 1.5f));
        EXPECT_TRUE (getShaderUniforms (high) == expected);

        auto filterExpected = crt->shader->createUniforms();
        EXPECT_TRUE (filterExpected.set ("intensity", 0.8f));
        EXPECT_TRUE (getShaderUniforms (filtered) == filterExpected);
    };

    // A first load, where the drawings are made before the root has its stylesheet
    ASSERT_TRUE (document.loadFromString (declarations + elements).wasOk());
    expectShaders();

    // A load of the same text, which keeps the stylesheet and the shaders it loaded
    const auto sheet = document.getStyleSheet();
    ASSERT_TRUE (document.loadFromString (declarations + elements).wasOk());
    EXPECT_EQ (document.getStyleSheet(), sheet);
    expectShaders();
    EXPECT_EQ (assetReads["fx/plasma.frag"], 1);
    EXPECT_EQ (assetReads["fx/crt.frag"], 1);

    // A load with another style, which replaces the stylesheet after the drawings are made: its shaders are the ones they run
    ASSERT_TRUE (document.loadFromString (declarations + "style { #filtered { opacity: 0.5; } }" + elements).wasOk());
    EXPECT_NE (document.getStyleSheet(), sheet);
    expectShaders();
    EXPECT_EQ (assetReads["fx/plasma.frag"], 2);
}

TEST_F (StyleContextTests, ADrawingSelectedOnAnElementFollowsTheFileOfItsShader)
{
    setAsset ("fx/plasma.frag", brokenFragment);

    YuiDocument document;
    serveAssets (document);

    const auto loaded = document.loadFromString ("shader Plasma { fragment: \"fx/plasma.frag\"; property intensity: number = 0.5; }\n"
                                                 "drawing Screen { rect { id: back; bounds: bounds; fill: #202020; } use Plasma { } }\n"
                                                 "Component { Component { id: screen; drawing: Screen; } }");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto* root = document.getRoot();
    auto* context = root->getStyleContext();
    auto* screen = document.findById ("screen");
    auto& native = showInAWindow (*root, *screen);

    // A source that does not compile is told once, where the file is named: the drawing is there, without its shader
    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("shader Plasma: ")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].line, 1);
    EXPECT_EQ (diagnostics[0].property, Identifier ("fragment"));

    const auto unloaded = screen->getStyle();
    ASSERT_NE (getDrawingStyle (screen), nullptr);
    ASSERT_EQ (getDrawingStyle (screen)->getShaders().size(), 1u);
    EXPECT_EQ (getDrawingStyle (screen)->getShaders().front().shader, nullptr);
    EXPECT_EQ (getDrawingStyle (screen)->measureElements (*screen).front().id, String ("back"));

    EXPECT_EQ (countProblemsOfARestyle (*root), 0u);
    EXPECT_EQ (screen->getStyle(), unloaded);

    // Once the source compiles, the restyle loading it makes the drawing again with the shader, which repaints the component
    setAsset ("fx/plasma.frag", proceduralFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    EXPECT_EQ (screen->getStyle(), unloaded);

    native.repaintAreas.clearQuick();
    restyle (*root);

    ASSERT_NE (context->findShader ("Plasma"), nullptr);
    const auto first = context->findShader ("Plasma")->shader;
    const auto loadedStyle = screen->getStyle();

    EXPECT_NE (loadedStyle, unloaded);
    ASSERT_NE (getDrawingStyle (screen), nullptr);
    EXPECT_EQ (getDrawingStyle (screen)->getShaders().front().shader, first);
    EXPECT_GT (native.getRepaintAreas().getNumRectangles(), 0);
    EXPECT_TRUE (context->getDiagnostics().empty()) << DataTreeDiagnostic::join (context->getDiagnostics());

    // Then follows the file: another source is another shader, laying its uniforms out its way
    setAsset ("fx/plasma.frag", reorderedFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*root);

    ASSERT_NE (context->findShader ("Plasma"), nullptr);
    const auto second = context->findShader ("Plasma")->shader;
    EXPECT_NE (second, first);
    EXPECT_NE (screen->getStyle(), loadedStyle);
    ASSERT_NE (getDrawingStyle (screen), nullptr);
    EXPECT_EQ (getDrawingStyle (screen)->getShaders().front().shader, second);

    auto expected = second->createUniforms();
    EXPECT_TRUE (expected.set ("intensity", 0.5f));
    EXPECT_TRUE (getShaderUniforms (screen) == expected);

    // A source that fails again is reported, and leaves the drawing with the shader it has
    const auto kept = screen->getStyle();
    setAsset ("fx/plasma.frag", brokenFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*root);

    EXPECT_EQ (screen->getStyle(), kept);
    ASSERT_EQ (context->getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (context->getDiagnostics());
    EXPECT_TRUE (context->getDiagnostics().front().message.startsWith ("shader Plasma: ")) << context->getDiagnostics().front().message;

    // A source reading a picture is no shader for the drawing to paint: told where it is used, as for a drawing a rule selects
    setAsset ("fx/plasma.frag", crtFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*root);

    ASSERT_EQ (context->getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (context->getDiagnostics());
    EXPECT_EQ (context->getDiagnostics().front().message, "drawing Screen: Screen/use[0]: the shader 'Plasma' reads a picture, run it in the 'filter' of a layer or as an 'effect:'");
    EXPECT_EQ (screen->getStyle(), kept);
}

TEST_F (StyleContextTests, ADrawingAnElementCantSelectAnymoreIsNotMadeAgainWithTheShadersOfTheSheet)
{
    setAsset ("fx/plasma.frag", proceduralFragment);

    YuiDocument document;
    serveAssets (document);

    const auto textOf = [] (const String& screen)
    {
        return "shader Plasma { fragment: \"fx/plasma.frag\"; property intensity: number = 0.5; }\n"
               "drawing Screen { "
             + screen
             + " }\n"
               "drawing Other { use Plasma { } }\n"
               "component Meter : Component { property level: number = 0.25; Component { drawing: Screen; } }\n"
               "Component { Meter { id: low; } Component { id: other; drawing: Other; } }";
    };

    ASSERT_TRUE (document.loadFromString (textOf ("use Plasma { intensity: root.level * 2; }")).wasOk());
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* meter = document.findById ("low");
    ASSERT_TRUE (meter != nullptr && meter->getNumChildComponents() == 1);

    auto* painted = meter->getChildComponent (0);
    ASSERT_NE (getDrawingStyle (painted), nullptr);

    // The drawing does not compile anymore, in a stylesheet made again. The element is kept, and has no drawing left of
    // the load before: that one read the instance of the composite where the build of that load had it
    ASSERT_TRUE (document.loadFromString (textOf ("use Plasma { intensity: root.level * 2; } rect { bounds: bounds; fill: nope; }")).wasOk());

    ASSERT_EQ (document.findById ("low"), meter);
    ASSERT_EQ (meter->getNumChildComponents(), 1);
    ASSERT_EQ (meter->getChildComponent (0), painted);
    EXPECT_EQ (painted->getStyle(), nullptr);

    // What prevents it is told once, by the build
    const auto& diagnostics = document.getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1u) << DataTreeDiagnostic::join (diagnostics);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("Component.drawing: Screen/rect[0].fill: ")) << diagnostics[0].message;
    EXPECT_EQ (diagnostics[0].line, 2);

    // The shader is loaded for the drawing still painting it. A file of it that changes makes that one again, and is
    // nothing to the element left without a drawing
    auto* context = document.getRoot()->getStyleContext();
    auto* other = document.findById ("other");
    ASSERT_NE (context->findShader ("Plasma"), nullptr);
    ASSERT_NE (getDrawingStyle (other), nullptr);

    const auto first = context->findShader ("Plasma")->shader;
    EXPECT_EQ (getDrawingStyle (other)->getShaders().front().shader, first);

    setAsset ("fx/plasma.frag", reorderedFragment);
    document.notifyAssetChanged ("fx/plasma.frag");
    restyle (*document.getRoot());

    ASSERT_NE (context->findShader ("Plasma"), nullptr);
    ASSERT_NE (getDrawingStyle (other), nullptr);

    const auto second = context->findShader ("Plasma")->shader;
    EXPECT_NE (second, first);
    EXPECT_EQ (getDrawingStyle (other)->getShaders().front().shader, second);
    EXPECT_EQ (painted->getStyle(), nullptr);
    EXPECT_TRUE (context->getDiagnostics().empty()) << DataTreeDiagnostic::join (context->getDiagnostics());
}

#endif // YUP_ENABLE_SHADER_TRANSPILER





