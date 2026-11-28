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

struct SceneComponentIds
{
    static inline const Identifier background { "background" };
    static inline const Identifier ambient { "ambient" };
    static inline const Identifier exposure { "exposure" };
    static inline const Identifier toneMapping { "toneMapping" };
    static inline const Identifier defaultLight { "defaultLight" };
    static inline const Identifier continuous { "continuous" };
    static inline const Identifier tier { "tier" };
    static inline const Identifier samples { "samples" };
    static inline const Identifier bloom { "bloom" };
    static inline const Identifier environment { "environment" };
    static inline const Identifier environmentIntensity { "environmentIntensity" };
    static inline const Identifier environmentVisible { "environmentVisible" };
    static inline const Identifier loading { "loading" };
};

/* Whether a glTF file names a WebP image, by its extension, its type or its bytes. libwebp, as it is built here, fills the
   tables its decoders share with no lock: such a model is parsed on the message thread, as every image is decoded today. */
bool isSceneModelNamingWebp (const MemoryBlock& bytes)
{
    const std::string_view text (static_cast<const char*> (bytes.getData()), bytes.getSize());

    return text.find ("webp") != std::string_view::npos || text.find ("WEBP") != std::string_view::npos;
}

/* Whether an image is one libwebp or libtiff decodes. The PixarLog codec of libtiff writes to statics too: both are
   decoded on the message thread. */
bool isSceneImageSharingItsDecoder (const MemoryBlock& bytes)
{
    ImageFormatManager formats;
    formats.registerDefaultFormats (ImageFormatType::webp | ImageFormatType::tiff);

    return formats.createReaderFor (new MemoryInputStream (bytes, false)) != nullptr;
}

/* The light a panorama surrounds a scene with: its columns go around the vertical axis and its rows from the zenith to the
   nadir. Its colors, sRGB encoded as those of the interface are, become linear light. */
EnvironmentMap::Ptr makeSceneEnvironment (const Image& panorama)
{
    const auto width = panorama.getWidth();
    const auto height = panorama.getHeight();

    const auto radiance = [&panorama, width, height] (const Vector3<float>& direction)
    {
        const auto at = EnvironmentMap::getTextureCoordinates (direction);
        const auto color = panorama.getPixelColor (jlimit (0, width - 1, static_cast<int> (at.getX() * static_cast<float> (width))),
                                                   jlimit (0, height - 1, static_cast<int> (at.getY() * static_cast<float> (height))));

        return Vector3<float> (lightingSrgbToLinear (color.getRedFloat()),
                               lightingSrgbToLinear (color.getGreenFloat()),
                               lightingSrgbToLinear (color.getBlueFloat()));
    };

    // Sampled no finer than the image is, nor than an environment made by code
    return new EnvironmentMap (radiance, jlimit (8, EnvironmentMap::defaultWidth, nextPowerOfTwo (width)));
}

/* The first active camera below a component, depth first, whose entity is in a scene and is not the one going. */
SceneCameraComponent* findActiveSceneCamera (const Component& parent, const Scene& scene, const Entity& leaving)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        auto* node = dynamic_cast<SceneNodeComponent*> (parent.getChildComponent (i));

        // A node that left the tree of the scene shows nothing in it, nor do the nodes inside it
        if (node == nullptr || node->getLeftScene() != nullptr)
            continue;

        if (auto* camera = dynamic_cast<SceneCameraComponent*> (node))
        {
            const auto entity = camera->getEntity();

            // While the nodes move to another scene, the ones not there yet still have the entity they had
            if (camera->isActive() && entity.isValid() && entity.getScene() == &scene && entity != leaving)
                return camera;
        }

        if (auto* camera = findActiveSceneCamera (*node, scene, leaving))
            return camera;
    }

    return nullptr;
}

/* The materials and the models below a component, which are told when a file changes. Kept weakly: telling one may destroy another. */
void findSceneAssetUsers (const Component& parent, std::vector<WeakReference<Component>>& users)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        auto* child = parent.getChildComponent (i);

        if (dynamic_cast<SceneMaterialComponent*> (child) != nullptr || dynamic_cast<SceneModelComponent*> (child) != nullptr)
            users.emplace_back (child);

        if (dynamic_cast<SceneNodeComponent*> (child) != nullptr)
            findSceneAssetUsers (*child, users);
    }
}

/* Whether a component inside a scene is shown as a component, over the scene it is written directly in or on the mesh of the
   node it is in: every component that is not part of the scene. */
bool isSceneSurfaceComponent (const Component& component)
{
    return dynamic_cast<const SceneNodeComponent*> (&component) == nullptr
        && dynamic_cast<const SceneMaterialComponent*> (&component) == nullptr;
}

/* Calls a function with each node below a component, a node before the ones inside it. The function must not change the components. */
template <class Function>
void forEachSceneNode (const Component& parent, Function&& function)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        if (auto* node = dynamic_cast<SceneNodeComponent*> (parent.getChildComponent (i)))
        {
            function (*node);
            forEachSceneNode (*node, function);
        }
    }
}

/* The node below a component whose entity is the given one: the first one met, depth first. */
SceneNodeComponent* findSceneNode (const Component& parent, const Entity& entity)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        auto* node = dynamic_cast<SceneNodeComponent*> (parent.getChildComponent (i));
        if (node == nullptr)
            continue;

        if (node->getEntity() == entity)
            return node;

        if (auto* found = findSceneNode (*node, entity))
            return found;
    }

    return nullptr;
}

/* How a scene is seen in an area: through its active camera, or the one the renderer draws a scene without a camera through. */
struct SceneViewpoint
{
    SceneViewpoint (const Scene& scene, Rectangle<float> areaToSeeIn)
        : area (areaToSeeIn)
    {
        if (const auto* active = scene.getActiveCamera())
        {
            camera = active;
            world = active->getEntity().isValid() ? active->getEntity().getWorldMatrix() : Matrix4();
        }
    }

    std::optional<Ray> getRay (Point<float> position) const
    {
        auto ray = camera->getRay (position, area);

        // A camera that has no entity gives the ray in its own space
        if (ray.has_value() && camera == &fallback)
            ray = ray->transformedBy (world);

        return ray;
    }

    Matrix4 getViewProjection() const
    {
        const auto aspect = area.getWidth() > 0.0f && area.getHeight() > 0.0f ? area.getWidth() / area.getHeight() : 1.0f;

        // A far plane at infinity can't be mapped back from. Where the points are shown does not depend on it: any plane does
        const auto isInfinite = camera->projection == CameraNode::Projection::perspective && camera->zFar <= camera->zNear;
        const auto projection = isInfinite ? Matrix4::perspective (camera->yFov, aspect, camera->zNear, camera->zNear * 2.0f)
                                           : camera->getProjectionMatrix (aspect);

        return world.inverted().followedBy (projection);
    }

