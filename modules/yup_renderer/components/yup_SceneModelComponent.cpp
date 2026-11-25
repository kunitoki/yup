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

struct SceneModelComponentIds
{
    static inline const Identifier scene { "scene" };
    static inline const Identifier source { "source" };
    static inline const Identifier clip { "clip" };
    static inline const Identifier playing { "playing" };
    static inline const Identifier speed { "speed" };
    static inline const Identifier loop { "loop" };
    static inline const Identifier time { "time" };
    static inline const Identifier duration { "duration" };
    static inline const Identifier clips { "clips" };
};

/* The index of the clip a name stands for, or -1: the first clip of that name, else the clip at that index when the name is a
   number, and the first clip for no name. */
int findSceneModelClip (const AnimationNode& animation, const String& name)
{
    if (name.isEmpty())
        return animation.getNumClips() > 0 ? 0 : -1;

    if (const auto index = animation.findClip (name); index >= 0)
        return index;

    const auto isIndex = name.length() < 10 && name.containsOnly ("0123456789");

    return isIndex && name.getIntValue() < animation.getNumClips() ? name.getIntValue() : -1;
}

} // namespace

//==============================================================================
/* Attached to the instance of a model while its clip plays. Its being there tells that the scene has to be updated on every
   display frame, and Scene::update() calls it once the clips have posed their models, for this one to tell where its clip is. */
struct SceneModelComponent::Playback final : Node
{
    explicit Playback (SceneModelComponent& modelToTell)
        : model (modelToTell)
    {
    }

    void attachedToEntity() override
    {
        // After the clips, and before the meshes follow their joints: what a binding moves when it is told where the clip
        // is, is deformed by the same update
        auto* scene = getEntity().getScene();

        scene->addSystemRule<AnimationNode, Playback>();
        scene->addSystemRule<Playback, DeformNode>();
    }

    void update (double) override
    {
        model.clipAdvanced();
    }

    SceneModelComponent& model;
};

//==============================================================================

SceneModelComponent::SceneModelComponent (StringRef componentID)
    : SceneNodeComponent (componentID)
{
}

//==============================================================================

void SceneModelComponent::setSource (const String& newSource)
{
    if (source == newSource)
        return;

    source = newSource;

    // Another model starts over: where the clip of the one before was, or that it had ended, says nothing of its own
    hasEnded = false;
    time = 0.0;

    reload ({ SceneModelComponentIds::source, SceneModelComponentIds::clips });
}

void SceneModelComponent::setSceneIndex (int newSceneIndex)
{
    if (sceneIndex == newSceneIndex)
        return;

    sceneIndex = newSceneIndex;

    // As another source does
    hasEnded = false;
    time = 0.0;

    reload ({ SceneModelComponentIds::scene, SceneModelComponentIds::clips });
}

Entity SceneModelComponent::getInstance() const noexcept
{
    // The instance is in the scene of the entity of the node, which is the one known to be alive
    return getEntity().isValid() && instance.isValid() ? instance : Entity();
}

void SceneModelComponent::assetChanged (const String& path)
{
    // The same model, made again: it carries on from where it was
    if (path == source)
        reload ({ SceneModelComponentIds::clips });
}

//==============================================================================

void SceneModelComponent::setClip (const String& newClip)
{
    if (clip == newClip)
        return;

    const auto previousDuration = getDuration();

    clip = newClip;

    // Another clip plays, also when the one before had stopped at its end
    hasEnded = false;

    if (auto* animation = findAnimation())
    {
        // Paused while it is given its clip, which puts it at its start: updatePlayback() then plays it from the end it
        // plays from, the other one when it plays backwards. The model is posed there at once
        animation->setPlaying (false);
        animation->setCurrentClip (findSceneModelClip (*animation, clip));
    }

    updatePlayback();
    showPose();

    tellAnimationChanged ({ SceneModelComponentIds::clip }, previousDuration);
}

void SceneModelComponent::setPlaying (bool shouldPlay)
{
    if (playing == shouldPlay)
        return;

    playing = shouldPlay;

    // Asked again, a clip that had stopped at its end starts over
    hasEnded = false;

    updatePlayback();
    repaintScene();

    notifyChanged ({ SceneModelComponentIds::playing, SceneModelComponentIds::time });
}

void SceneModelComponent::setSpeed (float newSpeed)
{
    // Not a speed: the clip would be nowhere after its first step
    if (! std::isfinite (newSpeed) || speed == newSpeed)
        return;

    speed = newSpeed;

    if (auto* animation = findAnimation())
        animation->setSpeed (speed);

    notifyPropertyChanged (SceneModelComponentIds::speed);
}

void SceneModelComponent::setLooping (bool shouldLoop)
{
    if (looping == shouldLoop)
        return;

    looping = shouldLoop;

    if (auto* animation = findAnimation())
        animation->setLooping (looping);

    notifyPropertyChanged (SceneModelComponentIds::loop);
}

