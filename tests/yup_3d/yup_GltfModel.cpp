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

#include <gtest/gtest.h>

#include <yup_3d/yup_3d.h>

using namespace yup;

class GltfModelTests : public ::testing::Test
{
protected:
    // 1x1 opaque red and 2x2 opaque green PNG images
    static constexpr const char* redPngBase64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGP4z8DwHwAFAAH/iZk9HQAAAABJRU5ErkJggg==";
    static constexpr const char* greenPngBase64 = "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAADklEQVR4nGNg+A+FMAYAQ84H+fei4u8AAAAASUVORK5CYII=";

    static File getDataDirectory()
    {
        return File (__FILE__)
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("data")
            .getChildFile ("gltf");
    }

    static MemoryBlock fromBase64 (const char* base64)
    {
        MemoryOutputStream out;
        Base64::convertFromBase64 (out, base64);
        return out.getMemoryBlock();
    }

    template <class T>
    static MemoryBlock toBlock (std::initializer_list<T> values)
    {
        return MemoryBlock (values.begin(), values.size() * sizeof (T));
    }

    static String toDataUri (const MemoryBlock& block, const String& mimeType = "application/octet-stream")
    {
        return "data:" + mimeType + ";base64," + Base64::toBase64 (block.getData(), block.getSize());
    }

    static String positionsUri()
    {
        return toDataUri (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }));
    }

    static ResultValue<GltfModel> loadJson (const String& json, const File& baseDirectory = {})
    {
        return GltfModel::loadFromData (Span<const uint8> (reinterpret_cast<const uint8*> (json.toRawUTF8()), json.getNumBytesAsUTF8()), baseDirectory);
    }

    static MemoryBlock makeGlb (const String& json, const MemoryBlock& bin)
    {
        MemoryBlock jsonChunk (json.toRawUTF8(), json.getNumBytesAsUTF8());
        while (jsonChunk.getSize() % 4 != 0)
            jsonChunk.append (" ", 1);

        MemoryBlock binChunk (bin);
        while (binChunk.getSize() % 4 != 0)
        {
            const char zero = 0;
            binChunk.append (&zero, 1);
        }

        const auto totalSize = 12 + 8 + jsonChunk.getSize() + (bin.isEmpty() ? 0 : 8 + binChunk.getSize());

        MemoryOutputStream out;
        out.writeInt (0x46546c67); // "glTF"
        out.writeInt (2);
        out.writeInt (static_cast<int> (totalSize));
        out.writeInt (static_cast<int> (jsonChunk.getSize()));
        out.writeInt (0x4e4f534a); // "JSON"
        out.write (jsonChunk.getData(), jsonChunk.getSize());

        if (! bin.isEmpty())
        {
            out.writeInt (static_cast<int> (binChunk.getSize()));
            out.writeInt (0x004e4942); // "BIN"
            out.write (binChunk.getData(), binChunk.getSize());
        }

        return out.getMemoryBlock();
    }

    /** A single triangle mesh on a node, with the given extra JSON members spliced into the root. */
    static String triangleJson (const String& primitiveExtras = {}, const String& rootExtras = {})
    {
        return String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Main", "nodes": [ 0 ] } ],
            "nodes": [ { "name": "Triangle", "mesh": 0 } ],
            "meshes": [ { "name": "TriangleMesh", "primitives": [ { "attributes": { "POSITION": 0 } $PRIMITIVE } ] } ],
            "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 } ],
            "bufferViews": [ { "buffer": 0, "byteLength": 36 } ],
            "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" } ]
            $ROOT
        })")
            .replace ("$POSITIONS", positionsUri())
            .replace ("$PRIMITIVE", primitiveExtras)
            .replace ("$ROOT", rootExtras);
    }

    /** A triangle whose material base color texture is texture 0, with the given textures and images. */
    static String texturedJson (const String& texturesAndImages, const String& materialExtras = {})
    {
        return triangleJson (R"(, "material": 0)",
                             R"(, "materials": [ { "pbrMetallicRoughness": { "baseColorTexture": { "index": 0 } } $MATERIAL } ], )"
                                 + texturesAndImages)
            .replace ("$MATERIAL", materialExtras);
    }

    /** Two animations of the nodes of animatedJson(): "Wave" moves Hand along a line and turns Arm in
        a step, and one without a name scales Spare along a cubic spline.
    */
    static String waveAnimationsJson()
    {
        return R"({ "name": "Wave",
                    "channels": [ { "sampler": 0, "target": { "node": 1, "path": "translation" } },
                                  { "sampler": 1, "target": { "node": 0, "path": "rotation" } } ],
                    "samplers": [ { "input": 0, "output": 1 },
                                  { "input": 2, "output": 3, "interpolation": "STEP" } ] },
                  { "channels": [ { "sampler": 0, "target": { "node": 2, "path": "scale" } } ],
                    "samplers": [ { "input": 2, "output": 4, "interpolation": "CUBICSPLINE" } ] })";
    }

    /** A scene holding Arm and its child Hand, another one holding Spare, the given animations and
        the accessors they pick from: the times 0, 1, 2 (0) of three translations (1), and the
        times 0, 2 (2) of two rotations (3) or of two scales with their tangents (4).
    */
    static String animatedJson (const String& animations = waveAnimationsJson())
    {
        return String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Main", "nodes": [ 0 ] }, { "name": "Extra", "nodes": [ 2 ] } ],
            "nodes": [ { "name": "Arm", "children": [ 1 ] }, { "name": "Hand", "translation": [ 1, 0, 0 ] }, { "name": "Spare" } ],
            "animations": [ $ANIMATIONS ],
            "buffers": [ { "uri": "$THREETIMES", "byteLength": 12 }, { "uri": "$TRANSLATIONS", "byteLength": 36 },
                         { "uri": "$TWOTIMES", "byteLength": 8 }, { "uri": "$ROTATIONS", "byteLength": 32 }, { "uri": "$SCALES", "byteLength": 72 } ],
            "bufferViews": [ { "buffer": 0, "byteLength": 12 }, { "buffer": 1, "byteLength": 36 },
                             { "buffer": 2, "byteLength": 8 }, { "buffer": 3, "byteLength": 32 }, { "buffer": 4, "byteLength": 72 } ],
            "accessors": [
                { "bufferView": 0, "componentType": 5126, "count": 3, "type": "SCALAR" },
                { "bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3" },
                { "bufferView": 2, "componentType": 5126, "count": 2, "type": "SCALAR" },
                { "bufferView": 3, "componentType": 5126, "count": 2, "type": "VEC4" },
                { "bufferView": 4, "componentType": 5126, "count": 6, "type": "VEC3" }
            ]
        })")
            .replace ("$ANIMATIONS", animations)
            .replace ("$THREETIMES", toDataUri (toBlock<float> ({ 0.0f, 1.0f, 2.0f })))
            .replace ("$TRANSLATIONS", toDataUri (toBlock<float> ({ 1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0.0f, 1.0f, 2.0f, 3.0f })))
            .replace ("$TWOTIMES", toDataUri (toBlock<float> ({ 0.0f, 2.0f })))
            .replace ("$ROTATIONS", toDataUri (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.70710678f, 0.70710678f })))
            .replace ("$SCALES", toDataUri (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f, 0.0f, 0.0f, 0.0f })));
    }

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    /** The buffers, buffer views and accessors of an asset: one buffer and one view for each block of
        data, and the accessors in the order they are added.
    */
    struct Accessors
    {
        /** Adds a buffer and its view, and returns the index of the view. */
        int addView (const MemoryBlock& block)
        {
            const auto size = String (static_cast<int> (block.getSize()));

            views.add (R"({ "buffer": )" + String (buffers.size()) + R"(, "byteLength": )" + size + " }");
            buffers.add (R"({ "uri": ")" + toDataUri (block) + R"(", "byteLength": )" + size + " }");
            return views.size() - 1;
        }

        /** Adds an accessor over a new view of the block, and returns its index. */
        int add (const MemoryBlock& block, int componentType, const String& type, int count, const String& extras = {})
        {
            return addWithoutView (componentType, type, count, R"(, "bufferView": )" + String (addView (block)) + extras);
        }

        /** Adds an accessor without a view, which reads as zeros, and returns its index. */
        int addWithoutView (int componentType, const String& type, int count, const String& extras = {})
        {
            accessors.add (R"({ "componentType": )" + String (componentType) + R"(, "count": )" + String (count) + R"(, "type": ")" + type + "\"" + extras + " }");
            return accessors.size() - 1;
        }

        /** The three members of the root of an asset. */
        String toJson() const
        {
            return R"("buffers": [ )" + buffers.joinIntoString (", ") + R"( ], "bufferViews": [ )" + views.joinIntoString (", ") + R"( ], "accessors": [ )" + accessors.joinIntoString (", ") + " ]";
        }

        StringArray buffers, views, accessors;
    };

    /** A strip standing in the plane that faces +z, 1 wide and 2 high, skinned to two joints: its foot
        follows Root, at the origin, and its top Tip, the child of Root 1 up, where the skin has it bound.
        A test changes the part it is about before asking for the JSON.

        The accessors are the positions (0), the normals (1), the joints (2) and the weights (3) of the
        four vertices, the inverse bind matrices (4), the indices (5), and the keys of a rotation: the
        times 0 and 1 (6) of no turn and of a quarter turn about x (7).
    */
    struct SkinnedStrip
    {
        String toJson() const
        {
            Accessors accessors;
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 1.0f, 2.0f, 0.0f }), 5126, "VEC3", 4);
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }), 5126, "VEC3", 4);
            accessors.add (joints, jointsType, "VEC4", 4);
            accessors.add (weights, weightsType, "VEC4", 4, weightsNormalized ? R"(, "normalized": true)" : "");
            accessors.add (inverseBindMatrices, 5126, "MAT4", numInverseBindMatrices);
            accessors.add (toBlock<uint16> ({ 0, 1, 2, 1, 3, 2 }), 5123, "SCALAR", 6);
            accessors.add (toBlock<float> ({ 0.0f, 1.0f }), 5126, "SCALAR", 2);
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.70710678f, 0.0f, 0.0f, 0.70710678f }), 5126, "VEC4", 2);

            return String (R"({
                "asset": { "version": "2.0" },
                "scene": 0,
                "scenes": [ { "name": "Main", "nodes": [ $SCENE ] } ],
                "nodes": [ { "name": "Strip", "mesh": 0, "skin": 0 }, { "name": "Root", "children": [ 2 ] }, { "name": "Tip", "translation": [ 0, 1, 0 ] } ],
                "skins": [ $SKIN ],
                "meshes": [ { "name": "StripMesh", "primitives": [ { "attributes": { $ATTRIBUTES }, "indices": 5 } ] } ],
                "animations": [ { "name": "Bend",
                                  "channels": [ { "sampler": 0, "target": { "node": 2, "path": "rotation" } } ],
                                  "samplers": [ { "input": 6, "output": 7 } ] } ],
                $ACCESSORS
            })")
                .replace ("$SCENE", sceneNodes)
                .replace ("$SKIN", skin)
                .replace ("$ATTRIBUTES", attributes)
                .replace ("$ACCESSORS", accessors.toJson());
        }

        MemoryBlock joints = toBlock<uint8> ({ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 });
        int jointsType = 5121;
        MemoryBlock weights = toBlock<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f });
        int weightsType = 5126;
        bool weightsNormalized = false;
        MemoryBlock inverseBindMatrices = toBlock<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f });
        int numInverseBindMatrices = 2;
        String attributes = R"("POSITION": 0, "NORMAL": 1, "JOINTS_0": 2, "WEIGHTS_0": 3)";
        String skin = R"({ "name": "Bend", "joints": [ 1, 2 ], "inverseBindMatrices": 4 })";
        String sceneNodes = "0, 1";
    };

    /** The triangle of positionsUri(), facing +z, with two morph targets: the first lifts its last vertex
        by 1 and leans the normal there from z to y, the second moves its second vertex by 2 along x.
        A test changes the part it is about before asking for the JSON.

        The accessors are the positions (0) and the normals (1) of the three vertices, the position (2)
        and normal (3) deltas of the first target, the position deltas of the second (4), which are
        sparse, the times 0 and 2 (5) and the two weights of each of their keys (6): 0 and 0, then 1
        and 0.5. The accessors a test adds come next.
    */
    struct MorphedTriangle
    {
        String toJson() const
        {
            Accessors accessors;
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }), 5126, "VEC3", 3);
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }), 5126, "VEC3", 3);
            accessors.add (lift, 5126, "VEC3", 3);
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f }), 5126, "VEC3", 3);

            // Only the second of three elements is stored: the others are the zeros of an accessor without a view
            const auto sparseIndices = accessors.addView (toBlock<uint16> ({ 1 }));
            const auto sparseValues = accessors.addView (toBlock<float> ({ 2.0f, 0.0f, 0.0f }));
            accessors.addWithoutView (5126, "VEC3", 3, R"(, "sparse": { "count": 1, "indices": { "bufferView": )" + String (sparseIndices) + R"(, "componentType": 5123 }, "values": { "bufferView": )" + String (sparseValues) + " } }");

            accessors.add (toBlock<float> ({ 0.0f, 2.0f }), 5126, "SCALAR", 2);
            accessors.add (toBlock<float> ({ 0.0f, 0.0f, 1.0f, 0.5f }), 5126, "SCALAR", 4);

            for (const auto& accessor : extraAccessors)
                accessors.accessors.add (accessor);

            return String (R"({
                "asset": { "version": "2.0" },
                "scene": 0,
                "scenes": [ { "name": "Main", "nodes": [ 0, 1 ] } ],
                "nodes": [ { "name": "Face", "mesh": 0 $NODE }, { "name": "Twin", "mesh": 0, "translation": [ 5, 0, 0 ] } ],
                "meshes": [ { "name": "FaceMesh", "primitives": [ { "attributes": { $ATTRIBUTES }, "targets": [ $TARGETS ] } ] $MESH } ],
                "animations": [ { "name": "Smile", "channels": [ $CHANNELS ], "samplers": [ $SAMPLERS ] } ],
                $ACCESSORS
            })")
                .replace ("$NODE", node)
                .replace ("$ATTRIBUTES", attributes)
                .replace ("$TARGETS", targets)
                .replace ("$MESH", mesh)
                .replace ("$CHANNELS", channels)
                .replace ("$SAMPLERS", samplers)
                .replace ("$ACCESSORS", accessors.toJson());
        }

        MemoryBlock lift = toBlock<float> ({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f });
        String attributes = R"("POSITION": 0, "NORMAL": 1)";
        String targets = R"({ "POSITION": 2, "NORMAL": 3 }, { "POSITION": 4 })";
        String mesh = R"(, "weights": [ 0.5, 0.25 ])";
        String node;
        String channels = R"({ "sampler": 0, "target": { "node": 0, "path": "weights" } })";
        String samplers = R"({ "input": 5, "output": 6 })";
        StringArray extraAccessors;
    };

    /** The vertices an entity of an instance draws, deformed or not. */
    static const std::vector<Mesh::Vertex>& getDrawnVertices (const Entity& entity)
    {
        return entity.get<MeshNode>()->mesh->getPrimitive (0).vertices;
    }

    Scene scene;
};

//==============================================================================

