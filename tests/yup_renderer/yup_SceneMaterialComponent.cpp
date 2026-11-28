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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

#if __has_include(<YuiTestResources.h>)
#include <YuiTestResources.h>
#endif

using namespace yup;

/** The `Material3D` of a `.yui` document: a component giving the entity of its parent a
    MaterialNode with the material its properties describe, and a MaterialShaderNode with the
    shader it names.
*/
class SceneMaterialComponentTests : public SceneDocumentTests
{
protected:
    void SetUp() override
    {
        SceneDocumentTests::SetUp();
        files["wood.png"] = redPng();
    }

    /** The material overriding the ones of the meshes of a node and of what is under it, or nullptr. */
    static Material::Ptr overrideOf (const Entity& entity)
    {
        const auto* node = entity.get<MaterialNode>();
        return node != nullptr ? node->material : Material::Ptr();
    }

    /** A document holding a quad with what is written inside it. */
    static String quadWith (const String& inside)
    {
        return "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " " + inside + " } }";
    }

    //==============================================================================
    /** A member of the uniform block of a material: a scalar or a vector at an offset in bytes, or
        an array of scalars, each of which takes 16 bytes.
    */
    static ShaderReflection::ResourceMember uniformMember (const char* name,
                                                           uint32 offset,
                                                           uint32 numComponents = 1,
                                                           ShaderReflection::BaseType type = ShaderReflection::BaseType::float32,
                                                           uint32 numElements = 0)
    {
        ShaderReflection::ResourceMember member;
        member.name = name;
        member.baseType = type;
        member.vecSize = numComponents;
        member.offset = offset;
        member.size = numComponents * 4;

        if (numElements > 0)
        {
            member.arraySizes = { numElements };
            member.arrayStride = 16;
            member.size = numElements * 16;
        }

        return member;
    }

    /** The uniform block of the material most tests use: `vec4 rim; float strength;`. */
    static std::vector<ShaderReflection::ResourceMember> glowMembers()
    {
        return { uniformMember ("rim", 0, 4), uniformMember ("strength", 16) };
    }

    /** The properties a `shader` block declares for that block. */
    static String glowInputs()
    {
        return "property rim: color = #00ffff; property strength: number = 1;";
    }

    /** The bytes of one of the two variants a build compiles a material into: what the template of
        the renderer declares for the raster tier, which writes one output, or for the full one,
        which writes two, around a material whose uniform block has the given members, or has no
        block when there is none.
    */
    static MemoryBlock variantFile (uint32 numOutputs, const std::vector<ShaderReflection::ResourceMember>& members)
    {
        const auto makeBlock = [] (const char* name, uint32 binding, std::vector<ShaderReflection::ResourceMember> blockMembers = {})
        {
            ShaderReflection::ResourceBinding block;
            block.name = name;
            block.type = ShaderReflection::ResourceType::uniformBuffer;
            block.set = 2;
            block.binding = binding;
            block.blockSize = blockMembers.empty() ? 16 : blockMembers.back().offset + blockMembers.back().size;
            block.members = std::move (blockMembers);
            return block;
        };

        // The draw of the mesh in the vertex stage, its material in the fragment one, then the block of the material
        ShaderReflection vertex, fragment;
        vertex.uniformBuffers = { makeBlock ("DrawBlock", 0) };
        fragment.uniformBuffers = { makeBlock ("MaterialBlock", 1) };

        if (! members.empty())
            fragment.uniformBuffers.push_back (makeBlock ("MaterialParams", MaterialShader::uniformBlockBinding, members));

        for (uint32 location = 0; location < numOutputs; ++location)
            fragment.stageOutputs.emplace_back().location = location;

        ShaderBundle::Ptr bundle (new ShaderBundle());

        for (const auto& [stage, reflection] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, fragment } })
        {
            ShaderInfo info;
            info.stage = stage;
            info.source = "void main() {}";
            info.reflection = reflection;
            bundle->addShader (std::move (info));
        }

        MemoryBlock bytes;
        EXPECT_TRUE (bundle->saveToMemoryBlock (bytes, ShaderBundle::Compression::none).wasOk());
        return bytes;
    }

    /** Serves the two variants of a material, as a build embeds them beside the path of its source. */
    void serveMaterial (const String& path, const std::vector<ShaderReflection::ResourceMember>& members)
    {
        files[path + ".raster.ysl"] = variantFile (1, members);
        files[path + ".full.ysl"] = variantFile (2, members);
    }

    /** A document whose stylesheet declares the shader Glow, the material `fx/glow.glsl` with the
        given inputs, then the rules of a style, around a quad holding what is written for it and
        followed by what is written after it in the scene.
    */
    static String glowWith (const String& inputs, const String& inside, const String& rules = {}, const String& after = {})
    {
        return "shader Glow { material: \"fx/glow.glsl\"; " + inputs + " }\n"
             + (rules.isNotEmpty() ? "style { " + rules + " }\n" : String())
             + "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " " + inside + " } " + after + " }";
    }

    /** The scene, the quad and the material `paint` of a document, shown as a window would. */
    struct Shown
    {
        explicit Shown (YuiDocument& document)
            : view (document.findById<SceneComponent> ("view"))
            , quad (document.findById<SceneMeshComponent> ("quad"))
            , paint (document.findById<SceneMaterialComponent> ("paint"))
        {
            if (view != nullptr)
                showScene (*view);
        }

        bool isValid() const
        {
            return view != nullptr && quad != nullptr && paint != nullptr && quad->getEntity().isValid();
        }

        /** The shader coloring the quad, once a frame of the scene has looked for it, or nullptr. */
        const MaterialShaderNode* shaderNode() const
        {
            return shaderNodeOf (*quad);
        }

        SceneComponent* view;
        SceneMeshComponent* quad;
        SceneMaterialComponent* paint;
    };

    static const MaterialShaderNode* shaderNodeOf (const SceneNodeComponent& node)
    {
        const auto entity = node.getEntity();
        return entity.isValid() ? entity.get<MaterialShaderNode>() : nullptr;
    }

    //==============================================================================
    /** The floats the uniform block of a shader holds from an offset in bytes. */
    static std::vector<float> floatsAt (const MaterialShaderNode* node, size_t offset, size_t count = 1)
    {
        std::vector<float> values (count, std::numeric_limits<float>::quiet_NaN());

        if (node == nullptr || node->getUniforms().getSizeInBytes() < offset + count * sizeof (float))
            ADD_FAILURE() << "No uniform block holding " << count << " floats at " << offset;
        else
            std::memcpy (values.data(), node->getUniforms().getData() + offset, count * sizeof (float));

        return values;
    }

    /** The integer the uniform block of a shader holds at an offset in bytes. */
    static int32 intAt (const MaterialShaderNode* node, size_t offset)
    {
        int32 value = -1;

        if (node == nullptr || node->getUniforms().getSizeInBytes() < offset + sizeof (int32))
            ADD_FAILURE() << "No uniform block holding an integer at " << offset;
        else
            std::memcpy (&value, node->getUniforms().getData() + offset, sizeof (int32));

        return value;
    }

    static void expectFloats (const std::vector<float>& actual, std::initializer_list<float> expected, float tolerance = 1.0e-5f)
    {
        ASSERT_EQ (actual.size(), expected.size());

        size_t index = 0;

        for (const auto value : expected)
        {
            EXPECT_NEAR (actual[index], value, tolerance) << "float " << index;
            ++index;
        }
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    //==============================================================================
    /** Stands in for a window drawing with a graphics API: its device tells that API and that it has a GPU, and is
        never drawn with. The API can be changed, as it is for a scene moved to another window.
    */
    class ApiWindow final : public ComponentNative
    {
    public:
        ApiWindow (Component& componentToShow, GpuPlatform api)
            : ComponentNative (componentToShow, ComponentNative::defaultFlags)
        {
            setGraphicsApi (api);
        }

        void setGraphicsApi (GpuPlatform api)
        {
            context = std::make_unique<Context> (api);
        }

        void setTitle (const String&) override {}

        String getTitle() const override { return {}; }

        void setVisible (bool) override {}

        bool isVisible() const override { return true; }

        bool isResizable() const override { return false; }

        void setResizable (bool) override {}

        void toFront() override {}

        void setSize (const Size<int>&) override {}

        Size<int> getSize() const override { return {}; }

        Size<int> getContentSize() const override { return {}; }

        Point<int> getPosition() const override { return {}; }

        void setPosition (const Point<int>&) override {}

        Rectangle<int> getBounds() const override { return {}; }

        void setBounds (const Rectangle<int>&) override {}

        Rectangle<int> getSafeAreaBounds() const override { return {}; }

        void setFullScreen (bool) override {}

        bool isFullScreen() const override { return false; }

        bool isDecorated() const override { return false; }

        void setAspectRatio (std::optional<float>) override {}

        std::optional<float> getAspectRatio() const override { return std::nullopt; }

        void setOpacity (float) override {}

        float getOpacity() const override { return 1.0f; }

        void setFocusedComponent (Component*, FocusChangeType) override {}

        Component* getFocusedComponent() const override { return nullptr; }

        Component* getPointerTarget() const override { return nullptr; }

        bool isContinuousRepaintingEnabled() const override { return false; }

        void enableContinuousRepainting (bool) override {}

        bool isAtomicModeEnabled() const override { return false; }

        void enableAtomicMode (bool) override {}

        bool isWireframeEnabled() const override { return false; }

        void enableWireframe (bool) override {}

        GpuTriangulationThresholds getTriangulationThresholds() const override { return {}; }

        void setTriangulationThresholds (const GpuTriangulationThresholds&) override {}

        void repaint() override {}

        void repaint (const Rectangle<float>&) override {}

        const RectangleList<float>& getRepaintAreas() const override { return repaintAreas; }

        void setGlobalMouseCaptureActive (bool) override {}

        void cancelCurrentMouseGesture() override {}

        void startTextInput (Component&) override {}

        void stopTextInput (Component&) override {}

        void updateTextInputRect (Component&) override {}

        float getScaleDpi() const override { return 1.0f; }

        float getCurrentFrameRate() const override { return 60.0f; }

        float getDesiredFrameRate() const override { return 60.0f; }

        void setDesiredFrameRate (std::optional<float>) override {}

        bool isVsyncEnabled() const override { return false; }

        void setVsyncEnabled (bool) override {}

        void* getNativeHandle() const override { return nullptr; }

        rive::Factory* getFactory() override { return nullptr; }

        GraphicsContext* getGraphicsContext() override { return context.get(); }

    private:
        class Device final : public GpuDevice
        {
        public:
            explicit Device (GpuPlatform apiToTell)
                : api (apiToTell)
            {
            }

            GpuPlatform getPlatform() const noexcept override { return api; }

            rive::ore::Context* getGpuContext() const noexcept override { return &oreContext; }

            std::unique_ptr<OffscreenTarget> createOffscreenTarget (int, int) override { return nullptr; }

            std::unique_ptr<RenderableTarget> createRenderableTarget (int, int) override { return nullptr; }

            void beginOffscreen (OffscreenTarget&, const GpuFrameDescriptor&) override {}

            void endOffscreen (OffscreenTarget&) override {}

            bool readOffscreenPixels (OffscreenTarget&, void*, size_t) override { return false; }

        private:
            const GpuPlatform api;
            mutable ::testing::NiceMock<MockOreContext> oreContext;
        };

        class Context final : public GraphicsContext
        {
        public:
            explicit Context (GpuPlatform api)
                : device (new Device (api))
            {
            }

            GpuPlatform getPlatform() const noexcept override { return device->getPlatform(); }

            GpuDevice::Ptr getGpuDevice() const noexcept override { return device; }

            rive::Factory* getFactory() override { return nullptr; }

            rive::gpu::RenderContext* getRenderContext() override { return nullptr; }

            rive::gpu::RenderTarget* getRenderTarget() override { return nullptr; }

            std::unique_ptr<rive::Renderer> makeRenderer (int, int) override { return nullptr; }

            void onSizeChanged (void*, int, int, float, uint32_t) override {}

            void begin (const rive::gpu::RenderContext::FrameDescriptor&) override {}

            void end (void*) override {}

        private:
            const GpuDevice::Ptr device;
        };

        std::unique_ptr<Context> context;
        RectangleList<float> repaintAreas;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApiWindow)
    };

    /** Makes a component a window drawing with a graphics API, visible and 200 pixels wide and high, and returns that window. */
    static ApiWindow& makeWindowDrawingWith (Component& component, GpuPlatform api)
    {
        auto* window = new ApiWindow (component, api);
        Internals::attachNative (component, window);

        component.setVisible (true);
        component.setBounds (0.0f, 0.0f, 200.0f, 200.0f);

        return *window;
    }

    /** A graphics API drawing in another language than the given one, which neither can draw the shaders of the other. */
    static GpuPlatform otherApiThan (GpuPlatform api)
    {
        return api == GpuPlatform::Metal ? GpuPlatform::Direct3D : GpuPlatform::Metal;
    }

    /** Returns true if a shader holds, on both tiers, the variants of the language of a graphics API and of no other. */
    static bool isCompiledFor (const MaterialShader::Ptr& shader, GpuPlatform api)
    {
        if (shader == nullptr)
            return false;

        for (const auto& bundle : { shader->getRasterBundle(), shader->getFullBundle() })
        {
            if (bundle == nullptr || bundle->getShaders().empty())
                return false;

            for (const auto& variant : bundle->getShaders())
            {
                if (variant.language != GpuImageShader::getShaderLanguage (api))
                    return false;
            }
        }

        return true;
    }

    /** A material with the uniform block `vec4 rim; float strength;`. */
    static constexpr const char* hologramSource = R"(YUP_MATERIAL_PARAMS
{
    vec4 rim;
    float strength;
} params;

