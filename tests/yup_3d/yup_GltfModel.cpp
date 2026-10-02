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

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }
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

    auto root = model.createEntity();
    ASSERT_NE (root, nullptr);
    EXPECT_EQ (root->getName(), "Main");
    ASSERT_EQ (root->getNumChildren(), 1);

    auto* triangle = root->getChild (0);
    EXPECT_EQ (triangle->getName(), "Triangle");
    ASSERT_NE (triangle->getNode<MeshNode>(), nullptr);
    EXPECT_EQ (triangle->getNode<MeshNode>()->mesh, model.getMeshes()[0]);
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
    EXPECT_EQ (model.getMeshes()[0]->getPrimitive (0).indices, (std::vector<uint32> { 2, 1, 0 }));

    // A scene without a default scene index: the first scene is built
    EXPECT_NE (model.createEntity(), nullptr);

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

    auto root = result.getReference().createEntity();
    ASSERT_NE (root, nullptr);
    EXPECT_EQ (root->getName(), "Rig");

    auto* parent = root->findChild ("Parent", false);
    ASSERT_NE (parent, nullptr);
    auto* child = parent->findChild ("Child", false);
    ASSERT_NE (child, nullptr);

    expectNear (parent->getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (parent->getScale(), { 2.0f, 2.0f, 2.0f });
    EXPECT_TRUE (parent->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi)));

    // (0, 0, -1) scaled by 2, turned 90 degrees around Y to (-2, 0, 0), moved by (1, 2, 3)
    expectNear (child->getWorldMatrix().transformPoint ({}), { -1.0f, 2.0f, 3.0f });
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

    auto root = result.getReference().createEntity();
    auto* node = root->findChild ("Matrix");
    ASSERT_NE (node, nullptr);

    expectNear (node->getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (node->getScale(), { 2.0f, 3.0f, 4.0f });
    EXPECT_TRUE (node->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, MathConstants<float>::halfPi)));
    EXPECT_TRUE (node->getLocalMatrix().approximatelyEqualTo (expected, 1.0e-4f));
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

    const auto& sampler = result.getReference().getMaterials()[0]->baseColorTexture->getSamplerDesc();
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

    auto root = result.getReference().createEntity();

    auto* lamp = root->findChild ("Lamp")->getNode<LightNode>();
    ASSERT_NE (lamp, nullptr);
    EXPECT_EQ (lamp->type, LightNode::Type::spot);
    EXPECT_FLOAT_EQ (lamp->color[1], 0.5f);
    EXPECT_FLOAT_EQ (lamp->intensity, 3.0f);
    EXPECT_FLOAT_EQ (lamp->range, 10.0f);
    EXPECT_FLOAT_EQ (lamp->innerConeAngle, 0.1f);
    EXPECT_FLOAT_EQ (lamp->outerConeAngle, 0.5f);

    auto* eye = root->findChild ("Eye")->getNode<CameraNode>();
    ASSERT_NE (eye, nullptr);
    EXPECT_EQ (eye->projection, CameraNode::Projection::perspective);
    EXPECT_FLOAT_EQ (eye->yFov, 0.5f);
    EXPECT_FLOAT_EQ (eye->zNear, 0.25f);
    EXPECT_FLOAT_EQ (eye->zFar, 0.0f);

    auto* top = root->findChild ("Top")->getNode<CameraNode>();
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
    auto first = model.createEntity();
    auto second = model.createEntity();

    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);
    EXPECT_NE (first.get(), second.get());
    EXPECT_NE (first->getChild (0), second->getChild (0));
    EXPECT_EQ (first->getChild (0)->getNode<MeshNode>()->mesh, second->getChild (0)->getNode<MeshNode>()->mesh);

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
    EXPECT_EQ (model.createEntity()->getName(), "Second");
    EXPECT_EQ (model.createEntity (0)->getName(), "First");
    EXPECT_EQ (model.createEntity (0)->getChild (0)->getName(), "A");
    EXPECT_EQ (model.createEntity (2), nullptr);
    EXPECT_EQ (model.createEntity (-2), nullptr);
}

TEST_F (GltfModelTests, AssetsWithoutScenesBuildTheirRootNodes)
{
    auto result = loadJson (R"({
        "asset": { "version": "2.0" },
        "nodes": [ { "name": "A", "children": [ 1 ] }, { "name": "B" }, { "name": "C" } ]
    })");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto root = result.getReference().createEntity();
    ASSERT_NE (root, nullptr);
    ASSERT_EQ (root->getNumChildren(), 2);
    EXPECT_EQ (root->getChild (0)->getName(), "A");
    EXPECT_EQ (root->getChild (1)->getName(), "C");
    EXPECT_EQ (root->getChild (0)->getChild (0)->getName(), "B");
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
        auto root = result.getReference().createEntity();
        ASSERT_NE (root, nullptr);
        EXPECT_EQ (root->getChild (0)->getChild (0)->getNumChildren(), 0);
    }
}