    CameraNode fallback;
    const CameraNode* camera = &fallback;
    Matrix4 world = Matrix4::translation ({ 0.0f, 0.0f, 5.0f });
    Rectangle<float> area;
};

} // namespace

//==============================================================================
/* Follows the document that built the component: its loads, and the files read through it that change. */
struct SceneComponent::DocumentLink final : ComponentDocument::Listener
{
    explicit DocumentLink (SceneComponent& ownerToUse)
        : owner (ownerToUse)
    {
    }

    void documentLoaded (ComponentDocument&) override
    {
        owner.documentLoaded();
    }

    void documentAssetChanged (ComponentDocument&, const String& path) override
    {
        owner.assetChanged (path);
    }

    SceneComponent& owner;
};

//==============================================================================
/* The thread of a scene: it runs what is slow of the loads it is given, one after the other, and has the scene keep what
   each one made. */
struct SceneComponent::Loader final
    : Thread
    , AsyncUpdater
{
    /* What is slow of a load, which returns what keeps its result: the message thread calls that. */
    using Task = std::packaged_task<std::function<void()>()>;

    struct Load
    {
        String path;                             // The one its users are told
        std::future<std::function<void()>> kept; // Ready once the thread has run its task
        bool isOfChangedFile = false;            // Loaded again: what was kept of the file as it was is still shown
    };

    explicit Loader (SceneComponent& ownerToTell)
        : Thread ("Scene3D loader")
        , owner (ownerToTell)
    {
    }

    ~Loader() override
    {
        // The task being run is waited for, the ones that are not started are dropped
        stopThread (-1);
    }

    void add (Task task)
    {
        {
            const ScopedLock sl (lock);
            tasks.push (std::move (task));
        }

        notify();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            Task task;

            {
                const ScopedLock sl (lock);

                if (! tasks.empty())
                {
                    task = std::move (tasks.front());
                    tasks.pop();
                }
            }

            if (! task.valid())
            {
                wait (-1);
                continue;
            }

            task();
            triggerAsyncUpdate();
        }
    }

    void handleAsyncUpdate() override
    {
        owner.keepLoaded();
    }

    SceneComponent& owner;
    CriticalSection lock;
    std::queue<Task> tasks;  // Under the lock: given by the message thread, taken by this one
    std::vector<Load> loads; // Of the message thread alone, in the order they were asked for
};

//==============================================================================

SceneComponent::SceneComponent (StringRef componentID)
    : Component (componentID)
{
    setOpaque (true);

    // Clicking takes the focus, so the mouse wheel reaches the scene and not a previously focused component
    setWantsKeyboardFocus (true);
}

SceneComponent::~SceneComponent()
{
    // What the nodes rendered of the components inside them goes with the scene it was shown in: the nodes may outlive it
    forEachSceneNode (*this, [] (SceneNodeComponent& node)
    {
        if (auto* surface = node.getSurface())
            surface->release();
    });

    // So do the entities kept for the nodes that left, which no frame will release. A node still below the component
    // is told by its parent that it goes
    const auto isBelow = [this] (const Component* component)
    {
        while (component != nullptr && component != this)
            component = component->getParentComponent();

        return component == this;
    };

    for (const auto& leaving : std::exchange (leavingNodes, {}))
    {
        if (auto* node = dynamic_cast<SceneNodeComponent*> (leaving.get()); node != nullptr && ! isBelow (node))
            node->releaseLeftEntity (*this);
    }
}

//==============================================================================

void SceneComponent::setScene (Scene::Ptr newScene)
{
    // The features read as the scene shown resolves them: another scene may give them other values
    const auto shown = getSettings();

    // The scene shown before stays until the components below have left it
    const auto previous = std::exchange (scene, std::move (newScene));

    // A node that can't be in the new scene as it was says so, and who hears about it may destroy this component
    const BailOutChecker checker (this);

    if (previous != scene)
        sceneChanged();

    if (checker.shouldBailOut())
        return;

    updateOpaqueState();
    repaintScene();

    // Told last, as any change of a feature is: who hears about it may destroy the component
    const auto settings = getSettings();

    const std::pair<const Identifier*, bool> features[] = { { &SceneComponentIds::background, settings.backgroundColor != shown.backgroundColor },
                                                            { &SceneComponentIds::ambient, settings.ambientColor != shown.ambientColor },
                                                            { &SceneComponentIds::exposure, settings.exposure != shown.exposure },
                                                            { &SceneComponentIds::toneMapping, settings.toneMapping != shown.toneMapping },
                                                            { &SceneComponentIds::defaultLight, settings.useDefaultLight != shown.useDefaultLight },
                                                            { &SceneComponentIds::environmentIntensity, settings.environmentIntensity != shown.environmentIntensity },
                                                            { &SceneComponentIds::environmentVisible, settings.environmentVisible != shown.environmentVisible } };

    for (const auto& [property, isChanged] : features)
    {
        if (isChanged)
            notifyPropertyChanged (*property);

        if (checker.shouldBailOut())
            return;
    }
}

Scene& SceneComponent::getOrCreateScene()
{
    if (scene == nullptr)
    {
        scene = new Scene();
        watchScene();
        updateOpaqueState();
    }

    return *scene;
}

void SceneComponent::sceneChanged()
{
    watchScene();
    lastPick.reset();

    // Their entities are made again in the scene, or in one made for them when it is nullptr. The ones kept for the
    // nodes that left the scene shown before have no scene to come back to
    const BailOutChecker checker (this);
    releaseLeavingNodes();

    if (checker.shouldBailOut())
        return;

    SceneNodeComponent::updateEntitiesBelow (*this);

    if (checker.shouldBailOut())
        return;

    updateActiveCamera();
    SceneMaterialComponent::updateHost (*this);

    // The image the component was given lights the scene it shows
    if (! checker.shouldBailOut() && environment.isNotEmpty())
        updateEnvironment();
}

void SceneComponent::watchScene()
{
    activeCamera = {};
    entityDestroyed = {};

    if (scene == nullptr)
        return;

    // The scene falls back on the first camera it finds once the one it was given goes, a camera of a model included
    entityDestroyed = scene->onDestroy().connect ([this] (Entity entity)
    {
        if (entity == activeCamera)
            updateActiveCamera (entity);
    });
}

void SceneComponent::updateActiveCamera (const Entity& leaving)
{
    if (scene == nullptr)
        return;

    const auto* camera = findActiveSceneCamera (*this, *scene, leaving);
    const auto entity = camera != nullptr ? camera->getEntity() : Entity();

    if (entity == activeCamera)
        return;

    activeCamera = entity;
    scene->setActiveCamera (entity);
    repaint();
}

//==============================================================================