vec4 yupMaterial (YupSurface surface)
{
    return vec4 (surface.baseColor.rgb + params.rim.rgb * params.strength, surface.baseColor.a);
}
)";

    /** A material comparing two structures, which WGSL does not take: every graphics API compiles it but WebGPU. */
    static constexpr const char* notForWebGpuSource = R"(struct Level { float value; };

vec4 yupMaterial (YupSurface surface)
{
    Level low = Level (0.0);
    Level high = Level (1.0);
    bool same = low == high;
    return same ? surface.baseColor : vec4 (surface.baseColor.rgb * 0.5, surface.baseColor.a);
}
)";
#endif
};

TEST_F (SceneMaterialComponentTests, PropertiesAreTheFactorsOfTheMaterial)
{
    YuiDocument document;
    load (document, quadWith (R"yui(
        Material3D {
            id: paint;
            baseColor: #80808040;
            metallic: 0.25;
            roughness: 0.4;
            normalScale: 2;
            occlusionStrength: 0.5;
            emissive: #ff0000;
            emissiveStrength: 3;
            alphaMode: blend;
            alphaCutoff: 0.3;
            doubleSided: true;
            clearcoat: 0.6;
            clearcoatRoughness: 0.1;
            sheenColor: #0000ff;
            sheenRoughness: 0.7;
        })yui"));

    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");

    ASSERT_NE (quad, nullptr);
    ASSERT_NE (paint, nullptr);

    const auto material = paint->getMaterial();
    ASSERT_NE (material, nullptr);
    EXPECT_EQ (overrideOf (quad->getEntity()), material);

    // The colors of the interface are sRGB encoded: the material gets them linear, but for the alpha
    const auto gray = toLinear (128.0f / 255.0f);

    EXPECT_NEAR (material->baseColorFactor[0], gray, 1.0e-5f);
    EXPECT_NEAR (material->baseColorFactor[1], gray, 1.0e-5f);
    EXPECT_NEAR (material->baseColorFactor[2], gray, 1.0e-5f);
    EXPECT_NEAR (material->baseColorFactor[3], 64.0f / 255.0f, 1.0e-5f);
    EXPECT_LT (gray, 0.25f);

    EXPECT_EQ (material->emissiveFactor, (std::array<float, 3> { 1.0f, 0.0f, 0.0f }));
    EXPECT_EQ (material->sheenColorFactor, (std::array<float, 3> { 0.0f, 0.0f, 1.0f }));

    EXPECT_FLOAT_EQ (material->metallicFactor, 0.25f);
    EXPECT_FLOAT_EQ (material->roughnessFactor, 0.4f);
    EXPECT_FLOAT_EQ (material->normalScale, 2.0f);
    EXPECT_FLOAT_EQ (material->occlusionStrength, 0.5f);
    EXPECT_FLOAT_EQ (material->emissiveStrength, 3.0f);
    EXPECT_EQ (material->alphaMode, Material::AlphaMode::blend);
    EXPECT_FLOAT_EQ (material->alphaCutoff, 0.3f);
    EXPECT_TRUE (material->doubleSided);
    EXPECT_FLOAT_EQ (material->clearcoatFactor, 0.6f);
    EXPECT_FLOAT_EQ (material->clearcoatRoughnessFactor, 0.1f);
    EXPECT_FLOAT_EQ (material->sheenRoughnessFactor, 0.7f);

    // They read back as they were set
    EXPECT_EQ (VariantConverter<Color>::fromVar (paint->getProperty (Identifier ("baseColor"))), Color (0x40, 0x80, 0x80, 0x80));
    EXPECT_EQ (VariantConverter<Color>::fromVar (paint->getProperty (Identifier ("emissive"))), Color (0xffff0000));
    EXPECT_EQ (paint->getProperty (Identifier ("alphaMode")).toString(), "blend");
    EXPECT_FLOAT_EQ (static_cast<float> (paint->getProperty (Identifier ("roughness"))), 0.4f);
}

TEST_F (SceneMaterialComponentTests, AMaterialThatSetsNothingIsTheDefaultOne)
{
    YuiDocument document;
    load (document, quadWith ("Material3D { id: paint; }"));

    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    ASSERT_NE (paint, nullptr);

    const Material defaults {};
    const auto material = paint->getMaterial();

    EXPECT_EQ (material->baseColorFactor, defaults.baseColorFactor);
    EXPECT_EQ (material->emissiveFactor, defaults.emissiveFactor);
    EXPECT_EQ (material->sheenColorFactor, defaults.sheenColorFactor);
    EXPECT_FLOAT_EQ (material->metallicFactor, defaults.metallicFactor);
    EXPECT_FLOAT_EQ (material->roughnessFactor, defaults.roughnessFactor);
    EXPECT_EQ (material->alphaMode, Material::AlphaMode::opaque);
    EXPECT_FALSE (material->doubleSided);
    EXPECT_EQ (material->baseColorTexture, nullptr);

    EXPECT_EQ (VariantConverter<Color>::fromVar (paint->getProperty (Identifier ("baseColor"))), Colors::white);
    EXPECT_EQ (paint->getProperty (Identifier ("alphaMode")).toString(), "opaque");
    EXPECT_FLOAT_EQ (static_cast<float> (paint->getProperty (Identifier ("metallic"))), 1.0f);
    EXPECT_TRUE (paint->getProperty (Identifier ("baseColorTexture")).toString().isEmpty());

    // It is not a node: it paints nothing and has no entity
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    ASSERT_NE (quad, nullptr);
    ASSERT_TRUE (quad->getEntity().isValid());

    EXPECT_FALSE (paint->isOpaque());
    EXPECT_EQ (quad->getEntity().getNumChildren(), 0);
}

TEST_F (SceneMaterialComponentTests, TheReflectionsOfANonMetalAreFactorsOfTheMaterial)
{
    // Declared with what a default material has
    const Material defaults {};
    const auto& type = SceneMaterialComponent::getClassTypeInfo();

    const auto* ior = type.findProperty (Identifier ("ior"));
    const auto* specular = type.findProperty (Identifier ("specular"));
    const auto* specularColor = type.findProperty (Identifier ("specularColor"));

    ASSERT_NE (ior, nullptr);
    ASSERT_NE (specular, nullptr);
    ASSERT_NE (specularColor, nullptr);

    EXPECT_FLOAT_EQ (static_cast<float> (ior->defaultValue), defaults.ior);
    EXPECT_FLOAT_EQ (static_cast<float> (specular->defaultValue), defaults.specularFactor);
    EXPECT_EQ (VariantConverter<Color>::fromVar (specularColor->defaultValue), Colors::white);

    YuiDocument document;
    load (document, quadWith ("Material3D { id: paint; ior: 1.33; specular: 0.5; specularColor: #ff8080; }"));

    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    ASSERT_NE (paint, nullptr);

    const auto material = paint->getMaterial();

    EXPECT_FLOAT_EQ (material->ior, 1.33f);
    EXPECT_FLOAT_EQ (material->specularFactor, 0.5f);

    // The color of the interface is sRGB encoded: the material gets it linear, as its other colors
    EXPECT_NEAR (material->specularColorFactor[0], 1.0f, 1.0e-5f);
    EXPECT_NEAR (material->specularColorFactor[1], toLinear (128.0f / 255.0f), 1.0e-5f);
    EXPECT_NEAR (material->specularColorFactor[2], toLinear (128.0f / 255.0f), 1.0e-5f);

    // They read back as they were set
    EXPECT_FLOAT_EQ (static_cast<float> (paint->getProperty (Identifier ("ior"))), 1.33f);
    EXPECT_FLOAT_EQ (static_cast<float> (paint->getProperty (Identifier ("specular"))), 0.5f);
    EXPECT_EQ (VariantConverter<Color>::fromVar (paint->getProperty (Identifier ("specularColor"))), Color (0xffff8080));
}

TEST_F (SceneMaterialComponentTests, TexturesAreReadThroughTheDocumentAndShared)
{
    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Mesh3D {
                vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0];

                Material3D {
                    id: paint;
                    baseColorTexture: "wood.png";
                    emissiveTexture: "wood.png";
                    normalTexture: "wood.png";
                    occlusionTexture: "wood.png";
                    metallicRoughnessTexture: "wood.png";
                }
            }

            Node3D { Material3D { id: other; baseColorTexture: "wood.png"; } }
        })yui");

    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    auto* other = document.findById<SceneMaterialComponent> ("other");

    ASSERT_NE (paint, nullptr);
    ASSERT_NE (other, nullptr);

    const auto material = paint->getMaterial();

    ASSERT_NE (material->baseColorTexture, nullptr);
    ASSERT_NE (material->emissiveTexture, nullptr);
    ASSERT_NE (material->normalTexture, nullptr);
    ASSERT_NE (material->occlusionTexture, nullptr);
    ASSERT_NE (material->metallicRoughnessTexture, nullptr);

    // The textures holding colors are sRGB encoded, those holding data are not
    EXPECT_TRUE (material->baseColorTexture->isSrgb());
    EXPECT_TRUE (material->emissiveTexture->isSrgb());
    EXPECT_FALSE (material->normalTexture->isSrgb());
    EXPECT_FALSE (material->occlusionTexture->isSrgb());
    EXPECT_FALSE (material->metallicRoughnessTexture->isSrgb());

    EXPECT_EQ (material->baseColorTexture->getImage().getWidth(), 1);
    EXPECT_EQ (material->baseColorTexture->getSamplerDesc().wrapU, GpuWrapMode::repeat);

    // One texture for each encoding of a file, whatever the number of materials using it
    EXPECT_EQ (material->emissiveTexture, material->baseColorTexture);
    EXPECT_EQ (material->occlusionTexture, material->normalTexture);
    EXPECT_NE (material->normalTexture, material->baseColorTexture);
    EXPECT_EQ (other->getMaterial()->baseColorTexture, material->baseColorTexture);
    EXPECT_EQ (reads["wood.png"], 2);

    EXPECT_EQ (paint->getProperty (Identifier ("normalTexture")).toString(), "wood.png");

    // A file that changed is read again, by every material made from it
    const auto before = material->baseColorTexture;
    document.notifyAssetChanged ("wood.png");
    finishLoading (document);

    ASSERT_NE (material->baseColorTexture, nullptr);
    EXPECT_NE (material->baseColorTexture, before);
    EXPECT_EQ (other->getMaterial()->baseColorTexture, material->baseColorTexture);
    EXPECT_EQ (reads["wood.png"], 4);

    // Without a path there is no texture
    paint->setProperty (Identifier ("normalTexture"), String());
    EXPECT_EQ (material->normalTexture, nullptr);
}

