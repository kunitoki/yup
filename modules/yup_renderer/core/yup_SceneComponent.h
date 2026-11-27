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

class SceneMeshComponent;
class SceneNodeComponent;

//==============================================================================
/** A Component that displays a Scene.

    The scene is rendered at the pixel size of the component (its bounds times the display
    scale) and drawn over the whole component. The component is opaque while the background of
    the scene is. With a BackgroundNode color of alpha below 1 it is not: the scene is then drawn
    over what lies behind the component, which shows wherever no surface covers it, through the
    share the background leaves. The component keeps isOpaque() up to date by itself, when the
    scene is set and on the display frames it takes, so the background may change at any time: do
    not call setOpaque() on it. A color written to the BackgroundNode of a scene that stood still
    is found by the frame that follows the paint asked for after it: that paint still fills the
    component, and the scene is painted once more. When no GPU is available it fills its
    background, made opaque, and shows a message instead. Clicking the component gives it the
    keyboard focus, so mouse wheel events reach it.

    By default the scene is drawn only when the component repaints: call repaint() after
    changing it. The full tier then keeps repainting on its own until its temporal anti-aliasing
    has converged, see SceneRenderer::isConverged(). With setContinuousUpdates (true) the
    component calls Scene::update() and repaints on every display frame, for animated scenes.

    The component only takes the display frames of its window that it has something to do in, so
    a window showing a scene that stands still is free to skip the frames it has nothing to paint
    in. It takes every frame while it updates continuously, while a clip of a `Model3D` plays,
    while a `Material3D` names a shader and while a node shows components on its mesh. Otherwise
    it takes the ones that follow a change of the components of the scene, and the one that
    follows each of its paints, which is where a change made to the scene from code is found:
    until nothing is left to do. While it is not showing (Component::isShowing) it takes no
    frame at all: the scene is neither updated nor looked at, and the renderer lets go of what it
    rendered into (SceneRenderer::releaseTargets). It carries on once it shows again.

    In a `.yui` document the component is the `Scene3D` element, which needs no code. It makes a
    scene of its own the first time one is needed, see getOrCreateScene(), and the 3D components
    written inside it (SceneNodeComponent and the types made of it) put their entities in that
    scene. Any other component written directly inside it is an ordinary child, painted over
    the scene and taking the pointer before it, which is how a heads-up display is written.
    Inside a node such a component is shown on the mesh of the node instead, see
    SceneNodeComponent. Its properties set the features of the scene, as the nodes attached to
    its root do:

    @code
    Scene3D {
        background: #101014; exposure: 1.2; toneMapping: agx;

        Camera3D { position: [0, 0.5, 4]; lookAt: [0, 0, 0]; }
        Light3D  { type: point; position: [2, 3, 2]; intensity: 20; }
        Model3D  { source: "models/device.glb"; }

        Label { text: "Device"; }
    }
    @endcode

    Three more properties are settings of the renderer, which stay when the component is given
    another scene: `tier` is its RenderSettings::tier (`automatic`, `raster` or `full`), `samples`
    its sample count (SceneRenderer::setSampleCount) and `bloom` its RenderSettings::bloom. They
    read what the renderer holds, so they follow what code sets on getRenderer(), which tells no
    change: repaint the component after it. The environment, the fog and the other RenderSettings
    have no property: set them from code, on the scene and on getRenderer().

    The nodes take the pointer where they are shown, see findNodeAt(): a mouse event goes to the
    node under it, and to the component only where no node is. A component that moves its camera
    with the mouse listens to the nodes too, with addMouseListener (listener, true).

    What the nodes change is followed by what depends on it before the scene is drawn: when a
    node asked for the scene to be painted again, the scene is updated once with no time elapsed
    (Scene::update), so a mesh follows the joint a node turned. Systems and nodes updating from
    there are called while the component paints, and must not repaint it.

    @see Scene, SceneRenderer, SceneNodeComponent
*/
class YUP_API SceneComponent : public Component
{
public:
    //==============================================================================
    /** Creates a component without a scene.

        @param componentID The ID of the component.
    */
    explicit SceneComponent (StringRef componentID = {});

    /** Destructor. */
    ~SceneComponent() override;

    //==============================================================================
    /** Changes the scene to display, and makes the component opaque or not after its background.

        The 3D components below this one leave the scene they were in and make their entities
        again in the new one. With nullptr they make them in a scene created for them, see
        getOrCreateScene().

        The features of the component are those of the scene it shows, so they are not carried
        over to the new one: the background, the ambient light, the exposure, the tone mapping
        and the default light read what the new scene resolves to, and the properties that read
        another value than before are told to have changed.

        A scene is meant to be shown by one SceneComponent at a time: every component showing it
        updates it on its display frames, so its clips and the time of its shaders would run
        once for each component.
    */
    void setScene (Scene::Ptr newScene);

