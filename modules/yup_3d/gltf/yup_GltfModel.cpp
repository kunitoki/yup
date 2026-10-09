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

struct GltfModel::Data
{
    struct NodeDesc
    {
        String name;
        Vector3<float> position;
        Quaternion rotation;
        Vector3<float> scale { 1.0f, 1.0f, 1.0f };
        int mesh = -1;
        int camera = -1;
        int light = -1;
        std::vector<int> children;
    };

    struct CameraDesc
    {
        CameraNode::Projection projection = CameraNode::Projection::perspective;
        float yFov = 0.8f;
        float zNear = 0.1f;
        float zFar = 0.0f;
        float xMag = 1.0f;
        float yMag = 1.0f;
    };

    struct LightDesc
    {
        LightNode::Type type = LightNode::Type::directional;
        std::array<float, 3> color { 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        float range = 0.0f;
        float innerConeAngle = 0.0f;
        float outerConeAngle = MathConstants<float>::pi * 0.25f;
    };

    struct SceneDesc
    {
        String name;
        std::vector<int> nodes;
    };

    std::vector<Mesh::Ptr> meshes;
    std::vector<Material::Ptr> materials;
    std::vector<Texture::Ptr> textures;
    std::vector<NodeDesc> nodes;
    std::vector<CameraDesc> cameras;
    std::vector<LightDesc> lights;
    std::vector<SceneDesc> scenes;
    std::vector<int> parentlessNodes;
    int defaultScene = -1;
    StringArray warnings;
};

//==============================================================================

namespace
{

String gltfToString (tg3_str text)
{
    return text.data != nullptr ? String::fromUTF8 (text.data, static_cast<int> (text.len)) : String();
}

/** Resolves a relative URI against the base directory, or returns File() when it would escape it. */
File resolveGltfExternalFile (const File& baseDirectory, const String& uri)
{
    if (baseDirectory == File() || uri.isEmpty())
        return {};

    // URL::removeEscapeChars also turns '+' into a space, which isn't what URIs in glTF mean
    const auto path = URL::removeEscapeChars (uri.replace ("+", "%2B")).replaceCharacter ('\\', '/');

    if (path.isEmpty() || File::isAbsolutePath (path) || path.startsWithChar ('/'))
        return {};

    StringArray segments;
    segments.addTokens (path, "/", {});
    if (segments.contains ("..") || segments[0].endsWithChar (':'))
        return {};

    const auto file = baseDirectory.getChildFile (path);
    return file.isAChildOf (baseDirectory) ? file : File();
}

int32_t gltfReadFile (uint8_t** outData, uint64_t* outSize, const char* path, uint32_t pathLength, void* userData)
{
    *outData = nullptr;
    *outSize = 0;

    const auto file = resolveGltfExternalFile (*static_cast<const File*> (userData), String::fromUTF8 (path, static_cast<int> (pathLength)));

    MemoryBlock block;
    if (file == File() || ! file.loadFileAsData (block))
        return 0;

    if (block.getSize() > 0)
    {
        *outData = static_cast<uint8_t*> (std::malloc (block.getSize()));
        if (*outData == nullptr)
            return 0;

        std::memcpy (*outData, block.getData(), block.getSize());
    }

    *outSize = block.getSize();
    return 1;
}

void gltfFreeFile (uint8_t* data, [[maybe_unused]] uint64_t size, [[maybe_unused]] void* userData)
{
    std::free (data);
}

double readGltfComponent (const uint8* source, int componentType, bool normalized) noexcept
{
    switch (componentType)
    {
        case TG3_COMPONENT_TYPE_BYTE:
        {
            int8 v;
            std::memcpy (&v, source, sizeof (v));
            return normalized ? jmax (v / 127.0, -1.0) : v;
        }

        case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
            return normalized ? source[0] / 255.0 : source[0];

        case TG3_COMPONENT_TYPE_SHORT:
        {
            int16 v;
            std::memcpy (&v, source, sizeof (v));
            return normalized ? jmax (v / 32767.0, -1.0) : v;
        }

        case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            uint16 v;
            std::memcpy (&v, source, sizeof (v));
            return normalized ? v / 65535.0 : v;
        }

        case TG3_COMPONENT_TYPE_INT:
        {
            int32 v;
            std::memcpy (&v, source, sizeof (v));
            return v;
        }

        case TG3_COMPONENT_TYPE_UNSIGNED_INT:
        {
            uint32 v;
            std::memcpy (&v, source, sizeof (v));
            return v;
        }

        case TG3_COMPONENT_TYPE_FLOAT:
        {
            float v;
            std::memcpy (&v, source, sizeof (v));
            return v;
        }

        case TG3_COMPONENT_TYPE_DOUBLE:
        {
            double v;
            std::memcpy (&v, source, sizeof (v));
            return v;
        }

        default:
            return 0.0;
    }
}

GpuWrapMode gltfWrapMode (int32_t wrap) noexcept
{
    switch (wrap)
    {
        case TG3_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return GpuWrapMode::clampToEdge;
        case TG3_TEXTURE_WRAP_MIRRORED_REPEAT:
            return GpuWrapMode::mirrorRepeat;
        default:
            return GpuWrapMode::repeat;
    }
}

/** Maps a glTF sampler, or the glTF defaults (repeat, linear, mipmapped) when there is none.

    Fully linear samplers ask for 16x anisotropic filtering: Texture lowers it on devices without it.
*/
GpuSamplerDesc gltfSamplerDesc (const tg3_sampler* sampler) noexcept
{
    GpuSamplerDesc desc (GpuFilter::linear, GpuWrapMode::repeat);
    desc.mipmapFilter = GpuFilter::linear;
    desc.maxAnisotropy = 16;

    if (sampler == nullptr)
        return desc;

    desc.magFilter = sampler->mag_filter == TG3_TEXTURE_FILTER_NEAREST ? GpuFilter::nearest : GpuFilter::linear;
    desc.wrapU = gltfWrapMode (sampler->wrap_s);
    desc.wrapV = gltfWrapMode (sampler->wrap_t);

    switch (sampler->min_filter)
    {
        case TG3_TEXTURE_FILTER_NEAREST:
            desc.minFilter = GpuFilter::nearest;
            desc.maxLod = 0.0f;
            break;

        case TG3_TEXTURE_FILTER_LINEAR:
            desc.maxLod = 0.0f;
            break;

        case TG3_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            desc.minFilter = GpuFilter::nearest;
            desc.mipmapFilter = GpuFilter::nearest;
            break;

        case TG3_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            desc.mipmapFilter = GpuFilter::nearest;
            break;

        case TG3_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            desc.minFilter = GpuFilter::nearest;
            break;

        default:
            break;
    }

    if (desc.minFilter != GpuFilter::linear || desc.magFilter != GpuFilter::linear || desc.mipmapFilter != GpuFilter::linear)
        desc.maxAnisotropy = 1;

    return desc;
}

/** Splits a glTF matrix into translation, rotation and scale. */
void decomposeGltfMatrix (const double* values, Vector3<float>& position, Quaternion& rotation, Vector3<float>& scale)
{
    std::array<float, 16> m {};
    for (size_t i = 0; i < m.size(); ++i)
        m[i] = static_cast<float> (values[i]);

    const Vector3<float> axisX { m[0], m[1], m[2] };
    const Vector3<float> axisY { m[4], m[5], m[6] };
    const Vector3<float> axisZ { m[8], m[9], m[10] };

    position = { m[12], m[13], m[14] };

    // A negative determinant means a mirroring, carried by the x scale
    const auto mirrored = axisX.dotProduct (axisY.crossProduct (axisZ)) < 0.0f;
    scale = { mirrored ? -axisX.length() : axisX.length(), axisY.length(), axisZ.length() };

    if (scale.getX() == 0.0f || scale.getY() == 0.0f || scale.getZ() == 0.0f)
    {
        rotation = Quaternion::identity();
        return;
    }

    const auto x = axisX / scale.getX();
    const auto y = axisY / scale.getY();
    const auto z = axisZ / scale.getZ();

    rotation = Quaternion::fromRotationMatrix (Matrix4 (std::array<float, 16> { x.getX(), x.getY(), x.getZ(), 0.0f,
                                                                                y.getX(), y.getY(), y.getZ(), 0.0f,
                                                                                z.getX(), z.getY(), z.getZ(), 0.0f,
                                                                                0.0f, 0.0f, 0.0f, 1.0f }));
}

} // namespace