TEST_F (SceneMaterialComponentTests, ATextureThatCanNotBeLoadedIsReported)
{
    files["notes.png"] = bytesOf ("this is not an image");

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    Material3D {\n"
                                                 "        id: paint;\n"
                                                 "        baseColorTexture: \"nowhere.png\";\n"
                                                 "        normalTexture: \"notes.png\";\n"
                                                 "        emissiveTexture: \"wood.png\";\n"
                                                 "    }\n"
                                                 "}");

    finishLoading (document);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    expectDiagnostic (document, "Material3D.baseColorTexture: the file 'nowhere.png' can't be read");
    expectDiagnostic (document, "Material3D.normalTexture: ");
    EXPECT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* diagnostic = findDiagnostic (document, "Material3D.normalTexture");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 5);

    // The material goes without the textures it could not get
    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    ASSERT_NE (paint, nullptr);

    EXPECT_EQ (paint->getMaterial()->baseColorTexture, nullptr);
    EXPECT_EQ (paint->getMaterial()->normalTexture, nullptr);
    EXPECT_NE (paint->getMaterial()->emissiveTexture, nullptr);
}

TEST_F (SceneMaterialComponentTests, TheMaterialOfTheSceneOverridesEveryMesh)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;

            Material3D { id: paint; roughness: 0.2; }
            Mesh3D { id: triangle; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    auto* triangle = document.findById<SceneMeshComponent> ("triangle");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (paint, nullptr);
    ASSERT_NE (triangle, nullptr);

    const auto root = view->getScene()->getRoot();

    EXPECT_EQ (overrideOf (root), paint->getMaterial());
    EXPECT_EQ (overrideOf (triangle->getEntity()), nullptr);
    ASSERT_EQ (root.getNumChildren(), 1);
    EXPECT_EQ (root.getChild (0), triangle->getEntity());
}

TEST_F (SceneMaterialComponentTests, AMaterialGivenASizeLeavesThePointerToTheNodes)
{
    // The materials come after the mesh among the components, so over it for the pointer
    YuiDocument document;
    load (document, sceneOf ("Mesh3D { id: plate; " + quadGeometry() + " Material3D { id: inner; } } Material3D { id: outer; }"));

    auto* view = showSceneOf (document);
    auto* plate = document.findById<SceneMeshComponent> ("plate");
    auto* inner = document.findById<SceneMaterialComponent> ("inner");
    auto* outer = document.findById<SceneMaterialComponent> ("outer");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (plate, nullptr);
    ASSERT_NE (inner, nullptr);
    ASSERT_NE (outer, nullptr);

    // A material shows nothing, whatever the bounds it has as a component
    inner->setBounds (0.0f, 0.0f, 200.0f, 200.0f);
    outer->setBounds (0.0f, 0.0f, 200.0f, 200.0f);
    ASSERT_TRUE (inner->isVisible());
    ASSERT_TRUE (outer->isVisible());

    view->refreshDisplay (0.0);

    EXPECT_EQ (view->findNodeAt ({ 100.0f, 100.0f }), plate);
    EXPECT_TRUE (plate->hitTest (100.0f, 100.0f));

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 100.0f, 100.0f }), plate);
    EXPECT_EQ (pointer.moveTo ({ 20.0f, 20.0f }), view);
}