    /** Returns the scene being displayed, or nullptr. */
    const Scene::Ptr& getScene() const noexcept { return scene; }

    /** Returns the scene being displayed, creating an empty one first when there is none.

        This is how a `Scene3D` written in a `.yui` document gets its scene: the first of its
        properties to be set, or the first 3D component added to it, asks for it.
    */
    Scene& getOrCreateScene();

    /** Returns the renderer, for example to read its last error. */
    SceneRenderer& getRenderer() noexcept { return renderer; }

    //==============================================================================
    /** Chooses whether the scene is updated and repainted on every display frame.

        The frames are those of the window the component is in, while the component is showing
        (Component::isShowing): hidden, the scene is not updated, and the first frame after it
        shows again is given the time of one frame.

        @param shouldUpdateContinuously True to call Scene::update() and repaint every frame.
    */
    void setContinuousUpdates (bool shouldUpdateContinuously);

    /** Returns true if the scene is updated and repainted on every display frame. */
    bool isUpdatingContinuously() const noexcept { return continuousUpdates; }

    //==============================================================================
    /** Changes the color the background of the scene is cleared to.

        The color goes to the BackgroundNode of the root of the scene, attached when it has none.
        An alpha below 1 lets what lies behind the component show through, and makes the
        component not opaque.

        @param newColor The background color, display referred.
    */
    void setBackgroundColor (Color newColor);

    /** Returns the color the scene resolves its background to, see SceneSettings. */
    Color getBackgroundColor() const;

    /** Changes the flat ambient light added to every surface, through the AmbientLightNode of
        the root of the scene.

        @param newColor The ambient light color, sRGB encoded.
    */
    void setAmbientColor (Color newColor);

    /** Returns the ambient light color the scene resolves to, see SceneSettings. */
    Color getAmbientColor() const;

    /** Changes the multiplier applied to the scene color before tone mapping, through the
        ExposureNode of the root of the scene.

        @param newExposure The multiplier, 1 leaves the scene as it is lit. One that is negative,
                           or that is not a finite number, is reported in the diagnostics of the
                           document and the scene keeps the one it has.
    */
    void setExposure (float newExposure);

    /** Returns the exposure the scene resolves to, see SceneSettings. */
    float getExposure() const;

    /** Changes the operator mapping the lit colors into the displayable range, through the
        ToneMappingNode of the root of the scene.

        @param newOperator The tone mapping operator.
    */
    void setToneMapping (ToneMappingOperator newOperator);

    /** Returns the tone mapping operator the scene resolves to, see SceneSettings. */
    ToneMappingOperator getToneMapping() const;

    /** Chooses whether a default directional light shines while the scene has no light, through
        the DefaultLightNode of the root of the scene.

        @param shouldBeEnabled False for a scene without lights to stay dark but for its ambient
                               light and its environment.
    */
    void setDefaultLightEnabled (bool shouldBeEnabled);

    /** Returns true if the scene resolves to using the default light, see SceneSettings. */
    bool isDefaultLightEnabled() const;

    //==============================================================================
    /** Loads a glTF model for the components of this scene, once for each path.

        The file is read through the document the component belongs to (see
        ComponentDocument::readAssetFor), so from a folder, from embedded resources or from the
        unsaved text of an editor, and the model is kept: the Model3D components naming the
        same path share its meshes, its materials and its textures. A path is read again once
        the document tells that its file changed.

        The buffers and the images a `.gltf` keeps in other files are read from the folder of
        its file, which a document loaded from embedded resources does not have: embed a `.glb`,
        or a `.gltf` holding everything as data URIs.

        @param path The path of the `.glb` or `.gltf` file, as written in the document.

        @returns The model, or what prevented reading or loading it.
    */
    ResultValue<GltfModel> loadModel (const String& path);

    /** Loads an image as a texture for the components of this scene, once for each path and
        color encoding.

        The file is read as loadModel() reads its files, decoded with the image formats the
        application links, and kept: the Material3D components naming the same path share the
        texture. It is filtered linearly with mipmaps and repeats, as the textures of a glTF
        model do by default.

        @param path   The path of the image, as written in the document.
        @param isSrgb True for the textures holding sRGB encoded colors, the base color and the
                      emissive ones, false for those holding data.

        @returns The texture, or what prevented reading or decoding it.
    */
    ResultValue<Texture::Ptr> loadTexture (const String& path, bool isSrgb);