//==============================================================================

class GltfModel::Loader
{
public:
    Loader (const tg3_model& modelToConvert, const File& baseDirectoryToUse, Data& dataToFill)
        : model (modelToConvert)
        , baseDirectory (baseDirectoryToUse)
        , data (dataToFill)
        , decodedImages (model.images_count)
        , imageDecoded (model.images_count, false)
    {
    }

    Result load()
    {
        static const char* supportedExtensions[] = { "KHR_lights_punctual", "EXT_texture_webp", "KHR_mesh_quantization" };

        for (uint32_t i = 0; i < model.extensions_required_count; ++i)
        {
            const auto& extension = model.extensions_required[i];
            const auto supported = std::any_of (std::begin (supportedExtensions), std::end (supportedExtensions), [&] (const char* name)
            {
                return tg3_str_equals_cstr (extension, name) != 0;
            });

            if (! supported)
                return Result::fail ("Unsupported required glTF extension: " + gltfToString (extension));
        }

        for (uint32_t i = 0; i < model.materials_count; ++i)
            data.materials.push_back (convertMaterial (model.materials[i]));

        for (uint32_t i = 0; i < model.meshes_count; ++i)
            data.meshes.push_back (convertMesh (model.meshes[i], static_cast<int> (i)));

        for (uint32_t i = 0; i < model.cameras_count; ++i)
            data.cameras.push_back (convertCamera (model.cameras[i]));

        for (uint32_t i = 0; i < model.lights_count; ++i)
            data.lights.push_back (convertLight (model.lights[i]));

        convertNodes();

        for (uint32_t i = 0; i < model.scenes_count; ++i)
        {
            const auto& scene = model.scenes[i];
            data.scenes.push_back ({ gltfToString (scene.name), std::vector<int> (scene.nodes, scene.nodes + scene.nodes_count) });
        }

        data.defaultScene = model.default_scene;
        return Result::ok();
    }

private:
    //==============================================================================
    void warn (const String& message)
    {
        data.warnings.addIfNotAlreadyThere (message);
    }