void SceneComponent::setContinuousUpdates (bool shouldUpdateContinuously)
{
    if (continuousUpdates == shouldUpdateContinuously)
        return;

    continuousUpdates = shouldUpdateContinuously;

    // Its frames come whether or not the component is painted
    requestFrame();
    featureChanged (SceneComponentIds::continuous);
}

//==============================================================================

SceneSettings SceneComponent::getSettings() const
{
    return scene != nullptr ? SceneSettings::resolve (*scene) : SceneSettings();
}

void SceneComponent::featureChanged (const Identifier& property)
{
    // Told last: who hears about the change may destroy the component
    repaint();
    notifyPropertyChanged (property);
}

template <class Value>
void SceneComponent::setRenderSetting (Value RenderSettings::* setting, Value newValue, const Identifier& property)
{
    auto settings = renderer.getRenderSettings();
    if (settings.*setting == newValue)
        return;

    settings.*setting = newValue;
    renderer.setRenderSettings (settings);
    featureChanged (property);
}

void SceneComponent::setBackgroundColor (Color newColor)
{
    if (getBackgroundColor() == newColor)
        return;

    getOrCreateScene().getRoot().getOrAttach<BackgroundNode>().color = newColor;
    updateOpaqueState();
    featureChanged (SceneComponentIds::background);
}

Color SceneComponent::getBackgroundColor() const
{
    return getSettings().backgroundColor;
}

void SceneComponent::setAmbientColor (Color newColor)
{
    if (getAmbientColor() == newColor)
        return;

    getOrCreateScene().getRoot().getOrAttach<AmbientLightNode>().color = newColor;
    featureChanged (SceneComponentIds::ambient);
}

Color SceneComponent::getAmbientColor() const
{
    return getSettings().ambientColor;
}

void SceneComponent::setExposure (float newExposure)
{
    clearProblems (*this, SceneComponentIds::exposure);

    // A multiplier that is negative, or that is not a number, exposes nothing: the scene keeps the one it has. Told last,
    // as who hears about it may destroy the component
    if (! (newExposure >= 0.0f && std::isfinite (newExposure)))
    {
        reportProblem (*this, SceneComponentIds::exposure, "expected a multiplier that is not negative");
        return;
    }

    if (getExposure() == newExposure)
        return;

    getOrCreateScene().getRoot().getOrAttach<ExposureNode>().exposure = newExposure;
    featureChanged (SceneComponentIds::exposure);
}

float SceneComponent::getExposure() const
{
    return getSettings().exposure;
}

void SceneComponent::setToneMapping (ToneMappingOperator newOperator)
{
    if (getToneMapping() == newOperator)
        return;

    getOrCreateScene().getRoot().getOrAttach<ToneMappingNode>().type = newOperator;
    featureChanged (SceneComponentIds::toneMapping);
}

ToneMappingOperator SceneComponent::getToneMapping() const
{
    return getSettings().toneMapping;
}

void SceneComponent::setDefaultLightEnabled (bool shouldBeEnabled)
{
    if (isDefaultLightEnabled() == shouldBeEnabled)
        return;

    getOrCreateScene().getRoot().getOrAttach<DefaultLightNode>().enabled = shouldBeEnabled;
    featureChanged (SceneComponentIds::defaultLight);
}

bool SceneComponent::isDefaultLightEnabled() const
{
    return getSettings().useDefaultLight;
}

//==============================================================================

