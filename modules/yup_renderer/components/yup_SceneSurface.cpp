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
namespace
{

/* The texture a surface is shown through in a material. */
Texture::Ptr& getSceneSurfaceTexture (Material& material, SceneNodeComponent::SurfaceSlot slot) noexcept
{
    return slot == SceneNodeComponent::SurfaceSlot::emissive ? material.emissiveTexture : material.baseColorTexture;
}

/* The red, green and blue the texture of a surface is multiplied by in a material. */
float* getSceneSurfaceFactor (Material& material, SceneNodeComponent::SurfaceSlot slot) noexcept
{
    return slot == SceneNodeComponent::SurfaceSlot::emissive ? material.emissiveFactor.data() : material.baseColorFactor.data();
}

bool isSceneSurfaceFactorWhite (const float* factor) noexcept
{
    return std::all_of (factor, factor + 3, [] (float value) { return value == 1.0f; });
}

/* Gives a material the look of another one: every field but its name, as a Material can't be copied. */
void copySceneMaterialLook (const Material& source, Material& target)
{
    target.baseColorFactor = source.baseColorFactor;
    target.baseColorTexture = source.baseColorTexture;
    target.metallicFactor = source.metallicFactor;
    target.roughnessFactor = source.roughnessFactor;
    target.metallicRoughnessTexture = source.metallicRoughnessTexture;
    target.normalTexture = source.normalTexture;
    target.normalScale = source.normalScale;
    target.occlusionTexture = source.occlusionTexture;
    target.occlusionStrength = source.occlusionStrength;
    target.emissiveFactor = source.emissiveFactor;
    target.emissiveTexture = source.emissiveTexture;
    target.emissiveStrength = source.emissiveStrength;
    target.ior = source.ior;
    target.specularFactor = source.specularFactor;
    target.specularColorFactor = source.specularColorFactor;
    target.clearcoatFactor = source.clearcoatFactor;
    target.clearcoatRoughnessFactor = source.clearcoatRoughnessFactor;
    target.sheenColorFactor = source.sheenColorFactor;
    target.sheenRoughnessFactor = source.sheenRoughnessFactor;
    target.alphaMode = source.alphaMode;
    target.alphaCutoff = source.alphaCutoff;
    target.doubleSided = source.doubleSided;
}

/* The material a mesh is drawn with when its entity has none: the one of the nearest MaterialNode above it, or else of its first primitive. */
const Material* findSceneSurfaceSource (const Entity& entity, const Mesh& mesh)
{
    for (auto parent = entity.getParent(); parent.isValid(); parent = parent.getParent())
    {
        if (const auto* node = parent.get<MaterialNode>(); node != nullptr && node->material != nullptr)
            return node->material.get();
    }

    return mesh.getNumPrimitives() > 0 ? mesh.getPrimitive (0).material.get() : nullptr;
}

/* How many of the last points asked about a surface remembers: the pointer and where it was pressed, for two pointers. */
constexpr size_t numSceneSurfaceAnswers = 4;

/* Whether the texture coordinates of a mesh span an area for the components to lie over: those of one of its triangles do.
   A mesh that was given none has them all at 0, 0.
*/
bool hasSceneSurfaceTextureArea (const Mesh& mesh)
{
    for (const auto& primitive : mesh.getPrimitives())
    {
        for (size_t i = 0; i + 2 < primitive.indices.size(); i += 3)
        {
            const auto corner = primitive.vertices[primitive.indices[i]].uv;
            const auto edge1 = primitive.vertices[primitive.indices[i + 1]].uv - corner;
            const auto edge2 = primitive.vertices[primitive.indices[i + 2]].uv - corner;

            if (edge1.getX() * edge2.getY() - edge2.getX() * edge1.getY() != 0.0f)
                return true;
        }
    }

    return false;
}

} // namespace

//==============================================================================

SceneSurface::SceneSurface (SceneNodeComponent& nodeToShowOn)
    : node (nodeToShowOn)
{
}

SceneSurface::~SceneSurface()
{
    release();
    setComponents ({});
}

//==============================================================================

void SceneSurface::setComponents (const std::vector<Component*>& newComponents)
{
    // Those that left are painted by their parent again
    for (const auto& previous : components)
    {
        if (auto* component = previous.get(); component != nullptr && std::find (newComponents.begin(), newComponents.end(), component) == newComponents.end())
            component->setManuallyComposited (false);
    }

    components.clear();

    for (auto* component : newComponents)
    {
        component->setManuallyComposited (true);
        components.emplace_back (component);
    }
}