    //==============================================================================
    bool getBufferView (int index, const uint8*& bytes, uint64& size) const
    {
        if (! isPositiveAndBelow (index, static_cast<int> (model.buffer_views_count)))
            return false;

        const auto& view = model.buffer_views[index];
        if (! isPositiveAndBelow (view.buffer, static_cast<int> (model.buffers_count)))
            return false;

        const auto& buffer = model.buffers[view.buffer];
        if (buffer.data.data == nullptr || view.byte_offset > buffer.data.count || view.byte_length > buffer.data.count - view.byte_offset)
            return false;

        bytes = buffer.data.data + view.byte_offset;
        size = view.byte_length;
        return true;
    }

    /** Reads every element of an accessor as doubles, dense and sparse, with range checks. */
    bool readAccessor (int index, std::vector<double>& values, int& numComponents) const
    {
        if (! isPositiveAndBelow (index, static_cast<int> (model.accessors_count)))
            return false;

        const auto& accessor = model.accessors[index];
        numComponents = tg3_num_components (accessor.type);
        const auto componentSize = static_cast<uint64> (tg3_component_size (accessor.component_type));
        const auto count = accessor.count;

        constexpr uint64 maxValues = 1ull << 26;
        if (numComponents <= 0 || numComponents > 16 || componentSize == 0 || count > maxValues / static_cast<uint64> (numComponents))
            return false;

        const auto elementSize = static_cast<uint64> (numComponents) * componentSize;
        const auto normalized = accessor.normalized != 0;
        values.assign (static_cast<size_t> (count) * static_cast<size_t> (numComponents), 0.0);

        if (accessor.buffer_view >= 0 && count > 0)
        {
            const uint8* bytes = nullptr;
            uint64 size = 0;
            if (! getBufferView (accessor.buffer_view, bytes, size))
                return false;

            const auto byteStride = model.buffer_views[accessor.buffer_view].byte_stride;
            const auto stride = byteStride != 0 ? static_cast<uint64> (byteStride) : elementSize;

            if (stride < elementSize || accessor.byte_offset > size)
                return false;

            const auto available = size - accessor.byte_offset;
            if (available < elementSize || (count - 1) > (available - elementSize) / stride)
                return false;

            for (uint64 i = 0; i < count; ++i)
            {
                const auto* element = bytes + accessor.byte_offset + i * stride;

                for (int c = 0; c < numComponents; ++c)
                    values[static_cast<size_t> (i * static_cast<uint64> (numComponents)) + static_cast<size_t> (c)] = readGltfComponent (element + static_cast<uint64> (c) * componentSize, accessor.component_type, normalized);
            }
        }

        if (accessor.sparse.is_sparse == 0)
            return true;

        const auto& sparse = accessor.sparse;
        const auto sparseCount = static_cast<uint64> (jmax (0, sparse.count));
        const auto indexSize = static_cast<uint64> (tg3_component_size (sparse.indices.component_type));

        const uint8* indexBytes = nullptr;
        const uint8* valueBytes = nullptr;
        uint64 indexViewSize = 0;
        uint64 valueViewSize = 0;

        if (sparseCount > count
            || indexSize == 0
            || ! getBufferView (sparse.indices.buffer_view, indexBytes, indexViewSize)
            || ! getBufferView (sparse.values.buffer_view, valueBytes, valueViewSize)
            || sparse.indices.byte_offset > indexViewSize
            || sparse.values.byte_offset > valueViewSize
            || sparseCount * indexSize > indexViewSize - sparse.indices.byte_offset
            || sparseCount * elementSize > valueViewSize - sparse.values.byte_offset)
        {
            return false;
        }

        for (uint64 k = 0; k < sparseCount; ++k)
        {
            const auto target = readGltfComponent (indexBytes + sparse.indices.byte_offset + k * indexSize, sparse.indices.component_type, false);
            if (target < 0.0 || target >= static_cast<double> (count))
                return false;

            const auto* element = valueBytes + sparse.values.byte_offset + k * elementSize;
            const auto base = static_cast<size_t> (target) * static_cast<size_t> (numComponents);

            for (int c = 0; c < numComponents; ++c)
                values[base + static_cast<size_t> (c)] = readGltfComponent (element + static_cast<uint64> (c) * componentSize, accessor.component_type, normalized);
        }

        return true;
    }