TEST_F (GltfModelTests, LoadsMinimalGltfWithDataUriBuffer)
{
    auto result = loadJson (triangleJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_EQ (model.getNumScenes(), 1);
    ASSERT_EQ (model.getMeshes().size(), 1u);
    EXPECT_EQ (model.getMeshes()[0]->getName(), "TriangleMesh");
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");

    const auto& mesh = *model.getMeshes()[0];
    ASSERT_EQ (mesh.getNumPrimitives(), 1);

    // No indices and no normals: drawn in order, with generated flat normals
    const auto& primitive = mesh.getPrimitive (0);
    EXPECT_EQ (primitive.indices, (std::vector<uint32> { 0, 1, 2 }));
    expectNear (primitive.vertices[1].position, { 1.0f, 0.0f, 0.0f });
    expectNear (primitive.vertices[0].normal, { 0.0f, 0.0f, 1.0f });
    EXPECT_EQ (primitive.material, nullptr);

    auto root = model.instantiate (scene);
    ASSERT_TRUE (root.isValid());
    EXPECT_EQ (root.getName(), "Main");
    ASSERT_EQ (root.getNumChildren(), 1);

    auto triangle = root.getChild (0);
    EXPECT_EQ (triangle.getName(), "Triangle");
    ASSERT_NE (triangle.get<MeshNode>(), nullptr);
    EXPECT_EQ (triangle.get<MeshNode>()->mesh, model.getMeshes()[0]);
}

TEST_F (GltfModelTests, LoadsExternalBufferAndImage)
{
    auto result = GltfModel::loadFromFile (getDataDirectory().getChildFile ("external/triangle.gltf"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    ASSERT_EQ (model.getMeshes().size(), 1u);

    const auto& primitive = model.getMeshes()[0]->getPrimitive (0);
    EXPECT_EQ (primitive.indices, (std::vector<uint32> { 0, 1, 2 }));
    EXPECT_NEAR (primitive.vertices[2].uv.getY(), 0.0f, 1.0e-6f);
    EXPECT_NEAR (primitive.vertices[0].uv.getY(), 1.0f, 1.0e-6f);

    ASSERT_EQ (model.getMaterials().size(), 1u);
    const auto& material = *model.getMaterials()[0];
    EXPECT_EQ (material.name, "Checker");
    ASSERT_NE (material.baseColorTexture, nullptr);
    EXPECT_EQ (material.baseColorTexture->getImage().getWidth(), 4);
    EXPECT_TRUE (material.baseColorTexture->isSrgb());

    // Textures without a sampler repeat and filter linearly with mipmaps, anisotropically
    const auto& sampler = material.baseColorTexture->getSamplerDesc();
    EXPECT_EQ (sampler.maxAnisotropy, 16u);
    EXPECT_EQ (sampler.wrapU, GpuWrapMode::repeat);
    EXPECT_EQ (sampler.wrapV, GpuWrapMode::repeat);
    EXPECT_EQ (sampler.minFilter, GpuFilter::linear);
    EXPECT_EQ (sampler.magFilter, GpuFilter::linear);
    EXPECT_EQ (sampler.mipmapFilter, GpuFilter::linear);

    EXPECT_EQ (model.getTextures().size(), 1u);
}

TEST_F (GltfModelTests, LoadsGlbWithImageInBufferView)
{
    const auto positions = toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f });
    const auto indices = toBlock<uint16> ({ 2, 1, 0, 0 });
    const auto png = fromBase64 (redPngBase64);

    MemoryBlock bin;
    bin.append (positions.getData(), positions.getSize());
    bin.append (indices.getData(), indices.getSize());
    bin.append (png.getData(), png.getSize());

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 }, "indices": 1, "material": 0 } ] } ],
        "materials": [ { "pbrMetallicRoughness": { "baseColorTexture": { "index": 0 } } } ],
        "textures": [ { "source": 0 } ],
        "images": [ { "bufferView": 2, "mimeType": "image/png" } ],
        "buffers": [ { "byteLength": $BINSIZE } ],
        "bufferViews": [
            { "buffer": 0, "byteOffset": 0, "byteLength": 36 },
            { "buffer": 0, "byteOffset": 36, "byteLength": 6 },
            { "buffer": 0, "byteOffset": 44, "byteLength": $PNGSIZE }
        ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
            { "bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR" }
        ]
    })")
                          .replace ("$BINSIZE", String (bin.getSize()))
                          .replace ("$PNGSIZE", String (png.getSize()));

    const auto glb = makeGlb (json, bin);
    auto result = GltfModel::loadFromData (glb.asBytes());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    ASSERT_EQ (model.getMeshes().size(), 1u);

    // Without normals the triangle gets flat normals and its own vertices, in the order of the
    // 16-bit indices read from the buffer view: 2, 1, 0
    const auto& primitive = model.getMeshes()[0]->getPrimitive (0);
    EXPECT_EQ (primitive.indices, (std::vector<uint32> { 0, 1, 2 }));
    ASSERT_EQ (primitive.vertices.size(), 3u);
    EXPECT_EQ (primitive.vertices[0].position, Vector3<float> (0.0f, 1.0f, 0.0f));
    EXPECT_EQ (primitive.vertices[1].position, Vector3<float> (1.0f, 0.0f, 0.0f));
    EXPECT_EQ (primitive.vertices[2].position, Vector3<float> (0.0f, 0.0f, 0.0f));

    // A scene without a default scene index: the first scene is built
    EXPECT_TRUE (model.instantiate (scene).isValid());

    ASSERT_EQ (model.getMaterials().size(), 1u);
    ASSERT_NE (model.getMaterials()[0]->baseColorTexture, nullptr);

    const auto& image = model.getMaterials()[0]->baseColorTexture->getImage();
    EXPECT_EQ (image.getWidth(), 1);
    EXPECT_EQ (image.getPixel (0, 0), 0xffff0000u);
}

TEST_F (GltfModelTests, LoadsImageFromDataUri)
{
    auto result = loadJson (texturedJson (String (R"("textures": [ { "source": 0 } ], "images": [ { "uri": "$IMAGE" } ])")
                                              .replace ("$IMAGE", toDataUri (fromBase64 (greenPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& texture = result.getReference().getMaterials()[0]->baseColorTexture;
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->getImage().getWidth(), 2);
    EXPECT_EQ (texture->getImage().getPixel (1, 1), 0xff00ff00u);
}

TEST_F (GltfModelTests, ExtTextureWebpSourceIsPreferred)
{
    // The extension source is a PNG here: decoding goes by content, the choice of source is what's tested
    auto result = loadJson (texturedJson (String (R"("textures": [ { "source": 0, "extensions": { "EXT_texture_webp": { "source": 1 } } } ],
                                                     "images": [ { "uri": "$RED" }, { "uri": "$GREEN" } ])")
                                              .replace ("$RED", toDataUri (fromBase64 (redPngBase64), "image/png"))
                                              .replace ("$GREEN", toDataUri (fromBase64 (greenPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& texture = result.getReference().getMaterials()[0]->baseColorTexture;
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->getImage().getWidth(), 2);
}

TEST_F (GltfModelTests, ExtTextureWebpFallsBackToSourceWhenUndecodable)
{
    auto result = loadJson (texturedJson (String (R"("textures": [ { "source": 0, "extensions": { "EXT_texture_webp": { "source": 1 } } } ],
                                                     "images": [ { "uri": "$RED" }, { "uri": "data:image/webp;base64,AAAAAAAA" } ])")
                                              .replace ("$RED", toDataUri (fromBase64 (redPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto& texture = model.getMaterials()[0]->baseColorTexture;
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->getImage().getWidth(), 1);
    EXPECT_FALSE (model.getWarnings().isEmpty());
}

TEST_F (GltfModelTests, ReadsNormalizedUnsignedByteColors)
{
    const auto colors = toBlock<uint8> ({ 255, 0, 0, 128, 0, 255, 0, 255, 0, 0, 51, 0 });

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "COLOR_0": 1 } } ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 }, { "uri": "$COLORS", "byteLength": 12 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 12 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
            { "bufferView": 1, "componentType": 5121, "normalized": true, "count": 3, "type": "VEC4" }
        ]
    })")
                          .replace ("$POSITIONS", positionsUri())
                          .replace ("$COLORS", toDataUri (colors));

    auto colored = loadJson (json);
    ASSERT_TRUE (colored.wasOk()) << colored.getErrorMessage();

    const auto& vertices = colored.getReference().getMeshes()[0]->getPrimitive (0).vertices;
    ASSERT_EQ (vertices.size(), 3u);
    EXPECT_FLOAT_EQ (vertices[0].color[0], 1.0f);
    EXPECT_FLOAT_EQ (vertices[0].color[1], 0.0f);
    EXPECT_NEAR (vertices[0].color[3], 128.0f / 255.0f, 1.0e-6f);
    EXPECT_FLOAT_EQ (vertices[1].color[1], 1.0f);
    EXPECT_NEAR (vertices[2].color[2], 0.2f, 1.0e-6f);
}

TEST_F (GltfModelTests, ColorsWithThreeComponentsAreOpaque)
{
    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "COLOR_0": 1 } } ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 }, { "uri": "$COLORS", "byteLength": 36 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 36 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
            { "bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3" }
        ]
    })")
                          .replace ("$POSITIONS", positionsUri())
                          .replace ("$COLORS", toDataUri (toBlock<float> ({ 0.5f, 0.25f, 0.0f, 0.5f, 0.25f, 0.0f, 0.5f, 0.25f, 0.0f })));

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& vertex = result.getReference().getMeshes()[0]->getPrimitive (0).vertices[0];
    EXPECT_FLOAT_EQ (vertex.color[0], 0.5f);
    EXPECT_FLOAT_EQ (vertex.color[1], 0.25f);
    EXPECT_FLOAT_EQ (vertex.color[3], 1.0f);
}

TEST_F (GltfModelTests, ReadsTangents)
{
    const auto normals = toDataUri (toBlock<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }));
    const auto tangents = toDataUri (toBlock<float> ({ 1.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.5f }));

    auto result = loadJson (String (R"({
        "asset": { "version": "2.0" },
        "scene": 0,
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "NORMAL": 1, "TANGENT": 2 } } ] } ],
        "buffers": [ { "uri": "$P", "byteLength": 36 }, { "uri": "$N", "byteLength": 36 }, { "uri": "$T", "byteLength": 48 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 36 }, { "buffer": 2, "byteLength": 48 } ],
        "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
                       { "bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3" },
                       { "bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC4" } ]
    })")
                                .replace ("$P", positionsUri())
                                .replace ("$N", normals)
                                .replace ("$T", tangents));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& vertices = result.getReference().getMeshes()[0]->getPrimitive (0).vertices;
    ASSERT_EQ (vertices.size(), 3u);
    EXPECT_EQ (vertices[0].tangent, (std::array<float, 4> { 1.0f, 0.0f, 0.0f, -1.0f }));

    // The direction is normalized, and the handedness is forced to one sign or the other
    EXPECT_FLOAT_EQ (vertices[1].tangent[0], 1.0f);
    EXPECT_FLOAT_EQ (vertices[1].tangent[3], -1.0f);
    EXPECT_FLOAT_EQ (vertices[2].tangent[3], 1.0f);
}

TEST_F (GltfModelTests, TangentWithoutNormalsIsIgnored)
{
    // glTF: when normals are not specified, tangents shall be ignored. These are malformed too (three
    // components): reading them would warn
    const auto tangents = toDataUri (toBlock<float> ({ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }));

    auto result = loadJson (String (R"({
        "asset": { "version": "2.0" },
        "scene": 0,
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "TANGENT": 1 } } ] } ],
        "buffers": [ { "uri": "$P", "byteLength": 36 }, { "uri": "$T", "byteLength": 36 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 36 } ],
        "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
                       { "bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3" } ]
    })")
                                .replace ("$P", positionsUri())
                                .replace ("$T", tangents));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto warnings = result.getReference().getWarnings().joinIntoString ("\n");
    EXPECT_FALSE (warnings.contains ("TANGENT")) << warnings;

    // Flat normals are generated, and generated frames drop authored tangents
    for (const auto& vertex : result.getReference().getMeshes()[0]->getPrimitive (0).vertices)
        EXPECT_EQ (vertex.tangent[3], 0.0f);
}

TEST_F (GltfModelTests, DegenerateTangentsAreDropped)
{
    const auto normals = toDataUri (toBlock<float> ({ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }));
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto inf = std::numeric_limits<float>::infinity();
    const auto tangents = toDataUri (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, nan, 0.0f, 0.0f, 1.0f, inf, 0.0f, 0.0f, 1.0f }));

    auto result = loadJson (String (R"({
        "asset": { "version": "2.0" },
        "scene": 0,
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "NORMAL": 1, "TANGENT": 2 } } ] } ],
        "buffers": [ { "uri": "$P", "byteLength": 36 }, { "uri": "$N", "byteLength": 36 }, { "uri": "$T", "byteLength": 48 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 36 }, { "buffer": 2, "byteLength": 48 } ],
        "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
                       { "bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3" },
                       { "bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC4" } ]
    })")
                                .replace ("$P", positionsUri())
                                .replace ("$N", normals)
                                .replace ("$T", tangents));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // Zero, NaN and infinite directions have no frame to give: the vertices fall back to the derivative frame
    for (const auto& vertex : result.getReference().getMeshes()[0]->getPrimitive (0).vertices)
        EXPECT_EQ (vertex.tangent, (std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f }));
}

TEST_F (GltfModelTests, BuildsHierarchyWithTrs)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "name": "Rig", "nodes": [ 0 ] } ],
        "nodes": [
            { "name": "Parent", "children": [ 1 ], "translation": [ 1, 2, 3 ], "rotation": [ 0, 0.70710678, 0, 0.70710678 ], "scale": [ 2, 2, 2 ] },
            { "name": "Child", "translation": [ 0, 0, -1 ] }
        ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().instantiate (scene);
    ASSERT_TRUE (root.isValid());
    EXPECT_EQ (root.getName(), "Rig");

    auto parent = root.findChild ("Parent", false);
    ASSERT_TRUE (parent.isValid());
    auto child = parent.findChild ("Child", false);
    ASSERT_TRUE (child.isValid());

    const auto* transform = parent.get<TransformNode>();
    ASSERT_NE (transform, nullptr);
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (transform->getScale(), { 2.0f, 2.0f, 2.0f });
    EXPECT_TRUE (transform->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi)));

    // (0, 0, -1) scaled by 2, turned 90 degrees around Y to (-2, 0, 0), moved by (1, 2, 3)
    expectNear (child.getWorldMatrix().transformPoint ({}), { -1.0f, 2.0f, 3.0f });
}

TEST_F (GltfModelTests, DecomposesMatrixNodes)
{
    const auto expected = Matrix4::scaling ({ 2.0f, 3.0f, 4.0f })
                              .followedBy (Matrix4::rotationZ (MathConstants<float>::halfPi))
                              .followedBy (Matrix4::translation ({ 1.0f, 2.0f, 3.0f }));

    StringArray values;
    for (int i = 0; i < 16; ++i)
        values.add (String (expected.getData()[i], 7));

    auto result = loadJson (String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Matrix", "matrix": [ $MATRIX ] } ]
    })")
                                .replace ("$MATRIX", values.joinIntoString (", ")));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().instantiate (scene);
    auto node = root.findChild ("Matrix");
    ASSERT_TRUE (node.isValid());

    const auto* transform = node.get<TransformNode>();
    ASSERT_NE (transform, nullptr);
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (transform->getScale(), { 2.0f, 3.0f, 4.0f });
    EXPECT_TRUE (transform->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi)));
    EXPECT_TRUE (node.getLocalMatrix().approximatelyEqualTo (expected, 1.0e-4f));
}

