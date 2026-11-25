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

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

#include <map>

using namespace yup;

class SceneDocumentTests;

namespace yup
{

/** What the tests of the 3D components read of what a component keeps to itself. */
template <>
class ComponentTestHelper<SceneDocumentTests>
{
public:
    /** Reads and clears the request to repaint of a component: repaint() records it even without a window. */
    static bool takeRepaintRequest (Component& component)
    {
        return component.subtreeDirty.exchange (false);
    }

    /** Returns how many problems a scene keeps, to tell them again after each load of its document. */
    static size_t getNumProblems (const SceneComponent& view)
    {
        return view.problems.size();
    }

    /** Returns how many meshes a scene is to build by its next frame. */
    static int getNumChangedMeshes (const SceneComponent& view)
    {
        return view.changedMeshes.size();
    }
};

} // namespace yup

/** The 3D components of a `.yui` document: the tests build them from text and look at the Scene
    they fill, so no GPU is involved.
*/
class SceneDocumentTests : public ::testing::Test
{
protected:
    using Internals = ComponentTestHelper<SceneDocumentTests>;

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

    //==============================================================================
    /** Loads a document that has nothing to report. */
    static void load (YuiDocument& document, const String& text)
    {
        const auto result = document.loadFromString (text);

        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
        EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());
    }

    /** Returns the first diagnostic of a document holding a text, or nullptr. */
    static const YuiDiagnostic* findDiagnostic (const YuiDocument& document, const String& text)
    {
        for (const auto& diagnostic : document.getDiagnostics())
            if (diagnostic.message.contains (text))
                return &diagnostic;

        return nullptr;
    }

    static void expectDiagnostic (const YuiDocument& document, const String& text, YuiDiagnostic::Severity severity = YuiDiagnostic::Severity::error)
    {
        const auto* diagnostic = findDiagnostic (document, text);

        ASSERT_NE (diagnostic, nullptr) << "No diagnostic holding '" << text << "' in: " << YuiDiagnostic::join (document.getDiagnostics());
        EXPECT_EQ (diagnostic->severity, severity) << diagnostic->toString();
    }

    //==============================================================================
    /** An array of numbers, as the value of a property. */
    static var numbers (std::initializer_list<double> values)
    {
        Array<var> array;

        for (const auto value : values)
            array.add (value);

        return var (std::move (array));
    }

    static void expectNumbers (const var& actual, std::initializer_list<double> expected)
    {
        const auto* values = actual.getArray();

        ASSERT_NE (values, nullptr) << actual.toString();
        ASSERT_EQ (static_cast<size_t> (values->size()), expected.size());

        int index = 0;

        for (const auto value : expected)
            EXPECT_NEAR (static_cast<double> ((*values)[index++]), value, 1.0e-5);
    }

    static void expectVector (const Vector3<float>& actual, float x, float y, float z, float tolerance = 1.0e-4f)
    {
        EXPECT_NEAR (actual.getX(), x, tolerance);
        EXPECT_NEAR (actual.getY(), y, tolerance);
        EXPECT_NEAR (actual.getZ(), z, tolerance);
    }

    static void expectRotation (const Quaternion& actual, const Quaternion& expected)
    {
        EXPECT_TRUE (actual.approximatelyEqualTo (expected, 1.0e-4f))
            << "(" << actual.getX() << ", " << actual.getY() << ", " << actual.getZ() << ", " << actual.getW() << ") is not ("
            << expected.getX() << ", " << expected.getY() << ", " << expected.getZ() << ", " << expected.getW() << ")";
    }

    //==============================================================================
    /** What places the entity of a node: the identity when it has no TransformNode. */
    static Vector3<float> positionOf (const SceneNodeComponent& node)
    {
        const auto* transform = node.getEntity().get<TransformNode>();
        return transform != nullptr ? transform->getPosition() : Vector3<float>();
    }

    static Quaternion rotationOf (const SceneNodeComponent& node)
    {
        const auto* transform = node.getEntity().get<TransformNode>();
        return transform != nullptr ? transform->getRotation() : Quaternion();
    }

    static Vector3<float> scaleOf (const SceneNodeComponent& node)
    {
        const auto* transform = node.getEntity().get<TransformNode>();
        return transform != nullptr ? transform->getScale() : Vector3<float> (1.0f, 1.0f, 1.0f);
    }

    /** The linear value of an sRGB encoded one, as the colors of the interface are given to the scene. */
    static float toLinear (float encoded)
    {
        return encoded <= 0.04045f ? encoded / 12.92f : std::pow ((encoded + 0.055f) / 1.055f, 2.4f);
    }

    //==============================================================================
    /** Serves the files of the fixture to a document, counting how many times each one is read. */
    void serve (YuiDocument& document)
    {
        document.setAssetReader ([this] (const String& path) -> std::optional<MemoryBlock>
        {
            const auto found = files.find (path);
            if (found == files.end())
                return std::nullopt;

            ++reads[path];
            return found->second;
        });
    }

    static MemoryBlock bytesOf (const String& text)
    {
        return MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    /** A 1x1 opaque red PNG image. */
    static MemoryBlock redPng()
    {
        MemoryOutputStream out;
        Base64::convertFromBase64 (out, "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGP4z8DwHwAFAAH/iZk9HQAAAABJRU5ErkJggg==");
        return out.getMemoryBlock();
    }

    /** A glTF model holding everything as data URIs: the scene Main with Body and its child Knob,
        which draws a triangle at the given place, then Lid and its child, named Knob too.
    */
    static MemoryBlock modelFile (const String& knobTranslation = "1, 2, 3")
    {
        const float positions[] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

        return bytesOf (String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Main", "nodes": [ 0, 2 ] } ],
            "nodes": [ { "name": "Body", "children": [ 1 ] },
                       { "name": "Knob", "mesh": 0, "translation": [ $TRANSLATION ] },
                       { "name": "Lid", "children": [ 3 ] },
                       { "name": "Knob", "translation": [ 0, 5, 0 ] } ],
            "meshes": [ { "name": "KnobMesh", "primitives": [ { "attributes": { "POSITION": 0 } } ] } ],
            "buffers": [ { "uri": "data:application/octet-stream;base64,$POSITIONS", "byteLength": 36 } ],
            "bufferViews": [ { "buffer": 0, "byteLength": 36 } ],
            "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" } ]
        })")
                            .replace ("$TRANSLATION", knobTranslation)
                            .replace ("$POSITIONS", Base64::toBase64 (positions, sizeof (positions))));
    }

    /** The geometry of a quad facing +z, as the properties of a Mesh3D. */
    static String quadGeometry()
    {
        return "vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0]; indices: [0, 1, 2,  0, 2, 3];";
    }

    /** The same quad with texture coordinates: 0, 0 at its top left corner and 1, 1 at its bottom right one. */
    static String texturedQuadGeometry()
    {
        return quadGeometry() + " uvs: [0, 1,  1, 1,  1, 0,  0, 0];";
    }

    //==============================================================================
    /** A camera looking down -z from 4 away, which shows 4 units of the scene in each direction of a
        square view whatever their distance: with showScene(), 50 pixels for each unit, the origin in
        the middle at 100, 100, x to the right and y up.
    */
    static String orthographicCamera()
    {
        return "Camera3D { position: [0, 0, 4]; projection: orthographic; orthographicSize: 2; }";
    }

    /** Makes a scene the one a window would show: visible, 200 pixels wide and high. */
    static void showScene (SceneComponent& view)
    {
        view.setVisible (true);
        view.setBounds (0.0f, 0.0f, 200.0f, 200.0f);
    }

    /** A document whose root is the scene `view`, seen through orthographicCamera(), with what is written inside it. */
    static String sceneOf (const String& content)
    {
        return "Scene3D { id: view; " + orthographicCamera() + " " + content + " }";
    }

    /** Returns the scene `view` of a document, shown as showScene() shows it, or nullptr when it has none. */
    static SceneComponent* showSceneOf (const YuiDocument& document)
    {
        auto* view = document.findById<SceneComponent> ("view");

        if (view != nullptr)
            showScene (*view);

        return view;
    }

    static void expectPoint (const std::optional<Point<float>>& actual, float x, float y, float tolerance = 1.0e-2f)
    {
        ASSERT_TRUE (actual.has_value());
        EXPECT_NEAR (actual->getX(), x, tolerance);
        EXPECT_NEAR (actual->getY(), y, tolerance);
    }

    //==============================================================================
    /** One buffer of a glTF file, with its bytes as a data URI. */
    template <class Value>
    static String bufferOf (std::initializer_list<Value> values)
    {
        const std::vector<Value> data (values);
        const auto numBytes = data.size() * sizeof (Value);

        return R"({ "uri": "data:application/octet-stream;base64,)" + Base64::toBase64 (data.data(), numBytes)
             + R"(", "byteLength": )" + String (static_cast<int> (numBytes)) + " }";
    }

    /** A glTF model holding everything as data URIs: the scene Device with Body and its children
        Knob and Screen.

        Body is a square facing +z, 2 wide around the origin, with a corner of 0.25 raised by 1 at
        its bottom left: its bounds enclose its children. Knob and Screen are squares 0.5 wide
        sharing one mesh, with texture coordinates from 0, 0 at their top left to 1, 1 at their
        bottom right: Knob 0.5 in front of Body, at its top right, and Screen 0.25 in front of it,
        at its top left.
    */
    static MemoryBlock deviceFile()
    {
        return bytesOf (String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Device", "nodes": [ 0 ] } ],
            "nodes": [ { "name": "Body", "mesh": 0, "children": [ 1, 2 ] },
                       { "name": "Knob", "mesh": 1, "translation": [ 0.5, 0.5, 0.5 ] },
                       { "name": "Screen", "mesh": 1, "translation": [ -0.5, 0.5, 0.25 ] } ],
            "meshes": [ { "name": "BodyMesh", "primitives": [ { "attributes": { "POSITION": 0 } } ] },
                        { "name": "PlateMesh", "primitives": [ { "attributes": { "POSITION": 1, "TEXCOORD_0": 2 } } ] } ],
            "buffers": [ $BODY, $PLATE, $UVS ],
            "bufferViews": [ { "buffer": 0, "byteLength": 144 }, { "buffer": 1, "byteLength": 72 }, { "buffer": 2, "byteLength": 48 } ],
            "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 12, "type": "VEC3" },
                           { "bufferView": 1, "componentType": 5126, "count": 6, "type": "VEC3" },
                           { "bufferView": 2, "componentType": 5126, "count": 6, "type": "VEC2" } ]
        })")
                            .replace ("$BODY", bufferOf<float> ({ -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                                                                   -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
                                                                   -1.0f, -1.0f, 1.0f, -0.75f, -1.0f, 1.0f, -0.75f, -0.75f, 1.0f,
                                                                   -1.0f, -1.0f, 1.0f, -0.75f, -0.75f, 1.0f, -1.0f, -0.75f, 1.0f }))
                            .replace ("$PLATE", bufferOf<float> ({ -0.25f, -0.25f, 0.0f, 0.25f, -0.25f, 0.0f, 0.25f, 0.25f, 0.0f,
                                                                    -0.25f, -0.25f, 0.0f, 0.25f, 0.25f, 0.0f, -0.25f, 0.25f, 0.0f }))
                            .replace ("$UVS", bufferOf<float> ({ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f })));
    }

    /** A glTF model of a strip facing +z, 1 wide and 2 high, skinned to two joints: its foot follows
        Root, at the origin, and its top Tip, the child of Root 1 up, where the skin has it bound.
        The four vertices are its bottom left and right, then its top left and right.
    */
    static MemoryBlock skinnedFile()
    {
        return bytesOf (String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Main", "nodes": [ 0, 1 ] } ],
            "nodes": [ { "name": "Strip", "mesh": 0, "skin": 0 }, { "name": "Root", "children": [ 2 ] }, { "name": "Tip", "translation": [ 0, 1, 0 ] } ],
            "skins": [ { "name": "Bend", "joints": [ 1, 2 ], "inverseBindMatrices": 4 } ],
            "meshes": [ { "name": "StripMesh", "primitives": [ { "attributes": { "POSITION": 0, "NORMAL": 1, "JOINTS_0": 2, "WEIGHTS_0": 3 }, "indices": 5 } ] } ],
            "buffers": [ $POSITIONS, $NORMALS, $JOINTS, $WEIGHTS, $MATRICES, $INDICES ],
            "bufferViews": [ { "buffer": 0, "byteLength": 48 }, { "buffer": 1, "byteLength": 48 }, { "buffer": 2, "byteLength": 16 },
                             { "buffer": 3, "byteLength": 64 }, { "buffer": 4, "byteLength": 128 }, { "buffer": 5, "byteLength": 12 } ],
            "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3" },
                           { "bufferView": 1, "componentType": 5126, "count": 4, "type": "VEC3" },
                           { "bufferView": 2, "componentType": 5121, "count": 4, "type": "VEC4" },
                           { "bufferView": 3, "componentType": 5126, "count": 4, "type": "VEC4" },
                           { "bufferView": 4, "componentType": 5126, "count": 2, "type": "MAT4" },
                           { "bufferView": 5, "componentType": 5123, "count": 6, "type": "SCALAR" } ]
        })")
                            .replace ("$POSITIONS", bufferOf<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 1.0f, 2.0f, 0.0f }))
                            .replace ("$NORMALS", bufferOf<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }))
                            .replace ("$JOINTS", bufferOf<uint8> ({ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 }))
                            .replace ("$WEIGHTS", bufferOf<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }))
                            .replace ("$MATRICES", bufferOf<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                                                       1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f }))
                            .replace ("$INDICES", bufferOf<uint16> ({ 0, 1, 2, 1, 3, 2 })));
    }

    //==============================================================================
    /** Hears the problems a document is told about after a load, when they are told. */
    struct DiagnosticListener final : YuiDocument::Listener
    {
        DiagnosticListener (YuiDocument& documentToHear, std::function<void (const YuiDiagnostic&)> functionToCall)
            : document (documentToHear)
            , function (std::move (functionToCall))
        {
            document.addListener (this);
        }

        ~DiagnosticListener() override
        {
            document.removeListener (this);
        }

        void documentLoaded (YuiDocument&, const Result&) override {}

        void documentDiagnosed (YuiDocument&, const YuiDiagnostic& diagnostic) override
        {
            function (diagnostic);
        }

        YuiDocument& document;
        std::function<void (const YuiDiagnostic&)> function;
    };

    /** Records the colors a component fills areas with when it paints. */
    class FillRecorder final : public rive::Renderer
    {
    public:
        using rive::Renderer::drawImage;
        using rive::Renderer::drawImageMesh;

        void save() override {}

        void restore() override {}

        void transform (const rive::Mat2D&) override {}

        void drawPath (rive::RenderPath*, rive::RenderPaint* paint) override
        {
            colors.push_back (Color (static_cast<rive::RiveRenderPaint*> (paint)->getColor()));
        }

        void clipPath (rive::RenderPath*) override {}

        void drawImage (const rive::RenderImage*, rive::ImageSampler, rive::BlendMode, float) override {}

        void drawImageMesh (const rive::RenderImage*,
                            rive::ImageSampler,
                            rive::rcp<rive::RenderBuffer>,
                            rive::rcp<rive::RenderBuffer>,
                            rive::rcp<rive::RenderBuffer>,
                            uint32_t,
                            uint32_t,
                            rive::BlendMode,
                            float) override {}

        void modulateOpacity (float) override {}

        std::vector<Color> colors;
    };

    /** Paints a component without a GPU, and returns the colors it filled areas with, in order. */
    static std::vector<Color> paintWithoutGpu (Component& component)
    {
        auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        if (context == nullptr)
            return {};

        FillRecorder recorder;
        Graphics g (*context, recorder);
        g.setDrawingArea (component.getLocalBounds());
        component.paint (g);

        return recorder.colors;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    std::map<String, MemoryBlock> files;
    std::map<String, int> reads;
};