    //==============================================================================
    const Image* getImage (int index)
    {
        if (! isPositiveAndBelow (index, static_cast<int> (model.images_count)))
            return nullptr;

        const auto i = static_cast<size_t> (index);
        if (! imageDecoded[i])
        {
            imageDecoded[i] = true;
            decodedImages[i] = decodeImage (index);
        }

        return decodedImages[i] ? &*decodedImages[i] : nullptr;
    }

    std::optional<Image> decodeImage (int index)
    {
        const auto& image = model.images[index];
        const auto where = "Image " + String (index);

        MemoryBlock bytes;

        if (image.buffer_view >= 0)
        {
            const uint8* viewBytes = nullptr;
            uint64 size = 0;
            if (! getBufferView (image.buffer_view, viewBytes, size))
            {
                warn (where + ": invalid buffer view");
                return std::nullopt;
            }

            bytes.append (viewBytes, static_cast<size_t> (size));
        }
        else
        {
            const auto uri = gltfToString (image.uri);

            if (uri.startsWith ("data:"))
            {
                const auto comma = uri.indexOfChar (',');
                MemoryOutputStream decoded;

                if (comma < 0 || ! uri.substring (0, comma).contains (";base64") || ! Base64::convertFromBase64 (decoded, uri.substring (comma + 1)))
                {
                    warn (where + ": unsupported data URI");
                    return std::nullopt;
                }

                bytes = decoded.getMemoryBlock();
            }
            else
            {
                const auto file = resolveGltfExternalFile (baseDirectory, uri);
                if (file == File())
                {
                    warn (where + ": \"" + uri + "\" is outside the base directory, or there is no base directory");
                    return std::nullopt;
                }

                if (! file.loadFileAsData (bytes))
                {
                    warn (where + ": unable to read " + file.getFullPathName());
                    return std::nullopt;
                }
            }
        }

        auto result = Image::loadFromData (bytes.asBytes());
        if (result.failed())
        {
            warn (where + " (" + gltfToString (image.mime_type) + "): " + result.getErrorMessage());
            return std::nullopt;
        }

        return std::move (result).getValue();
    }

