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

/* One of the textures of a material, with the property holding the path of its image. */
struct SceneMaterialTexture
{
    Identifier property;
    Texture::Ptr Material::* texture;
    bool isSrgb;
    const char* description;
};

const std::array<SceneMaterialTexture, 5>& getSceneMaterialTextures()
{
    static const std::array<SceneMaterialTexture, 5> textures { {
        { Identifier ("baseColorTexture"), &Material::baseColorTexture, true, "The path of the image of the base color and alpha, multiplied by baseColor." },
        { Identifier ("metallicRoughnessTexture"), &Material::metallicRoughnessTexture, false, "The path of the image holding the roughness in green and the metalness in blue, multiplied by roughness and metallic." },
        { Identifier ("normalTexture"), &Material::normalTexture, false, "The path of the image of the tangent space normals." },
        { Identifier ("occlusionTexture"), &Material::occlusionTexture, false, "The path of the image holding the ambient occlusion in red." },
        { Identifier ("emissiveTexture"), &Material::emissiveTexture, true, "The path of the image of the emitted light, multiplied by emissive." },
    } };

    return textures;
}

/* The entity that takes the material of a Material3D inside a component: the one of a node, or the root of a scene. */
Entity findSceneMaterialEntity (Component& host)
{
    if (const auto* node = dynamic_cast<const SceneNodeComponent*> (&host))
        return node->getEntity();

    if (auto* view = dynamic_cast<SceneComponent*> (&host))
        return view->getOrCreateScene().getRoot();

    return {};
}

const Identifier sceneMaterialShaderId { "shader" };

// The seconds given to a shader start again after an hour, as those of an image shader do: a float keeps them precise
constexpr double sceneMaterialTimePeriod = 3600.0;

} // namespace

//==============================================================================
/* Attached to the entity a Material3D naming a shader gives its material to. It tells which component the MaterialShaderNode
   of that entity is the one of, and has the scene call the component on every display frame. */
struct SceneMaterialComponent::ShaderLink final : Node
{
    explicit ShaderLink (SceneMaterialComponent& materialToCall)
        : material (materialToCall)
    {
    }

    SceneMaterialComponent& material;
};

/* What prevents a shader from coloring the meshes of a material, told once the frame has brought every material up to date. */
struct SceneMaterialComponent::ShaderProblem
{
    WeakReference<Component> material; // The one that found it
    DataTree element;                  // Of the `shader` block it is about, or none when it is about the `shader` of the material
    Identifier property;
    String message;
};

//==============================================================================

SceneMaterialComponent::SceneMaterialComponent (StringRef componentID)
    : Component (componentID)
{
    // It paints nothing, takes no room in a layout and leaves the pointer to the nodes, whatever size it is given
    setOpaque (false);
    setExcludedFromParentLayout (true);
    setWantsMouseEvents (false, false);
}

SceneMaterialComponent::~SceneMaterialComponent()
{
    isLeaving = true;
    detach();

    if (auto* parent = getParentComponent())
        updateHost (*parent);
}

//==============================================================================

template <class Value>
void SceneMaterialComponent::setValue (Value& value, const Value& newValue, const Identifier& property)
{
    if (value == newValue)
        return;

    value = newValue;

    // The renderer compares the values of the materials it draws at each frame: repainting shows the new one
    repaintScene();
    notifyPropertyChanged (property);
}

void SceneMaterialComponent::setColorValue (Color& color, Color newColor, const Identifier& property)
{
    if (color == newColor)
        return;

    color = newColor;

    // The colors of the interface are sRGB encoded, the factors of a material are linear
    const auto linear = [] (Color encoded)
    {
        return std::array<float, 3> { lightingSrgbToLinear (encoded.getRedFloat()),
                                      lightingSrgbToLinear (encoded.getGreenFloat()),
                                      lightingSrgbToLinear (encoded.getBlueFloat()) };
    };

    const auto base = linear (baseColor);

    material->baseColorFactor = { base[0], base[1], base[2], baseColor.getAlphaFloat() };
    material->emissiveFactor = linear (emissive);
    material->specularColorFactor = linear (specularColor);
    material->sheenColorFactor = linear (sheenColor);

    repaintScene();
    notifyPropertyChanged (property);
}