Rectangle<float> SceneSurface::getArea() const
{
    float right = 0.0f;
    float bottom = 0.0f;

    // Of every component, shown or not: the place of the others does not change when one is hidden
    for (const auto& reference : components)
    {
        if (const auto* component = reference.get())
        {
            right = jmax (right, component->getBounds().getRight());
            bottom = jmax (bottom, component->getBounds().getBottom());
        }
    }

    return { 0.0f, 0.0f, right, bottom };
}

Mesh::Ptr SceneSurface::findMesh() const
{
    const auto entity = node.getEntity();
    const auto* meshNode = entity.isValid() ? entity.get<MeshNode>() : nullptr;

    return meshNode != nullptr ? meshNode->mesh : Mesh::Ptr();
}

//==============================================================================

String SceneSurface::update (DataTreeDiagnostic::Severity& severity)
{
    auto* view = SceneComponent::findFor (node);
    auto entity = node.getEntity();
    const auto isShowable = updateMesh();
    const auto mesh = mappedMesh;

    // The material of the node shows the components, or else one of the surface's own, a copy of the one the mesh is drawn
    // with. On the entity it takes the place of the material of every primitive of the mesh: one made of several, drawn
    // with their own materials, does not look as it did
    const auto* materialNode = isShowable ? entity.get<MaterialNode>() : nullptr;
    auto material = materialNode != nullptr ? materialNode->material : Material::Ptr();
    const auto isOwn = material == nullptr || material == ownMaterial;
    const auto* source = isShowable ? findSceneSurfaceSource (entity, *mesh) : nullptr;
    const auto replacesSeveral = isShowable && isOwn && mesh->getNumPrimitives() > 1 && source == mesh->getPrimitive (0).material.get();

    // Told by the scene, last and once: who hears about a problem may destroy the node
    const auto found = view == nullptr || ! entity.isValid() ? Problem::none
                     : mesh == nullptr                       ? Problem::noMesh
                     : ! isShowable                          ? Problem::noTextureArea
                     : replacesSeveral                       ? Problem::severalPrimitives
                                                             : Problem::none;

    const auto reported = std::exchange (reportedProblem, found);
    String problem;

    if (found == Problem::noMesh && found != reported)
        problem = "the components inside a node are shown on the mesh its own entity draws, and this one draws none: write them in a Mesh3D, or in a Node3D bound to the part of a model that shows them";
    else if (found == Problem::noTextureArea && found != reported)
        problem = "the components inside a node are shown over the texture coordinates of its mesh, and the ones of this mesh span no area: give the Mesh3D its uvs, or the part of the model its texture coordinates";
    else if (found == Problem::severalPrimitives && found != reported)
        problem = "the components inside a node are shown by one material, which takes the place of the ones of the " + String (mesh->getNumPrimitives()) + " primitives of its mesh: give the node a Material3D to choose that material, or bind it to a part drawn with one";

    if (found == Problem::severalPrimitives)
        severity = DataTreeDiagnostic::Severity::warning;

    if (reported != Problem::none && reported != found && view != nullptr)
        view->clearProblems (node, {});

    // What the components keep of a device that is not the one of the scene anymore is let go here, where repainting is allowed
    if (std::exchange (areCanvasesOutdated, false))
    {
        for (const auto& reference : components)
        {
            if (auto* component = reference.get())
            {
                component->setManuallyComposited (false);
                component->setManuallyComposited (true);
            }
        }
    }

    // Without a mesh, or texture coordinates to lie over, the components are shown nowhere
    if (! isShowable)
    {
        releaseMaterial();
        return problem;
    }

    if (isOwn)
    {
        // The node has no material: the surface gives its entity one of its own, which looks as the mesh would without it
        if (ownMaterial == nullptr)
            ownMaterial = new Material();

        if (source != nullptr)
        {
            copySceneMaterialLook (*source, *ownMaterial);
        }
        else
        {
            // As the material the renderer draws a mesh that has none with
            Material fallback;
            fallback.metallicFactor = 0.0f;
            fallback.roughnessFactor = 0.5f;

            copySceneMaterialLook (fallback, *ownMaterial);
        }

        if (material == nullptr)
            entity.attach<MaterialNode> (ownMaterial);

        material = ownMaterial;
    }
    else
    {
        // The material of the node, a Material3D for example, took the place of the one of the surface
        ownMaterial = nullptr;
    }

    if (host != material)
    {
        restoreMaterial();
        host = material;
    }

    applyTexture();
    return problem;
}

void SceneSurface::repaintComponents()
{
    // Each one where it is showing: what a hidden one is asked is dropped, and it is rendered again once it is shown
    for (const auto& reference : components)
    {
        if (auto* component = reference.get())
            component->repaint();
    }
}