    int findWebpSource (const tg3_extras_ext& ext) const
    {
        for (uint32_t i = 0; i < ext.extensions_count; ++i)
        {
            const auto& extension = ext.extensions[i];
            if (! tg3_str_equals_cstr (extension.name, "EXT_texture_webp") || extension.value.type != TG3_VALUE_OBJECT)
                continue;

            for (uint32_t k = 0; k < extension.value.object_count; ++k)
            {
                const auto& pair = extension.value.object_data[k];
                if (! tg3_str_equals_cstr (pair.key, "source"))
                    continue;

                if (pair.value.type == TG3_VALUE_INT && pair.value.int_val >= 0 && pair.value.int_val < static_cast<int64> (model.images_count))
                    return static_cast<int> (pair.value.int_val);
            }
        }

        return -1;
    }

    Texture::Ptr getTexture (int index, int texCoord, bool srgb)
    {
        if (! isPositiveAndBelow (index, static_cast<int> (model.textures_count)))
            return nullptr;

        if (texCoord != 0)
            warn ("Texture " + String (index) + " uses TEXCOORD_" + String (texCoord) + ", only TEXCOORD_0 is supported");

        const auto key = index * 2 + (srgb ? 1 : 0);
        if (const auto it = textureCache.find (key); it != textureCache.end())
            return it->second;

        const auto& texture = model.textures[index];

        const Image* image = nullptr;
        if (const auto webpSource = findWebpSource (texture.ext); webpSource >= 0)
            image = getImage (webpSource);

        if (image == nullptr)
            image = getImage (texture.source);

        Texture::Ptr result;
        if (image != nullptr)
        {
            const auto* sampler = isPositiveAndBelow (texture.sampler, static_cast<int> (model.samplers_count)) ? &model.samplers[texture.sampler] : nullptr;
            result = new Texture (*image, gltfSamplerDesc (sampler), srgb);
            data.textures.push_back (result);
        }
        else
        {
            warn ("Texture " + String (index) + " has no image that can be decoded");
        }

        textureCache[key] = result;
        return result;
    }

    //==============================================================================
    Material::Ptr convertMaterial (const tg3_material& source)
    {
        auto material = Material::Ptr (new Material());
        const auto& pbr = source.pbr_metallic_roughness;

        material->name = gltfToString (source.name);

        for (size_t i = 0; i < material->baseColorFactor.size(); ++i)
            material->baseColorFactor[i] = static_cast<float> (pbr.base_color_factor[i]);

        material->baseColorTexture = getTexture (pbr.base_color_texture.index, pbr.base_color_texture.tex_coord, true);
        material->metallicFactor = static_cast<float> (pbr.metallic_factor);
        material->roughnessFactor = static_cast<float> (pbr.roughness_factor);
        material->metallicRoughnessTexture = getTexture (pbr.metallic_roughness_texture.index, pbr.metallic_roughness_texture.tex_coord, false);

        material->normalTexture = getTexture (source.normal_texture.index, source.normal_texture.tex_coord, false);
        material->normalScale = static_cast<float> (source.normal_texture.scale);

        material->occlusionTexture = getTexture (source.occlusion_texture.index, source.occlusion_texture.tex_coord, false);
        material->occlusionStrength = static_cast<float> (source.occlusion_texture.strength);

        for (size_t i = 0; i < material->emissiveFactor.size(); ++i)
            material->emissiveFactor[i] = static_cast<float> (source.emissive_factor[i]);

        material->emissiveTexture = getTexture (source.emissive_texture.index, source.emissive_texture.tex_coord, true);

        if (tg3_str_equals_cstr (source.alpha_mode, "MASK"))
            material->alphaMode = Material::AlphaMode::mask;
        else if (tg3_str_equals_cstr (source.alpha_mode, "BLEND"))
            material->alphaMode = Material::AlphaMode::blend;

        material->alphaCutoff = static_cast<float> (source.alpha_cutoff);
        material->doubleSided = source.double_sided != 0;

        return material;
    }

