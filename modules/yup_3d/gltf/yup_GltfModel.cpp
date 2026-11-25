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
        bool built = true; // False for a scene that is left out: its prefab would take the model over what it can keep
    };

    /** What makes the DeformNode of a node, once an instance is built. */
    struct DeformDesc
    {
        size_t node = 0; // The glTF node whose entity draws the mesh
        Mesh::Ptr mesh;
        Skin::Ptr skin;             // Only when it can move the mesh
        std::vector<float> weights; // Of the morph targets, when the node gives its own
    };

    std::vector<Mesh::Ptr> meshes;
    std::vector<Material::Ptr> materials;
    std::vector<Texture::Ptr> textures;
    std::vector<AnimationClip::Ptr> animations;
    std::vector<Skin::Ptr> skins;
    std::vector<NodeDesc> nodes;
    std::vector<CameraDesc> cameras;
    std::vector<LightDesc> lights;
    std::vector<SceneDesc> scenes;
    std::vector<int> parentlessNodes;
    int defaultScene = -1;
    StringArray warnings;

    /** The nodes whose mesh is drawn deformed. One list for every prefab: they are as many as the scenes. */
    std::shared_ptr<const std::vector<DeformDesc>> deformers = std::make_shared<const std::vector<DeformDesc>>();

    /** One per glTF scene, or a single one holding the parentless nodes when there are no scenes. A scene
        that is left out keeps its place, without a prefab.
    */
    std::vector<Prefab::Ptr> prefabs;

    //==============================================================================
    /** The most warnings a model keeps. A file can be made to give any number of them, and each one is
        looked for among the others: the last one then tells that more were left out.
    */
    static constexpr int maxWarnings = 200;

    /** Keeps a warning, of the parser or of the loader, unless the model has it or has all it keeps. */
    void warn (const String& message)
    {
        if (warnings.size() < maxWarnings - 1)
            warnings.addIfNotAlreadyThere (message);
        else if (warnings.size() == maxWarnings - 1 && ! warnings.contains (message))
            warnings.add ("More than " + String (maxWarnings - 1) + " warnings: the next ones are left out");
    }

    //==============================================================================
    void buildPrefabs()
    {
        if (scenes.empty())
            prefabs.push_back (buildPrefab ({}, parentlessNodes));

        for (const auto& scene : scenes)
            prefabs.push_back (scene.built ? buildPrefab (scene.name, scene.nodes) : nullptr);
    }

    Prefab::Ptr buildPrefab (const String& name, const std::vector<int>& roots) const
    {
        auto prefab = Prefab::Ptr (new Prefab (name));

        // Iterative depth-first build: deep hierarchies can't overflow the stack, and nodes reached
        // twice (invalid in glTF, but possible in a malformed file) are built once to stop cycles
        std::vector<int> nodeParts (nodes.size(), -1);
        std::vector<std::pair<int, Prefab::Part*>> pending;

        for (auto it = roots.rbegin(); it != roots.rend(); ++it)
            pending.emplace_back (*it, &prefab->getRoot());

        while (! pending.empty())
        {
            const auto [nodeIndex, parent] = pending.back();
            pending.pop_back();

            if (! isPositiveAndBelow (nodeIndex, static_cast<int> (nodes.size())) || nodeParts[static_cast<size_t> (nodeIndex)] >= 0)
                continue;

            const auto& node = nodes[static_cast<size_t> (nodeIndex)];
            auto& part = parent->addChild (node.name);
            nodeParts[static_cast<size_t> (nodeIndex)] = part.getIndex();
            part.attach<TransformNode> (node.position, node.rotation, node.scale);

            if (isPositiveAndBelow (node.mesh, static_cast<int> (meshes.size())))
                part.attach<MeshNode> (meshes[static_cast<size_t> (node.mesh)]);

            if (isPositiveAndBelow (node.camera, static_cast<int> (cameras.size())))
            {
                part.attachWith<CameraNode> ([desc = cameras[static_cast<size_t> (node.camera)]] (CameraNode& camera)
                {
                    camera.projection = desc.projection;
                    camera.yFov = desc.yFov;
                    camera.zNear = desc.zNear;
                    camera.zFar = desc.zFar;
                    camera.xMag = desc.xMag;
                    camera.yMag = desc.yMag;
                });
            }

            if (isPositiveAndBelow (node.light, static_cast<int> (lights.size())))
            {
                part.attachWith<LightNode> ([desc = lights[static_cast<size_t> (node.light)]] (LightNode& light)
                {
                    light.type = desc.type;
                    light.color = desc.color;
                    light.intensity = desc.intensity;
                    light.range = desc.range;
                    light.innerConeAngle = desc.innerConeAngle;
                    light.outerConeAngle = desc.outerConeAngle;
                });
            }

            for (auto it = node.children.rbegin(); it != node.children.rend(); ++it)
                pending.emplace_back (*it, &part);
        }

        // A mesh that deforms is copied for each instance and follows joints that are entities of that
        // instance, and clips target glTF nodes: the entity each node became is only known once an
        // instance is built
        if (! deformers->empty() || ! animations.empty())
        {
            prefab->addLink ([deformers = deformers, clips = animations, nodeParts = std::move (nodeParts)] (Span<const Entity> entities)
            {
                std::vector<Entity> targets (nodeParts.size());

                for (size_t i = 0; i < nodeParts.size(); ++i)
                {
                    if (nodeParts[i] >= 0)
                        targets[i] = entities[static_cast<size_t> (nodeParts[i])];
                }

                for (const auto& deformer : *deformers)
                {
                    // Not valid for a node this scene doesn't hold, as a joint can be too
                    auto entity = targets[deformer.node];
                    if (! entity.isValid())
                        continue;

                    std::vector<Entity> joints;

                    if (deformer.skin != nullptr)
                    {
                        for (const auto joint : deformer.skin->getJoints())
                            joints.push_back (targets[static_cast<size_t> (joint)]);
                    }

                    // Attached, the node has deformed the mesh already, so that an instance never updated shows
                    // its pose and not the bind pose of the vertices
                    auto& deform = entity.attach<DeformNode> (deformer.mesh, deformer.skin, std::move (joints));

                    // glTF: the weights of a node override the ones of its mesh
                    if (deformer.weights.empty())
                        continue;

                    deform.setWeights (deformer.weights);
                    deform.update (0.0);
                }

                if (clips.empty())
                    return;

                if (auto root = entities.front(); root.isValid())
                    root.attach<AnimationNode> (clips, std::move (targets));
            });
        }

        return prefab;
    }
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

    Fully linear samplers ask for 16x anisotropic filtering: GpuSampler lowers it on devices without it.
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