SceneComponent* SceneComponent::findFor (Component& component)
{
    auto* node = dynamic_cast<SceneNodeComponent*> (&component);

    for (auto* parent = component.getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
    {
        if (auto* view = dynamic_cast<SceneComponent*> (parent))
            return view;

        node = dynamic_cast<SceneNodeComponent*> (parent);

        if (node == nullptr)
            return nullptr;
    }

    // The topmost node is in no scene: the one it left, with what is inside it, for as long as it keeps its entity
    return node != nullptr ? node->getLeftScene() : nullptr;
}

ComponentDocument* SceneComponent::findDocument()
{
    auto* document = ComponentDocument::findFor (this);

    if (document != nullptr && documentLink == nullptr)
        documentLink = std::make_unique<DocumentLink> (*this);

    if (documentLink != nullptr)
        documentLink->follow (document);

    return document;
}

//==============================================================================

template <class Key, class Value, class Load>
std::optional<ResultValue<Value>> SceneComponent::loadAsset (std::map<Key, std::optional<ResultValue<Value>>>& kept, const Key& key, const String& path, bool canUseWorker, Load load)
{
    if (canUseWorker && loader == nullptr)
        loader = std::make_unique<Loader> (*this);

    // Where no thread can be started, as in a page that is not cross-origin isolated, the file is loaded here
    if (! canUseWorker || ! (loader->isThreadRunning() || loader->startThread()))
        return kept.insert_or_assign (key, load()).first->second;

    kept.insert_or_assign (key, std::nullopt);

    Loader::Task task ([&kept, key, load = std::move (load)]
    {
        return std::function<void()> ([&kept, key, result = load()]() mutable
        {
            kept.insert_or_assign (key, std::move (result));
        });
    });

    loader->loads.push_back (Loader::Load { path, task.get_future() });
    loader->add (std::move (task));

    // The `loading` state is told by the message loop: who asked for the file is in the middle of using it
    loader->triggerAsyncUpdate();

    return std::nullopt;
}

template <class Key, class Value, class Load>
void SceneComponent::reloadAsset (std::map<Key, std::optional<ResultValue<Value>>>& kept, const Key& key, Load load)
{
    auto previous = kept.extract (key);

    if (previous.empty() || load().has_value())
        return;

    // Read again at once, and shown as it was until the thread has loaded it: nothing goes missing meanwhile
    loader->loads.back().isOfChangedFile = previous.mapped().has_value();
    kept.insert_or_assign (key, std::move (previous.mapped()));
}

template <class Key, class Value, class Make>
std::optional<ResultValue<Value>> SceneComponent::loadImage (std::map<Key, std::optional<ResultValue<Value>>>& kept, const Key& key, const String& path, Make make)
{
    if (const auto found = kept.find (key); found != kept.end())
        return found->second;

    // Followed before reading, to be told when the file changes
    findDocument();
    auto bytes = ComponentDocument::readAssetFor (this, path);

    if (! bytes.has_value())
        return kept.insert_or_assign (key, ResultValue<Value>::fail ("the file '" + path + "' can't be read")).first->second;

    const auto canUseWorker = ! isSceneImageSharingItsDecoder (*bytes);

    return loadAsset (kept, key, path, canUseWorker, [bytes = std::move (*bytes), make = std::move (make)]() -> ResultValue<Value>
    {
        auto image = Image::loadFromData (bytes.asBytes());

        if (image.failed())
            return makeResultValueFail (image.getErrorMessage());

        return makeResultValueOk (make (std::move (image.getReference())));
    });
}

std::optional<ResultValue<GltfModel>> SceneComponent::loadModel (const String& path)
{
    if (const auto found = models.find (path); found != models.end())
        return found->second;

    // Followed before reading, to be told when the file changes
    findDocument();
    auto bytes = ComponentDocument::readAssetFor (this, path);

    if (! bytes.has_value())
        return models.insert_or_assign (path, ResultValue<GltfModel>::fail ("the file '" + path + "' can't be read")).first->second;

    // What a .gltf keeps in other files is read next to it, when the document has files
    const auto file = ComponentDocument::getAssetFileFor (this, path);
    const auto folder = file != File() ? file.getParentDirectory() : File();
    const auto canUseWorker = ! isSceneModelNamingWebp (*bytes);

    return loadAsset (models, path, path, canUseWorker, [bytes = std::move (*bytes), folder]
    {
        return GltfModel::loadFromData (bytes.asBytes(), folder);
    });
}

std::optional<ResultValue<Texture::Ptr>> SceneComponent::loadTexture (const String& path, bool isSrgb)
{
    return loadImage (textures, std::make_pair (path, isSrgb), path, [isSrgb] (Image image)
    {
        // As the textures of a glTF model are sampled when it says nothing
        GpuSamplerDesc sampler (GpuFilter::linear, GpuWrapMode::repeat);
        sampler.mipmapFilter = GpuFilter::linear;
        sampler.maxAnisotropy = 16;

        return Texture::Ptr (new Texture (std::move (image), sampler, isSrgb));
    });
}

ResultValue<MaterialShader::Ptr> SceneComponent::loadMaterialShader (const String& path)
{
    if (const auto found = materialShaders.find (path); found != materialShaders.end())
        return found->second;

    findDocument();

    const auto makeShader = [&]() -> ResultValue<MaterialShader::Ptr>
    {
        // What a build compiles the material into comes first: a variant for each tier of the renderer, either of which can be missing
        const auto raster = ComponentDocument::readAssetFor (this, path + ".raster.ysl");
        const auto full = ComponentDocument::readAssetFor (this, path + ".full.ysl");

        if (raster.has_value() || full.has_value())
        {
            const MemoryBlock missing;
            const auto& rasterBytes = raster.has_value() ? *raster : missing;
            const auto& fullBytes = full.has_value() ? *full : missing;

            return MaterialShader::createFromData (rasterBytes.getData(), rasterBytes.getSize(), fullBytes.getData(), fullBytes.getSize());
        }

        const auto source = ComponentDocument::readAssetFor (this, path);
        if (! source.has_value())
            return makeResultValueFail ("can't read '" + path + "' nor its compiled variants '" + path + ".raster.ysl' and '" + path + ".full.ysl'");

#if YUP_ENABLE_SHADER_TRANSPILER
        // For the graphics API drawing the scene, kept with the material: it is compiled again once another one draws it
        const auto api = findGraphicsApi();
        auto compiled = MaterialShader::createFromSource (source->toString(), MaterialShader::getSourceDirectory(), api);

        if (compiled.wasOk())
            compiledMaterials[path] = api;

        return compiled;
#else
        return makeResultValueFail ("'" + path + "' is the source of a material, which this build can't compile, and there is no '" + path + ".raster.ysl' nor '" + path + ".full.ysl'");
#endif
    };

    return materialShaders.emplace (path, makeShader()).first->second;
}

void SceneComponent::setEnvironment (const String& newPath)
{
    if (environment == newPath)
        return;

    environment = newPath;

    // What prevents reading the image is reported, and can reach a listener that destroys the scene
    const BailOutChecker checker (this);
    updateEnvironment();

    if (! checker.shouldBailOut())
        featureChanged (SceneComponentIds::environment);
}

void SceneComponent::updateEnvironment()
{
    clearProblems (*this, SceneComponentIds::environment);

    EnvironmentMap::Ptr map;
    String problem;

    if (environment.isNotEmpty())
    {
        const auto loaded = loadImage (environments, environment, environment, [] (Image panorama)
        {
            return makeSceneEnvironment (panorama);
        });

        // Being loaded: the scene keeps the environment it has, and is told when the image is there
        if (! loaded.has_value())
            return;

        if (loaded->wasOk())
            map = loaded->getReference();
        else
            problem = loaded->getErrorMessage();
    }

    // A scene that never had an environment is given no node to have none
    if (map != nullptr)
        getOrCreateScene().getRoot().getOrAttach<EnvironmentNode>().map = map;
    else if (auto* node = scene != nullptr ? scene->getRoot().get<EnvironmentNode>() : nullptr)
        node->map = nullptr;

    repaintScene();

    // Told last: who hears about it may destroy the scene
    if (problem.isNotEmpty())
        reportProblem (*this, SceneComponentIds::environment, problem);
}

//==============================================================================

bool SceneComponent::isLoading() const noexcept
{
    return loader != nullptr && ! loader->loads.empty();
}

void SceneComponent::finishLoading()
{
    if (! isLoading())
        return;

    // The users of a file that is kept may ask for others, and who hears about what it has wrong may destroy the scene
    const BailOutChecker checker (this);

    while (! checker.shouldBailOut() && isLoading())
    {
        loader->loads.front().kept.wait();
        keepLoaded();
    }

    if (! checker.shouldBailOut())
        contentLoaded();
}

void SceneComponent::keepLoaded()
{
    // Who hears about the state, or about what a file has wrong, may destroy the scene
    const BailOutChecker checker (this);

    // Told before the files that are there are kept: what read the state since has read it as it is told now
    tellLoading();

    if (checker.shouldBailOut())
        return;

    StringArray paths;

    for (auto load = loader->loads.begin(); load != loader->loads.end();)
    {
        if (load->kept.wait_for (std::chrono::seconds (0)) != std::future_status::ready)
        {
            ++load;
            continue;
        }

        load->kept.get()();
        paths.addIfNotAlreadyThere (load->path);
        load = loader->loads.erase (load);
    }

    // As when a file changes: a node on its way to another parent comes back with entities made from the files as they are
    if (! paths.isEmpty())
        releaseLeavingNodes();

    for (const auto& path : paths)
    {
        if (checker.shouldBailOut())
            return;

        tellAssetUsers (path);
    }

    if (! checker.shouldBailOut())
        tellLoading();
}

void SceneComponent::tellLoading()
{
    if (std::exchange (isLoadingTold, isLoading()) != isLoading())
        notifyPropertyChanged (SceneComponentIds::loading);
}

void SceneComponent::documentLoaded()
{
    const BailOutChecker checker (this);

    // A file that changed is waited for: what the load lists as wrong is what its users find in the file as it is now
    const auto isChanged = loader != nullptr && std::any_of (loader->loads.begin(), loader->loads.end(), [] (const Loader::Load& load)
    {
        return load.isOfChangedFile;
    });

    if (isChanged)
        finishLoading();
    else
        contentLoaded();

    // Once the materials have looked for their shader in the stylesheet of the load
    if (! checker.shouldBailOut())
        forgetUnusedAssets();
}

void SceneComponent::contentLoaded()
{
    // The values of a load are in, or the files it named: the meshes they describe are built, the nodes find the one that shows
    // the components inside them, and what is wrong is told with the rest. Who hears about a problem may destroy the scene
    const BailOutChecker checker (this);

    lastPick.reset();
    releaseLeavingNodes();

    if (! checker.shouldBailOut())
        updateMeshes();

    if (! checker.shouldBailOut())
        updateSurfaces();

    // The frames resolve the shaders of the materials: done here too, for what prevents one to be told with the load
    if (! checker.shouldBailOut())
        updateAnimatedComponents (0.0);

    if (! checker.shouldBailOut())
        sendProblems();
}

void SceneComponent::forgetUnusedAssets()
{
    std::vector<WeakReference<Component>> users;
    findSceneAssetUsers (*this, users);

    const auto isUnused = [this, &users] (const String& path)
    {
        return path != environment && std::none_of (users.begin(), users.end(), [&path] (const WeakReference<Component>& user)
        {
            if (const auto* model = dynamic_cast<const SceneModelComponent*> (user.get()))
                return model->getSource() == path;

            const auto* material = dynamic_cast<const SceneMaterialComponent*> (user.get());
            return material != nullptr && material->isUsingAsset (path);
        });
    };

    // A file being loaded is left to its thread: it is forgotten by the next load, once it is there
    std::erase_if (models, [&isUnused] (const auto& kept) { return kept.second.has_value() && isUnused (kept.first); });
    std::erase_if (textures, [&isUnused] (const auto& kept) { return kept.second.has_value() && isUnused (kept.first.first); });
    std::erase_if (environments, [&isUnused] (const auto& kept) { return kept.second.has_value() && isUnused (kept.first); });
    std::erase_if (materialShaders, [&isUnused] (const auto& kept) { return isUnused (kept.first); });
    std::erase_if (compiledMaterials, [&isUnused] (const auto& compiled) { return isUnused (compiled.first); });
}

void SceneComponent::reportProblem (const DataTree& element, const Identifier& property, const String& message)
{
    // Where the problems of the components go, see sendProblem(): the document finds where the element is written
    if (auto* document = findDocument())
        document->reportDiagnostic ({ .message = message, .element = element, .property = property });
}

void SceneComponent::assetChanged (const String& path)
{
    // A node on its way to another parent is not below the component, to be told: it comes back with entities made
    // again, from the files as they are by then
    const BailOutChecker checker (this);
    releaseLeavingNodes();

    if (checker.shouldBailOut())
        return;

    // Told before a file being loaded is let go of: what read the state since has read it as it is told now
    tellLoading();

    if (checker.shouldBailOut())
        return;

    const auto isOfFile = [&path] (const Loader::Load& load)
    {
        return load.path == path;
    };

    // What the thread makes of the file as it was is dropped when it comes
    if (loader != nullptr)
        std::erase_if (loader->loads, isOfFile);

    reloadAsset (models, path, [&]
    {
        return loadModel (path);
    });

    for (const auto isSrgb : { true, false })
    {
        reloadAsset (textures, std::make_pair (path, isSrgb), [&]
        {
            return loadTexture (path, isSrgb);
        });
    }

    // Forgotten before anything asks for it again: the scene keeps the environment it has until it has asked, and got it
    environments.erase (path);

    // A material shader is kept by the path of its source, whichever of its files changed: that one, or one of its two variants
    for (const auto* suffix : { "", ".raster.ysl", ".full.ysl" })
    {
        if (! path.endsWith (suffix))
            continue;

        const auto source = path.dropLastCharacters (static_cast<int> (std::strlen (suffix)));

        materialShaders.erase (source);
        compiledMaterials.erase (source);
    }

    // The users of a file its thread has are told once it is there, as they are when it is first loaded
    if (loader == nullptr || std::none_of (loader->loads.begin(), loader->loads.end(), isOfFile))
        tellAssetUsers (path);

    if (! checker.shouldBailOut())
        tellLoading();
}

void SceneComponent::tellAssetUsers (const String& path)
{
    // Found before any of them is told: one telling what is wrong with its file may be heard by something that
    // destroys the others, or takes them away
    std::vector<WeakReference<Component>> users;
    findSceneAssetUsers (*this, users);

    const BailOutChecker checker (this);

    for (const auto& user : users)
    {
        if (auto* material = dynamic_cast<SceneMaterialComponent*> (user.get()))
            material->assetChanged (path);
        else if (auto* model = dynamic_cast<SceneModelComponent*> (user.get()))
            model->assetChanged (path);
    }

    // The scene reads the image of its environment itself
    if (! checker.shouldBailOut() && path == environment)
        updateEnvironment();
}

GpuPlatform SceneComponent::findGraphicsApi()
{
    if (auto* native = getNativeComponent())
    {
        if (auto* context = native->getGraphicsContext(); context != nullptr && context->isGpuAvailable())
            return context->getPlatform();
    }

    return paintedApi.has_value() ? *paintedApi : ComponentNative::getDefaultGraphicsApi();
}

void SceneComponent::updateCompiledMaterials()
{
    if (compiledMaterials.empty())
        return;

    const auto api = findGraphicsApi();
    StringArray outdated;

    for (const auto& [path, compiledFor] : compiledMaterials)
    {
        if (compiledFor != api)
            outdated.add (path);
    }

    const BailOutChecker checker (this);

    for (const auto& path : outdated)
    {
        assetChanged (path);

        if (checker.shouldBailOut())
            return;
    }
}

//==============================================================================

void SceneComponent::reportProblem (Component& component, const Identifier& property, const String& message, DataTreeDiagnostic::Severity severity)
{
    problems.push_back ({ WeakReference<Component> (&component), property, message, severity });
    sendProblem (problems.back());
}

void SceneComponent::clearProblems (const Component& component, const Identifier& property)
{
    // With those of the components that are gone: nothing else lets go of them while no document loads
    std::erase_if (problems, [&] (const Problem& problem)
    {
        const auto* reported = problem.component.get();
        return reported == nullptr || (reported == &component && problem.property == property);
    });
}

void SceneComponent::sendProblem (const Problem& problem)
{
    auto* component = problem.component.get();
    auto* document = findDocument();

    if (component == nullptr || document == nullptr)
        return;

    auto subject = component->getTypeInfo().getTypeName().toString();
    if (problem.property.isValid())
        subject << "." << problem.property.toString();

    // The document finds the element the component was built from, and drops what it is told while it loads
    document->reportDiagnostic ({ .message = subject + ": " + problem.message,
                                  .property = problem.property,
                                  .severity = problem.severity },
                                component);
}

void SceneComponent::sendProblems()
{
    // What went away, or to another scene, has nothing left to tell here
    std::erase_if (problems, [this] (const Problem& problem)
    {
        auto* component = problem.component.get();
        return component == nullptr || (component != this && findFor (*component) != this);
    });

    // A copy: who hears about a problem may change the scene, or destroy it
    const BailOutChecker checker (this);

    for (const auto& problem : std::vector<Problem> (problems))
    {
        sendProblem (problem);

        if (checker.shouldBailOut())
            return;
    }
}

//==============================================================================

void SceneComponent::deferMeshUpdate (SceneMeshComponent& mesh)
{
    // Once, and without the ones that are gone: nothing else empties the list while no frame is drawn nor document loaded
    changedMeshes.removeIf ([&mesh] (const WeakReference<Component>& queued)
    {
        return queued.get() == nullptr || queued.get() == &mesh;
    });

    changedMeshes.add (WeakReference<Component> (&mesh));

    // Followed, to build the mesh once the values the document is applying are all in
    findDocument();

    // Built by the next frame, which tells what is wrong with its arrays
    requestFrame();
}

void SceneComponent::updateMeshes()
{
    for (const auto& changed : std::exchange (changedMeshes, {}))
        if (auto* mesh = dynamic_cast<SceneMeshComponent*> (changed.get()))
            mesh->getMesh();
}

void SceneComponent::buildMeshes()
{
    // While painting: the component is not asked to repaint, and what is wrong with the arrays is left for updateMeshes() to tell
    const ScopedValueSetter<bool> updating (isUpdatingScene, true);

    for (const auto& changed : changedMeshes)
        if (auto* mesh = dynamic_cast<SceneMeshComponent*> (changed.get()))
            mesh->updateMesh();
}

void SceneComponent::deferSurfaceUpdate()
{
    // Followed, to find what shows the components once the values the document is applying are all in
    findDocument();
    sceneContentChanged();
}

void SceneComponent::deferEntityRelease (SceneNodeComponent& node)
{
    // Once, and without the ones that are gone or back in a scene: nothing else empties the list while no frame is drawn
    std::erase_if (leavingNodes, [this, &node] (const WeakReference<Component>& leaving)
    {
        const auto* left = dynamic_cast<const SceneNodeComponent*> (leaving.get());
        return left == nullptr || left == &node || left->getLeftScene() != this;
    });

    leavingNodes.emplace_back (&node);

    // Followed, to release the entity when the document loads too: a load is a frame for what it keeps
    findDocument();

    // The scene is not seen through a camera that left it, and is drawn without what the node showed
    updateActiveCamera();
    sceneContentChanged();
}

void SceneComponent::releaseLeavingNodes()
{
    // A node whose entity goes tells what it showed, and who hears about it may destroy the scene
    const BailOutChecker checker (this);

    for (const auto& leaving : std::exchange (leavingNodes, {}))
    {
        if (auto* node = dynamic_cast<SceneNodeComponent*> (leaving.get()))
            node->releaseLeftEntity (*this);

        if (checker.shouldBailOut())
            return;
    }
}

void SceneComponent::updateSurfaces()
{
    struct Finding
    {
        WeakReference<Component> node;
        String problem;
        DataTreeDiagnostic::Severity severity;
    };

    std::vector<Finding> found;

    // Nothing tells when code changes the mesh or the material that show the components: while a node shows some, they
    // are looked at on every frame
    hasSurfaces = false;

    forEachSceneNode (*this, [this, &found] (SceneNodeComponent& node)
    {
        if (auto* surface = node.getSurface())
        {
            auto severity = DataTreeDiagnostic::Severity::error;
            hasSurfaces = true;

            if (const auto problem = surface->update (severity); problem.isNotEmpty())
                found.push_back ({ WeakReference<Component> (&node), problem, severity });
        }
    });

    // Told last, once every node was seen: who hears about a problem may destroy the nodes, and the scene
    const BailOutChecker checker (this);

    for (const auto& [node, problem, severity] : found)
    {
        if (auto* component = node.get())
            reportProblem (*component, {}, problem, severity);

        if (checker.shouldBailOut())
            return;
    }
}

//==============================================================================

SceneNodeComponent* SceneComponent::findNodeAt (Point<float> position)
{
    if (! lastPick.has_value() || lastPick->position != position)
        lastPick = Pick { position, WeakReference<Component> (pickNode (position)) };

    return static_cast<SceneNodeComponent*> (lastPick->node.get());
}

SceneNodeComponent* SceneComponent::pickNode (Point<float> position)
{
    const auto bounds = getLocalBounds();

    if (scene == nullptr || ! bounds.contains (position))
        return nullptr;

    // What is written over the scene takes the pointer first, wherever it is among the components
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        auto* child = getChildComponent (i);

        if (! child->isVisible() || ! isSceneSurfaceComponent (*child))
            continue;

        if (const auto point = child->getLocalPointFromParent (position); point.has_value() && child->findComponentAtForMouseEvent (*point) != nullptr)
            return nullptr;
    }

    const SceneViewpoint viewpoint (*scene, bounds);
    const auto ray = viewpoint.getRay (position);

    if (! ray.has_value())
        return nullptr;

    // Between the planes the camera clips at: a distance along its ray is a depth of its view
    const auto& camera = *viewpoint.camera;
    auto distance = camera.zFar > camera.zNear ? camera.zFar : std::numeric_limits<float>::max();
    Entity nearest;

    for (auto [entity, mesh] : scene->view<MeshNode>())
    {
        if (! entity.isActive())
            continue;

        if (const auto hit = mesh.intersectTriangles (*ray, camera.zNear, distance); hit.has_value())
        {
            nearest = entity;
            distance = *hit;
        }
    }

    // The node of the mesh, or of the nearest entity above it that a node made or is bound to
    for (auto entity = nearest; entity.isValid(); entity = entity.getParent())
    {
        if (auto* node = findSceneNode (*this, entity))
            return node;
    }

    return nullptr;
}