void SceneMaterialComponent::setTexturePath (size_t slot, const String& newPath)
{
    if (texturePaths[slot] == newPath)
        return;

    texturePaths[slot] = newPath;
    repaintScene();

    // What prevents the texture from loading is reported once it is set, and can reach a listener that deletes the component
    const BailOutChecker checker (this);
    updateTexture (slot);

    if (! checker.shouldBailOut())
        notifyPropertyChanged (getSceneMaterialTextures()[slot].property);
}

void SceneMaterialComponent::updateTexture (size_t slot)
{
    const auto& texture = getSceneMaterialTextures()[slot];
    auto* view = SceneComponent::findFor (*this);

    material.get()->*texture.texture = nullptr;

    // Read once the material is used, through the scene it is then in
    if (view == nullptr || ! target.isValid())
        return;

    view->clearProblems (*this, texture.property);

    if (texturePaths[slot].isEmpty())
        return;

    if (const auto loaded = view->loadTexture (texturePaths[slot], texture.isSrgb); loaded.wasOk())
        material.get()->*texture.texture = loaded.getReference();
    else
        view->reportProblem (*this, texture.property, loaded.getErrorMessage());
}

void SceneMaterialComponent::assetChanged (const String& path)
{
    if (shaderSheet != nullptr)
    {
        const auto source = shaderSheet->findShader (shaderName).getProperty ("material").toString();

        if (source.isEmpty() || path == source || path == source + ".raster.ysl" || path == source + ".full.ysl")
            shaderSheet = nullptr;
    }

    for (size_t slot = 0; slot < texturePaths.size(); ++slot)
    {
        if (texturePaths[slot] == path)
            updateTexture (slot);
    }

    repaintScene();
}

void SceneMaterialComponent::repaintScene()
{
    if (auto* view = SceneComponent::findFor (*this))
        view->repaintScene();
}

//==============================================================================

void SceneMaterialComponent::updateHost (Component& hostToUpdate)
{
    const SceneMaterialComponent* first = nullptr;

    for (int i = 0; i < hostToUpdate.getNumChildComponents(); ++i)
    {
        auto* child = dynamic_cast<SceneMaterialComponent*> (hostToUpdate.getChildComponent (i));

        if (child == nullptr || child->isLeaving)
            continue;

        if (first == nullptr)
            first = child;

        child->attachTo (child == first ? findSceneMaterialEntity (hostToUpdate) : Entity(), child == first);
    }

    auto* view = dynamic_cast<SceneComponent*> (&hostToUpdate);

    if (view == nullptr)
        view = SceneComponent::findFor (hostToUpdate);

    if (view != nullptr)
        view->repaintScene();
}

void SceneMaterialComponent::attachTo (Entity newTarget, bool isUsed)
{
    auto* view = SceneComponent::findFor (*this);

    if (view != nullptr)
    {
        view->clearProblems (*this, {});

        if (! isUsed)
            view->reportProblem (*this, {}, "the parent already has a Material3D, this one is not used");
    }

    // What is left of an entity that went with its node
    if (! target.isValid())
        detach();

    if (newTarget == target)
        return;

    detach();

    if (newTarget.isValid())
    {
        target = newTarget;
        scene = newTarget.getScene();
        target.attach<MaterialNode> (material);
        updateShaderLink();

        for (size_t slot = 0; slot < texturePaths.size(); ++slot)
            updateTexture (slot);
    }
}

void SceneMaterialComponent::detach()
{
    // Only its own: the material of another Material3D may have taken its place
    if (target.isValid())
    {
        if (const auto* node = target.get<MaterialNode>(); node != nullptr && node->material == material)
            target.detach<MaterialNode>();

        if (const auto* link = target.get<ShaderLink>(); link != nullptr && &link->material == this)
        {
            target.detach<MaterialShaderNode>();
            target.detach<ShaderLink>();
        }
    }

    target = {};
    scene = nullptr;
}

void SceneMaterialComponent::parentHierarchyChanged()
{
    auto* parent = getParentComponent();

    // Gone from the parent it was the material of, where the next Material3D takes over
    if (auto* previous = host.get(); previous != nullptr && previous != parent)
    {
        detach();
        updateHost (*previous);
    }

    host = parent;

    if (parent != nullptr)
        updateHost (*parent);
}

//==============================================================================

void SceneMaterialComponent::setShaderName (const String& newName)
{
    if (shaderName == newName)
        return;

    shaderName = newName;
    updateShaderLink();

    repaintScene();
    notifyPropertyChanged (sceneMaterialShaderId);
}