std::optional<AnimationClip::Path> gltfAnimationPath (tg3_str path) noexcept
{
    if (tg3_str_equals_cstr (path, "translation"))
        return AnimationClip::Path::translation;

    if (tg3_str_equals_cstr (path, "rotation"))
        return AnimationClip::Path::rotation;

    if (tg3_str_equals_cstr (path, "scale"))
        return AnimationClip::Path::scale;

    if (tg3_str_equals_cstr (path, "weights"))
        return AnimationClip::Path::weights;

    return std::nullopt;
}

std::optional<AnimationClip::Interpolation> gltfAnimationInterpolation (tg3_str interpolation) noexcept
{
    if (tg3_str_equals_cstr (interpolation, "STEP"))
        return AnimationClip::Interpolation::step;

    if (tg3_str_equals_cstr (interpolation, "LINEAR"))
        return AnimationClip::Interpolation::linear;

    if (tg3_str_equals_cstr (interpolation, "CUBICSPLINE"))
        return AnimationClip::Interpolation::cubicSpline;

    return std::nullopt;
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
        static const char* supportedExtensions[] = { "KHR_lights_punctual", "EXT_texture_webp", "KHR_mesh_quantization",
                                                     "KHR_materials_emissive_strength", "KHR_materials_ior", "KHR_materials_specular",
                                                     "KHR_materials_clearcoat", "KHR_materials_sheen" };

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
            data.materials.push_back (convertMaterial (model.materials[i], static_cast<int> (i)));

        for (uint32_t i = 0; i < model.meshes_count; ++i)
            data.meshes.push_back (convertMesh (model.meshes[i], static_cast<int> (i)));

        for (uint32_t i = 0; i < model.cameras_count; ++i)
            data.cameras.push_back (convertCamera (model.cameras[i]));

        for (uint32_t i = 0; i < model.lights_count; ++i)
            data.lights.push_back (convertLight (model.lights[i]));

        for (uint32 i = 0; i < model.skins_count; ++i)
        {
            skins.push_back (convertSkin (model.skins[i], static_cast<int> (i)));

            if (skins.back() != nullptr)
                data.skins.push_back (skins.back());
        }

        // What is counted last is left out first, see keepValues(): a model can do without the clips and
        // the deformations that no longer fit, not without its scenes
        convertNodes();
        convertScenes();
        convertDeformers();

        for (uint32 i = 0; i < model.animations_count; ++i)
        {
            if (auto clip = convertAnimation (model.animations[i], static_cast<int> (i)); clip != nullptr)
                data.animations.push_back (std::move (clip));
        }

        return Result::ok();
    }