TEST_F (GltfModelTests, MapsMaterialsAndTextures)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)",
                                          String (R"(,
        "materials": [
            {
                "name": "Full",
                "pbrMetallicRoughness": { "baseColorFactor": [ 0.5, 0.25, 1, 0.75 ], "metallicFactor": 0.3, "roughnessFactor": 0.6,
                                          "metallicRoughnessTexture": { "index": 0 } },
                "normalTexture": { "index": 0, "scale": 0.5 },
                "occlusionTexture": { "index": 0, "strength": 0.25 },
                "emissiveTexture": { "index": 0 },
                "emissiveFactor": [ 1, 0.5, 0 ],
                "alphaMode": "MASK",
                "alphaCutoff": 0.3,
                "doubleSided": true
            },
            { "name": "Glass", "alphaMode": "BLEND" }
        ],
        "textures": [ { "source": 0 } ],
        "images": [ { "uri": "$IMAGE" } ])")
                                              .replace ("$IMAGE", toDataUri (fromBase64 (redPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    ASSERT_EQ (model.getMaterials().size(), 2u);

    const auto& full = *model.getMaterials()[0];
    EXPECT_EQ (full.name, "Full");
    EXPECT_FLOAT_EQ (full.baseColorFactor[0], 0.5f);
    EXPECT_FLOAT_EQ (full.baseColorFactor[3], 0.75f);
    EXPECT_FLOAT_EQ (full.metallicFactor, 0.3f);
    EXPECT_FLOAT_EQ (full.roughnessFactor, 0.6f);
    EXPECT_FLOAT_EQ (full.normalScale, 0.5f);
    EXPECT_FLOAT_EQ (full.occlusionStrength, 0.25f);
    EXPECT_FLOAT_EQ (full.emissiveFactor[1], 0.5f);
    EXPECT_EQ (full.alphaMode, Material::AlphaMode::mask);
    EXPECT_FLOAT_EQ (full.alphaCutoff, 0.3f);
    EXPECT_TRUE (full.doubleSided);
    EXPECT_EQ (full.baseColorTexture, nullptr);

    // Data textures are linear and shared, color textures are sRGB
    ASSERT_NE (full.normalTexture, nullptr);
    EXPECT_FALSE (full.normalTexture->isSrgb());
    EXPECT_EQ (full.normalTexture, full.occlusionTexture);
    EXPECT_EQ (full.normalTexture, full.metallicRoughnessTexture);
    ASSERT_NE (full.emissiveTexture, nullptr);
    EXPECT_TRUE (full.emissiveTexture->isSrgb());
    EXPECT_EQ (model.getTextures().size(), 2u);

    const auto& glass = *model.getMaterials()[1];
    EXPECT_EQ (glass.alphaMode, Material::AlphaMode::blend);
    EXPECT_FLOAT_EQ (glass.metallicFactor, 1.0f);
    EXPECT_FALSE (glass.doubleSided);

    EXPECT_EQ (model.getMeshes()[0]->getPrimitive (0).material, model.getMaterials()[0]);
}

TEST_F (GltfModelTests, MapsSamplers)
{
    auto result = loadJson (texturedJson (String (R"("samplers": [ { "magFilter": 9728, "minFilter": 9729, "wrapS": 33071, "wrapT": 33648 } ],
                                                     "textures": [ { "source": 0, "sampler": 0 } ],
                                                     "images": [ { "uri": "$IMAGE" } ])")
                                              .replace ("$IMAGE", toDataUri (fromBase64 (redPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& texture = result.getReference().getMaterials()[0]->baseColorTexture;
    ASSERT_NE (texture, nullptr);

    const auto& sampler = texture->getSamplerDesc();
    EXPECT_EQ (sampler.magFilter, GpuFilter::nearest);
    EXPECT_EQ (sampler.minFilter, GpuFilter::linear);
    EXPECT_FLOAT_EQ (sampler.maxLod, 0.0f);
    EXPECT_EQ (sampler.wrapU, GpuWrapMode::clampToEdge);
    EXPECT_EQ (sampler.wrapV, GpuWrapMode::mirrorRepeat);

    // Anisotropic filtering needs linear filtering everywhere
    EXPECT_EQ (sampler.maxAnisotropy, 1u);
}

TEST_F (GltfModelTests, ReadsSparseAccessors)
{
    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "TEXCOORD_0": 1 } } ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 }, { "uri": "$SPARSE", "byteLength": 16 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 4 }, { "buffer": 1, "byteOffset": 4, "byteLength": 12 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3",
              "sparse": { "count": 1, "indices": { "bufferView": 1, "componentType": 5125 }, "values": { "bufferView": 2 } } },
            { "componentType": 5126, "count": 3, "type": "VEC2",
              "sparse": { "count": 1, "indices": { "bufferView": 1, "componentType": 5125 }, "values": { "bufferView": 2 } } }
        ]
    })")
                          .replace ("$POSITIONS", positionsUri());

    MemoryBlock sparse;
    const auto index = toBlock<uint32> ({ 1 });
    const auto values = toBlock<float> ({ 5.0f, 6.0f, 7.0f });
    sparse.append (index.getData(), index.getSize());
    sparse.append (values.getData(), values.getSize());

    auto result = loadJson (json.replace ("$SPARSE", toDataUri (sparse)));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& vertices = result.getReference().getMeshes()[0]->getPrimitive (0).vertices;
    ASSERT_EQ (vertices.size(), 3u);
    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (vertices[1].position, { 5.0f, 6.0f, 7.0f });
    expectNear (vertices[2].position, { 0.0f, 1.0f, 0.0f });

    // An accessor without a buffer view starts from zeros
    EXPECT_FLOAT_EQ (vertices[0].uv.getX(), 0.0f);
    EXPECT_FLOAT_EQ (vertices[1].uv.getX(), 5.0f);
    EXPECT_FLOAT_EQ (vertices[1].uv.getY(), 6.0f);
}

TEST_F (GltfModelTests, AttachesCamerasAndLights)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "extensionsUsed": [ "KHR_lights_punctual" ],
        "extensions": { "KHR_lights_punctual": { "lights": [
            { "type": "spot", "color": [ 1, 0.5, 0.25 ], "intensity": 3, "range": 10, "spot": { "innerConeAngle": 0.1, "outerConeAngle": 0.5 } }
        ] } },
        "cameras": [
            { "type": "perspective", "perspective": { "yfov": 0.5, "znear": 0.25 } },
            { "type": "orthographic", "orthographic": { "xmag": 2, "ymag": 3, "znear": 0.5, "zfar": 50 } }
        ],
        "scenes": [ { "nodes": [ 0, 1, 2 ] } ],
        "nodes": [
            { "name": "Lamp", "extensions": { "KHR_lights_punctual": { "light": 0 } } },
            { "name": "Eye", "camera": 0 },
            { "name": "Top", "camera": 1 }
        ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().instantiate (scene);

    auto* lamp = root.findChild ("Lamp").get<LightNode>();
    ASSERT_NE (lamp, nullptr);
    EXPECT_EQ (lamp->type, LightNode::Type::spot);
    EXPECT_FLOAT_EQ (lamp->color[1], 0.5f);
    EXPECT_FLOAT_EQ (lamp->intensity, 3.0f);
    EXPECT_FLOAT_EQ (lamp->range, 10.0f);
    EXPECT_FLOAT_EQ (lamp->innerConeAngle, 0.1f);
    EXPECT_FLOAT_EQ (lamp->outerConeAngle, 0.5f);

    auto* eye = root.findChild ("Eye").get<CameraNode>();
    ASSERT_NE (eye, nullptr);
    EXPECT_EQ (eye->projection, CameraNode::Projection::perspective);
    EXPECT_FLOAT_EQ (eye->yFov, 0.5f);
    EXPECT_FLOAT_EQ (eye->zNear, 0.25f);
    EXPECT_FLOAT_EQ (eye->zFar, 0.0f);

    auto* top = root.findChild ("Top").get<CameraNode>();
    ASSERT_NE (top, nullptr);
    EXPECT_EQ (top->projection, CameraNode::Projection::orthographic);
    EXPECT_FLOAT_EQ (top->xMag, 2.0f);
    EXPECT_FLOAT_EQ (top->yMag, 3.0f);
    EXPECT_FLOAT_EQ (top->zFar, 50.0f);
}

TEST_F (GltfModelTests, SkipsNonTrianglePrimitivesWithAWarning)
{
    auto result = loadJson (triangleJson (R"(, "mode": 1)"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    ASSERT_EQ (model.getMeshes().size(), 1u);
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 0);
    EXPECT_FALSE (model.getWarnings().isEmpty());
}

TEST_F (GltfModelTests, ExplicitTrianglesModeIsLoaded)
{
    auto result = loadJson (triangleJson (R"(, "mode": 4)"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (result.getReference().getMeshes()[0]->getNumPrimitives(), 1);
}

TEST_F (GltfModelTests, FailsOnOutOfRangeAccessors)
{
    const auto json = triangleJson().replace (R"("count": 3, "type": "VEC3")", R"("count": 30, "type": "VEC3")");

    // tinygltf rejects accessors reaching past their buffer view
    EXPECT_TRUE (loadJson (json).failed());
}

TEST_F (GltfModelTests, SkipsHugeAccessorsWithoutAllocating)
{
    // No buffer view: the accessor reads as zeros, so only its declared size bounds the allocation
    const auto json = triangleJson().replace (R"({ "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" })",
                                              R"({ "componentType": 5126, "count": 67108864, "type": "MAT4" })");

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (result.getReference().getMeshes()[0]->getNumPrimitives(), 0);
    EXPECT_FALSE (result.getReference().getWarnings().isEmpty());
}

TEST_F (GltfModelTests, SkipsPrimitivesWithOutOfRangeIndices)
{
    const auto indices = toBlock<uint16> ({ 0, 1, 7, 0 });

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 }, "indices": 1 } ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 }, { "uri": "$INDICES", "byteLength": 8 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 6 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
            { "bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR" }
        ]
    })")
                          .replace ("$POSITIONS", positionsUri())
                          .replace ("$INDICES", toDataUri (indices));

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (result.getReference().getMeshes()[0]->getNumPrimitives(), 0);
    EXPECT_FALSE (result.getReference().getWarnings().isEmpty());
}

//==============================================================================

TEST_F (GltfModelTests, FailsOnMalformedInput)
{
    EXPECT_TRUE (loadJson ("{ this is not json").failed());
    EXPECT_TRUE (loadJson ("").failed());
    EXPECT_TRUE (loadJson (R"({ "asset": { "version": "2.0" }, "nodes": [ { "mesh": 5 } ] })").failed());
}

TEST_F (GltfModelTests, FailsOnTruncatedGlb)
{
    const auto glb = makeGlb (triangleJson(), {});
    ASSERT_TRUE (GltfModel::loadFromData (glb.asBytes()).wasOk());

    const auto bytes = glb.asBytes();
    EXPECT_TRUE (GltfModel::loadFromData (Span<const uint8> (bytes.data(), bytes.size() - 10)).failed());
    EXPECT_TRUE (GltfModel::loadFromData (Span<const uint8> (bytes.data(), 16)).failed());
}

TEST_F (GltfModelTests, FailsOnUnsupportedRequiredExtensions)
{
    auto result = loadJson (triangleJson ({}, R"(, "extensionsUsed": [ "KHR_draco_mesh_compression" ], "extensionsRequired": [ "KHR_draco_mesh_compression" ])"));

    ASSERT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().contains ("KHR_draco_mesh_compression"));
}

TEST_F (GltfModelTests, UnsupportedExtensionsThatAreOnlyUsedAreReported)
{
    // Used and not required: the file loads without what the extension changes, which is told once
    auto result = loadJson (triangleJson ({}, R"(, "extensionsUsed": [ "KHR_texture_transform", "KHR_lights_punctual", "KHR_texture_transform" ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 1) << warnings;
    EXPECT_TRUE (warnings.contains ("KHR_texture_transform")) << warnings;
    EXPECT_FALSE (warnings.contains ("KHR_lights_punctual")) << warnings;
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 1);
}

TEST_F (GltfModelTests, ReadsKhrMaterialExtensions)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "extensionsUsed": [ "KHR_materials_emissive_strength", "KHR_materials_ior", "KHR_materials_specular" ],
        "materials": [ {
            "emissiveFactor": [ 1, 1, 1 ],
            "extensions": {
                "KHR_materials_emissive_strength": { "emissiveStrength": 5 },
                "KHR_materials_ior": { "ior": 1.33 },
                "KHR_materials_specular": { "specularFactor": 0.5, "specularColorFactor": [ 1, 0.5, 0.25 ] }
            }
        } ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.emissiveStrength, 5.0f);
    EXPECT_FLOAT_EQ (material.ior, 1.33f);
    EXPECT_FLOAT_EQ (material.specularFactor, 0.5f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[1], 0.5f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[2], 0.25f);
}

TEST_F (GltfModelTests, KhrMaterialExtensionsDefaultWhenAbsent)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(, "materials": [ {} ])"));
    ASSERT_TRUE (result.wasOk());

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.emissiveStrength, 1.0f);
    EXPECT_FLOAT_EQ (material.ior, 1.5f);
    EXPECT_FLOAT_EQ (material.specularFactor, 1.0f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[1], 1.0f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[2], 1.0f);
}

TEST_F (GltfModelTests, IntegerExtensionValuesAreRead)
{
    // JSON numbers without a fraction arrive as integers
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": { "KHR_materials_ior": { "ior": 2 } } } ])"));
    ASSERT_TRUE (result.wasOk());
    EXPECT_FLOAT_EQ (result.getReference().getMaterials()[0]->ior, 2.0f);
}

TEST_F (GltfModelTests, IorZeroMeansTotalReflection)
{
    // KHR_materials_ior reserves 0 for an infinite index of refraction, kept as is
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": { "KHR_materials_ior": { "ior": 0 } } } ])"));
    ASSERT_TRUE (result.wasOk());
    EXPECT_FLOAT_EQ (result.getReference().getMaterials()[0]->ior, 0.0f);
}

TEST_F (GltfModelTests, ShortSpecularColorIsIgnored)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": { "KHR_materials_specular": { "specularColorFactor": [ 0.5, 0.5 ] } } } ])"));
    ASSERT_TRUE (result.wasOk());

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.specularColorFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[1], 1.0f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[2], 1.0f);
}

TEST_F (GltfModelTests, RequiredKhrMaterialExtensionsAreSupported)
{
    for (const auto* name : { "KHR_materials_emissive_strength", "KHR_materials_ior", "KHR_materials_specular", "KHR_materials_clearcoat", "KHR_materials_sheen" })
    {
        auto result = loadJson (triangleJson ({}, String (R"(, "extensionsRequired": [ "$NAME" ], "extensionsUsed": [ "$NAME" ])").replace ("$NAME", name)));
        EXPECT_TRUE (result.wasOk()) << name << ": " << result.getErrorMessage();
    }
}

TEST_F (GltfModelTests, LoadsKhrMaterialsFixture)
{
    auto result = GltfModel::loadFromFile (getDataDirectory().getChildFile ("khr_materials.gltf"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.emissiveStrength, 5.0f);
    EXPECT_FLOAT_EQ (material.ior, 1.33f);
    EXPECT_FLOAT_EQ (material.specularFactor, 0.5f);
    EXPECT_FLOAT_EQ (material.specularColorFactor[2], 0.25f);
}

TEST_F (GltfModelTests, OutOfRangeKhrMaterialValuesAreLimited)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": {
            "KHR_materials_emissive_strength": { "emissiveStrength": -3 },
            "KHR_materials_ior": { "ior": 0.5 },
            "KHR_materials_specular": { "specularFactor": 2, "specularColorFactor": [ -1, 0.5, 2 ] }
        } }, { "extensions": {
            "KHR_materials_ior": { "ior": -2 },
            "KHR_materials_specular": { "specularFactor": -0.5 }
        } } ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& materials = result.getReference().getMaterials();
    ASSERT_EQ (materials.size(), 2u);

    // Limited to the extension schemas; an ior between 0 and 1 is undefined, so it is the default
    EXPECT_FLOAT_EQ (materials[0]->emissiveStrength, 0.0f);
    EXPECT_FLOAT_EQ (materials[0]->ior, 1.5f);
    EXPECT_FLOAT_EQ (materials[0]->specularFactor, 1.0f);
    EXPECT_FLOAT_EQ (materials[0]->specularColorFactor[0], 0.0f);
    EXPECT_FLOAT_EQ (materials[0]->specularColorFactor[1], 0.5f);
    EXPECT_FLOAT_EQ (materials[0]->specularColorFactor[2], 2.0f);

    EXPECT_FLOAT_EQ (materials[1]->ior, 1.5f);
    EXPECT_FLOAT_EQ (materials[1]->specularFactor, 0.0f);
}

TEST_F (GltfModelTests, IgnoredSpecularTexturesAreReported)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": { "KHR_materials_specular": {
            "specularTexture": { "index": 0 }, "specularColorTexture": { "index": 0 } } } } ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto warnings = result.getReference().getWarnings().joinIntoString ("\n");
    EXPECT_EQ (result.getReference().getWarnings().size(), 2) << warnings;
    EXPECT_TRUE (warnings.contains ("specularTexture")) << warnings;
    EXPECT_TRUE (warnings.contains ("specularColorTexture")) << warnings;
}