Matrix4 SceneComponent::getViewProjection() const
{
    return scene != nullptr ? SceneViewpoint (*scene, getLocalBounds()).getViewProjection() : Matrix4();
}

void SceneComponent::sceneContentChanged()
{
    lastPick.reset();

    // What changes while the scene is brought up to date for a paint is drawn by that paint
    if (isUpdatingScene)
        return;

    isSceneOutdated = true;
    repaintScene();
}

void SceneComponent::repaintScene()
{
    requestFrame();
    repaint();
}

void SceneComponent::updateScene()
{
    if (scene == nullptr || ! std::exchange (isSceneOutdated, false))
        return;

    // What the nodes changed is followed by what depends on it, a mesh by the joints a node turned, before it is drawn. No
    // time elapses: a clip that plays is posed where it was
    const ScopedValueSetter<bool> updating (isUpdatingScene, true);

    scene->update (0.0);
    lastPick.reset();
}

void SceneComponent::resized()
{
    lastPick.reset();
}

void SceneComponent::childrenChanged()
{
    lastPick.reset();
}

void SceneComponent::visibilityChanged()
{
    showingChanged();
}

void SceneComponent::parentHierarchyChanged()
{
    // Under another parent the component may show or not without its visibility being changed
    showingChanged();
}

void SceneComponent::showingChanged()
{
    const auto showing = isShowing();

    // Showing, its next frame carries on with what was left to do while it was hidden and finds out whether more frames
    // are needed. Hidden, it takes none, which leaves its window free to skip its frames
    frameAttachment.setActive (showing);

    // What a hidden scene was rendered into is of no use until it shows again, where it is made again
    if (! showing)
        renderer.releaseTargets();
}