void SceneSurface::render (GraphicsContext& context, float scale)
{
    if (host == nullptr)
        return;

    // What was rendered with another device is of no use to this one. The components let go of theirs at the next update
    if (const auto previous = std::exchange (device, context.getGpuDevice()); previous != nullptr && previous != device)
    {
        releaseTexture();
        areCanvasesOutdated = true;
    }

    // A node that is hidden shows nothing of them: they are rendered again once it is shown
    if (areCanvasesOutdated || ! node.getEntity().isActive())
        return;

    auto rendered = renderComponents (context, scale);

    if (rendered == nullptr)
    {
        releaseTexture();
        return;
    }

    // The renderer is told of a texture that changed: only when it did, as it then draws the scene again
    if (texture == nullptr)
        texture = new Texture (rendered, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge), true);
    else if (rendered != content)
        texture->setGpuTexture (rendered);

    content = std::move (rendered);
    applyTexture();
}

GpuTexture::Ptr SceneSurface::renderComponents (GraphicsContext& context, float scale)
{
    const auto area = getArea();
    std::vector<Layer> shown;

    for (const auto& reference : components)
    {
        auto* component = reference.get();

        if (component == nullptr || ! component->isVisible())
            continue;

        // Rendered again only when it was repainted, or has another size: a texture that is not the one it gave before
        if (auto rendered = component->renderToTexture (context, scale))
            shown.push_back ({ std::move (rendered), component->getLocalBounds(), component->getCombinedTransform().translated (component->getPosition()) });
    }

    if (shown.empty())
        return nullptr;

    // A component that fills the surface is shown as it is rendered
    if (shown.size() == 1 && shown.front().bounds == area && shown.front().transform.isIdentity())
    {
        composite = nullptr;
        layers.clear();

        return shown.front().texture;
    }

    const auto width = jmax (1, roundToInt (area.getWidth() * scale));
    const auto height = jmax (1, roundToInt (area.getHeight() * scale));
    const auto hasSize = composite != nullptr && composite->getWidth() == width && composite->getHeight() == height;

    if (! hasSize || shown != layers)
    {
        if (! hasSize)
            composite = GpuCanvas::create (context, width, height);

        layers.clear();

        if (composite == nullptr)
            return nullptr;

        auto& g = composite->beginDraw ({}, scale);

        for (const auto& layer : shown)
        {
            g.setTransform (layer.transform);
            g.drawTexture (layer.texture, layer.bounds);
        }

        composite->commit();
        layers = std::move (shown);
    }

    return composite->asTexture();
}

//==============================================================================

void SceneSurface::applyTexture()
{
    if (host == nullptr || texture == nullptr)
        return;

    const auto slot = node.getSurfaceSlot();

    // What the material holds there, when it is not what the surface put, is what its owner set since:
    // it is kept to be given back. The owner sets it at any time, so this is done at each paint
    if (auto& shown = getSceneSurfaceTexture (*host, slot); shown != texture)
        replacedTexture = std::exchange (shown, texture);

    if (auto* factor = getSceneSurfaceFactor (*host, slot); ! isSceneSurfaceFactorWhite (factor))
    {
        std::copy_n (factor, replacedFactor.size(), replacedFactor.begin());
        std::fill_n (factor, replacedFactor.size(), 1.0f);
    }
}

void SceneSurface::restoreMaterial()
{
    if (host != nullptr && texture != nullptr)
    {
        const auto slot = node.getSurfaceSlot();

        // Only what is still as the surface left it: its owner may have set it again since
        if (auto& shown = getSceneSurfaceTexture (*host, slot); shown == texture)
            shown = replacedTexture;

        if (auto* factor = getSceneSurfaceFactor (*host, slot); isSceneSurfaceFactorWhite (factor))
            std::copy_n (replacedFactor.begin(), replacedFactor.size(), factor);
    }

    replacedTexture = nullptr;
    replacedFactor.fill (1.0f);
}

void SceneSurface::releaseMaterial()
{
    restoreMaterial();
    host = nullptr;

    if (ownMaterial == nullptr)
        return;

    // Its own material goes with it, unless another one took its place on the entity
    if (auto entity = node.getEntity(); entity.isValid())
    {
        if (const auto* materialNode = entity.get<MaterialNode>(); materialNode != nullptr && materialNode->material == ownMaterial)
            entity.detach<MaterialNode>();
    }

    ownMaterial = nullptr;
}

void SceneSurface::releaseTexture()
{
    restoreMaterial();

    texture = nullptr;
    content = nullptr;
    composite = nullptr;
    layers.clear();
}