    //==============================================================================
    Mesh::Ptr convertMesh (const tg3_mesh& source, int meshIndex)
    {
        auto mesh = Mesh::Ptr (new Mesh (gltfToString (source.name)));

        for (uint32_t p = 0; p < source.primitives_count; ++p)
        {
            const auto where = "Mesh " + String (meshIndex) + " primitive " + String (p);
            const auto& primitive = source.primitives[p];

            if (primitive.mode != -1 && primitive.mode != TG3_MODE_TRIANGLES)
            {
                warn (where + ": mode " + String (primitive.mode) + " is not supported, only triangles are");
                continue;
            }

            int positionAccessor = -1, normalAccessor = -1, uvAccessor = -1, colorAccessor = -1;
            for (uint32_t a = 0; a < primitive.attributes_count; ++a)
            {
                const auto& attribute = primitive.attributes[a];

                if (tg3_str_equals_cstr (attribute.key, "POSITION"))
                    positionAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "NORMAL"))
                    normalAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "TEXCOORD_0"))
                    uvAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "COLOR_0"))
                    colorAccessor = attribute.value;
            }

            std::vector<double> values;
            int numComponents = 0;

            if (! readAccessor (positionAccessor, values, numComponents) || numComponents != 3)
            {
                warn (where + ": missing or invalid POSITION");
                continue;
            }

            std::vector<Mesh::Vertex> vertices (values.size() / 3);
            for (size_t v = 0; v < vertices.size(); ++v)
                vertices[v].position = { static_cast<float> (values[v * 3]), static_cast<float> (values[v * 3 + 1]), static_cast<float> (values[v * 3 + 2]) };

            const auto readAttribute = [&] (int accessor, const char* name, int minComponents, int maxComponents, auto&& assign)
            {
                if (accessor < 0)
                    return;

                if (! readAccessor (accessor, values, numComponents)
                    || numComponents < minComponents
                    || numComponents > maxComponents
                    || values.size() != vertices.size() * static_cast<size_t> (numComponents))
                {
                    warn (where + ": invalid " + name + ", ignored");
                    return;
                }

                for (size_t v = 0; v < vertices.size(); ++v)
                    assign (vertices[v], values.data() + v * static_cast<size_t> (numComponents));
            };

            readAttribute (normalAccessor, "NORMAL", 3, 3, [] (Mesh::Vertex& vertex, const double* x)
            {
                vertex.normal = { static_cast<float> (x[0]), static_cast<float> (x[1]), static_cast<float> (x[2]) };
            });

            readAttribute (uvAccessor, "TEXCOORD_0", 2, 2, [] (Mesh::Vertex& vertex, const double* x)
            {
                vertex.uv = { static_cast<float> (x[0]), static_cast<float> (x[1]) };
            });

            readAttribute (colorAccessor, "COLOR_0", 3, 4, [&numComponents] (Mesh::Vertex& vertex, const double* x)
            {
                vertex.color = { static_cast<float> (x[0]), static_cast<float> (x[1]), static_cast<float> (x[2]), numComponents == 4 ? static_cast<float> (x[3]) : 1.0f };
            });

            std::vector<uint32> indices;

            if (primitive.indices >= 0)
            {
                if (! readAccessor (primitive.indices, values, numComponents) || numComponents != 1)
                {
                    warn (where + ": invalid indices");
                    continue;
                }

                const auto inRange = std::all_of (values.begin(), values.end(), [&vertices] (double index)
                {
                    return index >= 0.0 && index < static_cast<double> (vertices.size());
                });

                if (! inRange || values.size() < 3 || values.size() % 3 != 0)
                {
                    warn (where + ": indices out of range or not a list of triangles");
                    continue;
                }

                indices.reserve (values.size());
                for (auto index : values)
                    indices.push_back (static_cast<uint32> (index));
            }
            else if (vertices.size() < 3 || vertices.size() % 3 != 0)
            {
                warn (where + ": vertices are not a list of triangles");
                continue;
            }

            const auto material = isPositiveAndBelow (primitive.material, static_cast<int> (data.materials.size()))
                                    ? data.materials[static_cast<size_t> (primitive.material)]
                                    : nullptr;

            mesh->addPrimitive (std::move (vertices), std::move (indices), material);
        }

        return mesh;
    }

    //==============================================================================
    static Data::CameraDesc convertCamera (const tg3_camera& source)
    {
        Data::CameraDesc camera;

        if (tg3_str_equals_cstr (source.type, "orthographic"))
        {
            camera.projection = CameraNode::Projection::orthographic;
            camera.xMag = static_cast<float> (source.orthographic.xmag);
            camera.yMag = static_cast<float> (source.orthographic.ymag);
            camera.zNear = static_cast<float> (source.orthographic.znear);
            camera.zFar = static_cast<float> (source.orthographic.zfar);
        }
        else
        {
            camera.yFov = static_cast<float> (source.perspective.yfov);
            camera.zNear = static_cast<float> (source.perspective.znear);
            camera.zFar = static_cast<float> (source.perspective.zfar);
        }

        return camera;
    }

    static Data::LightDesc convertLight (const tg3_light& source)
    {
        Data::LightDesc light;

        if (tg3_str_equals_cstr (source.type, "point"))
            light.type = LightNode::Type::point;
        else if (tg3_str_equals_cstr (source.type, "spot"))
            light.type = LightNode::Type::spot;

        for (size_t i = 0; i < light.color.size(); ++i)
            light.color[i] = static_cast<float> (source.color[i]);

        light.intensity = static_cast<float> (source.intensity);
        light.range = static_cast<float> (source.range);
        light.innerConeAngle = static_cast<float> (source.spot.inner_cone_angle);
        light.outerConeAngle = static_cast<float> (source.spot.outer_cone_angle);
        return light;
    }

    void convertNodes()
    {
        std::vector<bool> hasParent (model.nodes_count, false);

        for (uint32_t i = 0; i < model.nodes_count; ++i)
        {
            const auto& source = model.nodes[i];
            auto& node = data.nodes.emplace_back();

            node.name = gltfToString (source.name);
            node.mesh = source.mesh;
            node.camera = source.camera;
            node.light = source.light;
            node.children.assign (source.children, source.children + source.children_count);

            for (auto child : node.children)
            {
                if (isPositiveAndBelow (child, static_cast<int> (model.nodes_count)))
                    hasParent[static_cast<size_t> (child)] = true;
            }

            if (source.has_matrix != 0)
            {
                decomposeGltfMatrix (source.matrix, node.position, node.rotation, node.scale);
            }
            else
            {
                node.position = { static_cast<float> (source.translation[0]), static_cast<float> (source.translation[1]), static_cast<float> (source.translation[2]) };
                node.scale = { static_cast<float> (source.scale[0]), static_cast<float> (source.scale[1]), static_cast<float> (source.scale[2]) };
                node.rotation = Quaternion (static_cast<float> (source.rotation[0]),
                                            static_cast<float> (source.rotation[1]),
                                            static_cast<float> (source.rotation[2]),
                                            static_cast<float> (source.rotation[3]))
                                    .normalized();
            }
        }

        for (uint32_t i = 0; i < model.nodes_count; ++i)
        {
            if (! hasParent[i])
                data.parentlessNodes.push_back (static_cast<int> (i));
        }
    }

    //==============================================================================
    const tg3_model& model;
    const File& baseDirectory;
    Data& data;

    std::vector<std::optional<Image>> decodedImages;
    std::vector<bool> imageDecoded;
    std::map<int, Texture::Ptr> textureCache;
};