void SceneComponent::requestFrame()
{
    isFrameRequested = true;

    // A hidden scene waits: it is given its frames once it shows again
    if (! frameAttachment.isActive() && isShowing())
        frameAttachment.setActive (true);
}

//==============================================================================

const ComponentTypeInfo& SceneComponent::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<SceneComponent> (Identifier ("Scene3D"))
                                 .description ("The component showing a scene, laid out as any other. It holds the 3D types, and paints what else it holds over the scene.")
                                 .styleableProperty (SceneComponentIds::background, &SceneComponent::getBackgroundColor, &SceneComponent::setBackgroundColor, SceneSettings().backgroundColor, "The color the scene is cleared to. With an alpha below 1 what lies behind the scene shows through where no surface is drawn.")
                                 .styleableProperty (SceneComponentIds::ambient, &SceneComponent::getAmbientColor, &SceneComponent::setAmbientColor, SceneSettings().ambientColor, "The flat ambient light added to every surface.")
                                 .styleableProperty (SceneComponentIds::exposure, &SceneComponent::getExposure, &SceneComponent::setExposure, SceneSettings().exposure, "The multiplier applied to the lit scene before tone mapping.")
                                 .enumeration<ToneMappingOperator> (
                                     SceneComponentIds::toneMapping,
                                     { { "reinhard", ToneMappingOperator::reinhard },
                                       { "aces", ToneMappingOperator::aces },
                                       { "agx", ToneMappingOperator::agx } },
                                     &SceneComponent::getToneMapping,
                                     &SceneComponent::setToneMapping,
                                     "The operator mapping the lit colors into the displayable range.")
                                 .styleableProperty (SceneComponentIds::defaultLight, &SceneComponent::isDefaultLightEnabled, &SceneComponent::setDefaultLightEnabled, SceneSettings().useDefaultLight, "Whether a default directional light shines while the scene has no light.")
                                 .property (SceneComponentIds::continuous, &SceneComponent::isUpdatingContinuously, &SceneComponent::setContinuousUpdates, false, "Updates and repaints the scene on every display frame, instead of when something in it changes.")
                                 .enumeration<RenderSettings::TierPreference> (
                                     SceneComponentIds::tier,
                                     { { "automatic", RenderSettings::TierPreference::automatic },
                                       { "raster", RenderSettings::TierPreference::raster },
                                       { "full", RenderSettings::TierPreference::full } },
                                     [] (const SceneComponent& view) { return view.renderer.getRenderSettings().tier; },
                                     [] (SceneComponent& view, RenderSettings::TierPreference tier) { view.setRenderSetting (&RenderSettings::tier, tier, SceneComponentIds::tier); },
                                     "The tier of the renderer drawing the scene: the full one on the desktop GPUs that can run it and the raster one elsewhere (automatic), the raster one everywhere (raster), or the full one wherever the device can run it (full).")
                                 .property (
                                     SceneComponentIds::samples,
                                     [] (const SceneComponent& view) { return view.renderer.getSampleCount(); },
                                     [] (SceneComponent& view, int samples)
                                     {
                                         const auto previous = view.renderer.getSampleCount();
                                         view.renderer.setSampleCount (samples);

                                         if (view.renderer.getSampleCount() != previous)
                                             view.featureChanged (SceneComponentIds::samples);
                                     },
                                     SceneRenderer().getSampleCount(),
                                     "The MSAA samples per pixel of the raster tier, rounded down to a power of two the device supports. 1 turns multisampling off.")
                                 .property (SceneComponentIds::bloom, [] (const SceneComponent& view) { return view.renderer.getRenderSettings().bloom; }, [] (SceneComponent& view, bool bloom) { view.setRenderSetting (&RenderSettings::bloom, bloom, SceneComponentIds::bloom); }, RenderSettings().bloom, "Whether the bright highlights glow, where the device can draw it.")
                                 .property (SceneComponentIds::environment, &SceneComponent::getEnvironment, &SceneComponent::setEnvironment, String(), "The path of a panorama lighting the scene, twice as wide as it is high: the surfaces reflect it and receive its diffuse light.")
                                 .styleableProperty (
                                     SceneComponentIds::environmentIntensity,
                                     [] (const SceneComponent& view) { return view.getSettings().environmentIntensity; },
                                     [] (SceneComponent& view, float intensity)
                                     {
                                         if (view.getSettings().environmentIntensity == jmax (0.0f, intensity))
                                             return;

                                         view.getOrCreateScene().getRoot().getOrAttach<EnvironmentNode>().intensity = jmax (0.0f, intensity);
                                         view.featureChanged (SceneComponentIds::environmentIntensity);
                                     },
                                     SceneSettings().environmentIntensity,
                                     "The multiplier applied to the light of the environment.")
                                 .styleableProperty (
                                     SceneComponentIds::environmentVisible,
                                     [] (const SceneComponent& view) { return view.getSettings().environmentVisible; },
                                     [] (SceneComponent& view, bool isVisible)
                                     {
                                         if (view.getSettings().environmentVisible == isVisible)
                                             return;

                                         view.getOrCreateScene().getRoot().getOrAttach<EnvironmentNode>().visible = isVisible;
                                         view.featureChanged (SceneComponentIds::environmentVisible);
                                     },
                                     SceneSettings().environmentVisible,
                                     "Whether the environment is drawn behind the scene, instead of the background color.")
                                 .state (SceneComponentIds::loading, &SceneComponent::isLoading, false, "Whether a file of the scene is being loaded.")
                                 .build();

    return info;
}