private:
    //==============================================================================
    void warn (const String& message)
    {
        data.warn (message);
    }

    //==============================================================================
    /** The least a model can keep of what its accessors hold, counted in 32-bit values: as many as a
        vertex takes, which is a Mesh::Vertex whatever attributes the file gives it, one for an index or
        for a time or a value of an animation key, three for a morph delta, and so on. About 256 MB.

        A model whose buffers hold more can keep valuesPerBufferByte values for each of their bytes: a
        large model is a large file. What this bounds is what a file can make larger than itself. It
        only states the size of an accessor, and one without a buffer view reads as zeros, so a few
        bytes can ask for any amount of data. An accessor can be used any number of times, by
        primitives, morph targets and animation channels. Every node deforming a mesh takes a copy of
        it in each instance. And every scene has a part of its own for each node it holds, also when
        other scenes hold the same nodes. What would take a model over is left out, with a warning,
        and nothing is read from an accessor before what it says it holds is counted, or compared
        with a count that is.

        The budget never passes what an array of doubles can address, which the values of an accessor
        are read into: this only limits it where pointers are 32 bits, see countValuesBudget().
    */
    static constexpr uint64 minValuesKept = 1ull << 26;

    /** A quantized vertex of 4 bytes becomes the 16 values of a Mesh::Vertex: twice as many leave room
        for the copies of the meshes that deform.
    */
    static constexpr uint64 valuesPerBufferByte = 8;

    static constexpr uint64 valuesPerVertex = sizeof (Mesh::Vertex) / sizeof (float);
    static constexpr uint64 valuesPerInfluences = sizeof (Mesh::Influences) / sizeof (float);

    /** What a DeformNode keeps for a joint: its entity, the world matrix it last had, the joint matrix
        the mesh was deformed with and the one it would be next.
    */
    static constexpr uint64 valuesPerDeformedJoint = (sizeof (Entity) + 3 * sizeof (Matrix4)) / sizeof (float);

    /** What the prefab of a scene takes for a node it holds, see countSceneValues(): the part, its place
        among the children of its parent and the nodes it attaches, each with what it is made from. An
        estimate that is not below the truth on any platform: 640 bytes, for a part that seldom takes
        half of that.
    */
    static constexpr uint64 valuesPerPart = 160;

    /** What the prefab of a scene takes before it holds any node: itself, and the link that completes
        its instances. The same kind of estimate.
    */
    static constexpr uint64 valuesPerScene = 128;

    /** The values this model can keep: minValuesKept, or more for buffers that are larger. */
    static uint64 countValuesBudget (const tg3_model& source) noexcept
    {
        // Limited to more than buffers can hold, for what is counted against the budget never to overflow
        constexpr uint64 maxBufferBytes = 1ull << 48;
        uint64 numBytes = 0;

        for (uint32_t i = 0; i < source.buffers_count; ++i)
            numBytes = jmin (maxBufferBytes, numBytes + jmin (maxBufferBytes, static_cast<uint64> (source.buffers[i].data.count)));

        // And to what an array of doubles can address: the values of an accessor are read into one
        return jmax (minValuesKept, jmin (numBytes * valuesPerBufferByte, static_cast<uint64> (std::numeric_limits<size_t>::max() / sizeof (double))));
    }

    /** Takes values out of what the model can still keep, see minValuesKept. They stay taken when what
        they were taken for then turns out to be unusable.

        @return False, with a warning telling that @p where is left out, when they don't fit: none is
                taken then.
    */
    bool keepValues (uint64 numValues, const String& where)
    {
        if (numValues <= valuesLeft)
        {
            valuesLeft -= numValues;
            return true;
        }

        warn (where + ": over the " + String (valuesBudget) + " values a model can keep, ignored");
        return false;
    }

    /** The number of elements an accessor says it holds, 0 when there is no such accessor. */
    uint64 countElements (int index) const
    {
        return isPositiveAndBelow (index, static_cast<int> (model.accessors_count)) ? static_cast<uint64> (model.accessors[index].count) : 0u;
    }

    /** The number of values an accessor says it holds, 0 when there is no such accessor. */
    uint64 countValues (int index) const
    {
        if (! isPositiveAndBelow (index, static_cast<int> (model.accessors_count)))
            return 0;

        // Limited before it is multiplied: a file can state any count
        return jmin (countElements (index), valuesBudget + 1) * static_cast<uint64> (jmax (0, tg3_num_components (model.accessors[index].type)));
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

        // More than the model can keep is never read
        if (numComponents <= 0 || numComponents > 16 || componentSize == 0 || count > valuesBudget / static_cast<uint64> (numComponents))
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

    /** The object value of the material extension called @p name, or nullptr. */
    static const tg3_value* findExtension (const tg3_extras_ext& ext, const char* name)
    {
        for (uint32_t i = 0; i < ext.extensions_count; ++i)
        {
            const auto& extension = ext.extensions[i];
            if (tg3_str_equals_cstr (extension.name, name) && extension.value.type == TG3_VALUE_OBJECT)
                return &extension.value;
        }

        return nullptr;
    }

    /** The member called @p key of an object value, or nullptr. */
    static const tg3_value* findMember (const tg3_value* object, const char* key)
    {
        if (object == nullptr)
            return nullptr;

        for (uint32_t i = 0; i < object->object_count; ++i)
        {
            if (tg3_str_equals_cstr (object->object_data[i].key, key))
                return &object->object_data[i].value;
        }

        return nullptr;
    }

    /** A JSON number, which arrives as a real or as an integer, or @p fallback for other values. */
    static double toNumber (const tg3_value& value, double fallback)
    {
        if (value.type == TG3_VALUE_REAL)
            return value.real_val;

        if (value.type == TG3_VALUE_INT)
            return static_cast<double> (value.int_val);

        return fallback;
    }

    /** A number member of an object value, or @p fallback when absent or not a number. */
    static double readNumber (const tg3_value* object, const char* key, double fallback)
    {
        const auto* member = findMember (object, key);
        return member != nullptr ? toNumber (*member, fallback) : fallback;
    }

    /** Fills @p destination from a number array member, leaving it untouched when absent or too short. */
    template <size_t N>
    static void readNumbers (const tg3_value* object, const char* key, std::array<float, N>& destination)
    {
        const auto* member = findMember (object, key);
        if (member == nullptr || member->type != TG3_VALUE_ARRAY || member->array_count < N)
            return;

        for (size_t i = 0; i < N; ++i)
            destination[i] = static_cast<float> (toNumber (member->array_data[i], destination[i]));
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
    Material::Ptr convertMaterial (const tg3_material& source, int materialIndex)
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

        // Values outside the ranges of the extension schemas are limited to them
        const auto* emissiveStrength = findExtension (source.ext, "KHR_materials_emissive_strength");
        material->emissiveStrength = jmax (0.0f, static_cast<float> (readNumber (emissiveStrength, "emissiveStrength", material->emissiveStrength)));

        // 0 stands for an infinite index, values between 0 and 1 are undefined
        const auto* ior = findExtension (source.ext, "KHR_materials_ior");
        const auto iorValue = static_cast<float> (readNumber (ior, "ior", material->ior));
        if (iorValue >= 1.0f || iorValue == 0.0f)
            material->ior = iorValue;

        const auto* specular = findExtension (source.ext, "KHR_materials_specular");
        material->specularFactor = jlimit (0.0f, 1.0f, static_cast<float> (readNumber (specular, "specularFactor", material->specularFactor)));
        readNumbers (specular, "specularColorFactor", material->specularColorFactor);

        for (auto& component : material->specularColorFactor)
            component = jmax (0.0f, component);

        for (const auto* textureName : { "specularTexture", "specularColorTexture" })
        {
            if (findMember (specular, textureName) != nullptr)
                warn ("Material " + String (materialIndex) + ": the KHR_materials_specular " + textureName + " is not supported and is ignored");
        }

        const auto* clearcoat = findExtension (source.ext, "KHR_materials_clearcoat");
        material->clearcoatFactor = jlimit (0.0f, 1.0f, static_cast<float> (readNumber (clearcoat, "clearcoatFactor", material->clearcoatFactor)));
        material->clearcoatRoughnessFactor = jlimit (0.0f, 1.0f, static_cast<float> (readNumber (clearcoat, "clearcoatRoughnessFactor", material->clearcoatRoughnessFactor)));

        for (const auto* textureName : { "clearcoatTexture", "clearcoatRoughnessTexture", "clearcoatNormalTexture" })
        {
            if (findMember (clearcoat, textureName) != nullptr)
                warn ("Material " + String (materialIndex) + ": the KHR_materials_clearcoat " + textureName + " is not supported and is ignored");
        }

        const auto* sheen = findExtension (source.ext, "KHR_materials_sheen");
        readNumbers (sheen, "sheenColorFactor", material->sheenColorFactor);
        material->sheenRoughnessFactor = jlimit (0.0f, 1.0f, static_cast<float> (readNumber (sheen, "sheenRoughnessFactor", material->sheenRoughnessFactor)));

        for (auto& component : material->sheenColorFactor)
            component = jlimit (0.0f, 1.0f, component);

        for (const auto* textureName : { "sheenColorTexture", "sheenRoughnessTexture" })
        {
            if (findMember (sheen, textureName) != nullptr)
                warn ("Material " + String (materialIndex) + ": the KHR_materials_sheen " + textureName + " is not supported and is ignored");
        }

        return material;
    }

    //==============================================================================
    /** The joints and weights of the vertices of a primitive, or nothing, with a warning, when they can't be used. */
    std::vector<Mesh::Influences> readInfluences (const String& where, int jointsAccessor, int weightsAccessor, size_t numVertices)
    {
        std::vector<double> joints, weights;
        int numJointComponents = 0, numWeightComponents = 0;

        // Joints are indices whatever type stores them: integers come through a double exactly
        const auto isJoint = [] (double joint)
        {
            return joint >= 0.0 && joint <= 65535.0 && joint == std::floor (joint);
        };

        // Read only when they say they hold the joints and the weights of every vertex, which were counted
        if (countElements (jointsAccessor) != numVertices
            || countElements (weightsAccessor) != numVertices
            || ! readAccessor (jointsAccessor, joints, numJointComponents)
            || ! readAccessor (weightsAccessor, weights, numWeightComponents)
            || numJointComponents != 4
            || numWeightComponents != 4
            || ! std::all_of (joints.begin(), joints.end(), isJoint))
        {
            warn (where + ": invalid JOINTS_0 or WEIGHTS_0, the primitive is not skinned");
            return {};
        }

        std::vector<Mesh::Influences> influences (numVertices);

        for (size_t v = 0; v < numVertices; ++v)
        {
            const auto* joint = joints.data() + v * 4;
            auto* weight = weights.data() + v * 4;
            auto total = 0.0;

            // glTF: weights are not negative
            for (size_t i = 0; i < 4; ++i)
            {
                weight[i] = weight[i] > 0.0 ? weight[i] : 0.0;
                total += weight[i];
            }

            // glTF: no transformation applies to a vertex whose weights are all zero. Nothing says it follows
            // the first joint: it is left without weights, and stays where the bind pose has it
            if (total == 0.0 || ! std::isfinite (total))
                continue;

            // glTF asks for weights adding up to 1, which files only do nearly, and not at all once the joints
            // of JOINTS_1 are left out. A joint without weight is not used, whatever index it has
            for (size_t i = 0; i < 4; ++i)
            {
                if (weight[i] == 0.0)
                    continue;

                influences[v].joints[i] = static_cast<uint16> (joint[i]);
                influences[v].weights[i] = static_cast<float> (weight[i] / total);
            }
        }

        return influences;
    }

    /** The morph targets of a primitive, each with the deltas that can be used. One left without any stays, as
        a target that moves nothing, so that the weights keep the index of their target.

        @param numVertices      The vertices that were read, each of which a list of deltas has an element for.
        @param numVerticesHeld  The most the primitive holds while it is made, which a list of deltas counts for.
    */
    std::vector<Mesh::MorphTarget> readMorphTargets (const String& where, const tg3_primitive& primitive, size_t numVertices, size_t numVerticesHeld)
    {
        std::vector<Mesh::MorphTarget> targets (primitive.targets_count);
        std::vector<double> values;
        int numComponents = 0;

        for (uint32 t = 0; t < primitive.targets_count; ++t)
        {
            const auto targetWhere = where + " target " + String (t);
            const auto* attributes = primitive.targets[t];
            const auto numAttributes = attributes != nullptr ? primitive.target_attribute_counts[t] : 0u;

            for (uint32 a = 0; a < numAttributes; ++a)
            {
                const auto& attribute = attributes[a];
                std::vector<Vector3<float>>* deltas = nullptr;

                if (tg3_str_equals_cstr (attribute.key, "POSITION"))
                    deltas = &targets[t].positions;
                else if (tg3_str_equals_cstr (attribute.key, "NORMAL"))
                    deltas = &targets[t].normals;
                else if (tg3_str_equals_cstr (attribute.key, "TANGENT"))
                    deltas = &targets[t].tangents;

                const auto name = gltfToString (attribute.key);

                if (deltas == nullptr)
                {
                    if (name.startsWith ("TEXCOORD_") || name.startsWith ("COLOR_"))
                        warn (targetWhere + ": morphing " + name + " is not supported, ignored");

                    continue;
                }

                if (! keepValues (numVerticesHeld * 3, targetWhere))
                    continue;

                // glTF: a morph target accessor has as many elements as the vertices have. Another is not read
                if (countElements (attribute.value) != numVertices || ! readAccessor (attribute.value, values, numComponents) || numComponents != 3)
                {
                    warn (targetWhere + ": invalid " + name + ", ignored");
                    continue;
                }

                deltas->resize (numVertices);

                for (size_t v = 0; v < numVertices; ++v)
                    (*deltas)[v] = { static_cast<float> (values[v * 3]), static_cast<float> (values[v * 3 + 1]), static_cast<float> (values[v * 3 + 2]) };
            }
        }

        return targets;
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

            int positionAccessor = -1, normalAccessor = -1, tangentAccessor = -1, uvAccessor = -1, colorAccessor = -1;
            int jointsAccessor = -1, weightsAccessor = -1;
            for (uint32_t a = 0; a < primitive.attributes_count; ++a)
            {
                const auto& attribute = primitive.attributes[a];

                if (tg3_str_equals_cstr (attribute.key, "POSITION"))
                    positionAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "NORMAL"))
                    normalAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "TANGENT"))
                    tangentAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "TEXCOORD_0"))
                    uvAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "COLOR_0"))
                    colorAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "JOINTS_0"))
                    jointsAccessor = attribute.value;
                else if (tg3_str_equals_cstr (attribute.key, "WEIGHTS_0"))
                    weightsAccessor = attribute.value;
                else if (const auto name = gltfToString (attribute.key); name.startsWith ("JOINTS_"))
                    warn (where + ": only the four joints of JOINTS_0 move a vertex, " + name + " and its weights are ignored");
            }

            // A vertex takes the room of a Mesh::Vertex whatever attributes the file gives it, an index one value.
            // Without normals Mesh::addPrimitive() generates them, and gives every index a vertex of its own
            const auto numDeclaredVertices = countValues (positionAccessor) / 3;
            const auto numDeclaredIndices = primitive.indices >= 0 ? countValues (primitive.indices) : numDeclaredVertices;
            const auto numVerticesTaken = normalAccessor < 0 ? jmax (numDeclaredVertices, numDeclaredIndices) : numDeclaredVertices;

            if (! keepValues (numVerticesTaken * valuesPerVertex + numDeclaredIndices, where))
                continue;

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

                // Read only when it says it holds an element for every vertex, which were counted
                if (countElements (accessor) != vertices.size()
                    || ! readAccessor (accessor, values, numComponents)
                    || numComponents < minComponents
                    || numComponents > maxComponents)
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

            // glTF: when normals are not specified, tangents shall be ignored
            if (normalAccessor >= 0)
            {
                readAttribute (tangentAccessor, "TANGENT", 4, 4, [] (Mesh::Vertex& vertex, const double* x)
                {
                    // A zero or non-finite direction leaves the tangent cleared, for the derivative frame
                    const auto length = std::sqrt (x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
                    if (! std::isfinite (length) || length <= 0.0)
                        return;

                    vertex.tangent = { static_cast<float> (x[0] / length), static_cast<float> (x[1] / length), static_cast<float> (x[2] / length), x[3] < 0.0 ? -1.0f : 1.0f };
                });
            }

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

            // Normals that are there but all zero are generated too, which only shows once they are read: every
            // index then takes a vertex of its own, as it does without normals
            const auto hasNormals = std::any_of (vertices.begin(), vertices.end(), [] (const Mesh::Vertex& vertex)
            {
                return vertex.normal.lengthSquared() > 0.0f;
            });

            const auto numKeptVertices = (hasNormals || indices.empty()) ? vertices.size() : indices.size();

            if (numKeptVertices > numVerticesTaken && ! keepValues ((numKeptVertices - numVerticesTaken) * valuesPerVertex, where))
                continue;

            // What comes with the vertices is read for each of them, and kept for the ones the primitive ends with
            const auto numVerticesHeld = jmax (vertices.size(), numKeptVertices);
            std::vector<Mesh::Influences> influences;

            // glTF: JOINTS_n and WEIGHTS_n are always used together
            if ((jointsAccessor >= 0) != (weightsAccessor >= 0))
                warn (where + ": JOINTS_0 and WEIGHTS_0 come together, the primitive is not skinned");
            else if (jointsAccessor >= 0 && keepValues (numVerticesHeld * valuesPerInfluences, where + " JOINTS_0 and WEIGHTS_0"))
                influences = readInfluences (where, jointsAccessor, weightsAccessor, vertices.size());

            auto morphTargets = readMorphTargets (where, primitive, vertices.size(), numVerticesHeld);

            const auto material = isPositiveAndBelow (primitive.material, static_cast<int> (data.materials.size()))
                                    ? data.materials[static_cast<size_t> (primitive.material)]
                                    : nullptr;

            mesh->addPrimitive (std::move (vertices), std::move (indices), material, std::move (influences), std::move (morphTargets));
        }

        // glTF: the weights the morph targets have unless a node gives its own
        mesh->setMorphWeights (toFloats (std::vector<double> (source.weights, source.weights + source.weights_count)));

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

    //==============================================================================
    /** A skin, or nullptr, with a warning, for one that can't be used. */
    Skin::Ptr convertSkin (const tg3_skin& source, int skinIndex)
    {
        const auto where = "Skin " + String (skinIndex);
        std::vector<int> joints (source.joints, source.joints + source.joints_count);

        const auto jointsAreNodes = std::all_of (joints.begin(), joints.end(), [this] (int joint)
        {
            return isPositiveAndBelow (joint, static_cast<int> (model.nodes_count));
        });

        if (! jointsAreNodes)
        {
            warn (where + ": a joint is not a node, ignored");
            return nullptr;
        }

        std::vector<Matrix4> inverseBindMatrices;

        // glTF: without the accessor, every inverse bind matrix is the identity
        if (source.inverse_bind_matrices >= 0)
        {
            // glTF: the accessor holds at least as many matrices as there are joints, the first ones being theirs
            if (const auto numMatrices = countElements (source.inverse_bind_matrices); numMatrices < joints.size())
            {
                warn (where + ": " + String (numMatrices) + " inverse bind matrices for " + String (joints.size()) + " joints, ignored");
                return nullptr;
            }

            // It is read whole: all it says it holds counts, though only the matrices of the joints are kept
            if (! keepValues (countValues (source.inverse_bind_matrices), where))
                return nullptr;

            std::vector<double> values;
            int numComponents = 0;

            if (! readAccessor (source.inverse_bind_matrices, values, numComponents) || numComponents != 16)
            {
                warn (where + ": invalid inverseBindMatrices, ignored");
                return nullptr;
            }

            for (size_t j = 0; j < joints.size(); ++j)
            {
                std::array<float, 16> matrix {};

                for (size_t i = 0; i < matrix.size(); ++i)
                    matrix[i] = static_cast<float> (values[j * 16 + i]);

                inverseBindMatrices.emplace_back (matrix);
            }
        }

        return new Skin (gltfToString (source.name), std::move (joints), std::move (inverseBindMatrices));
    }

    /** The skin that moves the mesh of a node, or nullptr, with a warning when the node has one that can't. */
    Skin::Ptr findNodeSkin (const tg3_node& source, int nodeIndex)
    {
        if (! isPositiveAndBelow (source.skin, static_cast<int> (skins.size())) || ! isPositiveAndBelow (source.mesh, static_cast<int> (data.meshes.size())))
            return nullptr;

        // A skin that could not be read told so itself
        const auto skin = skins[static_cast<size_t> (source.skin)];
        if (skin == nullptr)
            return nullptr;

        const auto where = "Node " + String (nodeIndex) + ": mesh " + String (source.mesh);
        const auto numJoints = data.meshes[static_cast<size_t> (source.mesh)]->getNumJoints();

        if (numJoints == 0)
        {
            warn (where + " has no joints, skin " + String (source.skin) + " is ignored");
            return nullptr;
        }

        // glTF: the joints of the vertices of a skinned mesh are all joints of its skin
        if (numJoints > skin->getNumJoints())
        {
            warn (where + " uses joint " + String (numJoints - 1) + ", which skin " + String (source.skin) + " doesn't have, it is not skinned");
            return nullptr;
        }

        return skin;
    }

    /** What a DeformNode takes of a mesh in each instance, and whether there is anything for it to morph. */
    struct DeformedMesh
    {
        uint64 numValues = 0;
        bool hasDeltas = false;
    };

    static DeformedMesh measureDeformedMesh (const Mesh& mesh)
    {
        DeformedMesh measure;
        uint64 numVertices = 0;
        uint64 numIndices = 0;

        for (const auto& primitive : mesh.getPrimitives())
        {
            numVertices += primitive.vertices.size();
            numIndices += primitive.indices.size();

            // A target can be left without any delta: by the file, or by what was wrong with the ones it had
            for (const auto& target : primitive.morphTargets)
                measure.hasDeltas = measure.hasDeltas || ! target.positions.empty() || ! target.normals.empty() || ! target.tangents.empty();
        }

        // The node keeps a copy of the vertices and of the indices, as many vertices again to work on, and a
        // weight for each morph target
        measure.numValues = 2 * numVertices * valuesPerVertex + numIndices + static_cast<uint64> (mesh.getNumMorphTargets());
        return measure;
    }

    /** What deforms the mesh of a node in an instance, or nothing: when nothing does, and, with a warning,
        when what its DeformNode takes is more than the model can keep.
    */
    std::optional<Data::DeformDesc> findNodeDeformer (const tg3_node& source, int nodeIndex, Span<const DeformedMesh> deformedMeshes)
    {
        if (! isPositiveAndBelow (source.mesh, static_cast<int> (deformedMeshes.size())))
            return std::nullopt;

        const auto& deformed = deformedMeshes[static_cast<size_t> (source.mesh)];
        auto skin = findNodeSkin (source, nodeIndex);

        if (skin == nullptr && ! deformed.hasDeltas)
            return std::nullopt;

        // Every node deforms a copy of its own and follows joints of its own: a file can have any number
        const auto numJoints = static_cast<uint64> (skin != nullptr ? skin->getNumJoints() : 0);

        if (! keepValues (deformed.numValues + numJoints * valuesPerDeformedJoint, "Node " + String (nodeIndex) + " deforming mesh " + String (source.mesh)))
            return std::nullopt;

        Data::DeformDesc deformer;
        deformer.node = static_cast<size_t> (nodeIndex);
        deformer.mesh = data.meshes[static_cast<size_t> (source.mesh)];
        deformer.skin = std::move (skin);
        deformer.weights = toFloats (std::vector<double> (source.weights, source.weights + source.weights_count));
        return deformer;
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

    /** Finds the nodes that draw their mesh deformed, see findNodeDeformer(). */
    void convertDeformers()
    {
        // Measured once for each mesh, which any number of nodes can draw
        std::vector<DeformedMesh> deformedMeshes;
        for (const auto& mesh : data.meshes)
            deformedMeshes.push_back (measureDeformedMesh (*mesh));

        std::vector<Data::DeformDesc> deformers;

        for (uint32_t i = 0; i < model.nodes_count; ++i)
        {
            if (auto deformer = findNodeDeformer (model.nodes[i], static_cast<int> (i), deformedMeshes))
                deformers.push_back (std::move (*deformer));
        }

        data.deformers = std::make_shared<const std::vector<Data::DeformDesc>> (std::move (deformers));
    }

    //==============================================================================
    /** What the prefab of a scene takes more than the file states, see Data::buildPrefab(): a part for
        each node it reaches that a scene counted before reached already. The count stops once it is more
        than the model can still keep.

        The first scene reaching a node has its part for nothing. A file states its nodes one by one, so
        their parts are no more than it states: it is the scenes holding the same nodes again, which a
        file can have any number of, that make it larger than itself.

        @param numValues  What the scene takes before it holds any node, which the count starts from.
    */
    uint64 countSceneValues (const std::vector<int>& roots, uint64 numValues)
    {
        // A node reached twice is built once: each one is marked with the last scene that reached it
        ++reachingScene;
        auto pending = roots;

        while (! pending.empty() && numValues <= valuesLeft)
        {
            const auto index = pending.back();
            pending.pop_back();

            if (! isPositiveAndBelow (index, static_cast<int> (data.nodes.size())) || reachedBy[static_cast<size_t> (index)] == reachingScene)
                continue;

            if (std::exchange (reachedBy[static_cast<size_t> (index)], reachingScene) != 0)
                numValues += valuesPerPart;

            const auto& children = data.nodes[static_cast<size_t> (index)].children;
            pending.insert (pending.end(), children.begin(), children.end());
        }

        return numValues;
    }

    void convertScenes()
    {
        for (uint32_t i = 0; i < model.scenes_count; ++i)
        {
            const auto& source = model.scenes[i];
            auto& scene = data.scenes.emplace_back();

            scene.name = gltfToString (source.name);
            scene.nodes.assign (source.nodes, source.nodes + source.nodes_count);
        }

        data.defaultScene = model.default_scene;

        // The scene a model shows when none is asked for, see GltfModel::getPrefab(), is counted first and
        // takes nothing: all it keeps is what the file states, its nodes and its clips. So a model always
        // has that scene, whatever its meshes took, as it has the single one of a file without scenes
        const auto numScenes = static_cast<int> (data.scenes.size());
        const auto shownScene = jmax (0, data.defaultScene);

        reachedBy.assign (data.nodes.size(), 0);

        if (isPositiveAndBelow (shownScene, numScenes))
            countSceneValues (data.scenes[static_cast<size_t> (shownScene)].nodes, 0);

        // What the link of every other scene keeps, whatever nodes it holds: the part of each node of the
        // model, and a list of the clips, two values for a pointer. The clips are not converted yet: they
        // are as many as the file has, at most
        const auto valuesPerLink = static_cast<uint64> (data.nodes.size()) + 2 * static_cast<uint64> (model.animations_count);

        for (int i = 0; i < numScenes; ++i)
        {
            if (i == shownScene)
                continue;

            auto& scene = data.scenes[static_cast<size_t> (i)];
            scene.built = keepValues (countSceneValues (scene.nodes, valuesPerScene + valuesPerLink), "Scene " + String (i));
        }
    }

    //==============================================================================
    static std::vector<float> toFloats (const std::vector<double>& values)
    {
        std::vector<float> floats (values.size());
        std::transform (values.begin(), values.end(), floats.begin(), [] (double value)
        {
            return static_cast<float> (value);
        });

        return floats;
    }

    /** The number of morph targets of the mesh a node draws, or -1 for a node that draws none. */
    int countMorphTargets (int nodeIndex) const
    {
        if (! isPositiveAndBelow (nodeIndex, static_cast<int> (model.nodes_count)))
            return -1;

        const auto mesh = model.nodes[nodeIndex].mesh;
        return isPositiveAndBelow (mesh, static_cast<int> (data.meshes.size())) ? data.meshes[static_cast<size_t> (mesh)]->getNumMorphTargets() : -1;
    }

    /** One track per channel that can be played, or nullptr when there is none. */
    AnimationClip::Ptr convertAnimation (const tg3_animation& source, int animationIndex)
    {
        const auto where = "Animation " + String (animationIndex);
        const auto name = gltfToString (source.name);
        auto clip = AnimationClip::Ptr (new AnimationClip (name.isNotEmpty() ? name : where));

        for (uint32 c = 0; c < source.channels_count; ++c)
        {
            const auto channelWhere = where + " channel " + String (c);
            const auto& channel = source.channels[c];

            // glTF: when node isn't defined, channel should be ignored
            if (channel.target.node < 0)
            {
                warn (channelWhere + ": no target node, ignored");
                continue;
            }

            const auto path = gltfAnimationPath (channel.target.path);
            if (! path)
            {
                warn (channelWhere + ": unknown path \"" + gltfToString (channel.target.path) + "\", ignored");
                continue;
            }

            if (! isPositiveAndBelow (channel.sampler, static_cast<int> (source.samplers_count)))
            {
                warn (channelWhere + ": invalid sampler, ignored");
                continue;
            }

            const auto& sampler = source.samplers[channel.sampler];
            const auto interpolation = gltfAnimationInterpolation (sampler.interpolation);
            if (! interpolation)
            {
                warn (channelWhere + ": unknown interpolation \"" + gltfToString (sampler.interpolation) + "\", ignored");
                continue;
            }

            // What the keys take is known before any of them is read
            const auto numTimes = countValues (sampler.input);
            const auto numValues = countValues (sampler.output);

            // The number of weights of a key only comes from dividing the two: the mesh they morph, when the
            // node draws one, tells how many there can be
            if (const auto numTargets = countMorphTargets (channel.target.node); *path == AnimationClip::Path::weights && numTargets >= 0)
            {
                const auto numElements = numTimes * (*interpolation == AnimationClip::Interpolation::cubicSpline ? 3u : 1u);
                const auto numWeights = numElements > 0 ? numValues / numElements : 0u;

                if (numWeights > static_cast<uint64> (numTargets))
                {
                    warn (channelWhere + ": " + String (numWeights) + " weights for the " + String (numTargets) + " morph targets of the mesh, ignored");
                    continue;
                }
            }

            if (! keepValues (numTimes + numValues, channelWhere))
                continue;

            std::vector<double> times, values;
            int numTimeComponents = 0, numValueComponents = 0;

            if (! readAccessor (sampler.input, times, numTimeComponents)
                || numTimeComponents != 1
                || ! readAccessor (sampler.output, values, numValueComponents))
            {
                warn (channelWhere + ": invalid input or output, ignored");
                continue;
            }

            // The output is taken as it lies, key after key, whatever its type: addTrack() tells if it fits the keys
            AnimationClip::Track track;
            track.target = channel.target.node;
            track.path = *path;
            track.interpolation = *interpolation;
            track.times = toFloats (times);
            track.values = toFloats (values);

            if (const auto result = clip->addTrack (std::move (track)); result.failed())
                warn (channelWhere + ": " + result.getErrorMessage() + ", ignored");
        }

        if (clip->getTracks().empty())
        {
            warn (where + ": no channel can be played, ignored");
            return nullptr;
        }

        return clip;
    }

    //==============================================================================
    const tg3_model& model;
    const File& baseDirectory;
    Data& data;

    std::vector<std::optional<Image>> decodedImages;
    std::vector<bool> imageDecoded;
    std::map<int, Texture::Ptr> textureCache;
    std::vector<Skin::Ptr> skins;  // By glTF index, nullptr for a skin that can't be used
    std::vector<uint32> reachedBy; // For each node, the last scene counted that reaches it, see countSceneValues()
    uint32 reachingScene = 0;
    const uint64 valuesBudget = countValuesBudget (model);
    uint64 valuesLeft = valuesBudget;
};