//==============================================================================

GltfModel::GltfModel()
    : data (std::make_shared<const Data>())
{
}

ResultValue<GltfModel> GltfModel::loadFromFile (const File& file)
{
    MemoryBlock bytes;
    if (! file.loadFileAsData (bytes))
        return makeResultValueFail ("Unable to read " + file.getFullPathName());

    return loadFromData (bytes.asBytes(), file.getParentDirectory());
}

ResultValue<GltfModel> GltfModel::loadFromData (Span<const uint8> bytes, const File& baseDirectory)
{
    if (bytes.empty())
        return makeResultValueFail ("The glTF data is empty");

    // Error messages live in the model arena: copy them out before it is freed
    struct ParseState
    {
        ParseState() { tg3_error_stack_init (&errors); }

        ~ParseState()
        {
            tg3_model_free (&model);
            tg3_error_stack_free (&errors);
        }

        tg3_model model {};
        tg3_error_stack errors {};
    } state;

    tg3_parse_options options;
    tg3_parse_options_init (&options);
    options.validate_indices = 1;
    options.images_as_is = 1;
    options.fs.read_file = gltfReadFile;
    options.fs.free_file = gltfFreeFile;
    options.fs.user_data = const_cast<File*> (&baseDirectory);

    const auto code = tg3_parse_auto (&state.model, &state.errors, bytes.data(), bytes.size(), nullptr, 0, &options);

    auto loaded = std::make_shared<Data>();
    StringArray errors;

    for (uint32_t i = 0; i < tg3_errors_count (&state.errors); ++i)
    {
        const auto* entry = tg3_errors_get (&state.errors, i);

        auto message = entry->message != nullptr ? String::fromUTF8 (entry->message) : String ("Unknown error");
        if (entry->json_path != nullptr)
            message << " (" << String::fromUTF8 (entry->json_path) << ")";

        if (entry->severity == TG3_SEVERITY_ERROR)
            errors.add (message);
        else if (entry->severity == TG3_SEVERITY_WARNING)
            loaded->warnings.add (message);
    }

    if (code != TG3_OK || tg3_errors_has_error (&state.errors) != 0)
    {
        if (errors.isEmpty())
            errors.add ("glTF parsing failed with error " + String (static_cast<int> (code)));

        return makeResultValueFail (errors.joinIntoString ("; "));
    }

    if (auto result = Loader (state.model, baseDirectory, *loaded).load(); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    GltfModel gltfModel;
    gltfModel.data = std::move (loaded);
    return makeResultValueOk (std::move (gltfModel));
}

//==============================================================================

EntityNode::Ptr GltfModel::createEntity (int sceneIndex) const
{
    const auto& d = *data;

    String name;
    std::vector<int> roots;

    if (d.scenes.empty())
    {
        if (sceneIndex != -1)
            return nullptr;

        roots = d.parentlessNodes;
    }
    else
    {
        const auto index = sceneIndex == -1 ? jmax (0, d.defaultScene) : sceneIndex;
        if (! isPositiveAndBelow (index, static_cast<int> (d.scenes.size())))
            return nullptr;

        name = d.scenes[static_cast<size_t> (index)].name;
        roots = d.scenes[static_cast<size_t> (index)].nodes;
    }

    auto root = EntityNode::Ptr (new EntityNode (name));

    // Iterative depth-first build: deep hierarchies can't overflow the stack, and nodes reached
    // twice (invalid in glTF, but possible in a malformed file) are built once to stop cycles
    std::vector<bool> built (d.nodes.size(), false);
    std::vector<std::pair<int, EntityNode*>> pending;

    for (auto it = roots.rbegin(); it != roots.rend(); ++it)
        pending.emplace_back (*it, root.get());

    while (! pending.empty())
    {
        const auto [nodeIndex, parent] = pending.back();
        pending.pop_back();

        if (! isPositiveAndBelow (nodeIndex, static_cast<int> (d.nodes.size())) || built[static_cast<size_t> (nodeIndex)])
            continue;

        built[static_cast<size_t> (nodeIndex)] = true;

        const auto& node = d.nodes[static_cast<size_t> (nodeIndex)];
        auto entity = EntityNode::Ptr (new EntityNode (node.name));
        entity->setPosition (node.position);
        entity->setRotation (node.rotation);
        entity->setScale (node.scale);

        if (isPositiveAndBelow (node.mesh, static_cast<int> (d.meshes.size())))
            entity->attach<MeshNode> (d.meshes[static_cast<size_t> (node.mesh)]);

        if (isPositiveAndBelow (node.camera, static_cast<int> (d.cameras.size())))
        {
            const auto& desc = d.cameras[static_cast<size_t> (node.camera)];
            auto& camera = entity->attach<CameraNode>();
            camera.projection = desc.projection;
            camera.yFov = desc.yFov;
            camera.zNear = desc.zNear;
            camera.zFar = desc.zFar;
            camera.xMag = desc.xMag;
            camera.yMag = desc.yMag;
        }

        if (isPositiveAndBelow (node.light, static_cast<int> (d.lights.size())))
        {
            const auto& desc = d.lights[static_cast<size_t> (node.light)];
            auto& light = entity->attach<LightNode> (desc.type);
            light.color = desc.color;
            light.intensity = desc.intensity;
            light.range = desc.range;
            light.innerConeAngle = desc.innerConeAngle;
            light.outerConeAngle = desc.outerConeAngle;
        }

        for (auto it = node.children.rbegin(); it != node.children.rend(); ++it)
            pending.emplace_back (*it, entity.get());

        parent->addChild (std::move (entity));
    }

    return root;
}

int GltfModel::getNumScenes() const noexcept
{
    return static_cast<int> (data->scenes.size());
}

const std::vector<Mesh::Ptr>& GltfModel::getMeshes() const noexcept
{
    return data->meshes;
}

const std::vector<Material::Ptr>& GltfModel::getMaterials() const noexcept
{
    return data->materials;
}

const std::vector<Texture::Ptr>& GltfModel::getTextures() const noexcept
{
    return data->textures;
}

const StringArray& GltfModel::getWarnings() const noexcept
{
    return data->warnings;
}

} // namespace yup