TEST_F (GltfModelTests, ReadsClearcoatAndSheenFactors)
{
    auto result = GltfModel::loadFromFile (getDataDirectory().getChildFile ("clearcoat_sheen.gltf"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_FALSE (result.getReference().getMaterials().empty());

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.clearcoatFactor, 0.8f);
    EXPECT_FLOAT_EQ (material.clearcoatRoughnessFactor, 0.1f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[0], 0.9f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[1], 0.2f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[2], 0.3f);
    EXPECT_FLOAT_EQ (material.sheenRoughnessFactor, 0.5f);
    EXPECT_TRUE (result.getReference().getWarnings().isEmpty()) << result.getReference().getWarnings().joinIntoString ("\n");
}

TEST_F (GltfModelTests, RequiredClearcoatAndSheenLoad)
{
    // A file that cannot render without the layers lists them as required: they are supported
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "extensionsUsed": [ "KHR_materials_clearcoat", "KHR_materials_sheen" ],
        "extensionsRequired": [ "KHR_materials_clearcoat", "KHR_materials_sheen" ],
        "materials": [ { "extensions": {
            "KHR_materials_clearcoat": { "clearcoatFactor": 0.5 },
            "KHR_materials_sheen": { "sheenColorFactor": [ 0.25, 0.5, 1 ] } } } ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_FALSE (result.getReference().getMaterials().empty());

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.clearcoatFactor, 0.5f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[2], 1.0f);
}

TEST_F (GltfModelTests, MaterialsWithoutTheExtensionsKeepDefaults)
{
    auto result = GltfModel::loadFromFile (getDataDirectory().getChildFile ("external").getChildFile ("triangle.gltf"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_FALSE (result.getReference().getMaterials().empty());

    const auto& material = *result.getReference().getMaterials()[0];
    EXPECT_FLOAT_EQ (material.clearcoatFactor, 0.0f);
    EXPECT_FLOAT_EQ (material.clearcoatRoughnessFactor, 0.0f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[0], 0.0f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[1], 0.0f);
    EXPECT_FLOAT_EQ (material.sheenColorFactor[2], 0.0f);
    EXPECT_FLOAT_EQ (material.sheenRoughnessFactor, 0.0f);
}

TEST_F (GltfModelTests, OutOfRangeClearcoatAndSheenValuesAreLimited)
{
    // The extension schemas limit every factor to [0, 1]
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": {
            "KHR_materials_clearcoat": { "clearcoatFactor": 2, "clearcoatRoughnessFactor": -0.5 },
            "KHR_materials_sheen": { "sheenColorFactor": [ -1, 0.5, 3 ], "sheenRoughnessFactor": 1.5 }
        } }, { "extensions": {
            "KHR_materials_clearcoat": { "clearcoatFactor": -1, "clearcoatRoughnessFactor": 1 },
            "KHR_materials_sheen": { "sheenColorFactor": [ 1, 1 ] }
        } } ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& materials = result.getReference().getMaterials();
    ASSERT_EQ (materials.size(), 2u);

    EXPECT_FLOAT_EQ (materials[0]->clearcoatFactor, 1.0f);
    EXPECT_FLOAT_EQ (materials[0]->clearcoatRoughnessFactor, 0.0f);
    EXPECT_FLOAT_EQ (materials[0]->sheenColorFactor[0], 0.0f);
    EXPECT_FLOAT_EQ (materials[0]->sheenColorFactor[1], 0.5f);
    EXPECT_FLOAT_EQ (materials[0]->sheenColorFactor[2], 1.0f);
    EXPECT_FLOAT_EQ (materials[0]->sheenRoughnessFactor, 1.0f);

    // A color too short is ignored, as for KHR_materials_specular
    EXPECT_FLOAT_EQ (materials[1]->clearcoatFactor, 0.0f);
    EXPECT_FLOAT_EQ (materials[1]->clearcoatRoughnessFactor, 1.0f);
    EXPECT_FLOAT_EQ (materials[1]->sheenColorFactor[0], 0.0f);
    EXPECT_FLOAT_EQ (materials[1]->sheenColorFactor[1], 0.0f);
}

TEST_F (GltfModelTests, IgnoredClearcoatAndSheenTexturesAreReported)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)", R"(,
        "materials": [ { "extensions": {
            "KHR_materials_clearcoat": { "clearcoatFactor": 1, "clearcoatTexture": { "index": 0 },
                                         "clearcoatRoughnessTexture": { "index": 0 }, "clearcoatNormalTexture": { "index": 0 } },
            "KHR_materials_sheen": { "sheenColorTexture": { "index": 0 }, "sheenRoughnessTexture": { "index": 0 } } } } ])"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto warnings = result.getReference().getWarnings().joinIntoString ("\n");
    EXPECT_EQ (result.getReference().getWarnings().size(), 5) << warnings;

    for (const auto* name : { "clearcoatTexture", "clearcoatRoughnessTexture", "clearcoatNormalTexture" })
        EXPECT_TRUE (warnings.contains (String ("the KHR_materials_clearcoat ") + name)) << name << "\n" << warnings;

    for (const auto* name : { "sheenColorTexture", "sheenRoughnessTexture" })
        EXPECT_TRUE (warnings.contains (String ("the KHR_materials_sheen ") + name)) << name << "\n" << warnings;

    // The factors still apply
    EXPECT_FLOAT_EQ (result.getReference().getMaterials()[0]->clearcoatFactor, 1.0f);
}

TEST_F (GltfModelTests, FailsOnMissingFile)
{
    EXPECT_TRUE (GltfModel::loadFromFile (getDataDirectory().getChildFile ("does_not_exist.gltf")).failed());
}

TEST_F (GltfModelTests, ExternalReadsAreConfinedToTheBaseDirectory)
{
    const auto baseDirectory = getDataDirectory().getChildFile ("external");
    ASSERT_TRUE (getDataDirectory().getChildFile ("outside.png").existsAsFile());

    const auto images = { String ("../outside.png"),
                          String ("..%2Foutside.png"),
                          getDataDirectory().getChildFile ("outside.png").getFullPathName() };

    for (const auto& uri : images)
    {
        auto result = loadJson (texturedJson (String (R"("textures": [ { "source": 0 } ], "images": [ { "uri": "$URI" } ])")
                                                  .replace ("$URI", uri.replace ("\\", "/"))),
                                baseDirectory);
        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

        EXPECT_EQ (result.getReference().getMaterials()[0]->baseColorTexture, nullptr) << uri;
        EXPECT_FALSE (result.getReference().getWarnings().isEmpty()) << uri;
    }

    // A buffer outside the base directory can't be read, which fails the load
    const auto json = triangleJson().replace (positionsUri(), "../outside.png");
    EXPECT_TRUE (loadJson (json, baseDirectory).failed());

    // The same image inside the base directory loads fine
    auto inside = loadJson (texturedJson (R"("textures": [ { "source": 0 } ], "images": [ { "uri": "checker.png" } ])"), baseDirectory);
    ASSERT_TRUE (inside.wasOk()) << inside.getErrorMessage();
    EXPECT_NE (inside.getReference().getMaterials()[0]->baseColorTexture, nullptr);
}

TEST_F (GltfModelTests, ExternalFilesNeedABaseDirectory)
{
    MemoryBlock data;
    ASSERT_TRUE (getDataDirectory().getChildFile ("external/triangle.gltf").loadFileAsData (data));

    EXPECT_TRUE (GltfModel::loadFromData (data.asBytes()).failed());
    EXPECT_TRUE (GltfModel::loadFromData (data.asBytes(), getDataDirectory().getChildFile ("external")).wasOk());
}

//==============================================================================

TEST_F (GltfModelTests, CreateEntityBuildsFreshTreesSharingResources)
{
    auto result = loadJson (triangleJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    auto first = model.instantiate (scene);
    auto second = model.instantiate (scene);

    ASSERT_TRUE (first.isValid());
    ASSERT_TRUE (second.isValid());
    EXPECT_NE (first, second);
    EXPECT_NE (first.getChild (0), second.getChild (0));
    EXPECT_EQ (first.getChild (0).get<MeshNode>()->mesh, second.getChild (0).get<MeshNode>()->mesh);

    // Copies of the model share the resources too
    const auto copy = model;
    EXPECT_EQ (copy.getMeshes()[0], model.getMeshes()[0]);
}

TEST_F (GltfModelTests, CreateEntityHandlesSceneIndices)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "scene": 1,
        "scenes": [ { "name": "First", "nodes": [ 0 ] }, { "name": "Second", "nodes": [ 1 ] } ],
        "nodes": [ { "name": "A" }, { "name": "B" } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_EQ (model.getNumScenes(), 2);
    EXPECT_EQ (model.instantiate (scene).getName(), "Second");
    EXPECT_EQ (model.instantiate (scene, {}, 0).getName(), "First");
    EXPECT_EQ (model.instantiate (scene, {}, 0).getChild (0).getName(), "A");
    EXPECT_FALSE (model.instantiate (scene, {}, 2).isValid());
    EXPECT_FALSE (model.instantiate (scene, {}, -2).isValid());
}

TEST_F (GltfModelTests, EachSceneHasItsPrefab)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "scene": 1,
        "scenes": [ { "name": "First", "nodes": [ 0 ] }, { "name": "Second", "nodes": [ 1 ] } ],
        "nodes": [ { "name": "A" }, { "name": "B" } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    ASSERT_NE (model.getPrefab(), nullptr);
    EXPECT_EQ (model.getPrefab(), model.getPrefab (1));
    EXPECT_EQ (model.getPrefab (0)->getRoot().getName(), "First");
    EXPECT_EQ (model.getPrefab (0)->getRoot().getChild (0).getName(), "A");
    EXPECT_EQ (model.getPrefab (2), nullptr);

    const auto copy = model;
    EXPECT_EQ (copy.getPrefab (0), model.getPrefab (0));
    EXPECT_NE (GltfModel().getPrefab(), nullptr);
    EXPECT_EQ (GltfModel().getPrefab (0), nullptr);
}

TEST_F (GltfModelTests, AssetsWithoutScenesBuildTheirRootNodes)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "nodes": [ { "name": "A", "children": [ 1 ] }, { "name": "B" }, { "name": "C" } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().instantiate (scene);
    ASSERT_TRUE (root.isValid());
    ASSERT_EQ (root.getNumChildren(), 2);
    EXPECT_EQ (root.getChild (0).getName(), "A");
    EXPECT_EQ (root.getChild (1).getName(), "C");
    EXPECT_EQ (root.getChild (0).getChild (0).getName(), "B");
}

TEST_F (GltfModelTests, NodeCyclesDoNotRecurseForever)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "A", "children": [ 1 ] }, { "name": "B", "children": [ 0 ] } ]
    })");

    if (result.wasOk())
    {
        auto root = result.getReference().instantiate (scene);
        ASSERT_TRUE (root.isValid());
        EXPECT_EQ (root.getChild (0).getChild (0).getNumChildren(), 0);
    }
}

TEST_F (GltfModelTests, ReadsEveryComponentType)
{
    const auto loadUvs = [] (int componentType, bool normalized, const MemoryBlock& uvs)
    {
        const auto json = String (R"({
            "asset": { "version": "2.0" },
            "scenes": [ { "nodes": [ 0 ] } ],
            "nodes": [ { "mesh": 0 } ],
            "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0, "TEXCOORD_0": 1 } } ] } ],
            "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 }, { "uri": "$UVS", "byteLength": $SIZE } ],
            "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": $SIZE } ],
            "accessors": [
                { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
                { "bufferView": 1, "componentType": $TYPE, "normalized": $NORMALIZED, "count": 3, "type": "VEC2" }
            ]
        })")
                              .replace ("$POSITIONS", positionsUri())
                              .replace ("$UVS", toDataUri (uvs))
                              .replace ("$SIZE", String ((int) uvs.getSize()))
                              .replace ("$TYPE", String (componentType))
                              .replace ("$NORMALIZED", normalized ? "true" : "false");

        auto result = loadJson (json);
        EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();

        std::vector<Point<float>> points;
        if (result.wasOk())
            for (const auto& vertex : result.getReference().getMeshes()[0]->getPrimitive (0).vertices)
                points.push_back (vertex.uv);

        return points;
    };

    auto bytes = loadUvs (5120, true, toBlock<int8> ({ 127, -127, -128, 0, 0, 0 }));
    ASSERT_EQ (bytes.size(), 3u);
    EXPECT_FLOAT_EQ (bytes[0].getX(), 1.0f);
    EXPECT_FLOAT_EQ (bytes[0].getY(), -1.0f);
    EXPECT_FLOAT_EQ (bytes[1].getX(), -1.0f);

    auto rawBytes = loadUvs (5120, false, toBlock<int8> ({ -3, 4, 0, 0, 0, 0 }));
    ASSERT_EQ (rawBytes.size(), 3u);
    EXPECT_FLOAT_EQ (rawBytes[0].getX(), -3.0f);
    EXPECT_FLOAT_EQ (rawBytes[0].getY(), 4.0f);

    auto unsignedBytes = loadUvs (5121, false, toBlock<uint8> ({ 200, 1, 0, 0, 0, 0 }));
    ASSERT_EQ (unsignedBytes.size(), 3u);
    EXPECT_FLOAT_EQ (unsignedBytes[0].getX(), 200.0f);

    auto shorts = loadUvs (5122, true, toBlock<int16> ({ 32767, -32768, -1000, 0, 0, 0 }));
    ASSERT_EQ (shorts.size(), 3u);
    EXPECT_FLOAT_EQ (shorts[0].getX(), 1.0f);
    EXPECT_FLOAT_EQ (shorts[0].getY(), -1.0f);
    EXPECT_NEAR (shorts[1].getX(), -1000.0f / 32767.0f, 1.0e-6f);

    auto rawShorts = loadUvs (5122, false, toBlock<int16> ({ -300, 0, 0, 0, 0, 0 }));
    ASSERT_EQ (rawShorts.size(), 3u);
    EXPECT_FLOAT_EQ (rawShorts[0].getX(), -300.0f);

    auto unsignedShorts = loadUvs (5123, true, toBlock<uint16> ({ 65535, 0, 0, 0, 0, 0 }));
    ASSERT_EQ (unsignedShorts.size(), 3u);
    EXPECT_FLOAT_EQ (unsignedShorts[0].getX(), 1.0f);

    auto rawUnsignedShorts = loadUvs (5123, false, toBlock<uint16> ({ 1000, 0, 0, 0, 0, 0 }));
    ASSERT_EQ (rawUnsignedShorts.size(), 3u);
    EXPECT_FLOAT_EQ (rawUnsignedShorts[0].getX(), 1000.0f);
}

TEST_F (GltfModelTests, MapsEveryMinFilterAndRepeatWrap)
{
    struct Case
    {
        int minFilter;
        GpuFilter expectedMin;
        GpuFilter expectedMipmap;
        bool mipmapped;
    };

    for (const auto& c : { Case { 9728, GpuFilter::nearest, GpuFilter::linear, false },
                           Case { 9984, GpuFilter::nearest, GpuFilter::nearest, true },
                           Case { 9985, GpuFilter::linear, GpuFilter::nearest, true },
                           Case { 9986, GpuFilter::nearest, GpuFilter::linear, true },
                           Case { 9987, GpuFilter::linear, GpuFilter::linear, true } })
    {
        auto result = loadJson (texturedJson (String (R"("samplers": [ { "minFilter": $MIN, "wrapS": 10497 } ],
                                                         "textures": [ { "source": 0, "sampler": 0 } ],
                                                         "images": [ { "uri": "$IMAGE" } ])")
                                                  .replace ("$MIN", String (c.minFilter))
                                                  .replace ("$IMAGE", toDataUri (fromBase64 (redPngBase64), "image/png"))));
        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

        const auto& texture = result.getReference().getMaterials()[0]->baseColorTexture;
        ASSERT_NE (texture, nullptr);

        const auto& sampler = texture->getSamplerDesc();
        EXPECT_EQ (sampler.minFilter, c.expectedMin) << c.minFilter;
        EXPECT_EQ (sampler.wrapU, GpuWrapMode::repeat) << c.minFilter;

        if (c.mipmapped)
        {
            EXPECT_EQ (sampler.mipmapFilter, c.expectedMipmap) << c.minFilter;
            EXPECT_GT (sampler.maxLod, 0.0f) << c.minFilter;
        }
        else
        {
            EXPECT_FLOAT_EQ (sampler.maxLod, 0.0f) << c.minFilter;
        }
    }
}