void SceneMaterialComponent::updateShaderLink()
{
    if (! target.isValid())
        return;

    const auto* link = target.get<ShaderLink>();
    const auto isLinked = link != nullptr && &link->material == this;

    // The shader is looked for by the next frame of the scene, which the link has call the component
    shaderSheet = nullptr;

    if (shaderName.isNotEmpty() == isLinked)
        return;

    // The shader that was there goes: the one of the material this one takes the place of, or its own once it names none
    target.detach<MaterialShaderNode>();

    if (isLinked)
        target.detach<ShaderLink>();
    else
        target.attach<ShaderLink> (*this);
}

bool SceneMaterialComponent::updateShaders (Scene& shownScene, double elapsedSeconds)
{
    bool changed = false;
    std::vector<ShaderProblem> problems;

    shownScene.view<ShaderLink>().forEach ([&] (Entity, ShaderLink& link)
    {
        changed = link.material.refreshShader (elapsedSeconds, problems) || changed;
    });

    // What is wrong is told once every material is up to date: it can reach a listener that deletes a material, takes
    // it out of its scene or lets go of the scene, so each of them is looked for again
    for (const auto& problem : problems)
    {
        auto* material = problem.material.get();
        auto* view = material != nullptr ? SceneComponent::findFor (*material) : nullptr;

        if (view == nullptr)
            continue;

        if (problem.element.isValid())
        {
            // What a `shader` block has wrong is told where it is written, as the styles tell what prevents an image
            // shader from loading: once, whatever the number of materials naming it
            view->reportProblem (problem.element, problem.property, problem.message);
            continue;
        }

        view->reportProblem (*material, problem.property, problem.message);

        // Not left for the scene to tell again with each load: by then the stylesheet may be another one, which the
        // material only finds out on a frame, where it tells what is wrong itself
        material = problem.material.get();

        if (auto* current = material != nullptr ? SceneComponent::findFor (*material) : nullptr)
            current->clearProblems (*material, problem.property);
    }

    return changed;
}

bool SceneMaterialComponent::hasShadersIn (Scene& shownScene)
{
    return ! shownScene.view<ShaderLink>().isEmpty();
}