TEST_F (SceneMaterialComponentTests, AParentTakesItsFirstMaterialAndReportsTheOthers)
{
    YuiDocument document;
    const auto result = document.loadFromString (quadWith ("\nMaterial3D { id: first; roughness: 0.1; }\nMaterial3D { id: second; roughness: 0.9; }\n"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* first = document.findById<SceneMaterialComponent> ("first");
    auto* second = document.findById<SceneMaterialComponent> ("second");

    ASSERT_NE (quad, nullptr);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().message, "Material3D: the parent already has a Material3D, this one is not used");
    EXPECT_EQ (document.getDiagnostics().front().line, 3);

    EXPECT_EQ (overrideOf (quad->getEntity()), first->getMaterial());

    // Once the first one is gone the next one is the material of the parent, with nothing to report
    load (document, quadWith ("\nMaterial3D { id: second; roughness: 0.9; }\n"));

    quad = document.findById<SceneMeshComponent> ("quad");
    second = document.findById<SceneMaterialComponent> ("second");

    ASSERT_NE (quad, nullptr);
    ASSERT_NE (second, nullptr);
    EXPECT_EQ (overrideOf (quad->getEntity()), second->getMaterial());
    EXPECT_FLOAT_EQ (overrideOf (quad->getEntity())->roughnessFactor, 0.9f);
}

TEST_F (SceneMaterialComponentTests, RemovingTheMaterialRemovesTheOverride)
{
    YuiDocument document;
    load (document, quadWith ("Material3D { id: paint; }"));

    auto* quad = document.findById<SceneMeshComponent> ("quad");
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (overrideOf (quad->getEntity()), nullptr);

    load (document, quadWith (""));

    quad = document.findById<SceneMeshComponent> ("quad");
    ASSERT_NE (quad, nullptr);
    EXPECT_EQ (quad->getEntity().get<MaterialNode>(), nullptr);
}

TEST_F (SceneMaterialComponentTests, TheMaterialFollowsItsComponent)
{
    SceneComponent view;
    SceneNodeComponent left, right;
    SceneMaterialComponent paint, spare;

    // Inside a node that is not in a scene yet
    left.addAndMakeVisible (paint);
    view.addAndMakeVisible (left);
    view.addAndMakeVisible (right);

    EXPECT_EQ (overrideOf (left.getEntity()), paint.getMaterial());

    // The first of two is the one used, and the next one takes over when it leaves
    left.addAndMakeVisible (spare);
    EXPECT_EQ (overrideOf (left.getEntity()), paint.getMaterial());

    right.addAndMakeVisible (paint);
    EXPECT_EQ (overrideOf (left.getEntity()), spare.getMaterial());
    EXPECT_EQ (overrideOf (right.getEntity()), paint.getMaterial());

    left.removeChildComponent (spare);
    EXPECT_EQ (left.getEntity().get<MaterialNode>(), nullptr);

    // The node that leaves the scene keeps its entity, with its material, for the frame that follows to let go of
    view.removeChildComponent (right);
    EXPECT_EQ (overrideOf (right.getEntity()), paint.getMaterial());

    view.refreshDisplay (0.0);
    EXPECT_FALSE (right.getEntity().isValid());

    // And gets its material again when it comes back
    view.addAndMakeVisible (right);
    EXPECT_EQ (overrideOf (right.getEntity()), paint.getMaterial());
}

TEST_F (SceneMaterialComponentTests, AStyleRuleRestylesTheMeshesOfTheMaterial)
{
    YuiDocument document;
    load (document, "style {\n"
                    "    .hot > Material3D { emissive: #ffffff; roughness: 0.1; alphaMode: mask; }\n"
                    "    .glow { transition: emissiveStrength 100ms; }\n"
                    "    .bright { emissiveStrength: 5; }\n"
                    "}\n"
                        + quadWith ("Material3D { id: paint; class: \"glow\"; roughness: 0.8; }"));

    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");

    ASSERT_NE (quad, nullptr);
    ASSERT_NE (paint, nullptr);

    const auto material = paint->getMaterial();
    auto& context = *document.getRoot()->getStyleContext();

    quad->addStyleClass (Identifier ("hot"));
    context.restyleNow();

    EXPECT_EQ (material->emissiveFactor, (std::array<float, 3> { 1.0f, 1.0f, 1.0f }));
    EXPECT_FLOAT_EQ (material->roughnessFactor, 0.1f);
    EXPECT_EQ (material->alphaMode, Material::AlphaMode::mask);

    quad->removeStyleClass (Identifier ("hot"));
    context.restyleNow();

    EXPECT_EQ (material->emissiveFactor, (std::array<float, 3> { 0.0f, 0.0f, 0.0f }));
    EXPECT_FLOAT_EQ (material->roughnessFactor, 0.8f);
    EXPECT_EQ (material->alphaMode, Material::AlphaMode::opaque);

    // A factor goes through the values in between as a transition runs
    paint->addStyleClass (Identifier ("bright"));
    context.refresh (0.0);
    EXPECT_FLOAT_EQ (material->emissiveStrength, 1.0f);

    context.refresh (0.05);
    EXPECT_NEAR (material->emissiveStrength, 3.0f, 1.0e-3f);

    context.refresh (0.06);
    EXPECT_FLOAT_EQ (material->emissiveStrength, 5.0f);
}

//==============================================================================

TEST_F (SceneMaterialComponentTests, AMaterialNamingAShaderOfTheStylesheetColorsItsMeshesWithIt)
{
    serveMaterial ("fx/glow.glsl", glowMembers());
    files["fx/glow.glsl"] = bytesOf ("this is not a material");

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(), "Material3D { id: paint; shader: Glow; baseColor: #204060; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    EXPECT_EQ (shown.paint->getShaderName(), "Glow");
    EXPECT_EQ (shown.paint->getProperty (Identifier ("shader")).toString(), "Glow");

    // The shader is looked for by a frame of the scene: by then the stylesheet of the document styles its components
    shown.view->refreshDisplay (0.0);

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);
    ASSERT_NE (node->getShader(), nullptr);
    EXPECT_NE (node->getShader()->getRasterBundle(), nullptr);
    EXPECT_NE (node->getShader()->getFullBundle(), nullptr);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // The variants a build compiles the material into come first: its source is not read
    EXPECT_EQ (reads["fx/glow.glsl.raster.ysl"], 1);
    EXPECT_EQ (reads["fx/glow.glsl.full.ysl"], 1);
    EXPECT_EQ (reads["fx/glow.glsl"], 0);

    // The material stays beside the shader: it still tells how the meshes are drawn, and what the function reads of them
    EXPECT_EQ (overrideOf (shown.quad->getEntity()), shown.paint->getMaterial());
    EXPECT_NEAR (shown.paint->getMaterial()->baseColorFactor[2], toLinear (96.0f / 255.0f), 1.0e-5f);

    // What nothing gives a value takes the default of the block: the color linear, as the function returns its own
    expectFloats (floatsAt (node, 0, 4), { 0.0f, 1.0f, 1.0f, 1.0f });
    expectFloats (floatsAt (node, 16), { 1.0f });
}

TEST_F (SceneMaterialComponentTests, AShaderTheStylesheetDoesNotHaveOrThatIsNotAMaterialIsReported)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("shader Glow { material: \"fx/glow.glsl\"; }\n"
                                                 "shader Wave { fragment: \"fx/wave.frag\"; }\n"
                                                 "Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Mesh3D { id: first; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: \"Nothing\"; } }\n"
                                                 "    Mesh3D { id: second; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Wave; } }\n"
                                                 "    Mesh3D { id: third; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { id: quoted; shader: \"Glow\"; } }\n"
                                                 "}");

    // A name is written bare, and a bare word that names no block is refused when the document is built: a name the
    // stylesheet does not have reaches the material as a string, which is also a way to write one it has
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* view = document.findById<SceneComponent> ("view");
    auto* first = document.findById<SceneMeshComponent> ("first");
    auto* second = document.findById<SceneMeshComponent> ("second");
    auto* third = document.findById<SceneMeshComponent> ("third");
    auto* quoted = document.findById<SceneMaterialComponent> ("quoted");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);
    ASSERT_NE (third, nullptr);
    ASSERT_NE (quoted, nullptr);
    EXPECT_EQ (quoted->getShaderName(), "Glow");

    showScene (*view);
    view->refreshDisplay (0.0);

    expectDiagnostic (document, "Material3D.shader: the stylesheet has no shader 'Nothing'");
    expectDiagnostic (document, "Material3D.shader: the shader 'Wave' has a 'fragment', a material takes a shader with a 'material'");
    EXPECT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* unknown = findDiagnostic (document, "'Nothing'");
    ASSERT_NE (unknown, nullptr);
    EXPECT_EQ (unknown->line, 5);

    const auto* image = findDiagnostic (document, "'Wave'");
    ASSERT_NE (image, nullptr);
    EXPECT_EQ (image->line, 6);

    // The meshes are left to their materials
    EXPECT_EQ (shaderNodeOf (*first), nullptr);
    EXPECT_EQ (shaderNodeOf (*second), nullptr);
    EXPECT_NE (shaderNodeOf (*third), nullptr);
    EXPECT_NE (first->getEntity().get<MaterialNode>(), nullptr);
    EXPECT_NE (second->getEntity().get<MaterialNode>(), nullptr);

    // What is wrong was told once: the frames that follow have nothing to add
    view->refreshDisplay (0.0);
    EXPECT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());

    // A document forgets what it was told when it loads: the first frame after it tells what is still wrong
    const auto again = document.loadFromString ("shader Glow { material: \"fx/glow.glsl\"; }\n"
                                                "Scene3D {\n"
                                                "    id: view;\n"
                                                "    Mesh3D { id: first; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: \"Nothing\"; } }\n"
                                                "    Mesh3D { id: second; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Glow; } }\n"
                                                "}");

    ASSERT_TRUE (again.wasOk()) << again.getErrorMessage();
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);

    view->refreshDisplay (0.0);

    expectDiagnostic (document, "Material3D.shader: the stylesheet has no shader 'Nothing'");
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_NE (shaderNodeOf (*document.findById<SceneMeshComponent> ("second")), nullptr);
}