TEST_F (GltfModelTests, ZeroScaleMatrixNodesHaveNoRotation)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Flat", "matrix": [ 0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  4, 5, 6, 1 ] } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().instantiate (scene);
    const auto* node = root.findChild ("Flat").get<TransformNode>();
    ASSERT_NE (node, nullptr);

    expectNear (node->getPosition(), { 4.0f, 5.0f, 6.0f });
    EXPECT_FLOAT_EQ (node->getScale().getX(), 0.0f);
    EXPECT_TRUE (node->getRotation().approximatelyEqualTo (Quaternion::identity()));
}

TEST_F (GltfModelTests, UnloadableImagesAreReported)
{
    const auto expectImageWarning = [] (const String& image, const String& expected, const File& baseDirectory = {})
    {
        auto result = loadJson (texturedJson (R"("textures": [ { "source": 0 } ], "images": [ )" + image + " ]"), baseDirectory);
        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

        const auto& model = result.getReference();
        EXPECT_EQ (model.getMaterials()[0]->baseColorTexture, nullptr) << image;

        const auto warnings = model.getWarnings().joinIntoString ("\n");
        EXPECT_TRUE (warnings.contains (expected)) << image << "\n" << warnings;
    };

    expectImageWarning (R"({ "uri": "data:image/png,notbase64" })", "unsupported data URI");
    expectImageWarning (R"({ "uri": "missing.png" })", "outside the base directory");
    expectImageWarning (R"({ "uri": "does_not_exist.png" })", "unable to read", getDataDirectory());
}

TEST_F (GltfModelTests, ExtTextureWebpIgnoresOtherMembers)
{
    auto result = loadJson (texturedJson (String (R"("textures": [ { "source": 0, "extensions": { "EXT_texture_webp": { "comment": 1, "source": 1 } } } ],
                                                     "images": [ { "uri": "$RED" }, { "uri": "$GREEN" } ])")
                                              .replace ("$RED", toDataUri (fromBase64 (redPngBase64), "image/png"))
                                              .replace ("$GREEN", toDataUri (fromBase64 (greenPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& texture = result.getReference().getMaterials()[0]->baseColorTexture;
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->getImage().getWidth(), 2);
}

TEST_F (GltfModelTests, SecondaryTexCoordSetsAreReported)
{
    auto result = loadJson (triangleJson (R"(, "material": 0)",
                                          String (R"(, "materials": [ { "pbrMetallicRoughness": { "baseColorTexture": { "index": 0, "texCoord": 1 } } } ],
                                                      "textures": [ { "source": 0 } ], "images": [ { "uri": "$IMAGE" } ])")
                                              .replace ("$IMAGE", toDataUri (fromBase64 (redPngBase64), "image/png"))));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto warnings = result.getReference().getWarnings().joinIntoString ("\n");
    EXPECT_TRUE (warnings.contains ("TEXCOORD_1")) << warnings;
}

TEST_F (GltfModelTests, InvalidAttributesAndIndicesAreReported)
{
    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [
            { "attributes": { "POSITION": 0, "NORMAL": 1 } },
            { "attributes": { "POSITION": 0 }, "indices": 2 }
        ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 36 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 36 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "SCALAR" },
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" }
        ]
    })")
                          .replace ("$POSITIONS", positionsUri());

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 1);

    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_TRUE (warnings.contains ("invalid NORMAL")) << warnings;
    EXPECT_TRUE (warnings.contains ("invalid indices")) << warnings;
}

TEST_F (GltfModelTests, NonIndexedPrimitivesMustBeTriangleLists)
{
    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 } } ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 24 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 24 } ],
        "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 2, "type": "VEC3" } ]
    })")
                          .replace ("$POSITIONS", toDataUri (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f })));

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 0);
    EXPECT_TRUE (model.getWarnings().joinIntoString ("\n").contains ("not a list of triangles"));
}

TEST_F (GltfModelTests, PrimitivesTheMeshRefusesAreReported)
{
    // A triangle of two vertices: its indices are in range, and a primitive still needs three vertices
    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 }, "indices": 1 } ] } ],
        "buffers": [ { "uri": "$POSITIONS", "byteLength": 24 }, { "uri": "$INDICES", "byteLength": 8 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 24 }, { "buffer": 1, "byteLength": 6 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 2, "type": "VEC3" },
            { "bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR" }
        ]
    })")
                          .replace ("$POSITIONS", toDataUri (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f })))
                          .replace ("$INDICES", toDataUri (toBlock<uint16> ({ 0, 1, 0, 0 })));

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 0);
    EXPECT_EQ (model.getWarnings().size(), 1) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0: A primitive needs at least three vertices")) << warnings;
}

TEST_F (GltfModelTests, PositionsThatAreNotFiniteAreReported)
{
    // The second vertex of the triangle has no place
    const auto positions = toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 1.0f, 0.0f });

    auto result = loadJson (triangleJson().replace (positionsUri(), toDataUri (positions)));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 0);
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0: missing or invalid POSITION")) << warnings;
}

TEST_F (GltfModelTests, NodeTransformsThatAreNotFiniteAreReported)
{
    // A translation too large for the floats a transform is made of
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0, 1 ] } ],
        "nodes": [ { "name": "Lost", "translation": [ 1e300, 2, 3 ], "scale": [ 2, 2, 2 ] },
                   { "name": "Placed", "translation": [ 1, 2, 3 ] } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 1) << warnings;
    EXPECT_TRUE (warnings.contains ("Node 0: a transform that is not finite, ignored")) << warnings;

    // The node is there, where its parent has it
    const auto root = model.instantiate (scene);
    const auto* lost = root.findChild ("Lost").get<TransformNode>();
    ASSERT_NE (lost, nullptr);
    expectNear (lost->getPosition(), { 0.0f, 0.0f, 0.0f });
    expectNear (lost->getScale(), { 1.0f, 1.0f, 1.0f });
    expectNear (root.findChild ("Placed").get<TransformNode>()->getPosition(), { 1.0f, 2.0f, 3.0f });
}

TEST_F (GltfModelTests, AttachesPointLights)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "extensionsUsed": [ "KHR_lights_punctual" ],
        "extensions": { "KHR_lights_punctual": { "lights": [ { "type": "point", "intensity": 2 } ] } },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Bulb", "extensions": { "KHR_lights_punctual": { "light": 0 } } } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().instantiate (scene);
    auto* bulb = root.findChild ("Bulb").get<LightNode>();
    ASSERT_NE (bulb, nullptr);
    EXPECT_EQ (bulb->type, LightNode::Type::point);
    EXPECT_FLOAT_EQ (bulb->intensity, 2.0f);
}

TEST_F (GltfModelTests, SceneIndexWithoutScenesCreatesNothing)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "nodes": [ { "name": "Loose" } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_FALSE (model.instantiate (scene, {}, 0).isValid());

    auto root = model.instantiate (scene);
    ASSERT_TRUE (root.isValid());
    EXPECT_TRUE (root.findChild ("Loose").isValid());
}

//==============================================================================

TEST_F (GltfModelTests, LoadsAnimationsAsClips)
{
    auto result = loadJson (animatedJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");
    ASSERT_EQ (model.getAnimations().size(), 2u);

    const auto& wave = *model.getAnimations()[0];
    EXPECT_EQ (wave.getName(), "Wave");
    EXPECT_FLOAT_EQ (wave.getDuration(), 2.0f);
    ASSERT_EQ (wave.getTracks().size(), 2u);

    // The target is the glTF node, and a sampler without an interpolation is linear
    const auto& translation = wave.getTracks()[0];
    EXPECT_EQ (translation.target, 1);
    EXPECT_EQ (translation.path, AnimationClip::Path::translation);
    EXPECT_EQ (translation.interpolation, AnimationClip::Interpolation::linear);
    EXPECT_EQ (translation.times, (std::vector<float> { 0.0f, 1.0f, 2.0f }));
    EXPECT_EQ (translation.values, (std::vector<float> { 1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0.0f, 1.0f, 2.0f, 3.0f }));

    const auto& rotation = wave.getTracks()[1];
    EXPECT_EQ (rotation.target, 0);
    EXPECT_EQ (rotation.path, AnimationClip::Path::rotation);
    EXPECT_EQ (rotation.interpolation, AnimationClip::Interpolation::step);
    EXPECT_EQ (rotation.times, (std::vector<float> { 0.0f, 2.0f }));
    EXPECT_EQ (rotation.values, (std::vector<float> { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.70710678f, 0.70710678f }));

    // An animation without a name is called after its index
    const auto& unnamed = *model.getAnimations()[1];
    EXPECT_EQ (unnamed.getName(), "Animation 1");
    ASSERT_EQ (unnamed.getTracks().size(), 1u);

    const auto& scale = unnamed.getTracks()[0];
    EXPECT_EQ (scale.target, 2);
    EXPECT_EQ (scale.path, AnimationClip::Path::scale);
    EXPECT_EQ (scale.interpolation, AnimationClip::Interpolation::cubicSpline);
    EXPECT_EQ (scale.times, (std::vector<float> { 0.0f, 2.0f }));
    EXPECT_EQ (scale.values.size(), 18u);

    // Copies of the model share the clips too
    const auto copy = model;
    EXPECT_EQ (copy.getAnimations()[0], model.getAnimations()[0]);
}

TEST_F (GltfModelTests, ReadsNormalizedAnimationOutputs)
{
    // The weights are those of the two morph targets of the triangle the node draws
    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Joint", "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 3 }, "targets": [ { "POSITION": 3 }, { "POSITION": 3 } ] } ] } ],
        "animations": [ { "channels": [ { "sampler": 0, "target": { "node": 0, "path": "rotation" } },
                                        { "sampler": 1, "target": { "node": 0, "path": "weights" } } ],
                          "samplers": [ { "input": 0, "output": 1 }, { "input": 0, "output": 2 } ] } ],
        "buffers": [ { "uri": "$TIMES", "byteLength": 8 }, { "uri": "$ROTATIONS", "byteLength": 16 }, { "uri": "$WEIGHTS", "byteLength": 4 },
                     { "uri": "$POSITIONS", "byteLength": 36 } ],
        "bufferViews": [ { "buffer": 0, "byteLength": 8 }, { "buffer": 1, "byteLength": 16 }, { "buffer": 2, "byteLength": 4 },
                         { "buffer": 3, "byteLength": 36 } ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 2, "type": "SCALAR" },
            { "bufferView": 1, "componentType": 5122, "normalized": true, "count": 2, "type": "VEC4" },
            { "bufferView": 2, "componentType": 5121, "normalized": true, "count": 4, "type": "SCALAR" },
            { "bufferView": 3, "componentType": 5126, "count": 3, "type": "VEC3" }
        ]
    })")
                          .replace ("$TIMES", toDataUri (toBlock<float> ({ 0.0f, 1.0f })))
                          .replace ("$ROTATIONS", toDataUri (toBlock<int16> ({ 0, 0, 0, 32767, 0, -32768, 0, 0 })))
                          .replace ("$WEIGHTS", toDataUri (toBlock<uint8> ({ 0, 255, 51, 0 })))
                          .replace ("$POSITIONS", positionsUri());

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");
    ASSERT_EQ (model.getAnimations().size(), 1u);
    ASSERT_EQ (model.getAnimations()[0]->getTracks().size(), 2u);

    const auto& rotation = model.getAnimations()[0]->getTracks()[0];
    EXPECT_EQ (rotation.values, (std::vector<float> { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f }));

    // Two keys of four scalars: two morph target weights each
    const auto& weights = model.getAnimations()[0]->getTracks()[1];
    EXPECT_EQ (weights.path, AnimationClip::Path::weights);
    EXPECT_EQ (weights.getNumComponents(), 2);
    ASSERT_EQ (weights.values.size(), 4u);
    EXPECT_FLOAT_EQ (weights.values[1], 1.0f);
    EXPECT_NEAR (weights.values[2], 0.2f, 1.0e-6f);
}

TEST_F (GltfModelTests, InstancesPlayTheClipsOnTheirOwnEntities)
{
    auto result = loadJson (animatedJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    auto first = model.instantiate (scene);
    auto second = model.instantiate (scene);

    auto* firstAnimation = first.get<AnimationNode>();
    auto* secondAnimation = second.get<AnimationNode>();
    ASSERT_NE (firstAnimation, nullptr);
    ASSERT_NE (secondAnimation, nullptr);

    // Every instance plays the clips of the model, and none until asked
    ASSERT_EQ (firstAnimation->getNumClips(), 2);
    EXPECT_EQ (firstAnimation->getClip (0), model.getAnimations()[0]);
    EXPECT_EQ (secondAnimation->getClip (1), model.getAnimations()[1]);
    EXPECT_FALSE (firstAnimation->isPlaying());

    const auto* firstArm = first.findChild ("Arm").get<TransformNode>();
    const auto* firstHand = first.findChild ("Hand").get<TransformNode>();
    const auto* secondArm = second.findChild ("Arm").get<TransformNode>();
    const auto* secondHand = second.findChild ("Hand").get<TransformNode>();
    ASSERT_NE (firstArm, nullptr);
    ASSERT_NE (firstHand, nullptr);
    ASSERT_NE (secondArm, nullptr);
    ASSERT_NE (secondHand, nullptr);

    scene.update (1.0);
    expectNear (firstHand->getPosition(), { 1.0f, 0.0f, 0.0f });

    // The translation of Hand is at its second key, the rotation of Arm holds its first one
    firstAnimation->setPlaying (true);
    scene.update (1.0);
    expectNear (firstHand->getPosition(), { 1.0f, 2.0f, 0.0f });
    EXPECT_TRUE (firstArm->getRotation().approximatelyEqualTo (Quaternion::identity()));

    scene.update (0.5);
    expectNear (firstHand->getPosition(), { 1.0f, 2.0f, 1.5f });

    // The other instance stayed as it was built, and is posed on its own
    expectNear (secondHand->getPosition(), { 1.0f, 0.0f, 0.0f });

    secondAnimation->setTime (2.0);
    expectNear (secondHand->getPosition(), { 1.0f, 2.0f, 3.0f });
    EXPECT_TRUE (secondArm->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi)));

    expectNear (firstHand->getPosition(), { 1.0f, 2.0f, 1.5f });
    EXPECT_TRUE (firstArm->getRotation().approximatelyEqualTo (Quaternion::identity()));
}