void SceneModelComponent::setTime (double newTime)
{
    // Not a time: a clip put there would never reach its end, and would tell where it is on every update
    if (std::isnan (newTime))
        return;

    auto* animation = findAnimation();

    // Limited to the clip, as its animation limits it
    if (animation != nullptr)
        newTime = jlimit (0.0, animation->getDuration(), newTime);

    if (getTime() == newTime)
        return;

    time = newTime;

    if (animation != nullptr)
    {
        animation->setTime (time);
        showPose();
    }

    notifyPropertyChanged (SceneModelComponentIds::time);
}

double SceneModelComponent::getTime() const noexcept
{
    const auto* animation = findAnimation();
    return animation != nullptr ? animation->getTime() : time;
}

double SceneModelComponent::getDuration() const noexcept
{
    const auto* animation = findAnimation();
    return animation != nullptr ? animation->getDuration() : 0.0;
}

StringArray SceneModelComponent::getClipNames() const
{
    StringArray names;

    if (const auto* animation = findAnimation())
    {
        for (int index = 0; index < animation->getNumClips(); ++index)
        {
            const auto animationClip = animation->getClip (index);
            names.add (animationClip != nullptr ? animationClip->getName() : String());
        }
    }

    return names;
}

bool SceneModelComponent::isPlayingIn (Scene& scene)
{
    bool isShownPlaying = false;

    // A model that is hidden is not updated: its clip stands still, and asks for nothing
    scene.view<Playback>().forEach ([&isShownPlaying] (Entity entity, Playback&)
    {
        isShownPlaying = isShownPlaying || entity.isActive();
    });

    return isShownPlaying;
}

//==============================================================================

AnimationNode* SceneModelComponent::findAnimation() const noexcept
{
    return getInstance().get<AnimationNode>();
}

void SceneModelComponent::updateAnimation()
{
    auto* animation = findAnimation();
    const auto position = time;

    clipEndedConnection = {};

    if (animation == nullptr)
        return;

    clipEndedConnection = animation->onClipEnded().connect ([this]
    {
        handleClipEnded();
    });

    animation->setSpeed (speed);
    animation->setLooping (looping);

    // An animation starts on its first clip without posing its model, which stays as its file has it: only a clip that
    // is named is selected
    if (clip.isNotEmpty())
        animation->setCurrentClip (findSceneModelClip (*animation, clip));

    // Where the instance before was, as far as the clip of this one goes
    if (position != animation->getTime())
        animation->setTime (position);

    updatePlayback();

    if (clip.isNotEmpty() || position != 0.0)
        showPose();
}

void SceneModelComponent::updatePlayback()
{
    auto* animation = findAnimation();
    if (animation == nullptr)
        return;

    // A clip that reached its end stays there until the model is asked to play again: playing then starts it over
    animation->setPlaying (playing && ! hasEnded && animation->getCurrentClip() >= 0);
    time = animation->getTime();

    if (animation->isPlaying())
        instance.getOrAttach<Playback> (*this);
    else
        instance.detach<Playback>();
}

void SceneModelComponent::showPose()
{
    if (findAnimation() == nullptr)
        return;

    // The meshes a skin or morph targets deform follow the pose when the scene is updated. An update that is running
    // has them do so by itself: the animations are updated before them
    if (auto* scene = getEntity().getScene(); ! scene->isIterating())
        scene->update (0.0);

    repaintScene();
}

void SceneModelComponent::clipAdvanced()
{
    const auto* animation = findAnimation();
    if (animation == nullptr)
        return;

    // Once for each update of the scene, and only when the clip moved: what follows it, a slider showing where it is, moves with it
    const auto previous = std::exchange (time, animation->getTime());

    if (previous != time)
        notifyPropertyChanged (SceneModelComponentIds::time);
}

void SceneModelComponent::handleClipEnded()
{
    // The clip stays at its end, and the model asked to play, until it is asked again
    hasEnded = true;
    updatePlayback();

    const BailOutChecker checker (this);
    notifyPropertyChanged (SceneModelComponentIds::time);

    if (! checker.shouldBailOut() && onClipEnded != nullptr)
        onClipEnded();
}

void SceneModelComponent::updateClipProblem()
{
    const auto* animation = findAnimation();
    auto* view = SceneComponent::findFor (*this);

    if (view == nullptr)
        return;

    view->clearProblems (*this, SceneModelComponentIds::clip);

    // A model that could not be loaded says so itself, and one without clips only when it is asked for one
    if (clip.isNotEmpty() && getInstance().isValid() && (animation == nullptr || animation->getCurrentClip() < 0))
        view->reportProblem (*this, SceneModelComponentIds::clip, "the model has no clip '" + clip + "'");
}

void SceneModelComponent::tellAnimationChanged (std::initializer_list<Identifier> properties, double previousDuration)
{
    const BailOutChecker checker (this);
    const auto isShorter = getDuration() < previousDuration;

    updateClipProblem();

    if (checker.shouldBailOut())
        return;

    notifyChanged (properties);

    if (checker.shouldBailOut())
        return;

    // A slider limited to the duration and linked to the time limits its value when its range shrinks, and gives it back
    // to the time. It hears where the clip is first when the clip got shorter, to be inside its new range by then, and
    // how long the clip is first when it got longer, to have room for a clip that starts at its end
    if (isShorter)
        notifyChanged ({ SceneModelComponentIds::time, SceneModelComponentIds::duration });
    else
        notifyChanged ({ SceneModelComponentIds::duration, SceneModelComponentIds::time });
}