    /** Loads a material shader for the components of this scene, once for each path.

        The file is read as loadModel() reads its files, and the shader is kept: the Material3D
        components whose `shader` block names the same path share it, whatever happens to the
        stylesheet, and only a change of its files loads it again. A material compiled from its
        source takes a moment, which is another reason to keep it.

        The two variants a build compiles the material into are read first, `path` followed by
        `.raster.ysl` and by `.full.ysl` (see MaterialShader::createFromData), either of which
        can be missing. When there is neither, the file itself is compiled as the GLSL source of
        the material, where the shader transpiler is built in (see
        MaterialShader::createFromSource).

        A source is compiled for one graphics API, the one drawing the scene: the API of the
        window the component is in, else the one of what painted it last, as a snapshot does, and
        until there is either the one a window is given by default (see
        ComponentNative::getDefaultGraphicsApi). When the scene turns out to be drawn with
        another API, its next display frame forgets the material as it does when its file
        changes, and the components naming it have it compiled again for that one, once. So what
        a source has wrong for another API than the one in use is not told here: `yup_yui_tool`, and
        compiling the material ahead into its variants, check the others.

        @param path The path of the material, as the `material` of a `shader` block writes it.

        @returns The shader, or what prevented it: a message starting with "can't read" when
                 none of the three files can be read, else what MaterialShader says of the bytes
                 or of the source it was given.
    */
    ResultValue<MaterialShader::Ptr> loadMaterialShader (const String& path);

    /** Reports a problem with an element of the document that no component stands for, as what
        prevents the material of a `shader` block from loading.

        It is added to the diagnostics of the document that built this component, at the line
        the element, or its property, is written at, as the problems of reportProblem() are. It
        is not kept: who reports it tells it again when it still stands after a load. A document
        lists a problem once, however many times it is told the same one at the same place.

        @param element  The element of the document, or of its stylesheet.
        @param property The property of the element the problem is about, or a null identifier
                        when it is about the element as a whole.
        @param message  What is wrong, as it is listed.
    */
    void reportProblem (const DataTree& element, const Identifier& property, const String& message);

    //==============================================================================
    /** Reports a problem with a value of a component below this one.

        The problem is added to the diagnostics of the document that built the component (see
        ComponentDocument::reportDiagnostic), at the line its property is written at, and reported
        again after each load for as long as it stands: withdraw it with clearProblems() once
        the value is right. Nothing is reported for a component that no document built.

        @param component The component the problem is about.
        @param property  The property the problem is about, or a null identifier when it is
                         about the component as a whole.
        @param message   What is wrong, completing "Type.property: ".
        @param severity  Whether the problem prevents the value from being used.
    */
    void reportProblem (Component& component,
                        const Identifier& property,
                        const String& message,
                        DataTreeDiagnostic::Severity severity = DataTreeDiagnostic::Severity::error);

    /** Withdraws the problems reported about a property of a component.

        The diagnostics a document already lists stay until it is loaded again.

        @param component The component the problems are about.
        @param property  The property, or a null identifier for the problems reported about the
                         component as a whole.
    */
    void clearProblems (const Component& component, const Identifier& property);

    //==============================================================================
    /** Returns the node shown under a position of the component: the one a pointer there is over.

        A ray goes from the camera of the scene through the position, and the nearest triangle
        it hits among the meshes that are drawn tells the node: the SceneNodeComponent whose
        entity is the one of that mesh, or the nearest one above it. So a `Mesh3D` answers for
        its mesh, a `Node3D` bound to a part of a model for that part and the ones under it, and
        a `Model3D` for the rest of its model. A mesh no node made hides the nodes behind it.
        Hidden entities are not hit, nor what the camera clips, nor the triangles seen from
        behind unless their material is double sided. Where a component written directly in the
        scene, over it, takes the pointer, there is no node.

        This is what the nodes answer Component::hitTest() with, which gives them the mouse
        events, the `:hover` and `:pressed` states and their mouse cursor. The answer is kept for
        the position until the scene may have changed: when a node changes, when the scene is
        painted and at each display frame it takes. Each triangle of the meshes whose bounds the
        ray enters is tested, see Mesh::intersectTriangles().

        @param position A position in the component.

        @returns The node, or nullptr when no node shows anything there.
    */
    SceneNodeComponent* findNodeAt (Point<float> position);

    //==============================================================================
    /** Returns the SceneComponent a 3D component belongs to.

        This is the closest one above the component, reached through 3D components only: a
        SceneNodeComponent inside an ordinary component is not part of the scene around it. A
        node taken out of its scene still belongs to it, with the components inside it, for as
        long as it keeps its entity: until the next display frame of that scene, see
        SceneNodeComponent.

        @param component A SceneNodeComponent, or a component placed inside one.

        @returns The scene component, or nullptr when the component is in none.
    */
    static SceneComponent* findFor (Component& component);