TEST_F (GltfModelTests, AnimationTargetsOutsideTheSceneAreSkipped)
{
    auto result = loadJson (animatedJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();

    // The second clip scales Spare, which the first scene doesn't hold: nothing to pose there
    auto first = model.instantiate (scene, {}, 0);
    const auto numEntities = scene.getNumEntities();
    auto* firstAnimation = first.get<AnimationNode>();
    ASSERT_NE (firstAnimation, nullptr);

    firstAnimation->setCurrentClip (1);
    firstAnimation->setPlaying (true);
    scene.update (1.0);
    EXPECT_EQ (scene.getNumEntities(), numEntities);
    expectNear (first.findChild ("Hand").get<TransformNode>()->getPosition(), { 1.0f, 0.0f, 0.0f });

    // The second scene holds it: halfway along the spline, between scales of 1 and 3 without tangents
    auto second = model.instantiate (scene, {}, 1);
    auto* secondAnimation = second.get<AnimationNode>();
    ASSERT_NE (secondAnimation, nullptr);

    secondAnimation->setCurrentClip (1);
    secondAnimation->setTime (1.0);
    expectNear (second.findChild ("Spare").get<TransformNode>()->getScale(), { 2.0f, 2.0f, 2.0f });
}

TEST_F (GltfModelTests, ModelsWithoutAnimationsHaveNoAnimationNode)
{
    auto result = loadJson (triangleJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getAnimations().empty());
    EXPECT_FALSE (model.instantiate (scene).has<AnimationNode>());
    EXPECT_TRUE (GltfModel().getAnimations().empty());
}

TEST_F (GltfModelTests, AnimationChannelsThatCannotBePlayedAreReported)
{
    const auto expectWarning = [this] (const String& channel, const String& sampler, const String& expected)
    {
        auto result = loadJson (animatedJson (R"({ "name": "Broken", "channels": [ )" + channel + R"( ], "samplers": [ )" + sampler + " ] }"));
        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

        const auto& model = result.getReference();
        const auto warnings = model.getWarnings().joinIntoString ("\n");
        EXPECT_TRUE (warnings.contains ("Animation 0 channel 0: " + expected)) << channel << "\n" << warnings;

        // Its only channel skipped, the animation has nothing to play: it keeps its place, without a clip
        EXPECT_TRUE (warnings.contains ("Animation 0: no channel can be played")) << channel << "\n" << warnings;
        ASSERT_EQ (model.getAnimations().size(), 1u) << channel;
        EXPECT_EQ (model.getAnimations()[0], nullptr) << channel;
        EXPECT_FALSE (model.instantiate (scene).has<AnimationNode>()) << channel;
    };

    const String translationOfHand = R"({ "sampler": 0, "target": { "node": 1, "path": "translation" } })";

    expectWarning (R"({ "sampler": 0, "target": { "path": "translation" } })", R"({ "input": 0, "output": 1 })", "no target node");
    expectWarning (R"({ "sampler": 0, "target": { "node": 1, "path": "color" } })", R"({ "input": 0, "output": 1 })", "unknown path \"color\"");
    expectWarning (translationOfHand, R"({ "input": 0, "output": 1, "interpolation": "BEZIER" })", "unknown interpolation \"BEZIER\"");

    // Two keys for three translations, three translations where times should be, and no tangents for a spline
    expectWarning (translationOfHand, R"({ "input": 2, "output": 1 })", "The number of values (9) doesn't match the number of keys (2)");
    expectWarning (translationOfHand, R"({ "input": 1, "output": 1 })", "invalid input or output");
    expectWarning (translationOfHand, R"({ "input": 0, "output": 1, "interpolation": "CUBICSPLINE" })", "The number of values (9) doesn't match the number of keys (3)");

    // A weight for each of three keys, for a node that draws no mesh to morph
    expectWarning (R"({ "sampler": 0, "target": { "node": 1, "path": "weights" } })", R"({ "input": 0, "output": 0 })", "the node draws no mesh for the weights to morph");
}

TEST_F (GltfModelTests, AnimationsKeepTheChannelsThatCanBePlayed)
{
    auto result = loadJson (animatedJson (R"({ "channels": [ { "sampler": 0, "target": { "node": 1, "path": "color" } },
                                                              { "sampler": 0, "target": { "node": 1, "path": "translation" } } ],
                                               "samplers": [ { "input": 0, "output": 1 } ] })"));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_EQ (model.getWarnings().size(), 1) << model.getWarnings().joinIntoString ("\n");
    ASSERT_EQ (model.getAnimations().size(), 1u);
    ASSERT_EQ (model.getAnimations()[0]->getTracks().size(), 1u);
    EXPECT_EQ (model.getAnimations()[0]->getTracks()[0].path, AnimationClip::Path::translation);
}

//==============================================================================

TEST_F (GltfModelTests, LoadsSkinsWithTheirJointsWeightsAndInverseBindMatrices)
{
    auto result = loadJson (SkinnedStrip().toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");

    // The joints of a skin are glTF nodes, and its matrices come in the order of the joints
    ASSERT_EQ (model.getSkins().size(), 1u);
    const auto& skin = *model.getSkins()[0];
    EXPECT_EQ (skin.getName(), "Bend");
    ASSERT_EQ (skin.getNumJoints(), 2);
    EXPECT_EQ (skin.getJoints()[0], 1);
    EXPECT_EQ (skin.getJoints()[1], 2);
    EXPECT_TRUE (skin.getInverseBindMatrices()[0].isIdentity());
    EXPECT_TRUE (skin.getInverseBindMatrices()[1] == Matrix4::translation ({ 0.0f, -1.0f, 0.0f }));

    // The foot of the strip follows the first joint of the skin and its top the second
    const auto& mesh = *model.getMeshes()[0];
    const auto& influences = mesh.getPrimitive (0).influences;
    ASSERT_EQ (influences.size(), 4u);
    EXPECT_EQ (influences[0].joints, (std::array<uint16, 4> { 0, 0, 0, 0 }));
    EXPECT_EQ (influences[1].weights, (std::array<float, 4> { 1.0f, 0.0f, 0.0f, 0.0f }));
    EXPECT_EQ (influences[2].weights, (std::array<float, 4> { 1.0f, 0.0f, 0.0f, 0.0f }));
    EXPECT_EQ (influences[3].joints, (std::array<uint16, 4> { 1, 0, 0, 0 }));
    EXPECT_EQ (mesh.getNumJoints(), 2);
    EXPECT_EQ (mesh.getNumMorphTargets(), 0);

    // Copies of the model share the skins too, and a model without skins has none
    const auto copy = model;
    EXPECT_EQ (copy.getSkins()[0], model.getSkins()[0]);
    EXPECT_TRUE (GltfModel().getSkins().empty());
}

TEST_F (GltfModelTests, SkinnedInstancesFollowTheirOwnJoints)
{
    auto result = loadJson (SkinnedStrip().toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    auto first = model.instantiate (scene);
    auto second = model.instantiate (scene);

    // The entity of the node that has the skin deforms a copy of the mesh, with the joints of its instance
    const auto firstStrip = first.findChild ("Strip");
    const auto secondStrip = second.findChild ("Strip");
    const auto* firstDeform = firstStrip.get<DeformNode>();
    const auto* secondDeform = secondStrip.get<DeformNode>();
    ASSERT_NE (firstDeform, nullptr);
    ASSERT_NE (secondDeform, nullptr);
    EXPECT_EQ (firstDeform->getSkin(), model.getSkins()[0]);
    EXPECT_EQ (firstDeform->getSourceMesh(), model.getMeshes()[0]);
    EXPECT_EQ (firstStrip.get<MeshNode>()->mesh, firstDeform->getDeformedMesh());
    EXPECT_NE (firstDeform->getDeformedMesh(), secondDeform->getDeformedMesh());

    // The joints stand where the strip was bound to them: it starts in its bind pose
    const auto& firstVertices = getDrawnVertices (firstStrip);
    const auto& secondVertices = getDrawnVertices (secondStrip);
    expectNear (firstVertices[2].position, { 0.0f, 2.0f, 0.0f });

    // The clip gives Tip a quarter turn about x, which takes the top of the strip from (0, 1, 0) and
    // (1, 1, 0) above it to (0, 0, 1) and (1, 0, 1): one update poses the joint, then deforms the mesh
    auto* animation = first.get<AnimationNode>();
    ASSERT_NE (animation, nullptr);
    animation->setPlaying (true);
    scene.update (1.0);

    expectNear (firstVertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (firstVertices[1].position, { 1.0f, 0.0f, 0.0f });
    expectNear (firstVertices[2].position, { 0.0f, 1.0f, 1.0f });
    expectNear (firstVertices[3].position, { 1.0f, 1.0f, 1.0f });
    expectNear (firstVertices[2].normal, { 0.0f, -1.0f, 0.0f });

    // The other instance and the mesh of the model are still in the bind pose
    expectNear (secondVertices[2].position, { 0.0f, 2.0f, 0.0f });
    expectNear (model.getMeshes()[0]->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 0.0f });
    EXPECT_EQ (model.getMeshes()[0]->getChangeCount(), 0u);

    // A joint moved by code is followed like one moved by a clip: Root carries Tip and the whole strip
    second.findChild ("Root").get<TransformNode>()->setPosition ({ 0.0f, 0.0f, 3.0f });
    scene.update (0.0);

    expectNear (secondVertices[0].position, { 0.0f, 0.0f, 3.0f });
    expectNear (secondVertices[2].position, { 0.0f, 2.0f, 3.0f });
    expectNear (firstVertices[2].position, { 0.0f, 1.0f, 1.0f });
}

TEST_F (GltfModelTests, SkinsWithoutInverseBindMatricesAreBoundAtTheOrigin)
{
    SkinnedStrip asset;
    asset.skin = R"({ "joints": [ 1, 2 ] })";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");
    ASSERT_EQ (model.getSkins().size(), 1u);
    EXPECT_TRUE (model.getSkins()[0]->getName().isEmpty());
    ASSERT_EQ (model.getSkins()[0]->getInverseBindMatrices().size(), 2u);
    EXPECT_TRUE (model.getSkins()[0]->getInverseBindMatrices()[1].isIdentity());

    // Tip, which stands 1 up, then carries the top of the strip 1 up. An instance shows where its joints
    // put the mesh as soon as it is made, without an update
    const auto strip = model.instantiate (scene).findChild ("Strip");
    expectNear (getDrawnVertices (strip)[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (strip)[2].position, { 0.0f, 3.0f, 0.0f });
    expectNear (getDrawnVertices (strip)[3].position, { 1.0f, 3.0f, 0.0f });
    expectNear (model.getMeshes()[0]->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 0.0f });
}

TEST_F (GltfModelTests, ReadsJointsAndWeightsOfEveryType)
{
    // Joints as unsigned shorts and weights as normalized bytes. The second vertex names a joint the skin
    // doesn't have, without a weight, and the third has no weight at all
    SkinnedStrip asset;
    asset.jointsType = 5123;
    asset.joints = toBlock<uint16> ({ 0, 0, 0, 0, 0, 7, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 });
    asset.weightsType = 5121;
    asset.weightsNormalized = true;
    asset.weights = toBlock<uint8> ({ 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 128, 127, 0, 0 });

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");

    const auto& influences = model.getMeshes()[0]->getPrimitive (0).influences;
    ASSERT_EQ (influences.size(), 4u);
    EXPECT_FLOAT_EQ (influences[0].weights[0], 1.0f);

    // A joint without a weight is not used: its index is dropped, and the mesh doesn't refer to it
    EXPECT_EQ (influences[1].joints, (std::array<uint16, 4> { 0, 0, 0, 0 }));
    EXPECT_EQ (model.getMeshes()[0]->getNumJoints(), 2);

    EXPECT_EQ (influences[2].weights, (std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f }));

    // 128 and 127 out of 255
    EXPECT_EQ (influences[3].joints, (std::array<uint16, 4> { 0, 1, 0, 0 }));
    EXPECT_NEAR (influences[3].weights[0], 0.5019608f, 1.0e-6f);
    EXPECT_NEAR (influences[3].weights[1], 0.4980392f, 1.0e-6f);

    auto instance = model.instantiate (scene);
    const auto& vertices = getDrawnVertices (instance.findChild ("Strip"));
    instance.findChild ("Tip").get<TransformNode>()->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, MathConstants<float>::halfPi));
    scene.update (0.0);

    // glTF gives a vertex without weights no joint to follow: it stays in the bind pose
    expectNear (vertices[2].position, { 0.0f, 2.0f, 0.0f });

    // The last vertex stays at (1, 2, 0) with Root for 128 / 255, and goes to (1, 1, 1) with Tip for the rest
    expectNear (vertices[3].position, { 1.0f, 1.5019608f, 0.4980392f });
}

TEST_F (GltfModelTests, SkinWeightsAreMadeToAddUpToOne)
{
    // Weights adding up to 4, to 0.25 and to 3, and a negative one
    SkinnedStrip asset;
    asset.joints = toBlock<uint8> ({ 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 });
    asset.weights = toBlock<float> ({ 2.0f, 2.0f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f });

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");

    const auto& influences = model.getMeshes()[0]->getPrimitive (0).influences;
    ASSERT_EQ (influences.size(), 4u);
    EXPECT_EQ (influences[0].joints, (std::array<uint16, 4> { 0, 1, 0, 0 }));
    EXPECT_EQ (influences[0].weights, (std::array<float, 4> { 0.5f, 0.5f, 0.0f, 0.0f }));
    EXPECT_EQ (influences[1].weights, (std::array<float, 4> { 1.0f, 0.0f, 0.0f, 0.0f }));
    EXPECT_EQ (influences[3].weights, (std::array<float, 4> { 1.0f, 0.0f, 0.0f, 0.0f }));

    // glTF: weights are not negative. That one counts for nothing, which leaves its vertex without any
    EXPECT_EQ (influences[2].joints, (std::array<uint16, 4> { 0, 0, 0, 0 }));
    EXPECT_EQ (influences[2].weights, (std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f }));
}

TEST_F (GltfModelTests, SkinsThatCannotMoveTheirMeshLeaveItAsItIs)
{
    const auto expectNotSkinned = [this] (const SkinnedStrip& asset, const String& expected, bool keepsSkin)
    {
        auto result = loadJson (asset.toJson());
        ASSERT_TRUE (result.wasOk()) << expected << "\n" << result.getErrorMessage();

        const auto& model = result.getReference();
        const auto warnings = model.getWarnings().joinIntoString ("\n");
        EXPECT_TRUE (warnings.contains (expected)) << expected << "\n" << warnings;

        // A skin keeps its place among the skins of the model, without a Skin when it can't be used
        ASSERT_EQ (model.getSkins().size(), 1u) << expected;
        EXPECT_EQ (model.getSkins()[0] != nullptr, keepsSkin) << expected;

        // The node draws the mesh of the model, in its bind pose
        const auto strip = model.instantiate (scene).findChild ("Strip");
        EXPECT_FALSE (strip.has<DeformNode>()) << expected;
        EXPECT_EQ (strip.get<MeshNode>()->mesh, model.getMeshes()[0]) << expected;
    };

    // Joints without weights and weights without joints. The node then has a skin for a mesh without joints
    SkinnedStrip jointsOnly;
    jointsOnly.attributes = R"("POSITION": 0, "NORMAL": 1, "JOINTS_0": 2)";
    expectNotSkinned (jointsOnly, "Mesh 0 primitive 0: JOINTS_0 and WEIGHTS_0 come together, the primitive is not skinned", true);

    SkinnedStrip weightsOnly;
    weightsOnly.attributes = R"("POSITION": 0, "NORMAL": 1, "WEIGHTS_0": 3)";
    expectNotSkinned (weightsOnly, "Mesh 0 primitive 0: JOINTS_0 and WEIGHTS_0 come together, the primitive is not skinned", true);
    expectNotSkinned (weightsOnly, "Node 0: mesh 0 has no joints, skin 0 is ignored", true);

    // The last vertex follows a third joint, and the skin has two
    SkinnedStrip jointOutside;
    jointOutside.joints = toBlock<uint8> ({ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0 });
    expectNotSkinned (jointOutside, "Node 0: mesh 0 uses joint 2, which skin 0 doesn't have, it is not skinned", true);

    // Joints that are not whole numbers, and the three components of the normals for weights
    SkinnedStrip fractionalJoints;
    fractionalJoints.jointsType = 5126;
    fractionalJoints.joints = toBlock<float> ({ 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f });
    expectNotSkinned (fractionalJoints, "Mesh 0 primitive 0: invalid JOINTS_0 or WEIGHTS_0, the primitive is not skinned", true);

    SkinnedStrip normalsForWeights;
    normalsForWeights.attributes = R"("POSITION": 0, "NORMAL": 1, "JOINTS_0": 2, "WEIGHTS_0": 1)";
    expectNotSkinned (normalsForWeights, "Mesh 0 primitive 0: invalid JOINTS_0 or WEIGHTS_0, the primitive is not skinned", true);

    // One matrix for two joints, positions for matrices, and a matrix that is not finite: such a skin is left out
    SkinnedStrip oneMatrix;
    oneMatrix.numInverseBindMatrices = 1;
    expectNotSkinned (oneMatrix, "Skin 0: 1 inverse bind matrices for 2 joints, ignored", false);

    SkinnedStrip positionsForMatrices;
    positionsForMatrices.skin = R"({ "joints": [ 1, 2 ], "inverseBindMatrices": 0 })";
    expectNotSkinned (positionsForMatrices, "Skin 0: invalid inverseBindMatrices, ignored", false);

    SkinnedStrip matrixWithoutPlace;
    matrixWithoutPlace.inverseBindMatrices = toBlock<float> ({ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, std::numeric_limits<float>::infinity(), 0.0f, 1.0f });
    expectNotSkinned (matrixWithoutPlace, "Skin 0: invalid inverseBindMatrices, ignored", false);
}