TEST_F (SceneMaterialComponentTests, AMaterialLoadsFromEitherOfItsVariantsAndReportsTheFilesItCanNotUse)
{
    files["fx/flat.glsl.raster.ysl"] = variantFile (1, {});
    files["fx/deep.glsl.full.ysl"] = variantFile (2, glowMembers());
    files["fx/broken.glsl.raster.ysl"] = bytesOf ("this is not a shader bundle");
    files["fx/swapped.glsl.raster.ysl"] = variantFile (2, {});

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("shader Flat { material: \"fx/flat.glsl\"; }\n"
                                                 "shader Deep { material: \"fx/deep.glsl\"; }\n"
                                                 "shader Missing { material: \"fx/missing.glsl\"; }\n"
                                                 "shader Broken { material: \"fx/broken.glsl\"; }\n"
                                                 "shader Swapped { material: \"fx/swapped.glsl\"; }\n"
                                                 "Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Mesh3D { id: flat; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Flat; } }\n"
                                                 "    Mesh3D { id: deep; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Deep; } }\n"
                                                 "    Mesh3D { id: missing; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Missing; } }\n"
                                                 "    Mesh3D { id: broken; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Broken; } }\n"
                                                 "    Mesh3D { id: swapped; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Swapped; } }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);

    showScene (*view);
    view->refreshDisplay (0.0);

    const auto nodeOf = [&document] (const char* id) -> const MaterialShaderNode*
    {
        const auto* mesh = document.findById<SceneMeshComponent> (id);
        return mesh != nullptr ? shaderNodeOf (*mesh) : nullptr;
    };

    // A material compiled for one tier of the renderer only draws on that one
    const auto* flat = nodeOf ("flat");
    ASSERT_NE (flat, nullptr);
    ASSERT_NE (flat->getShader(), nullptr);
    EXPECT_NE (flat->getShader()->getRasterBundle(), nullptr);
    EXPECT_EQ (flat->getShader()->getFullBundle(), nullptr);
    EXPECT_TRUE (flat->getUniforms().isEmpty());

    const auto* deep = nodeOf ("deep");
    ASSERT_NE (deep, nullptr);
    ASSERT_NE (deep->getShader(), nullptr);
    EXPECT_EQ (deep->getShader()->getRasterBundle(), nullptr);
    EXPECT_NE (deep->getShader()->getFullBundle(), nullptr);
    EXPECT_EQ (deep->getUniforms().getSizeInBytes(), 32u);

    // What can't be read, is no bundle, or is not the material template of its tier, is told at the block of the shader,
    // worded as what prevents an image shader from loading is
    EXPECT_EQ (nodeOf ("missing"), nullptr);
    EXPECT_EQ (nodeOf ("broken"), nullptr);
    EXPECT_EQ (nodeOf ("swapped"), nullptr);

    EXPECT_EQ (document.getDiagnostics().size(), 3u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* missing = findDiagnostic (document, "'Missing'");
    ASSERT_NE (missing, nullptr);
    EXPECT_EQ (missing->message, "The shader 'Missing' can't read 'fx/missing.glsl' nor its compiled variants 'fx/missing.glsl.raster.ysl' and 'fx/missing.glsl.full.ysl'");
    EXPECT_EQ (missing->line, 3);
    EXPECT_EQ (missing->property.toString(), "material");

    const auto* broken = findDiagnostic (document, "shader Broken: ");
    ASSERT_NE (broken, nullptr);
    EXPECT_EQ (broken->line, 4);

    const auto* swapped = findDiagnostic (document, "shader Swapped: ");
    ASSERT_NE (swapped, nullptr);
    EXPECT_EQ (swapped->line, 5);
    EXPECT_TRUE (swapped->message.contains ("raster tier")) << swapped->message;
}

TEST_F (SceneMaterialComponentTests, TheShaderOfAFileIsLoadedOnceWhateverTheMaterialsAndTheStylesheet)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    const auto text = [] (const String& rules)
    {
        return "shader Glow { material: \"fx/glow.glsl\"; property strength: number = 1; }\n"
               "shader Twin { material: \"fx/glow.glsl\"; property strength: number = 2; }\n"
               "style { "
             + rules
             + " }\n"
               "Scene3D {\n"
               "    id: view;\n"
               "    Mesh3D { id: quad; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { id: paint; shader: Glow; } }\n"
               "    Mesh3D { id: other; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Glow; strength: 3; } }\n"
               "    Mesh3D { id: twin; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Twin; } }\n"
               "}";
    };

    YuiDocument document;
    serve (document);
    load (document, text (".armed > Material3D { strength: 4; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto* other = document.findById<SceneMeshComponent> ("other");
    auto* twin = document.findById<SceneMeshComponent> ("twin");

    ASSERT_NE (other, nullptr);
    ASSERT_NE (twin, nullptr);

    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    ASSERT_NE (shaderNodeOf (*other), nullptr);
    ASSERT_NE (shaderNodeOf (*twin), nullptr);

    // One shader for the file, whatever the number of materials and of blocks naming it, read once
    const auto shader = shown.shaderNode()->getShader();
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shaderNodeOf (*other)->getShader(), shader);
    EXPECT_EQ (shaderNodeOf (*twin)->getShader(), shader);
    EXPECT_EQ (reads["fx/glow.glsl.raster.ysl"], 1);
    EXPECT_EQ (reads["fx/glow.glsl.full.ysl"], 1);

    // Each material gives it its own values
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });
    expectFloats (floatsAt (shaderNodeOf (*other), 16), { 3.0f });
    expectFloats (floatsAt (shaderNodeOf (*twin), 16), { 2.0f });

    // Another stylesheet looks for the shader again, and finds the one that was loaded
    const auto sheet = document.getStyleSheet();
    load (document, text (".armed > Material3D { strength: 5; }"));

    ASSERT_NE (document.getStyleSheet(), sheet);
    ASSERT_EQ (document.findById<SceneComponent> ("view"), shown.view);
    ASSERT_EQ (document.findById<SceneMeshComponent> ("quad"), shown.quad);

    shown.quad->addStyleClass (Identifier ("armed"));
    document.getRoot()->getStyleContext()->restyleNow();
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), shader);
    EXPECT_EQ (reads["fx/glow.glsl.raster.ysl"], 1);
    EXPECT_EQ (reads["fx/glow.glsl.full.ysl"], 1);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 5.0f });
}

TEST_F (SceneMaterialComponentTests, AMaterialIsLoadedAgainWhenOneOfItsFilesChangesAndKeepsItsShaderWhenThatFails)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(), "Material3D { id: paint; shader: Glow; strength: 2; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);
    ASSERT_NE (shown.shaderNode(), nullptr);

    const auto first = shown.shaderNode()->getShader();
    ASSERT_NE (first, nullptr);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 2.0f });

    // A file that is not one of the material changes nothing
    document.notifyAssetChanged ("fx/other.glsl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), first);
    EXPECT_EQ (reads["fx/glow.glsl.raster.ysl"], 1);

    // One of its variants changed: the material is loaded again, into another shader that is given the values at once
    files["fx/glow.glsl.full.ysl"] = variantFile (2, glowMembers());
    document.notifyAssetChanged ("fx/glow.glsl.full.ysl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);

    const auto second = shown.shaderNode()->getShader();
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    EXPECT_EQ (reads["fx/glow.glsl.raster.ysl"], 2);
    EXPECT_EQ (reads["fx/glow.glsl.full.ysl"], 2);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 2.0f });
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // A file that can't be loaded anymore, as one being edited, leaves the shader as it was and is reported
    files["fx/glow.glsl.raster.ysl"] = bytesOf ("this is not a shader bundle");
    document.notifyAssetChanged ("fx/glow.glsl.raster.ysl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), second);
    expectDiagnostic (document, "shader Glow: ");
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().line, 1);

    // Its values still follow the properties
    shown.paint->setProperty (Identifier ("strength"), 4.0);
    shown.view->refreshDisplay (0.0);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 4.0f });

    // Right again, it is loaded
    files["fx/glow.glsl.raster.ysl"] = variantFile (1, glowMembers());
    document.notifyAssetChanged ("fx/glow.glsl.raster.ysl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    ASSERT_NE (shown.shaderNode()->getShader(), nullptr);
    EXPECT_NE (shown.shaderNode()->getShader(), second);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 4.0f });
}

TEST_F (SceneMaterialComponentTests, AFileThatChangesHasOnlyTheMaterialsReadingItLookForTheirShaderAgain)
{
    serveMaterial ("fx/glow.glsl", glowMembers());
    serveMaterial ("fx/other.glsl", glowMembers());

    YuiDocument document;
    serve (document);

    int numTold = 0;
    DiagnosticListener listener (document, [&numTold] (const DataTreeDiagnostic&)
    {
        ++numTold;
    });

    // The default of a property of Glow can't be used, which a material tells each time it looks for that shader
    const auto result = document.loadFromString ("shader Glow { material: \"fx/glow.glsl\"; property strength: number = 1; property rim: color = nothing.value; }\n"
                                                 "shader Other { material: \"fx/other.glsl\"; property strength: number = 3; }\n"
                                                 "Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Mesh3D { id: quad; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { id: paint; shader: Glow; } }\n"
                                                 "    Mesh3D { id: other; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Other; } }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto* other = document.findById<SceneMeshComponent> ("other");
    ASSERT_NE (other, nullptr);

    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    ASSERT_NE (shaderNodeOf (*other), nullptr);
    expectDiagnostic (document, "The default of the property 'rim' of the shader 'Glow' can't be used: ");

    const auto glow = shown.shaderNode()->getShader();
    const auto otherShader = shaderNodeOf (*other)->getShader();
    const auto toldByTheLoad = numTold;

    // A file of the other material: that one is loaded again, and the material of Glow is left with its shader, which
    // it does not look for again
    document.notifyAssetChanged ("fx/other.glsl.raster.ysl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    ASSERT_NE (shaderNodeOf (*other), nullptr);
    EXPECT_NE (shaderNodeOf (*other)->getShader(), otherShader);
    EXPECT_EQ (shown.shaderNode()->getShader(), glow);
    EXPECT_EQ (numTold, toldByTheLoad);
    EXPECT_EQ (reads["fx/other.glsl.raster.ysl"], 2);
    EXPECT_EQ (reads["fx/glow.glsl.raster.ysl"], 1);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });
    expectFloats (floatsAt (shaderNodeOf (*other), 16), { 3.0f });

    // A file of its own: it is loaded again. What its block has wrong was told already, and a document tells a problem once
    document.notifyAssetChanged ("fx/glow.glsl.full.ysl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_NE (shown.shaderNode()->getShader(), glow);
    EXPECT_EQ (numTold, toldByTheLoad);
    EXPECT_EQ (reads["fx/glow.glsl.full.ysl"], 2);
    EXPECT_EQ (reads["fx/other.glsl.raster.ysl"], 2);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });
}