bool SceneMaterialComponent::refreshShader (double elapsedSeconds, std::vector<ShaderProblem>& problems)
{
    // Out of the tree of its scene with a node that may only be on its way to another parent, the material is styled
    // by nothing: its shader is left as it is, and looked at again once the node is back
    if (! target.isActive())
    {
        for (const auto* parent = getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
        {
            if (const auto* node = dynamic_cast<const SceneNodeComponent*> (parent); node != nullptr && node->getLeftScene() != nullptr)
                return false;
        }
    }

    const auto* context = findStyleContext();
    const auto* document = ComponentDocument::findFor (this);

    auto sheet = context != nullptr ? context->getStyleSheet() : StyleSheet::Ptr();
    const auto load = document != nullptr ? document->getSourceMap() : nullptr;

    // Looked for again when the material names another shader, is given to another entity, is styled by another stylesheet
    // or is told that a file changed. And once after each load of its document, which forgets what it was told, for what
    // still stands to be told again: the source map is another one for each load
    if (sheet != shaderSheet || load != shaderLoad.lock())
    {
        shaderLoad = load;
        rendererProblem.clear();
        updateShader (std::move (sheet), problems);
    }

    auto* node = target.get<MaterialShaderNode>();
    if (node == nullptr)
        return false;

    // What the renderer found of the shader when the scene was last painted: known a frame after it was first drawn with
    updateRendererProblem (*node, problems);

    // Hidden, the meshes are not drawn: the values are read again once they show
    return target.isActive() && updateUniforms (*node, elapsedSeconds);
}

void SceneMaterialComponent::updateRendererProblem (const MaterialShaderNode& node, std::vector<ShaderProblem>& problems)
{
    auto* view = SceneComponent::findFor (*this);

    if (view == nullptr || node.getShader() == nullptr)
        return;

    // Told once for as long as it stands, and again after each load: the renderer finds it at every frame it paints
    auto cause = view->getRenderer().getMaterialShaderError (*node.getShader());

    if (cause == rendererProblem)
        return;

    rendererProblem = std::move (cause);

    if (rendererProblem.isEmpty())
        return;

    // A problem of the block, as what prevents its material from loading is: once, whatever the number of materials naming it
    const auto block = shaderSheet != nullptr ? shaderSheet->findShader (shaderName) : DataTree();

    addShaderProblem (problems,
                      block,
                      block.isValid() ? Identifier ("material") : sceneMaterialShaderId,
                      "shader " + shaderName + ": the renderer can't draw with it, which leaves the meshes to their material: " + rendererProblem);
}

void SceneMaterialComponent::addShaderProblem (std::vector<ShaderProblem>& problems, const DataTree& element, const Identifier& property, const String& message)
{
    // What a block has wrong is the same for every material naming it
    const auto isKnown = element.isValid() && std::any_of (problems.begin(), problems.end(), [&] (const ShaderProblem& known)
    {
        return known.element == element && known.property == property && known.message == message;
    });

    if (! isKnown)
        problems.push_back ({ WeakReference<Component> (this), element, property, message });
}

void SceneMaterialComponent::updateShader (StyleSheet::Ptr sheet, std::vector<ShaderProblem>& problems)
{
    auto* view = SceneComponent::findFor (*this);
    shaderSheet = std::move (sheet);

    if (view == nullptr)
        return;

    const auto block = shaderSheet != nullptr ? shaderSheet->findShader (shaderName) : DataTree();
    const auto path = block.getProperty ("material").toString();

    MaterialShader::Ptr shader;
    std::vector<ShaderInput> inputs;

    if (! block.isValid())
    {
        addShaderProblem (problems, {}, sceneMaterialShaderId, "the stylesheet has no shader '" + shaderName + "'");
    }
    else if (path.isEmpty())
    {
        addShaderProblem (problems, {}, sceneMaterialShaderId, "the shader '" + shaderName + "' has a 'fragment', a material takes a shader with a 'material'");
    }
    else if (const auto loaded = view->loadMaterialShader (path); loaded.failed())
    {
        // Told at the block and worded as what prevents an image shader from loading is: files that can't be read by
        // the name of the shader, and what the loader or the compiler says after it
        const auto cause = loaded.getErrorMessage();

        addShaderProblem (problems, block, "material", cause.startsWith ("can't read ") ? "The shader '" + shaderName + "' " + cause : "shader " + shaderName + ": " + cause);
    }
    else if (readShaderInputs (block, loaded.getReference()->createUniforms(), inputs, problems))
    {
        shader = loaded.getReference();
    }

    auto* node = target.get<MaterialShaderNode>();

    // What can't be used leaves the shader it was to replace coloring the meshes, when it is the one of the same file: a
    // mistake made while editing a material does not turn it off
    if (shader == nullptr && node != nullptr && path.isNotEmpty() && path == shaderPath)
        return;

    shaderPath = shader != nullptr ? path : String();
    shaderInputs = std::move (inputs);
    fillsShaderTime = false;

    if (shader == nullptr)
    {
        target.detach<MaterialShaderNode>();
    }
    else
    {
        // The values of a node start again from nothing when it is given another shader: the frame reads them at once
        if (node == nullptr)
            node = &target.attach<MaterialShaderNode> (shader);
        else if (node->getShader() != shader)
            node->setShader (shader);

        const auto hasTimeInput = std::any_of (shaderInputs.begin(), shaderInputs.end(), [] (const ShaderInput& input)
        {
            return input.property.name == StringRef ("time");
        });

        // The seconds the material has been shown for, unless a property of that name gives the member its value
        fillsShaderTime = ! hasTimeInput && node->getUniforms().hasMember ("time");
    }

    repaintScene();
}

bool SceneMaterialComponent::readShaderInputs (const DataTree& block, const ShaderUniforms& uniforms, std::vector<ShaderInput>& inputs, std::vector<ShaderProblem>& problems)
{
    // What a default names is found as the values of the document find it: its elements by their id, and its models
    const auto* document = ComponentDocument::findFor (this);
    const auto resolver = document != nullptr ? document->getComponentResolver() : DrawingScope::ComponentResolver();
    bool isUsable = true;

    for (const auto& property : block)
    {
        const auto name = property.getProperty ("name").toString().trim();

        if (property.getType().toString() != "property" || name.isEmpty())
            continue;

        const auto typeName = property.getProperty ("type").toString().trim();
        const auto type = ComponentProperty::getTypeFromName (typeName);

        // Of the arrays, the one of numbers fills a uniform
        const auto isFilling = type != ComponentProperty::Type::array
                            || ComponentProperty::getElementTypeFromName (typeName) == ComponentProperty::Type::number;

        const auto numComponents = isFilling ? ShaderEffect::getNumUniformComponents (type) : 0;
        String problem;

        if (numComponents == 0)
            problem = "The property '" + name + "' of the shader '" + shaderName + "' can't be of type '" + typeName + "': a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]";
        else if (! uniforms.hasMember (name))
            problem = "The uniform block of the shader '" + shaderName + "' has no member '" + name + "'";
        else if (uniforms.getNumComponents (name) < numComponents)
            problem = "The member '" + name + "' of the uniform block of the shader '" + shaderName + "' is too small for a " + typeName;

        // Told at the property, as the ones of an image shader are
        if (problem.isNotEmpty())
        {
            addShaderProblem (problems, property, {}, problem);
            isUsable = false;
            continue;
        }

        auto& input = inputs.emplace_back();
        input.property = { Identifier (name), type };

        const auto fallback = property.getProperty ("value").toString().trim();
        if (fallback.isEmpty())
            continue;

        // The default of a block is an expression of the document, evaluated for the material: a constant is kept, and what
        // reads a role of the palette or a property is evaluated again as long as the property has no value
        auto compiled = PropertyExpression::compile (*this, fallback, type, resolver);

        if (compiled.failed())
            addShaderProblem (problems, property, {}, "The default of the property '" + name + "' of the shader '" + shaderName + "' can't be used: " + compiled.getErrorMessage());
        else if (compiled.getReference()->hasInputs())
            input.expression = std::move (compiled.getReference());
        else
            input.fallback = compiled.getReference()->evaluate();
    }

    return isUsable;
}

var SceneMaterialComponent::findShaderInput (const Identifier& name) const
{
    // What a component holds under a name its type declares is its own property, the scale of a node or the time of a
    // model, also when a style gives it: it was not given to the shaders inside it
    const auto owns = [&name] (const Component& holder)
    {
        const auto& type = holder.getTypeInfo();
        return type.findProperty (name) != nullptr || type.findPropertyByStorageKey (name) != nullptr;
    };

    for (const Component* from = this; from != nullptr;)
    {
        if (from != this && owns (*from))
        {
            from = from->getParentComponent();
            continue;
        }

        // A property of the material itself, a value it was given, or else the nearest one a component around it holds
        const auto source = from->findPropertySource (name);

        if (source.component == nullptr || source.component == this || ! owns (*source.component))
            return source.value;

        from = source.component->getParentComponent();
    }

    return {};
}

bool SceneMaterialComponent::updateUniforms (MaterialShaderNode& node, double elapsedSeconds)
{
    if (node.getShader() == nullptr)
        return false;

    // From zeros, which is what a property without a value leaves its member at
    auto values = node.getShader()->createUniforms();

    for (const auto& input : shaderInputs)
    {
        auto value = findShaderInput (input.property.name);

        if (value.isVoid())
            value = input.expression != nullptr ? input.expression->evaluate() : input.fallback;

        if (input.property.type != ComponentProperty::Type::color)
        {
            ShaderEffect::setUniform (values, input.property, value);
            continue;
        }

        // The colors of the interface are sRGB encoded, and a material returns a linear color: its inputs are linear too
        const auto color = VariantConverter<Color>::fromVar (value);
        const float linear[] = { lightingSrgbToLinear (color.getRedFloat()),
                                 lightingSrgbToLinear (color.getGreenFloat()),
                                 lightingSrgbToLinear (color.getBlueFloat()),
                                 color.getAlphaFloat() };

        values.set (input.property.name.toString(), linear);
    }

    if (fillsShaderTime)
    {
        shaderTime = std::fmod (shaderTime + elapsedSeconds, sceneMaterialTimePeriod);
        values.set ("time", static_cast<float> (shaderTime));
    }

    // Values that change restart what the renderer accumulates over the frames: equal ones are not given to it again
    if (values == node.getUniforms())
        return false;

    node.getUniforms() = std::move (values);
    return true;
}

//==============================================================================

const ComponentTypeInfo& SceneMaterialComponent::getClassTypeInfo()
{
    static const auto info = []
    {
        using Self = SceneMaterialComponent;

        const Material defaults {};
        ComponentTypeBuilder<Self> builder (Identifier ("Material3D"));

        // A factor of the material, which holds it
        const auto addFactor = [&]<class Value> (const char* name, Value Material::* field, const char* description)
        {
            const Identifier property (name);

            builder.styleableProperty (
                property,
                [field] (const Self& self) { return self.material.get()->*field; },
                [field, property] (Self& self, Value value) { self.setValue (self.material.get()->*field, value, property); },
                defaults.*field,
                description);
        };

        // A color is kept as it is given, and reads back the same: the material holds what it is converted to
        const auto addColor = [&] (const char* name, Color Self::* color, Color defaultColor, const char* description)
        {
            const Identifier property (name);

            builder.styleableProperty (
                property,
                [color] (const Self& self) { return self.*color; },
                [color, property] (Self& self, Color value) { self.setColorValue (self.*color, value, property); },
                defaultColor,
                description);
        };

        const auto addTexture = [&] (size_t slot)
        {
            const auto& texture = getSceneMaterialTextures()[slot];

            builder.styleableProperty (
                texture.property,
                [slot] (const Self& self) { return self.texturePaths[slot]; },
                [slot] (Self& self, const String& path) { self.setTexturePath (slot, path); },
                String(),
                texture.description);
        };

        const auto addAlphaMode = [&]
        {
            const Identifier property ("alphaMode");
            const StringArray modes { "opaque", "mask", "blend" };

            builder.addProperty (ComponentProperty {
                .name = property,
                .type = ComponentProperty::Type::enumeration,
                .defaultValue = modes[static_cast<int> (defaults.alphaMode)],
                .getter = [modes] (const Component& component)
                {
                    return var (modes[static_cast<int> (static_cast<const Self&> (component).material->alphaMode)]);
                },
                .setter = [modes, property] (Component& component, const var& value)
                {
                    auto& self = static_cast<Self&> (component);

                    if (const auto index = modes.indexOf (value.toString()); index >= 0)
                        self.setValue (self.material->alphaMode, static_cast<Material::AlphaMode> (index), property);
                },
                .enumValues = modes,
                .description = "How the alpha of the base color is used: ignored (opaque), cut out below alphaCutoff (mask) or blended over what is behind (blend)." });
        };

        addColor ("baseColor", &Self::baseColor, Colors::white, "The base color of the surface, and its alpha.");
        addTexture (0);
        addFactor ("metallic", &Material::metallicFactor, "How metallic the surface is, from 0 for a non-metal to 1.");
        addFactor ("roughness", &Material::roughnessFactor, "How rough the surface is, from 0 for a mirror to 1.");
        addTexture (1);
        addTexture (2);
        addFactor ("normalScale", &Material::normalScale, "How strongly the normal texture bends the surface.");
        addTexture (3);
        addFactor ("occlusionStrength", &Material::occlusionStrength, "How much of the occlusion texture is applied, from 0 to 1.");
        addColor ("emissive", &Self::emissive, Colors::black, "The color of the light the surface emits.");
        addTexture (4);
        addFactor ("emissiveStrength", &Material::emissiveStrength, "The multiplier of the emitted light, above 1 for emitters brighter than white.");
        addAlphaMode();
        addFactor ("alphaCutoff", &Material::alphaCutoff, "The alpha below which a masked surface is cut out.");
        addFactor ("doubleSided", &Material::doubleSided, "Whether the back faces are drawn too.");
        addFactor ("ior", &Material::ior, "The index of refraction of a non-metal, which tells how much it reflects seen from the front: 4% at 1.5, everything at 0.");
        addFactor ("specular", &Material::specularFactor, "The strength of the reflections of a non-metal, from 0 for none to 1.");
        addColor ("specularColor", &Self::specularColor, Colors::white, "The color tinting the reflections of a non-metal.");
        addFactor ("clearcoat", &Material::clearcoatFactor, "The strength of a clear varnish over the surface, 0 for none.");
        addFactor ("clearcoatRoughness", &Material::clearcoatRoughnessFactor, "How rough the varnish is.");
        addColor ("sheenColor", &Self::sheenColor, Colors::black, "The color of a velvet-like sheen at grazing angles, black for none.");
        addFactor ("sheenRoughness", &Material::sheenRoughnessFactor, "How wide the sheen spreads.");

        // A name of the document, written bare as the one of a drawing is: the type of the names
        builder.addProperty (ComponentProperty {
            .name = sceneMaterialShaderId,
            .type = ComponentProperty::Type::drawing,
            .defaultValue = String(),
            .getter = [] (const Component& component)
            {
                return var (static_cast<const Self&> (component).getShaderName());
            },
            .setter = [] (Component& component, const var& value)
            {
                static_cast<Self&> (component).setShaderName (value.toString());
            },
            .description = "The name of a shader block of the stylesheet whose material colors the meshes instead of the lighting of this one. The properties the block declares are read from the Material3D." });

        return builder.build();
    }();

    return info;
}

} // namespace yup