//==============================================================================

GltfModel::GltfModel()
{
    auto empty = std::make_shared<Data>();
    empty->buildPrefabs();
    data = std::move (empty);
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
            loaded->warn (message);
    }

    if (code != TG3_OK || tg3_errors_has_error (&state.errors) != 0)
    {
        if (errors.isEmpty())
            errors.add ("glTF parsing failed with error " + String (static_cast<int> (code)));

        return makeResultValueFail (errors.joinIntoString ("; "));
    }

    if (auto result = Loader (state.model, baseDirectory, *loaded).load(); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    loaded->buildPrefabs();

    GltfModel gltfModel;
    gltfModel.data = std::move (loaded);
    return makeResultValueOk (std::move (gltfModel));
}

//==============================================================================

Entity GltfModel::instantiate (Scene& scene, Entity parent, int sceneIndex) const
{
    if (const auto* prefab = getPrefab (sceneIndex))
        return prefab->instantiate (scene, parent);

    return {};
}

const Prefab* GltfModel::getPrefab (int sceneIndex) const noexcept
{
    const auto& d = *data;

    if (d.scenes.empty())
        return sceneIndex == -1 ? d.prefabs.front().get() : nullptr;

    const auto index = sceneIndex == -1 ? jmax (0, d.defaultScene) : sceneIndex;
    if (! isPositiveAndBelow (index, static_cast<int> (d.prefabs.size())))
        return nullptr;

    return d.prefabs[static_cast<size_t> (index)].get();
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

const std::vector<AnimationClip::Ptr>& GltfModel::getAnimations() const noexcept
{
    return data->animations;
}

const std::vector<Skin::Ptr>& GltfModel::getSkins() const noexcept
{
    return data->skins;
}

const StringArray& GltfModel::getWarnings() const noexcept
{
    return data->warnings;
}

} // namespace yup