TEST_F (SceneMaterialComponentTests, TheShaderFollowsTheMaterialAndTheNameItIsGiven)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(), "Material3D { id: paint; shader: Glow; }", {}, "Mesh3D { id: other; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto* other = document.findById<SceneMeshComponent> ("other");
    ASSERT_NE (other, nullptr);

    shown.view->refreshDisplay (0.0);
    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shaderNodeOf (*other), nullptr);

    // Without a name the meshes are left to the material, at once
    shown.paint->setShaderName ({});
    EXPECT_EQ (shown.shaderNode(), nullptr);
    EXPECT_EQ (overrideOf (shown.quad->getEntity()), shown.paint->getMaterial());

    shown.view->refreshDisplay (0.0);
    EXPECT_EQ (shown.shaderNode(), nullptr);

    // Named again, the frame that follows finds the shader
    shown.paint->setProperty (Identifier ("shader"), "Glow");
    shown.view->refreshDisplay (0.0);
    ASSERT_NE (shown.shaderNode(), nullptr);

    // Moved to another mesh, the material takes its shader along
    other->addAndMakeVisible (*shown.paint);
    EXPECT_EQ (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.quad->getEntity().get<MaterialNode>(), nullptr);

    shown.view->refreshDisplay (0.0);
    ASSERT_NE (shaderNodeOf (*other), nullptr);
    EXPECT_EQ (overrideOf (other->getEntity()), shown.paint->getMaterial());
    expectFloats (floatsAt (shaderNodeOf (*other), 16), { 1.0f });

    // And it goes with the material
    load (document, glowWith (glowInputs(), "", {}, "Mesh3D { id: other; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; }"));

    other = document.findById<SceneMeshComponent> ("other");
    ASSERT_NE (other, nullptr);
    ASSERT_EQ (document.findById<SceneComponent> ("view"), shown.view);
    EXPECT_EQ (document.findById ("paint"), nullptr);
    EXPECT_EQ (shaderNodeOf (*other), nullptr);
    EXPECT_EQ (other->getEntity().get<MaterialNode>(), nullptr);

    shown.view->refreshDisplay (0.0);
    EXPECT_EQ (shaderNodeOf (*other), nullptr);
}

TEST_F (SceneMaterialComponentTests, TheInputsOfAShaderAreThePropertiesOfTheMaterial)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(),
                              "Material3D { id: paint; shader: Glow; strength: knob.value * 2; }",
                              ".armed > Material3D { rim: #ff0000; strength: 9; }",
                              "Slider { id: knob; value: 0.25; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto* knob = document.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);

    shown.view->refreshDisplay (0.0);

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);

    // A value the document gives, here through a binding, and the default of the block for the rest
    expectFloats (floatsAt (node, 0, 4), { 0.0f, 1.0f, 1.0f, 1.0f });
    expectFloats (floatsAt (node, 16), { 0.5f });

    // Values that did not change are not given to the renderer again, which would start accumulating its frames over
    const auto* bytes = node->getUniforms().getData();
    Internals::takeRepaintRequest (*shown.view);

    shown.view->refreshDisplay (0.25);
    EXPECT_EQ (node->getUniforms().getData(), bytes);
    EXPECT_FALSE (Internals::takeRepaintRequest (*shown.view));

    // The binding follows the slider, and the frame that comes next shows it
    knob->setValue (1.0);
    Internals::takeRepaintRequest (*shown.view);

    shown.view->refreshDisplay (0.25);
    expectFloats (floatsAt (node, 16), { 2.0f });
    EXPECT_TRUE (Internals::takeRepaintRequest (*shown.view));

    // A rule gives a value to what the material was given none, and leaves the rest
    auto& context = *document.getRoot()->getStyleContext();

    shown.quad->addStyleClass (Identifier ("armed"));
    context.restyleNow();
    shown.view->refreshDisplay (0.25);

    expectFloats (floatsAt (node, 0, 4), { 1.0f, 0.0f, 0.0f, 1.0f });
    expectFloats (floatsAt (node, 16), { 2.0f });

    shown.quad->removeStyleClass (Identifier ("armed"));
    context.restyleNow();
    shown.view->refreshDisplay (0.25);

    expectFloats (floatsAt (node, 0, 4), { 0.0f, 1.0f, 1.0f, 1.0f });

    // A color is given linear, as the function returns its own, with its alpha as it is
    shown.paint->setProperty (Identifier ("rim"), VariantConverter<Color>::toVar (Color (0x40, 0x80, 0x80, 0x80)));
    shown.view->refreshDisplay (0.25);

    const auto gray = toLinear (128.0f / 255.0f);
    expectFloats (floatsAt (node, 0, 4), { gray, gray, gray, 64.0f / 255.0f });
    EXPECT_LT (gray, 0.25f);
}

TEST_F (SceneMaterialComponentTests, AnInputIsNotGivenWhatTheComponentsAroundItHoldForThemselves)
{
    serveMaterial ("fx/rings.glsl", { uniformMember ("scale", 0), uniformMember ("roughness", 4) });

    YuiDocument document;
    serve (document);
    load (document, "shader Rings { material: \"fx/rings.glsl\"; property scale: number = 8; property roughness: number = 0.25; }\n"
                    "style { Mesh3D.big { scale: 1.5; transition: scale 100ms; } }\n"
                    "Scene3D { id: view; Mesh3D { id: quad; "
                        + quadGeometry()
                        + " Material3D { id: paint; shader: Rings; roughness: 0.75; } } }");

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto& context = *document.getRoot()->getStyleContext();

    shown.view->refreshDisplay (0.0);

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);

    // The default of the block and, for an input named like a property of the material, that property: never its default
    expectFloats (floatsAt (node, 0), { 8.0f });
    expectFloats (floatsAt (node, 4), { 0.75f });

    // A rule gives the mesh a scale, which is its own: the shader is not given it, while the transition runs nor after
    shown.quad->addStyleClass (Identifier ("big"));
    context.refresh (0.0);
    context.refresh (0.05);
    shown.view->refreshDisplay (0.05);
    expectFloats (floatsAt (node, 0), { 8.0f });

    context.refresh (0.06);
    shown.view->refreshDisplay (0.06);
    expectVector (scaleOf (*shown.quad), 1.5f, 1.5f, 1.5f);
    expectFloats (floatsAt (node, 0), { 8.0f });

    // What a component with no property of that name is given under it is for the shaders inside it, past the mesh
    shown.view->setProperty (Identifier ("scale"), 3.0);
    shown.view->refreshDisplay (0.0);
    expectFloats (floatsAt (node, 0), { 3.0f });

    shown.quad->removeStyleClass (Identifier ("big"));
    context.refresh (0.0);
    context.refresh (0.11);
    shown.view->refreshDisplay (0.11);
    expectVector (scaleOf (*shown.quad), 1.0f, 1.0f, 1.0f);
    expectFloats (floatsAt (node, 0), { 3.0f });

    // What the material is given itself comes first
    shown.paint->setProperty (Identifier ("scale"), 5.0);
    shown.view->refreshDisplay (0.0);
    expectFloats (floatsAt (node, 0), { 5.0f });
}

TEST_F (SceneMaterialComponentTests, ATransitionBlendsTheInputsOfAShader)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(),
                              "Material3D { id: paint; shader: Glow; }",
                              "Material3D { strength: 1; transition: strength 100ms; } .armed > Material3D { strength: 3; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto& context = *document.getRoot()->getStyleContext();

    shown.view->refreshDisplay (0.0);
    ASSERT_NE (shown.shaderNode(), nullptr);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });

    shown.quad->addStyleClass (Identifier ("armed"));
    context.refresh (0.0);
    shown.view->refreshDisplay (0.0);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });

    // Each frame reads the value the transition has reached
    context.refresh (0.05);
    Internals::takeRepaintRequest (*shown.view);

    shown.view->refreshDisplay (0.05);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 2.0f }, 1.0e-3f);
    EXPECT_TRUE (Internals::takeRepaintRequest (*shown.view));

    context.refresh (0.06);
    shown.view->refreshDisplay (0.06);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 3.0f });
}

TEST_F (SceneMaterialComponentTests, AShaderIsNamedByAStyleRule)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(), "Material3D { id: paint; }", ".glowing > Material3D { shader: Glow; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto& context = *document.getRoot()->getStyleContext();

    shown.view->refreshDisplay (0.0);
    EXPECT_EQ (shown.shaderNode(), nullptr);

    shown.quad->addStyleClass (Identifier ("glowing"));
    context.restyleNow();
    EXPECT_EQ (shown.paint->getShaderName(), "Glow");

    shown.view->refreshDisplay (0.0);
    ASSERT_NE (shown.shaderNode(), nullptr);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });

    shown.quad->removeStyleClass (Identifier ("glowing"));
    context.restyleNow();
    EXPECT_TRUE (shown.paint->getShaderName().isEmpty());
    EXPECT_EQ (shown.shaderNode(), nullptr);
}