TEST_F (GltfModelTests, SkinsAndAnimationsKeepTheIndexTheyHaveInTheFile)
{
    // The first skin has positions for matrices, and the first animation a path that is not played
    SkinnedStrip asset;
    asset.skin = R"({ "joints": [ 1, 2 ], "inverseBindMatrices": 0 }, { "name": "Bend", "joints": [ 1, 2 ], "inverseBindMatrices": 4 })";

    const auto json = asset.toJson()
                          .replace (R"("skin": 0)", R"("skin": 1)")
                          .replace (R"("animations": [)", R"("animations": [ { "name": "Broken", "channels": [ { "sampler": 0, "target": { "node": 2, "path": "color" } } ], "samplers": [ { "input": 6, "output": 7 } ] },)");

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // Both keep their place, without a Skin or a clip: the ones after them are found where the file has them
    const auto& model = result.getReference();
    ASSERT_EQ (model.getSkins().size(), 2u);
    EXPECT_EQ (model.getSkins()[0], nullptr);
    ASSERT_NE (model.getSkins()[1], nullptr);
    EXPECT_EQ (model.getSkins()[1]->getName(), "Bend");

    ASSERT_EQ (model.getAnimations().size(), 2u);
    EXPECT_EQ (model.getAnimations()[0], nullptr);
    ASSERT_NE (model.getAnimations()[1], nullptr);
    EXPECT_EQ (model.getAnimations()[1]->getName(), "Bend");

    // An instance has the clips at the same indices, and starts on the first one there is
    auto instance = model.instantiate (scene);
    auto* animation = instance.get<AnimationNode>();
    ASSERT_NE (animation, nullptr);
    ASSERT_EQ (animation->getNumClips(), 2);
    EXPECT_EQ (animation->getClip (0), nullptr);
    EXPECT_EQ (animation->getClip (1), model.getAnimations()[1]);
    EXPECT_EQ (animation->getCurrentClip(), 1);

    // The node is skinned by the skin it names, and follows the clip
    const auto strip = instance.findChild ("Strip");
    ASSERT_TRUE (strip.has<DeformNode>());
    EXPECT_EQ (strip.get<DeformNode>()->getSkin(), model.getSkins()[1]);

    animation->setPlaying (true);
    scene.update (1.0);
    expectNear (getDrawnVertices (strip)[2].position, { 0.0f, 1.0f, 1.0f });
}

TEST_F (GltfModelTests, JointsBeyondTheFirstFourOfAVertexAreReported)
{
    SkinnedStrip asset;
    asset.attributes = R"("POSITION": 0, "NORMAL": 1, "JOINTS_0": 2, "WEIGHTS_0": 3, "JOINTS_1": 2, "WEIGHTS_1": 3)";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 1) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0: only the four joints of JOINTS_0 move a vertex, JOINTS_1 and its weights are ignored")) << warnings;

    // The mesh is skinned with the joints that are read
    EXPECT_EQ (model.getMeshes()[0]->getNumJoints(), 2);
    EXPECT_TRUE (model.instantiate (scene).findChild ("Strip").has<DeformNode>());
}

TEST_F (GltfModelTests, FailsOnSkinsReferringToWhatDoesNotExist)
{
    // tinygltf rejects every index out of range: the nodes a skin names as joints, and its matrices
    SkinnedStrip missingJoint;
    missingJoint.skin = R"({ "joints": [ 1, 9 ] })";
    EXPECT_TRUE (loadJson (missingJoint.toJson()).failed());

    SkinnedStrip missingMatrices;
    missingMatrices.skin = R"({ "joints": [ 1, 2 ], "inverseBindMatrices": 99 })";
    EXPECT_TRUE (loadJson (missingMatrices.toJson()).failed());

    // The node asks for the first skin, and there is none
    EXPECT_TRUE (loadJson (SkinnedStrip().toJson().replace (R"("skins": [)", R"("unused": [)")).failed());
}

TEST_F (GltfModelTests, JointsTheSceneDoesNotHoldStandAtTheOrigin)
{
    // The scene only holds the node drawing the strip: its joints are nodes of no scene
    SkinnedStrip asset;
    asset.sceneNodes = "0";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto instance = result.getReference().instantiate (scene);
    const auto strip = instance.findChild ("Strip");
    ASSERT_TRUE (strip.has<DeformNode>());

    // Root leaves the foot where it is. Tip, bound 1 up, takes the top 1 down
    expectNear (getDrawnVertices (strip)[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (strip)[2].position, { 0.0f, 1.0f, 0.0f });

    // The clip has no entity to turn there
    ASSERT_TRUE (instance.has<AnimationNode>());
    instance.get<AnimationNode>()->setPlaying (true);
    scene.update (1.0);
    expectNear (getDrawnVertices (strip)[2].position, { 0.0f, 1.0f, 0.0f });
}

//==============================================================================

TEST_F (GltfModelTests, LoadsMorphTargetsAndTheWeightsOfTheirMesh)
{
    auto result = loadJson (MorphedTriangle().toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");
    EXPECT_TRUE (model.getSkins().empty());

    const auto& mesh = *model.getMeshes()[0];
    const auto& targets = mesh.getPrimitive (0).morphTargets;
    ASSERT_EQ (targets.size(), 2u);
    EXPECT_EQ (mesh.getNumMorphTargets(), 2);

    // The first target has deltas for the positions and for the normals
    ASSERT_EQ (targets[0].positions.size(), 3u);
    ASSERT_EQ (targets[0].normals.size(), 3u);
    EXPECT_TRUE (targets[0].tangents.empty());
    expectNear (targets[0].positions[0], { 0.0f, 0.0f, 0.0f });
    expectNear (targets[0].positions[2], { 0.0f, 1.0f, 0.0f });
    expectNear (targets[0].normals[2], { 0.0f, 1.0f, -1.0f });

    // The second is sparse: the file only has the delta of its second vertex
    ASSERT_EQ (targets[1].positions.size(), 3u);
    EXPECT_TRUE (targets[1].normals.empty());
    expectNear (targets[1].positions[0], { 0.0f, 0.0f, 0.0f });
    expectNear (targets[1].positions[1], { 2.0f, 0.0f, 0.0f });
    expectNear (targets[1].positions[2], { 0.0f, 0.0f, 0.0f });

    const auto weights = mesh.getMorphWeights();
    ASSERT_EQ (weights.size(), 2u);
    EXPECT_FLOAT_EQ (weights[0], 0.5f);
    EXPECT_FLOAT_EQ (weights[1], 0.25f);

    // An instance starts with the weights of the mesh, and shows them as soon as it is made: half the
    // lift of the last vertex, its normal halfway from z to y, and a quarter of the move of the second
    const auto face = model.instantiate (scene).findChild ("Face");
    const auto* deform = face.get<DeformNode>();
    ASSERT_NE (deform, nullptr);
    EXPECT_EQ (deform->getSkin(), nullptr);
    ASSERT_EQ (deform->getWeights().size(), 2u);
    EXPECT_FLOAT_EQ (deform->getWeights()[0], 0.5f);
    EXPECT_FLOAT_EQ (deform->getWeights()[1], 0.25f);

    const auto& vertices = getDrawnVertices (face);
    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (vertices[1].position, { 1.5f, 0.0f, 0.0f });
    expectNear (vertices[2].position, { 0.0f, 1.5f, 0.0f });
    expectNear (vertices[2].normal, { 0.0f, 0.70710678f, 0.70710678f });
    expectNear (vertices[0].normal, { 0.0f, 0.0f, 1.0f });

    // The mesh of the model is not the one that is deformed
    expectNear (mesh.getPrimitive (0).vertices[2].position, { 0.0f, 1.0f, 0.0f });
    EXPECT_EQ (mesh.getChangeCount(), 0u);
}

TEST_F (GltfModelTests, TheWeightsOfANodeReplaceTheOnesOfItsMesh)
{
    MorphedTriangle asset;
    asset.node = R"(, "weights": [ 1.0, 0.0 ])";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");
    auto instance = model.instantiate (scene);

    // Face has weights of its own, and shows them at once: its last vertex is lifted by 1, its second stays
    const auto face = instance.findChild ("Face");
    ASSERT_TRUE (face.has<DeformNode>());
    EXPECT_FLOAT_EQ (face.get<DeformNode>()->getWeights()[0], 1.0f);
    EXPECT_FLOAT_EQ (face.get<DeformNode>()->getWeights()[1], 0.0f);
    expectNear (getDrawnVertices (face)[1].position, { 1.0f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (face)[2].position, { 0.0f, 2.0f, 0.0f });

    // Twin draws the same mesh with the weights of the mesh, which are still the ones it had
    const auto twin = instance.findChild ("Twin");
    ASSERT_TRUE (twin.has<DeformNode>());
    EXPECT_FLOAT_EQ (twin.get<DeformNode>()->getWeights()[0], 0.5f);
    EXPECT_FLOAT_EQ (twin.get<DeformNode>()->getWeights()[1], 0.25f);
    expectNear (getDrawnVertices (twin)[2].position, { 0.0f, 1.5f, 0.0f });
    EXPECT_FLOAT_EQ (model.getMeshes()[0]->getMorphWeights()[0], 0.5f);
}

TEST_F (GltfModelTests, WeightsTracksMorphTheMeshOfTheirNode)
{
    auto result = loadJson (MorphedTriangle().toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    ASSERT_EQ (model.getAnimations().size(), 1u);

    auto instance = model.instantiate (scene);
    const auto face = instance.findChild ("Face");
    const auto twin = instance.findChild ("Twin");
    auto* animation = instance.get<AnimationNode>();
    ASSERT_NE (animation, nullptr);

    // In 2 seconds the weights of Face go from 0 and 0 to 1 and 0.5: after 1.5 they are 0.75 and 0.375,
    // which lifts the last vertex by 0.75 and moves the second by 0.75
    animation->setPlaying (true);
    scene.update (1.5);

    EXPECT_FLOAT_EQ (face.get<DeformNode>()->getWeights()[0], 0.75f);
    EXPECT_FLOAT_EQ (face.get<DeformNode>()->getWeights()[1], 0.375f);
    expectNear (getDrawnVertices (face)[1].position, { 1.75f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (face)[2].position, { 0.0f, 1.75f, 0.0f });

    // Twin draws the same mesh and is not animated: it keeps the weights of the mesh
    expectNear (getDrawnVertices (twin)[1].position, { 1.5f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (twin)[2].position, { 0.0f, 1.5f, 0.0f });

    // The clip ends on its last key
    scene.update (1.0);

    EXPECT_FALSE (animation->isPlaying());
    expectNear (getDrawnVertices (face)[1].position, { 2.0f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (face)[2].position, { 0.0f, 2.0f, 0.0f });
}

TEST_F (GltfModelTests, MorphTargetsOfAPrimitiveWithoutNormalsStayFlat)
{
    // Without normals the triangle is flat, facing +z. Its only target takes the last vertex from
    // (0, 1, 0) to (0, 0, 1), where the triangle faces -y
    MorphedTriangle asset;
    asset.attributes = R"("POSITION": 0)";
    asset.targets = R"({ "POSITION": 2 })";
    asset.lift = toBlock<float> ({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f });
    asset.mesh = "";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // The target gets the deltas that take the normals from the one to the other
    const auto& model = result.getReference();
    const auto& targets = model.getMeshes()[0]->getPrimitive (0).morphTargets;
    ASSERT_EQ (targets.size(), 1u);
    ASSERT_EQ (targets[0].normals.size(), 3u);

    for (const auto& delta : targets[0].normals)
        expectNear (delta, { 0.0f, -1.0f, -1.0f });

    const auto face = model.instantiate (scene).findChild ("Face");
    ASSERT_TRUE (face.has<DeformNode>());
    face.get<DeformNode>()->setWeights (std::vector<float> { 1.0f });
    scene.update (0.0);

    const auto& vertices = getDrawnVertices (face);
    ASSERT_EQ (vertices.size(), 3u);
    expectNear (vertices[2].position, { 0.0f, 0.0f, 1.0f });

    for (const auto& vertex : vertices)
        expectNear (vertex.normal, { 0.0f, -1.0f, 0.0f });
}

TEST_F (GltfModelTests, MorphTargetsThatCannotBeUsedAreReported)
{
    // The position deltas of the first target are those of two vertices, not three, and the second
    // target also morphs texture coordinates, colors and an attribute of its own
    MorphedTriangle asset;
    asset.targets = R"({ "POSITION": 7, "NORMAL": 3 }, { "POSITION": 4, "TEXCOORD_0": 3, "COLOR_0": 3, "_CUSTOM": 3 })";
    asset.extraAccessors = { R"({ "componentType": 5126, "count": 2, "type": "VEC3" })" };

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 3) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0 target 0: invalid POSITION, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0 target 1: morphing TEXCOORD_0 is not supported, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0 target 1: morphing COLOR_0 is not supported, ignored")) << warnings;

    // Each target keeps what can be used of it, and its place: the weights are still the ones of their target
    const auto& targets = model.getMeshes()[0]->getPrimitive (0).morphTargets;
    ASSERT_EQ (targets.size(), 2u);
    EXPECT_TRUE (targets[0].positions.empty());
    EXPECT_EQ (targets[0].normals.size(), 3u);
    EXPECT_EQ (targets[1].positions.size(), 3u);

    // With the weights of the mesh, 0.5 and 0.25: the first target only leans the normal of the last
    // vertex, the second still moves the second vertex
    const auto face = model.instantiate (scene).findChild ("Face");
    expectNear (getDrawnVertices (face)[1].position, { 1.5f, 0.0f, 0.0f });
    expectNear (getDrawnVertices (face)[2].position, { 0.0f, 1.0f, 0.0f });
    expectNear (getDrawnVertices (face)[2].normal, { 0.0f, 0.70710678f, 0.70710678f });
}

TEST_F (GltfModelTests, MorphDeltasThatAreNotFiniteAreReported)
{
    // The first target would take the last vertex nowhere
    MorphedTriangle asset;
    asset.lift = toBlock<float> ({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, std::numeric_limits<float>::quiet_NaN(), 0.0f });

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 1) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0 target 0: invalid POSITION, ignored")) << warnings;

    // The target keeps its place and the deltas of its normals
    const auto& targets = model.getMeshes()[0]->getPrimitive (0).morphTargets;
    ASSERT_EQ (targets.size(), 2u);
    EXPECT_TRUE (targets[0].positions.empty());
    EXPECT_EQ (targets[0].normals.size(), 3u);

    // With the weights of the mesh, the last vertex stays where it is
    const auto face = model.instantiate (scene).findChild ("Face");
    expectNear (getDrawnVertices (face)[2].position, { 0.0f, 1.0f, 0.0f });
}

TEST_F (GltfModelTests, ReadsTheNamesOfMorphTargetsFromTheExtrasOfTheirMesh)
{
    // glTF has no place for them: exporters write them there. The mesh has two targets, so a third name is not kept
    MorphedTriangle asset;
    asset.mesh = R"(, "weights": [ 0.5, 0.25 ], "extras": { "targetNames": [ "Lift", "Slide", "Spare" ] })";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");

    const auto& mesh = *model.getMeshes()[0];
    EXPECT_EQ (mesh.getMorphTargetNames().joinIntoString (", "), "Lift, Slide");
    EXPECT_EQ (mesh.findMorphTarget ("Lift"), 0);
    EXPECT_EQ (mesh.findMorphTarget ("Slide"), 1);
    EXPECT_EQ (mesh.findMorphTarget ("Spare"), -1);

    // A mesh without them has targets without names
    auto unnamed = loadJson (MorphedTriangle().toJson());
    ASSERT_TRUE (unnamed.wasOk()) << unnamed.getErrorMessage();
    EXPECT_TRUE (unnamed.getReference().getMeshes()[0]->getMorphTargetNames().isEmpty());
}

TEST_F (GltfModelTests, WeightsTracksWiderThanTheMorphTargetsOfTheirMeshAreSkipped)
{
    // For a mesh of two morph targets: keys of three weights (accessor 7), then keys of 33 million weights
    // (accessor 8), which an accessor without a view only has to state. Neither is read. The channel whose
    // keys have two weights is played
    MorphedTriangle asset;
    asset.extraAccessors = { R"({ "componentType": 5126, "count": 6, "type": "SCALAR" })",
                             R"({ "componentType": 5126, "count": 67108000, "type": "SCALAR" })" };
    asset.samplers = R"({ "input": 5, "output": 6 }, { "input": 5, "output": 7 }, { "input": 5, "output": 8 })";
    asset.channels = R"({ "sampler": 1, "target": { "node": 0, "path": "weights" } },
                        { "sampler": 2, "target": { "node": 0, "path": "weights" } },
                        { "sampler": 0, "target": { "node": 0, "path": "weights" } })";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 2) << warnings;
    EXPECT_TRUE (warnings.contains ("Animation 0 channel 0: 3 weights for the 2 morph targets of the mesh, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Animation 0 channel 1: 33554000 weights for the 2 morph targets of the mesh, ignored")) << warnings;

    ASSERT_EQ (model.getAnimations().size(), 1u);
    ASSERT_EQ (model.getAnimations()[0]->getTracks().size(), 1u);
    EXPECT_EQ (model.getAnimations()[0]->getTracks()[0].path, AnimationClip::Path::weights);
    EXPECT_EQ (model.getAnimations()[0]->getTracks()[0].getNumComponents(), 2);
}

TEST_F (GltfModelTests, ModelsKeepNoMoreThanTheirBudgetOfValues)
{
    // A model keeps 2^26 values at most, as long as its buffers are small, and an accessor without a view
    // is only a count. Accessor 3 states 2^26 - 8 values: with its two times, a channel would keep it
    // alone. Accessor 4 states 2^22 + 1 vertices, and a vertex takes 16 values: more than a model keeps.
    // Accessor 5 states 4 200 000 indices, which each take a vertex of their own in a primitive without
    // normals. None is ever read
    Accessors accessors;
    accessors.add (toBlock<float> ({ 0.0f, 1.0f }), 5126, "SCALAR", 2);
    accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f }), 5126, "VEC3", 2);
    accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }), 5126, "VEC3", 3);
    accessors.addWithoutView (5126, "SCALAR", 67108856);
    accessors.addWithoutView (5126, "VEC3", 4194305);
    accessors.addWithoutView (5125, "SCALAR", 4200000);

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Triangle", "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 2 } },
                                      { "attributes": { "POSITION": 4 } },
                                      { "attributes": { "POSITION": 2 }, "indices": 5 } ] } ],
        "animations": [ { "channels": [ { "sampler": 0, "target": { "node": 0, "path": "translation" } },
                                        { "sampler": 1, "target": { "node": 0, "path": "translation" } },
                                        { "sampler": 1, "target": { "node": 0, "path": "rotation" } },
                                        { "sampler": 0, "target": { "node": 0, "path": "scale" } } ],
                          "samplers": [ { "input": 0, "output": 1 }, { "input": 0, "output": 3 } ] } ],
        $ACCESSORS
    })")
                          .replace ("$ACCESSORS", accessors.toJson());

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // The triangle took 51 values, 16 for each vertex and 1 for each index, and the first channel 8, for
    // its two times and its two translations: the 2^26 - 6 values of the next channels no longer fit
    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 4) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 1: over the 67108864 values a model can keep, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 2: over the 67108864 values a model can keep, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Animation 0 channel 1: over the 67108864 values a model can keep, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Animation 0 channel 2: over the 67108864 values a model can keep, ignored")) << warnings;

    // What fits is kept, also after something was left out
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 1);
    ASSERT_EQ (model.getAnimations().size(), 1u);

    const auto tracks = model.getAnimations()[0]->getTracks();
    ASSERT_EQ (tracks.size(), 2u);
    EXPECT_EQ (tracks[0].path, AnimationClip::Path::translation);
    EXPECT_EQ (tracks[1].path, AnimationClip::Path::scale);
    EXPECT_EQ (tracks[1].values, (std::vector<float> { 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f }));
}