    //==============================================================================
    /** Returns the static type info describing the Scene3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void childrenChanged() override;
    /** @internal */
    void visibilityChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal Runs a display frame of the scene, whether the component is showing or not: what the frames of its window call while it shows and has something for them to do, and a snapshot before each of its paints. */
    void refreshDisplay (double lastFrameTimeSeconds);
    /** @internal Paints the scene again, with a display frame before the paint: what a component calls after it changed how the scene is drawn, as sceneContentChanged() does for what it shows. */
    void repaintScene();
    /** @internal Paints the scene again after a component changed what it shows: what was under the pointer is forgotten, and the scene is updated before it is drawn. */
    void sceneContentChanged();
    /** @internal Updates the scene with no time elapsed when a component changed it since it was last updated: what paint() does before it renders. */
    void updateScene();
    /** @internal Returns the matrix taking the scene to the clip space the component shows it in, with a far plane that can be mapped back from. */
    Matrix4 getViewProjection() const;
    /** @internal Makes the first active Camera3D below the component the camera of the scene, leaving out the one whose entity is going. */
    void updateActiveCamera (const Entity& leaving = {});
    /** @internal Has the mesh of a Mesh3D built before the next frame, and when its document has loaded. */
    void deferMeshUpdate (SceneMeshComponent& mesh);
    /** @internal Has the mesh and the material that show the components inside a node looked for before the next frame, and when its document has loaded. */
    void deferSurfaceUpdate();
    /** @internal Keeps the entity of a node that left the scene, out of the tree of the scene, until the next frame, where it goes unless the node is back in the scene by then. */
    void deferEntityRelease (SceneNodeComponent& node);

private:
    struct DocumentLink;

    struct Problem
    {
        WeakReference<Component> component;
        Identifier property;
        String message;
        DataTreeDiagnostic::Severity severity = DataTreeDiagnostic::Severity::error;
    };

    struct Pick
    {
        Point<float> position;
        WeakReference<Component> node;
    };

    void updateOpaqueState();
    void sceneChanged();
    void watchScene();
    SceneSettings getSettings() const;
    void featureChanged (const Identifier& property);
    template <class Value>
    void setRenderSetting (Value RenderSettings::* setting, Value newValue, const Identifier& property);
    ComponentDocument* findDocument();
    void sendProblem (const Problem& problem);
    void sendProblems();
    void updateMeshes();
    void buildMeshes();
    void updateSurfaces();
    void releaseLeavingNodes();
    SceneNodeComponent* pickNode (Point<float> position);
    void assetChanged (const String& path);
    GpuPlatform findGraphicsApi();
    void updateCompiledMaterials();
    bool updateAnimatedComponents (double elapsedSeconds);
    void requestFrame();
    void takeFrame (double elapsedSeconds);
    bool needsEveryFrame() const;
    void showingChanged();

    Scene::Ptr scene;
    SceneRenderer renderer;
    bool continuousUpdates = false;
    bool rendered = false; // A paint rendered the scene since its last display frame
    Entity activeCamera;
    SignalConnection entityDestroyed;
    std::map<String, ResultValue<GltfModel>> models;
    std::map<std::pair<String, bool>, ResultValue<Texture::Ptr>> textures;
    std::map<String, ResultValue<MaterialShader::Ptr>> materialShaders;
    std::map<String, GpuPlatform> compiledMaterials; // The graphics API each material shader compiled from its source was compiled for, by its path
    std::optional<GpuPlatform> paintedApi;           // The graphics API of what painted the scene last
    std::vector<Problem> problems;
    Array<WeakReference<Component>> changedMeshes;
    std::vector<WeakReference<Component>> leavingNodes; // The nodes that left the scene with an entity it keeps for them until its next frame
    std::optional<Pick> lastPick;
    bool isSceneOutdated = false;
    bool isUpdatingScene = false;
    bool isFrameRequested = false; // Something asked for a display frame since the last one started
    bool hasSurfaces = false;      // A node showed components on its mesh when the nodes were last looked at
    std::unique_ptr<DocumentLink> documentLink;
    FrameAttachment frameAttachment { *this, bindFront (&SceneComponent::takeFrame, this) };

    template <class T>
    friend class ComponentTestHelper;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneComponent)
};

//==============================================================================
/** Registers the type infos of the 3D components a `.yui` document is written with: `Scene3D`,
    `Node3D`, `Camera3D`, `Light3D`, `Mesh3D`, `Model3D` and `Material3D`.

    The yup_gui_markup module calls it in the applications linking both, so a `.yui` document
    needs no call. Call it to create the 3D components by their type name without that module,
    through the ComponentTypeRegistry.
*/
YUP_API void registerSceneComponentTypes();

} // namespace yup