TEST_F (SceneMaterialComponentTests, TheDefaultsOfABlockFillTheInputsOfEveryType)
{
    using BaseType = ShaderReflection::BaseType;

    serveMaterial ("fx/glow.glsl", { uniformMember ("rim", 0, 4), uniformMember ("strength", 16), uniformMember ("count", 20, 1, BaseType::int32), uniformMember ("lit", 24, 1, BaseType::uint32), uniformMember ("center", 32, 2), uniformMember ("area", 48, 4), uniformMember ("levels", 64, 1, BaseType::float32, 3) });

    YuiDocument document;
    serve (document);
    load (document, glowWith ("property rim: color = #ff000080;"
                              " property strength: number = 0.5;"
                              " property count: integer = 3;"
                              " property lit: boolean = true;"
                              " property center: point = [0.25, 0.75];"
                              " property area: rectangle = [1, 2, 3, 4];"
                              " property levels: number[] = [0.125, 0.25, 0.5];",
                              "Material3D { id: paint; shader: Glow; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);
    ASSERT_EQ (node->getUniforms().getSizeInBytes(), 112u);

    expectFloats (floatsAt (node, 0, 4), { 1.0f, 0.0f, 0.0f, 128.0f / 255.0f });
    expectFloats (floatsAt (node, 16), { 0.5f });
    EXPECT_EQ (intAt (node, 20), 3);
    EXPECT_EQ (intAt (node, 24), 1);
    expectFloats (floatsAt (node, 32, 2), { 0.25f, 0.75f });
    expectFloats (floatsAt (node, 48, 4), { 1.0f, 2.0f, 3.0f, 4.0f });
    expectFloats (floatsAt (node, 64), { 0.125f });
    expectFloats (floatsAt (node, 80), { 0.25f });
    expectFloats (floatsAt (node, 96), { 0.5f });

    // A value of the material takes the place of a default
    shown.paint->setProperty (Identifier ("levels"), numbers ({ 1.0, 2.0 }));
    shown.paint->setProperty (Identifier ("lit"), false);
    shown.view->refreshDisplay (0.0);

    expectFloats (floatsAt (node, 64), { 1.0f });
    expectFloats (floatsAt (node, 80), { 2.0f });
    expectFloats (floatsAt (node, 96), { 0.0f });
    EXPECT_EQ (intAt (node, 24), 0);
}

TEST_F (SceneMaterialComponentTests, ADefaultCanBeAnExpressionEvaluatedForTheMaterial)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    YuiDocument document;
    serve (document);
    load (document, glowWith ("property rim: color = palette.accent; property strength: number = knob.value + 1;",
                              "Material3D { id: paint; shader: Glow; }",
                              {},
                              "Slider { id: knob; value: 0.5; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto* knob = document.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);

    shown.view->refreshDisplay (0.0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);

    // The role of the palette the material is under, as a value of the material written the same way would be
    auto expression = PropertyExpression::compile (*shown.paint, "palette.accent", ComponentProperty::Type::color);
    ASSERT_TRUE (expression.wasOk()) << expression.getErrorMessage();

    const auto accent = VariantConverter<Color>::fromVar (expression.getReference()->evaluate());
    ASSERT_GT (accent.getAlphaFloat(), 0.0f);

    expectFloats (floatsAt (node, 0, 4), { toLinear (accent.getRedFloat()), toLinear (accent.getGreenFloat()), toLinear (accent.getBlueFloat()), accent.getAlphaFloat() }, 1.0e-4f);
    expectFloats (floatsAt (node, 16), { 1.5f });

    // It is read again on every frame, so it follows what it reads
    knob->setValue (1.0);
    shown.view->refreshDisplay (0.0);
    expectFloats (floatsAt (node, 16), { 2.0f });
}

TEST_F (SceneMaterialComponentTests, APropertyThatDoesNotFitTheUniformBlockIsReportedWhereItIsDeclared)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    const auto text = [] (const String& properties)
    {
        return "shader Glow {\n"
               "    material: \"fx/glow.glsl\";\n"
             + properties
             + "}\n"
               "Scene3D {\n"
               "    id: view;\n"
               "    Mesh3D { id: quad; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { id: paint; shader: Glow; } }\n"
               "    Mesh3D { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Glow; } }\n"
               "}";
    };

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString (text ("    property label: string = \"wide\";\n"
                                                       "    property missing: number = 1;\n"
                                                       "    property strength: color = #ffffff;\n"
                                                       "    property rim: color = nothing.value;\n"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);

    const auto expectAt = [&document] (int line, const String& message)
    {
        const auto* diagnostic = findDiagnostic (document, message);

        ASSERT_NE (diagnostic, nullptr) << "No diagnostic holding '" << message << "' in: " << DataTreeDiagnostic::join (document.getDiagnostics());
        EXPECT_TRUE (diagnostic->message.startsWith (message)) << diagnostic->message;
        EXPECT_EQ (diagnostic->line, line) << diagnostic->toString();
        EXPECT_TRUE (diagnostic->isError());
    };

    // Each one once for the two materials naming the shader, at the property and worded as the ones of an image shader are
    expectAt (3, "The property 'label' of the shader 'Glow' can't be of type 'string': a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]");
    expectAt (4, "The uniform block of the shader 'Glow' has no member 'missing'");
    expectAt (5, "The member 'strength' of the uniform block of the shader 'Glow' is too small for a color");
    expectAt (6, "The default of the property 'rim' of the shader 'Glow' can't be used: ");
    EXPECT_EQ (document.getDiagnostics().size(), 4u) << DataTreeDiagnostic::join (document.getDiagnostics());

    // A block that does not fit the material is not used
    EXPECT_EQ (shown.shaderNode(), nullptr);
    EXPECT_EQ (overrideOf (shown.quad->getEntity()), shown.paint->getMaterial());

    // Once it does, the material is: only a default that can't be used is left out, and told
    const auto fixed = document.loadFromString (text ("    property strength: number = 1;\n"
                                                      "    property rim: color = nothing.value;\n"));

    ASSERT_TRUE (fixed.wasOk()) << fixed.getErrorMessage();
    ASSERT_EQ (document.findById<SceneComponent> ("view"), shown.view);
    ASSERT_EQ (document.findById<SceneMeshComponent> ("quad"), shown.quad);

    shown.view->refreshDisplay (0.0);

    expectAt (4, "The default of the property 'rim' of the shader 'Glow' can't be used: ");
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    ASSERT_NE (shown.shaderNode(), nullptr);
    expectFloats (floatsAt (shown.shaderNode(), 0, 4), { 0.0f, 0.0f, 0.0f, 0.0f });
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });
}

TEST_F (SceneMaterialComponentTests, WhatPreventsAMaterialFromLoadingIsReportedOnceAtItsBlockUntilItLoads)
{
    const String text ("shader Broken {\n"
                       "    material: \"fx/broken.glsl\";\n"
                       "    property strength: number = 1;\n"
                       "}\n"
                       "Scene3D {\n"
                       "    id: view;\n"
                       "    Material3D { shader: Broken; }\n"
                       "    Mesh3D { id: quad; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { id: paint; shader: Broken; } }\n"
                       "    Mesh3D { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Broken; } }\n"
                       "}");

    YuiDocument document;
    serve (document);

    int numTold = 0;
    DiagnosticListener listener (document, [&numTold] (const DataTreeDiagnostic&)
    {
        ++numTold;
    });

    ASSERT_TRUE (document.loadFromString (text).wasOk());

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);

    // Three materials name the shader: one problem, where its block names the file, and nothing from the materials
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto problem = document.getDiagnostics().front();
    EXPECT_EQ (problem.message, "The shader 'Broken' can't read 'fx/broken.glsl' nor its compiled variants 'fx/broken.glsl.raster.ysl' and 'fx/broken.glsl.full.ysl'");
    EXPECT_EQ (problem.line, 2);
    EXPECT_EQ (problem.property.toString(), "material");
    EXPECT_TRUE (problem.isError());
    EXPECT_EQ (numTold, 1);
    EXPECT_EQ (shown.shaderNode(), nullptr);

    // The frames that follow have nothing to add
    shown.view->refreshDisplay (0.0);
    EXPECT_EQ (numTold, 1);

    // It stands: a document forgets what it was told when it loads, and is told again
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    ASSERT_EQ (document.findById<SceneComponent> ("view"), shown.view);
    ASSERT_EQ (document.findById<SceneMeshComponent> ("quad"), shown.quad);

    shown.view->refreshDisplay (0.0);

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().message, problem.message);
    EXPECT_EQ (document.getDiagnostics().front().line, 2);

    // Until the material loads
    serveMaterial ("fx/broken.glsl", glowMembers());
    document.notifyAssetChanged ("fx/broken.glsl.raster.ysl");
    document.notifyAssetChanged ("fx/broken.glsl.full.ysl");
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 1.0f });

    ASSERT_TRUE (document.loadFromString (text).wasOk());
    shown.view->refreshDisplay (0.0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (SceneMaterialComponentTests, TheTimeOfAShaderHasTheSceneDrawnAgainOnEveryFrame)
{
    serveMaterial ("fx/glow.glsl", { uniformMember ("rim", 0, 4), uniformMember ("strength", 16), uniformMember ("time", 20) });

    YuiDocument document;
    serve (document);
    load (document, glowWith ("property strength: number = 1;", "Material3D { id: paint; shader: Glow; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);
    expectFloats (floatsAt (node, 20), { 0.0f });

    // A `time` member no property is declared for takes the seconds the material has been shown for
    Internals::takeRepaintRequest (*shown.view);
    shown.view->refreshDisplay (0.25);

    expectFloats (floatsAt (node, 20), { 0.25f });
    EXPECT_TRUE (Internals::takeRepaintRequest (*shown.view));

    shown.view->refreshDisplay (0.5);
    expectFloats (floatsAt (node, 20), { 0.75f });
    EXPECT_TRUE (Internals::takeRepaintRequest (*shown.view));

    // They start again after an hour, which a float keeps precise
    shown.view->refreshDisplay (3599.5);
    expectFloats (floatsAt (node, 20), { 0.25f }, 1.0e-3f);

    // A mesh that is not shown is not drawn: nothing is asked of the frames for it
    shown.quad->setVisible (false);
    Internals::takeRepaintRequest (*shown.view);

    shown.view->refreshDisplay (0.25);
    EXPECT_FALSE (Internals::takeRepaintRequest (*shown.view));
    expectFloats (floatsAt (node, 20), { 0.25f }, 1.0e-3f);

    shown.quad->setVisible (true);
    shown.view->refreshDisplay (0.25);
    expectFloats (floatsAt (node, 20), { 0.5f }, 1.0e-3f);

    // Nor once the material names no shader
    shown.paint->setShaderName ({});
    Internals::takeRepaintRequest (*shown.view);

    shown.view->refreshDisplay (0.25);
    EXPECT_FALSE (Internals::takeRepaintRequest (*shown.view));

    // A block declaring a property of that name gives the member its value, as any other
    YuiDocument still;
    serve (still);
    load (still, glowWith ("property time: number = 5;", "Material3D { id: paint; shader: Glow; }"));

    const Shown shownStill (still);
    ASSERT_TRUE (shownStill.isValid());

    shownStill.view->refreshDisplay (0.0);
    ASSERT_NE (shownStill.shaderNode(), nullptr);
    expectFloats (floatsAt (shownStill.shaderNode(), 20), { 5.0f });

    Internals::takeRepaintRequest (*shownStill.view);
    shownStill.view->refreshDisplay (0.25);

    expectFloats (floatsAt (shownStill.shaderNode(), 20), { 5.0f });
    EXPECT_FALSE (Internals::takeRepaintRequest (*shownStill.view));
}

TEST_F (SceneMaterialComponentTests, AMaterialNamingAShaderHasItsSceneTakeEveryFrameOfItsWindow)
{
    serveMaterial ("fx/glow.glsl", glowMembers());

    Component window;
    auto& native = makeWindow (window);

    YuiDocument document;
    serve (document);
    load (document, glowWith (glowInputs(), "Material3D { id: paint; shader: Glow; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    window.addChildComponent (*shown.view);

    // The values its shader reads are those of properties nothing tells the changes of: they are read on every frame
    sendFrames (native, 3);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_TRUE (native.hasFrameListeners());

    // Without a shader the scene stands still, and leaves the window free to skip its frames
    shown.paint->setShaderName ({});
    sendFrames (native, 3);

    EXPECT_FALSE (native.hasFrameListeners());
}

#if __has_include(<YuiTestResources.h>)

TEST_F (SceneMaterialComponentTests, AMaterialABuildCompiledIsLoadedFromItsEmbeddedVariants)
{
    for (const auto* path : { "fx/rim.glsl.raster.ysl", "fx/rim.glsl.full.ysl" })
    {
        std::size_t size = 0;
        const auto* data = YuiTestResources_find (path, size);

        ASSERT_NE (data, nullptr) << path;
        files[path] = MemoryBlock (data, size);
    }

    YuiDocument document;
    serve (document);
    load (document, "shader Rim { material: \"fx/rim.glsl\"; property rim: color = #ffffff80; property strength: number = 2; }\n"
                        + quadWith ("Material3D { id: paint; shader: Rim; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);
    ASSERT_NE (node->getShader(), nullptr);
    EXPECT_NE (node->getShader()->getRasterBundle(), nullptr);
    EXPECT_NE (node->getShader()->getFullBundle(), nullptr);

    // `vec4 rim; float strength;`, as the compiler lays the block out
    ASSERT_EQ (node->getUniforms().getSizeInBytes(), 32u);
    expectFloats (floatsAt (node, 0, 4), { 1.0f, 1.0f, 1.0f, 128.0f / 255.0f });
    expectFloats (floatsAt (node, 16), { 2.0f });
}

#endif

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (SceneMaterialComponentTests, AMaterialWithoutVariantsIsCompiledFromItsSource)
{
    files["fx/hologram.glsl"] = bytesOf (R"(YUP_MATERIAL_PARAMS
{
    vec4 rim;
    float strength;
} params;

vec4 yupMaterial (YupSurface surface)
{
    float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);
    return vec4 (surface.baseColor.rgb + params.rim.rgb * fresnel * params.strength, surface.baseColor.a);
}
)");

    files["fx/wrong.glsl"] = bytesOf ("vec4 yupMaterial (YupSurface surface) { return nothing; }\n");

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("shader Hologram { material: \"fx/hologram.glsl\"; property rim: color = #00ffff; property strength: number = 2; }\n"
                                                 "shader Wrong { material: \"fx/wrong.glsl\"; }\n"
                                                 "Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Mesh3D { id: quad; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { id: paint; shader: Hologram; } }\n"
                                                 "    Mesh3D { id: other; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Hologram; } }\n"
                                                 "    Mesh3D { id: wrong; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Wrong; } }\n"
                                                 "    Mesh3D { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; Material3D { shader: Wrong; } }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    auto* other = document.findById<SceneMeshComponent> ("other");
    auto* wrong = document.findById<SceneMeshComponent> ("wrong");

    ASSERT_NE (other, nullptr);
    ASSERT_NE (wrong, nullptr);

    shown.view->refreshDisplay (0.0);

    // Both variants, compiled once for the two materials
    const auto* node = shown.shaderNode();
    ASSERT_NE (node, nullptr);
    ASSERT_NE (node->getShader(), nullptr);
    EXPECT_NE (node->getShader()->getRasterBundle(), nullptr);
    EXPECT_NE (node->getShader()->getFullBundle(), nullptr);

    ASSERT_NE (shaderNodeOf (*other), nullptr);
    EXPECT_EQ (shaderNodeOf (*other)->getShader(), node->getShader());
    EXPECT_EQ (reads["fx/hologram.glsl"], 1);

    ASSERT_EQ (node->getUniforms().getSizeInBytes(), 32u);
    expectFloats (floatsAt (node, 0, 4), { 0.0f, 1.0f, 1.0f, 1.0f });
    expectFloats (floatsAt (node, 16), { 2.0f });

    // What the compiler says of a source it can't compile is told once, at the block, for the two materials naming it
    EXPECT_EQ (shaderNodeOf (*wrong), nullptr);
    expectDiagnostic (document, "shader Wrong: ");
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().line, 2);
    EXPECT_EQ (document.getDiagnostics().front().property.toString(), "material");

    // And the source is not compiled again for it
    shown.view->refreshDisplay (0.0);
    EXPECT_EQ (reads["fx/wrong.glsl"], 1);
}

TEST_F (SceneMaterialComponentTests, AMaterialIsCompiledForTheGraphicsApiDrawingItsSceneAndOnceForAnother)
{
    files["fx/hologram.glsl"] = bytesOf (hologramSource);

    Component window;

    YuiDocument document;
    serve (document);
    load (document, "shader Hologram { material: \"fx/hologram.glsl\"; property strength: number = 2; }\n"
                        + quadWith ("Material3D { id: paint; shader: Hologram; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto defaultApi = ComponentNative::getDefaultGraphicsApi();
    const auto otherApi = otherApiThan (defaultApi);

    // In no window, and never painted: for the API a window is given by default, and for that one alone
    ASSERT_NE (shown.shaderNode(), nullptr);

    const auto first = shown.shaderNode()->getShader();
    EXPECT_TRUE (isCompiledFor (first, defaultApi));
    EXPECT_EQ (reads["fx/hologram.glsl"], 1);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 2.0f });

    // A window drawing with that API has nothing to change
    auto& native = makeWindowDrawingWith (window, defaultApi);
    window.addChildComponent (*shown.view);
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), first);
    EXPECT_EQ (reads["fx/hologram.glsl"], 1);

    // One drawing with another API has the next frame compile the material for it, into a shader given the values at once
    native.setGraphicsApi (otherApi);
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);

    const auto second = shown.shaderNode()->getShader();
    EXPECT_NE (second, first);
    EXPECT_TRUE (isCompiledFor (second, otherApi));
    EXPECT_TRUE (second->createUniforms() == first->createUniforms());
    EXPECT_EQ (reads["fx/hologram.glsl"], 2);
    expectFloats (floatsAt (shown.shaderNode(), 16), { 2.0f });
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // Once: the frames that follow keep it
    shown.view->refreshDisplay (0.0);
    shown.view->refreshDisplay (0.0);

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), second);
    EXPECT_EQ (reads["fx/hologram.glsl"], 2);
}

TEST_F (SceneMaterialComponentTests, AMaterialTheGraphicsApiOfTheSceneDoesNotCompileIsReportedOnceAndKeepsItsShader)
{
    // What WebGPU alone refuses is not told while another API is the one in use
    if (ComponentNative::getDefaultGraphicsApi() == GpuPlatform::WebGPU)
        GTEST_SKIP() << "The source of this test is one the default graphics API does not compile";

    files["fx/level.glsl"] = bytesOf (notForWebGpuSource);

    Component window;

    YuiDocument document;
    serve (document);
    load (document, "shader Level { material: \"fx/level.glsl\"; }\n" + quadWith ("Material3D { id: paint; shader: Level; }"));

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    ASSERT_NE (shown.shaderNode(), nullptr);

    const auto first = shown.shaderNode()->getShader();
    ASSERT_NE (first, nullptr);
    EXPECT_EQ (reads["fx/level.glsl"], 1);

    // In a window drawing with WebGPU the material is compiled for it, which fails: told at the block, and the shader
    // that colored the meshes is left to
    makeWindowDrawingWith (window, GpuPlatform::WebGPU);
    window.addChildComponent (*shown.view);
    shown.view->refreshDisplay (0.0);

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics().front().message.startsWith ("shader Level: ")) << document.getDiagnostics().front().message;
    EXPECT_TRUE (document.getDiagnostics().front().message.containsIgnoreCase ("wgsl")) << document.getDiagnostics().front().message;
    EXPECT_EQ (document.getDiagnostics().front().line, 1);
    EXPECT_EQ (document.getDiagnostics().front().property.toString(), "material");

    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), first);
    EXPECT_EQ (reads["fx/level.glsl"], 2);

    // The frames that follow neither compile it nor tell it again
    shown.view->refreshDisplay (0.0);
    shown.view->refreshDisplay (0.0);

    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (reads["fx/level.glsl"], 2);
    ASSERT_NE (shown.shaderNode(), nullptr);
    EXPECT_EQ (shown.shaderNode()->getShader(), first);
}

#else

TEST_F (SceneMaterialComponentTests, AMaterialWithoutVariantsCanNotBeCompiledWithoutTheTranspiler)
{
    files["fx/hologram.glsl"] = bytesOf ("vec4 yupMaterial (YupSurface surface) { return surface.baseColor; }\n");

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString ("shader Hologram { material: \"fx/hologram.glsl\"; }\n"
                                                     + quadWith ("Material3D { id: paint; shader: Hologram; }"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const Shown shown (document);
    ASSERT_TRUE (shown.isValid());

    shown.view->refreshDisplay (0.0);

    EXPECT_EQ (shown.shaderNode(), nullptr);
    expectDiagnostic (document, "shader Hologram: 'fx/hologram.glsl' is the source of a material, which this build can't compile, and there is no 'fx/hologram.glsl.raster.ysl' nor 'fx/hologram.glsl.full.ysl'");
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
}

#endif