void SceneModelComponent::notifyChanged (std::initializer_list<Identifier> properties)
{
    const BailOutChecker checker (this);

    for (const auto& property : properties)
    {
        if (checker.shouldBailOut())
            return;

        notifyPropertyChanged (property);
    }
}

//==============================================================================

void SceneModelComponent::entityChanged()
{
    const BailOutChecker checker (this);

    // The instance went with the entity it was under
    instance = {};
    instantiate();

    // What is reported about the file can reach a listener that deletes the component
    if (checker.shouldBailOut())
        return;

    // The new instance plays as the model was asked. A clip it does not have is told last: what gave the node its entity
    // looks at whether the node is still there before it goes on. No property is told to have changed, as for the other
    // values of a node that read differently once it has an entity
    updateAnimation();
    updateClipProblem();
}

void SceneModelComponent::reload (std::initializer_list<Identifier> properties)
{
    const BailOutChecker checker (this);
    const auto previousDuration = getDuration();

    instantiate();

    // What is reported about the file can reach a listener that deletes the component
    if (checker.shouldBailOut())
        return;

    updateAnimation();

    // The nodes bound to the entities of the instance that went are bound to the ones of the new one
    updateEntitiesBelow (*this);

    if (checker.shouldBailOut())
        return;

    repaintScene();
    tellAnimationChanged (properties, previousDuration);
}

void SceneModelComponent::instantiate()
{
    auto entity = getEntity();
    auto* view = SceneComponent::findFor (*this);

    if (entity.isValid() && instance.isValid())
        instance.destroy();

    instance = {};

    if (view == nullptr || ! entity.isValid())
        return;

    view->clearProblems (*this, SceneModelComponentIds::source);
    view->clearProblems (*this, SceneModelComponentIds::scene);

    if (source.isEmpty())
        return;

    const auto model = view->loadModel (source);

    if (model.failed())
    {
        view->reportProblem (*this, SceneModelComponentIds::source, model.getErrorMessage());
        return;
    }

    for (const auto& warning : model.getReference().getWarnings())
        view->reportProblem (*this, SceneModelComponentIds::source, warning, DataTreeDiagnostic::Severity::warning);

    instance = model.getReference().instantiate (*entity.getScene(), entity, sceneIndex);

    if (! instance.isValid())
        view->reportProblem (*this, SceneModelComponentIds::scene, "the model has no scene " + String (sceneIndex));
}

//==============================================================================

const ComponentTypeInfo& SceneModelComponent::getClassTypeInfo()
{
    // The scene comes first, for a document that sets both to load its model once. Playing comes last: it starts the clip
    // that was selected, from where it was put and at the speed it was given
    static const auto info = ComponentTypeBuilder<SceneModelComponent> (Identifier ("Model3D"), SceneNodeComponent::getClassTypeInfo())
                                 .property (SceneModelComponentIds::scene, &SceneModelComponent::getSceneIndex, &SceneModelComponent::setSceneIndex, -1, "The index of the scene of the file to show, -1 for its default one.")
                                 .property (SceneModelComponentIds::source, &SceneModelComponent::getSource, &SceneModelComponent::setSource, String(), "The path of the .glb or .gltf file of the model, relative to the document.")
                                 .styleableProperty (SceneModelComponentIds::clip, &SceneModelComponent::getClip, &SceneModelComponent::setClip, String(), "The clip to play: its name, or its index among the clips of the model when no clip has that name. Empty for the first clip.")
                                 .styleableProperty (SceneModelComponentIds::speed, &SceneModelComponent::getSpeed, &SceneModelComponent::setSpeed, 1.0f, "How fast the clip plays: 1 as it was made, negative backwards.")
                                 .styleableProperty (SceneModelComponentIds::loop, &SceneModelComponent::isLooping, &SceneModelComponent::setLooping, false, "Whether the clip carries on from its other end when it reaches one, instead of stopping there.")
                                 .styleableProperty (SceneModelComponentIds::time, &SceneModelComponent::getTime, &SceneModelComponent::setTime, 0.0, "Where the clip is, in seconds. Setting it moves the clip there, playing or not.")
                                 .styleableProperty (SceneModelComponentIds::playing, &SceneModelComponent::isPlaying, &SceneModelComponent::setPlaying, false, "Plays the clip. One that does not loop stops at its end, and plays again from its start the next time this turns true.")
                                 .readOnly (SceneModelComponentIds::duration, &SceneModelComponent::getDuration, 0.0, "The length of the clip in seconds, 0 without one.")
                                 .readOnly (SceneModelComponentIds::clips, &SceneModelComponent::getClipNames, {}, "The names of the clips of the model.")
                                 .signal ("onClipEnded", &SceneModelComponent::onClipEnded, "Emitted when a clip that does not loop stops at the end it plays towards.")
                                 .build();

    return info;
}

} // namespace yup