//==============================================================================

void SceneComponent::paint (Graphics& g)
{
    rendered = false;

    // Whoever asked for this paint may have changed the scene without telling, as code holding it does: what is under
    // the pointer is looked for again, and the frame that follows looks at the scene
    lastPick.reset();
    requestFrame();

    const auto bounds = getLocalBounds();
    const auto background = getSettings().backgroundColor;

    // An opaque component paints all its pixels. One that is not draws the scene over what lies
    // behind it: its translucent background is in the texture already, premultiplied, and a fill
    // would count it twice. The fill is opaque even when the background turned translucent after
    // the opaque state was resolved, which the frame being painted relies on
    const auto opaqueBackground = background.withAlpha (1.0f);
    const auto filled = isOpaque();

    if (filled)
    {
        g.setFillColor (opaqueBackground);
        g.fillAll();
    }

    if (scene == nullptr)
        return;

    // A scene painted with no display frame before, as a snapshot is, draws the meshes its arrays make all the same
    buildMeshes();

    auto& context = g.getGraphicsContext();

    String error;
    if (! context.isGpuAvailable())
    {
        error = "GPU rendering is not available";
    }
    else
    {
        paintedApi = context.getPlatform();

        const auto width = roundToInt (bounds.getWidth() * g.getContextScale());
        const auto height = roundToInt (bounds.getHeight() * g.getContextScale());
        if (width < 1 || height < 1)
            return;

        // The scene follows what its nodes changed, and what they show of the components inside them is rendered: it is
        // drawn with both
        updateScene();

        forEachSceneNode (*this, [&] (SceneNodeComponent& node)
        {
            if (auto* surface = node.getSurface())
                surface->render (context, g.getContextScale());
        });

        if (auto texture = renderer.render (context.getGpuDevice(), *scene, width, height))
        {
            g.drawTexture (texture, bounds);
            rendered = true;
            return;
        }

        error = renderer.getLastError();
    }

    // The message has to be readable whatever lies behind the component: over the background made opaque
    if (! filled)
    {
        g.setFillColor (opaqueBackground);
        g.fillAll();
    }

    // Without a theme there is no font to write the message with
    const auto theme = ApplicationTheme::getGlobalTheme();
    if (theme == nullptr)
        return;

    g.setFillColor (opaqueBackground.contrasting());
    g.fillFittedText (error, theme->getDefaultFont(), bounds.reduced (10.0f));
}

void SceneComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    // What was under the pointer may be something else in the frame to come
    lastPick.reset();

    // Here and not in paint(), as building a mesh, or finding that a node draws none for the components inside it, may
    // report what is wrong: who hears about it may destroy the scene. So may a node that left the scene and is not
    // back in it, whose entity goes now
    const BailOutChecker checker (this);

    releaseLeavingNodes();

    if (! checker.shouldBailOut())
        updateMeshes();

    if (! checker.shouldBailOut())
        updateSurfaces();

    if (checker.shouldBailOut() || scene == nullptr)
        return;

    if (! updateAnimatedComponents (lastFrameTimeSeconds))
        return;

    // Nothing tells when the background of the scene changes, its color being a plain field, so
    // it is resolved on every frame. Here and not in paint(): by then the frame has read which
    // components are opaque, to skip painting what they cover
    const auto wasOpaque = isOpaque();
    updateOpaqueState();

    // Frames still accumulating are requested here, as paint() must not call repaint(). Only after
    // a paint that rendered since the frame before: a hidden or empty component would never converge
    const auto isAccumulating = std::exchange (rendered, false) && ! renderer.isConverged();

    // A background that turned opaque or not was painted as it was, when no frame came before its paint
    if (continuousUpdates || isAccumulating || isOpaque() != wasOpaque)
        repaintScene();
}

void SceneComponent::takeFrame (double elapsedSeconds)
{
    // Hidden, the scene is left as it is: its next frame, once it shows again, carries on from there
    if (! isShowing())
    {
        frameAttachment.setActive (false);
        return;
    }

    // What the frame finds can reach a listener that destroys the component
    const BailOutChecker checker (this);

    isFrameRequested = false;
    refreshDisplay (elapsedSeconds);

    // Nothing is left for the frames to do, and nothing asked for the next one meanwhile: the window is free to skip the
    // frames it has nothing to paint in, until a change or a paint of the scene asks for them again
    if (! checker.shouldBailOut() && ! isFrameRequested && ! needsEveryFrame())
        frameAttachment.setActive (false);
}

bool SceneComponent::needsEveryFrame() const
{
    // What changes without telling is looked at on every frame: the scene that is updated, and what the shaders of the
    // materials and the components shown on the meshes read
    return continuousUpdates
        || hasSurfaces
        || (scene != nullptr && (SceneModelComponent::isPlayingIn (*scene) || SceneMaterialComponent::hasShadersIn (*scene)));
}

bool SceneComponent::updateAnimatedComponents (double elapsedSeconds)
{
    // What the components tell while they are brought up to date, a clip that ended or a shader that can't be used, can
    // reach a listener that lets go of the scene or deletes this component: the scene is kept, and false tells that the
    // component went
    const BailOutChecker checker (this);
    const auto shown = scene;

    if (shown == nullptr)
        return true;

    // The scene is updated once on a frame, when it is asked to be on every one or while a clip plays, which poses its
    // model: whatever the number of models playing
    const auto isUpdated = continuousUpdates || SceneModelComponent::isPlayingIn (*shown);

    if (isUpdated)
    {
        // The update follows what the nodes changed too. What they change while it runs is followed by the paint
        isSceneOutdated = false;
        shown->update (elapsedSeconds);
    }

    if (checker.shouldBailOut())
        return false;

    updateCompiledMaterials();

    if (checker.shouldBailOut())
        return false;

    // The shaders of the materials take the values their properties have by then, and tell when one changed
    const auto isChanged = SceneMaterialComponent::updateShaders (*shown, elapsedSeconds) || isUpdated;

    if (checker.shouldBailOut())
        return false;

    if (isChanged)
        repaint();

    return true;
}

void SceneComponent::updateOpaqueState()
{
    // Without a scene the component fills itself
    setOpaque (scene == nullptr || SceneSettings::resolve (*scene).backgroundColor.isOpaque());
}

//==============================================================================

void registerSceneComponentTypes()
{
    ComponentTypeRegistry::getInstance().registerDefaultTypes ({
        SceneComponent::getClassTypeInfo(),
        SceneNodeComponent::getClassTypeInfo(),
        SceneCameraComponent::getClassTypeInfo(),
        SceneLightComponent::getClassTypeInfo(),
        SceneMeshComponent::getClassTypeInfo(),
        SceneModelComponent::getClassTypeInfo(),
        SceneMaterialComponent::getClassTypeInfo(),
    });
}

} // namespace yup