void SceneSurface::release()
{
    releaseMaterial();
    releaseTexture();
    mappedMesh = nullptr;
    answers.clear();

    // The scene the components are shown in next may be drawn with another device: what they keep of this one is let go
    // by the next update, where repainting is allowed
    areCanvasesOutdated = areCanvasesOutdated || device != nullptr;
    device = nullptr;

    if (std::exchange (reportedProblem, Problem::none) != Problem::none)
    {
        if (auto* view = SceneComponent::findFor (node))
            view->clearProblems (node, {});
    }
}

//==============================================================================

bool SceneSurface::updateMesh()
{
    const auto mesh = findMesh();

    // Looked at again when it is another mesh, or one whose vertices changed. Its triangles are only given to the mapper
    // when a point is next asked for: a mesh deformed on every frame is looked at on each of them, and seldom asked about
    if (mesh != mappedMesh || (mesh != nullptr && mesh->getChangeCount() != mappedChangeCount))
    {
        mappedMesh = mesh;
        mappedChangeCount = mesh != nullptr ? mesh->getChangeCount() : 0;
        hasTextureArea = mesh != nullptr && hasSceneSurfaceTextureArea (*mesh);
        isMapperOutdated = true;
    }

    return mappedMesh != nullptr && hasTextureArea;
}

bool SceneSurface::updateMapping (SceneComponent& view)
{
    const auto area = getArea();
    const auto viewport = view.getLocalBounds();

    if (! updateMesh() || ! (area.getWidth() > 0.0f && area.getHeight() > 0.0f && viewport.getWidth() > 0.0f && viewport.getHeight() > 0.0f))
        return false;

    if (std::exchange (isMapperOutdated, false))
    {
        std::vector<Vector3<float>> positions;
        std::vector<Point<float>> uvs;
        std::vector<uint32> indices;

        for (const auto& primitive : mappedMesh->getPrimitives())
        {
            const auto first = static_cast<uint32> (positions.size());

            for (const auto& vertex : primitive.vertices)
            {
                positions.push_back (vertex.position);
                uvs.push_back (vertex.uv);
            }

            for (const auto index : primitive.indices)
                indices.push_back (first + index);
        }

        mapper.setMesh (positions, uvs, indices);
    }

    // As the scene is drawn now: where the entity and the camera are, and the faces its material shows
    mapper.setModelViewProjection (node.getEntity().getWorldMatrix().followedBy (view.getViewProjection()), viewport);
    mapper.setBackFaceCulling (host == nullptr || ! host->doubleSided);

    return true;
}

std::optional<Point<float>> SceneSurface::getSurfacePoint (Point<float> scenePoint)
{
    auto* view = SceneComponent::findFor (node);

    if (view == nullptr || ! updateMapping (*view))
        return std::nullopt;

    const auto find = [this] (Point<float> point)
    {
        return std::find_if (answers.begin(), answers.end(), [&point] (const Answer& answer)
        {
            return answer.scenePoint == point;
        });
    };

    const auto known = find (scenePoint);
    std::optional<Point<float>> surfacePoint;

    if (const auto uv = mapper.viewportToUV (scenePoint))
    {
        const auto area = getArea();
        surfacePoint = Point<float> (uv->getX() * area.getWidth(), uv->getY() * area.getHeight());
    }
    else if (const auto shown = known != answers.end() ? known : find (pointerPoint); shown != answers.end())
    {
        // No triangle faces the point anymore, past the horizon of a mesh that leans back or on one that turned away: it
        // is where the mesh last showed it, or else where it last showed the pointer. A component that holds the pointer
        // stays there, instead of being given the units of the scene, which are not its own
        surfacePoint = shown->surfacePoint;
    }

    if (! surfacePoint.has_value())
        return std::nullopt;

    // The pointer is where a point is asked about for the first time: the other ones an event asks about, where it was
    // pressed and where it rests, are among the last points asked about
    if (known == answers.end())
        pointerPoint = scenePoint;
    else
        answers.erase (known);

    answers.insert (answers.begin(), Answer { scenePoint, *surfacePoint });
    answers.resize (jmin (answers.size(), numSceneSurfaceAnswers));

    return surfacePoint;
}

std::optional<Point<float>> SceneSurface::getScenePoint (Point<float> surfacePoint)
{
    auto* view = SceneComponent::findFor (node);

    if (view == nullptr || ! updateMapping (*view))
        return std::nullopt;

    const auto area = getArea();
    return mapper.uvToViewport ({ surfacePoint.getX() / area.getWidth(), surfacePoint.getY() / area.getHeight() });
}

} // namespace yup