TEST_F (GltfModelTests, NodesDeformingAMeshTakeTheirCopyOutOfTheBudget)
{
    // A file without buffers. The 19 998 vertices of its mesh are an accessor without a view, which also
    // gives its morph target a delta for each of them: the mesh takes 399 960 values, 17 for each vertex
    // with its index and 3 for each delta. Every node drawing it deforms a copy of its own, which takes
    // 659 935: 32 for each vertex, the copy and the one it works on, 1 for each index and 1 for the weight
    // of the target. Of the 2^26 values such a model can keep, less the 872 its second scene takes first,
    // that leaves room for 101 nodes
    StringArray nodes, all;

    for (int i = 0; i < 104; ++i)
    {
        nodes.add (R"({ "name": "Copy)" + String (i) + R"(", "mesh": 0 })");
        all.add (String (i));
    }

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scene": 0,
        "scenes": [ { "nodes": [ $ALL ] }, { "nodes": [ 0, 100, 101, 103 ] } ],
        "nodes": [ $NODES ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 }, "targets": [ { "POSITION": 0 } ] } ] } ],
        "accessors": [ { "componentType": 5126, "count": 19998, "type": "VEC3" } ]
    })")
                          .replace ("$ALL", all.joinIntoString (", "))
                          .replace ("$NODES", nodes.joinIntoString (", "));

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // The last three nodes are told, and draw the mesh as it is
    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 3) << warnings;
    EXPECT_TRUE (warnings.contains ("Node 101 deforming mesh 0: over the 67108864 values a model can keep, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Node 102 deforming mesh 0: over the 67108864 values a model can keep, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Node 103 deforming mesh 0: over the 67108864 values a model can keep, ignored")) << warnings;

    // The second scene holds the first node, the last that fits and two that don't: an instance of the
    // first scene would take all the budget lets it
    const auto instance = model.instantiate (scene, {}, 1);
    const auto& mesh = model.getMeshes()[0];

    for (const auto* name : { "Copy0", "Copy100" })
    {
        const auto copy = instance.findChild (name);
        ASSERT_TRUE (copy.has<DeformNode>()) << name;
        EXPECT_EQ (copy.get<DeformNode>()->getSourceMesh(), mesh) << name;
        EXPECT_NE (copy.get<MeshNode>()->mesh, mesh) << name;
    }

    for (const auto* name : { "Copy101", "Copy103" })
    {
        const auto copy = instance.findChild (name);
        EXPECT_FALSE (copy.has<DeformNode>()) << name;
        EXPECT_EQ (copy.get<MeshNode>()->mesh, mesh) << name;
    }
}

TEST_F (GltfModelTests, MorphTargetsThatHoldNoDeltaDeformNothing)
{
    MorphedTriangle asset;
    asset.targets = "{}, {}";

    auto result = loadJson (asset.toJson());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // The targets are there, for the weights of the mesh and of the clip, and move nothing
    const auto& model = result.getReference();
    EXPECT_TRUE (model.getWarnings().isEmpty()) << model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getMeshes()[0]->getNumMorphTargets(), 2);

    // So the nodes draw the mesh of the model, without a copy to deform
    auto instance = model.instantiate (scene);
    const auto face = instance.findChild ("Face");
    EXPECT_FALSE (face.has<DeformNode>());
    EXPECT_EQ (face.get<MeshNode>()->mesh, model.getMeshes()[0]);

    // The clip animating their weights has nothing to set them on
    ASSERT_TRUE (instance.has<AnimationNode>());
    instance.get<AnimationNode>()->setPlaying (true);
    scene.update (1.0);
    expectNear (getDrawnVertices (face)[2].position, { 0.0f, 1.0f, 0.0f });
}

TEST_F (GltfModelTests, AccessorsAreOnlyReadOnceTheirSizeIsKnownToFit)
{
    // Three accessors without a view, which only state what they hold: 2^24 VEC4 (1), 22 369 621 VEC3 (2)
    // and 2^22 MAT4 (3), about 2^26 values each, 512 MB once read as doubles. A file can use them any
    // number of times. Here they stand for the normals, the joints, the weights and the morph deltas of
    // triangles, which have three vertices, and for the inverse bind matrices of skins of one joint,
    // which would keep one matrix of them. None is read: all of this would take gigabytes otherwise
    Accessors accessors;
    accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }), 5126, "VEC3", 3);
    accessors.addWithoutView (5126, "VEC4", 16777216);
    accessors.addWithoutView (5126, "VEC3", 22369621);
    accessors.addWithoutView (5126, "MAT4", 4194304);

    const String primitive = R"({ "attributes": { "POSITION": 0, "NORMAL": 2, "JOINTS_0": 1, "WEIGHTS_0": 1 },
                                  "targets": [ { "POSITION": 2 }, { "NORMAL": 2, "TANGENT": 2 } ] })";

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Triangles", "mesh": 0 } ],
        "meshes": [ { "primitives": [ $PRIMITIVE, $PRIMITIVE ] } ],
        "skins": [ { "joints": [ 0 ], "inverseBindMatrices": 3 }, { "joints": [ 0 ], "inverseBindMatrices": 3 } ],
        $ACCESSORS
    })")
                          .replace ("$PRIMITIVE", primitive)
                          .replace ("$ACCESSORS", accessors.toJson());

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 12) << warnings;

    // What has another size than the vertices is told to be invalid without being read
    for (const auto* primitiveWhere : { "Mesh 0 primitive 0", "Mesh 0 primitive 1" })
    {
        const auto where = String (primitiveWhere);
        EXPECT_TRUE (warnings.contains (where + ": invalid NORMAL, ignored")) << warnings;
        EXPECT_TRUE (warnings.contains (where + ": invalid JOINTS_0 or WEIGHTS_0, the primitive is not skinned")) << warnings;
        EXPECT_TRUE (warnings.contains (where + " target 0: invalid POSITION, ignored")) << warnings;
        EXPECT_TRUE (warnings.contains (where + " target 1: invalid NORMAL, ignored")) << warnings;
        EXPECT_TRUE (warnings.contains (where + " target 1: invalid TANGENT, ignored")) << warnings;
    }

    // Inverse bind matrices are read whole, so all of them count: here for more than the model can keep
    EXPECT_TRUE (warnings.contains ("Skin 0: over the 67108864 values a model can keep, ignored")) << warnings;
    EXPECT_TRUE (warnings.contains ("Skin 1: over the 67108864 values a model can keep, ignored")) << warnings;
    ASSERT_EQ (model.getSkins().size(), 2u);
    EXPECT_EQ (model.getSkins()[0], nullptr);
    EXPECT_EQ (model.getSkins()[1], nullptr);

    // The triangles are there, with nothing to deform them
    const auto& mesh = *model.getMeshes()[0];
    ASSERT_EQ (mesh.getNumPrimitives(), 2);
    EXPECT_TRUE (mesh.getPrimitive (1).influences.empty());
    ASSERT_EQ (mesh.getPrimitive (1).morphTargets.size(), 2u);
    EXPECT_TRUE (mesh.getPrimitive (1).morphTargets[0].positions.empty());
    EXPECT_TRUE (mesh.getPrimitive (1).morphTargets[1].normals.empty());
    EXPECT_FALSE (model.instantiate (scene).findChild ("Triangles").has<DeformNode>());
}

TEST_F (GltfModelTests, ScenesTakeTheirPartsOutOfTheBudget)
{
    // A file without buffers. Its mesh is nothing but 3 946 368 indices, an accessor without a view that
    // is never read: they are counted before the primitive is found to have no positions, 17 values each
    // as for the vertices they would take, which leaves 20 608 of the 2^26 values a model can keep. Each
    // of its 8 scenes holds the same 50 nodes. The default one, the last, is counted first and takes
    // nothing: what it keeps is what the file states. Each of the others takes 8 178 values: 128 for
    // itself, 1 for each node of the model, and 160 for each part it makes of a node another scene has
    // already. Two of those fit
    StringArray scenes, children, leaves;

    for (int i = 0; i < 8; ++i)
        scenes.add (R"({ "nodes": [ 0 ] })");

    for (int i = 1; i < 50; ++i)
    {
        children.add (String (i));
        leaves.add ("{}");
    }

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scene": 7,
        "scenes": [ $SCENES ],
        "nodes": [ { "children": [ $CHILDREN ] }, $LEAVES ],
        "meshes": [ { "primitives": [ { "indices": 0 } ] } ],
        "accessors": [ { "componentType": 5125, "count": 3946368, "type": "SCALAR" } ]
    })")
                          .replace ("$SCENES", scenes.joinIntoString (", "))
                          .replace ("$CHILDREN", children.joinIntoString (", "))
                          .replace ("$LEAVES", leaves.joinIntoString (", "));

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 6) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 0: missing or invalid POSITION")) << warnings;

    // The scenes that are left out keep their place among the others, without a prefab
    EXPECT_EQ (model.getNumScenes(), 8);

    for (int i = 0; i < 8; ++i)
    {
        const auto built = i < 2 || i == 7;
        EXPECT_EQ (model.getPrefab (i) != nullptr, built) << i;
        EXPECT_EQ (warnings.contains ("Scene " + String (i) + ": over the 67108864 values a model can keep, ignored"), ! built) << warnings;
    }

    // The scene the model shows when none is asked for is built as ever: its root node, and the 49
    // others under it
    EXPECT_EQ (model.getPrefab(), model.getPrefab (7));

    const auto instance = model.instantiate (scene);
    ASSERT_TRUE (instance.isValid());
    ASSERT_EQ (instance.getNumChildren(), 1);
    EXPECT_EQ (instance.getChild (0).getNumChildren(), 49);

    // So is one of the others that fit, and one that is left out has nothing to instantiate
    EXPECT_TRUE (model.instantiate (scene, {}, 1).isValid());
    EXPECT_FALSE (model.instantiate (scene, {}, 5).isValid());
}

TEST_F (GltfModelTests, AModelThatUsedItsBudgetUpStillShowsItsScene)
{
    // The triangle takes 51 values, and the indices of the second primitive, counted as in the test above,
    // all the others of the 2^26 but 4. The scene the model shows takes none: it keeps what the file states
    Accessors accessors;
    accessors.add (toBlock<float> ({ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }), 5126, "VEC3", 3);
    accessors.addWithoutView (5125, "SCALAR", 3947577);

    const auto json = String (R"({
        "asset": { "version": "2.0" },
        "scenes": [ { "nodes": [ 0 ] } ],
        "nodes": [ { "name": "Triangle", "mesh": 0 } ],
        "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 } }, { "indices": 1 } ] } ],
        $ACCESSORS
    })")
                          .replace ("$ACCESSORS", accessors.toJson());

    auto result = loadJson (json);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& model = result.getReference();
    const auto warnings = model.getWarnings().joinIntoString ("\n");
    EXPECT_EQ (model.getWarnings().size(), 1) << warnings;
    EXPECT_TRUE (warnings.contains ("Mesh 0 primitive 1: missing or invalid POSITION")) << warnings;

    // The model shows what fit: its node, drawing the triangle
    ASSERT_NE (model.getPrefab(), nullptr);
    EXPECT_EQ (model.getMeshes()[0]->getNumPrimitives(), 1);

    const auto triangle = model.instantiate (scene).findChild ("Triangle");
    ASSERT_TRUE (triangle.has<MeshNode>());
    EXPECT_EQ (triangle.get<MeshNode>()->mesh, model.getMeshes()[0]);
}

TEST_F (GltfModelTests, AModelKeepsTwoHundredWarningsAtMost)
{
    // 300 primitives of a mode that is not converted, each with a warning of its own
    StringArray primitives;

    for (int i = 0; i < 300; ++i)
        primitives.add (R"({ "mode": 1 })");

    auto result = loadJson (String (R"({ "asset": { "version": "2.0" }, "meshes": [ { "primitives": [ $PRIMITIVES ] } ] })")
                                .replace ("$PRIMITIVES", primitives.joinIntoString (", ")));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    // The first 199 are kept as they came, and the last one tells that there were more
    const auto& warnings = result.getReference().getWarnings();
    ASSERT_EQ (warnings.size(), 200);
    EXPECT_EQ (warnings[0], "Mesh 0 primitive 0: mode 1 is not supported, only triangles are");
    EXPECT_EQ (warnings[198], "Mesh 0 primitive 198: mode 1 is not supported, only triangles are");
    EXPECT_EQ (warnings[199], "More than 199 warnings: the next ones are left out");
}
