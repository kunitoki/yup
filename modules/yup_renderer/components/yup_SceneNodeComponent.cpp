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

struct SceneNodeComponentIds
{
    static inline const Identifier node { "node" };
    static inline const Identifier position { "position" };
    static inline const Identifier rotation { "rotation" };
    static inline const Identifier scale { "scale" };
    static inline const Identifier lookAt { "lookAt" };
    static inline const Identifier travel { "travel" };
    static inline const Identifier turn { "turn" };
    static inline const Identifier value { "value" };
    static inline const Identifier morphWeights { "morphWeights" };
    static inline const Identifier surface { "surface" };
};

bool isSceneNumber (const var& value) noexcept
{
    return value.isDouble() || value.isInt() || value.isInt64();
}

var makeSceneVector (double x, double y, double z)
{
    Array<var> values;
    values.add (x);
    values.add (y);
    values.add (z);

    return var (std::move (values));
}

/* The three numbers of a position, of Euler angles or of a scale, or nothing for any other value. */
std::optional<Vector3<float>> readSceneVector (const var& value)
{
    const auto* numbers = value.getArray();

    if (numbers == nullptr || numbers->size() != 3 || ! std::all_of (numbers->begin(), numbers->end(), isSceneNumber))
        return std::nullopt;

    return Vector3<float> (static_cast<float> (numbers->getReference (0)),
                           static_cast<float> (numbers->getReference (1)),
                           static_cast<float> (numbers->getReference (2)));
}

std::optional<Vector3<float>> readSceneScale (const var& value)
{
    if (! isSceneNumber (value))
        return readSceneVector (value);

    const auto factor = static_cast<float> (value);
    return Vector3<float> (factor, factor, factor);
}

/* Euler angles as a rotation: the turn around z comes first, then the one around x, then the one around y. */
Quaternion makeSceneRotation (const Vector3<float>& angles)
{
    return Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, angles.getY())
         * Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, angles.getX())
         * Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, angles.getZ());
}

/* The entity of a model instance a `node` names: the first of that name, or the one a path of names leads to. */
Entity findSceneModelEntity (const Entity& instance, const String& path)
{
    if (! path.containsChar ('/'))
        return instance.findChild (path);

    auto entity = instance;

    for (const auto& name : StringArray::fromTokens (path, "/", ""))
    {
        if (name.isNotEmpty() && entity.isValid())
            entity = entity.findChild (name, false);
    }

    return entity != instance ? entity : Entity();
}

/* How far the pointer goes, up or down, to take a node that only turns through its whole range. */
constexpr float sceneTurnDragDistance = 200.0f;

/* The length under which the travel of a node, as the scene shows it, is too short to steer the node along: it points at the camera. */
constexpr float sceneShortestDraggedTravel = 4.0f;

/* Where an area shows a point, given the matrix taking the point to clip space, or nothing for a point behind the camera. */
std::optional<Point<float>> findScenePointInArea (const Matrix4& toClipSpace, const Rectangle<float>& area, const Vector3<float>& point)
{
    const auto clip = toClipSpace.transformPoint4 (point.getX(), point.getY(), point.getZ(), 1.0f);

    if (clip[3] <= 0.0f)
        return std::nullopt;

    return Point<float> (area.getX() + (clip[0] / clip[3] + 1.0f) * 0.5f * area.getWidth(),
                         area.getY() + (1.0f - clip[1] / clip[3]) * 0.5f * area.getHeight());
}

/* Whether a component inside a node is shown on its mesh: every component that is not part of the scene. */
bool isSceneSurfaceComponent (const Component& component)
{
    return dynamic_cast<const SceneNodeComponent*> (&component) == nullptr
        && dynamic_cast<const SceneMaterialComponent*> (&component) == nullptr;
}

} // namespace

//==============================================================================

SceneNodeComponent::SceneNodeComponent (StringRef componentID)
    : Component (componentID)
{
    // A node paints nothing, and has the area of its scene whatever lays out the components around it
    setOpaque (false);
    setExcludedFromParentLayout (true);
}

SceneNodeComponent::~SceneNodeComponent()
{
    // While the entity is there: what the surface gave it goes first
    surface.reset();
    releaseEntity();
}

//==============================================================================

Entity SceneNodeComponent::getEntity() const noexcept
{
    return entity.isValid() ? entity : Entity();
}

void SceneNodeComponent::setNodeName (const String& newName)
{
    if (nodeName == newName)
        return;

    nodeName = newName;

    // A node that can't be bound says so, and who hears about it may destroy this one
    const BailOutChecker checker (this);

    updateEntity();

    if (! checker.shouldBailOut())
        updateEntitiesBelow (*this);

    if (! checker.shouldBailOut())
        notifyPropertyChanged (SceneNodeComponentIds::node);
}

bool SceneNodeComponent::isBound() const
{
    // A type that shows something of its own, a light or a mesh, would put it on the entity of the model for good
    return nodeName.isNotEmpty() && &getDefaultTypeInfo() == &SceneNodeComponent::getClassTypeInfo();
}

//==============================================================================

void SceneNodeComponent::updateEntity()
{
    const auto previous = getEntity();
    String problem;

    // What is left of an entity that went with its parent or with its model
    if (! previous.isValid())
        releaseEntity();

    if (auto* view = SceneComponent::findFor (*this))
        view->clearProblems (*this, SceneNodeComponentIds::node);

    if (isBound())
    {
        if (const auto target = findModelEntity (problem); target != previous)
        {
            releaseEntity();

            if (target.isValid())
            {
                entity = target;
                scene = target.getScene();
                boundVisible = target.isVisible();

                if (const auto* transform = target.get<TransformNode>())
                    boundTransform = Transform { transform->getPosition(), transform->getRotation(), transform->getScale() };
            }
        }
    }
    else
    {
        if (nodeName.isNotEmpty())
            problem = "only a Node3D can be bound to an entity of a model";

        const auto parentEntity = findParentEntity();

        // An entity can't go to another scene: it is made again there
        if (! isEntityOwned || ! previous.isValid() || previous.getScene() != parentEntity.getScene())
        {
            releaseEntity();

            if (parentEntity.isValid())
            {
                entity = parentEntity.getScene()->createEntity ({}, parentEntity);
                scene = entity.getScene();
                isEntityOwned = true;
            }
        }

        if (parentEntity.isValid())
            placeEntity (parentEntity);
    }

    // What is wrong is told last, once the node has the entity it can have: who hears about it may destroy the node
    const BailOutChecker checker (this);

    if (getEntity() != previous)
    {
        if (entity.isValid())
            entity.setVisible (isVisible());

        applyTransform();
        entityChanged();

        if (checker.shouldBailOut())
            return;

        repaintScene();
        SceneMaterialComponent::updateHost (*this);

        if (checker.shouldBailOut())
            return;
    }

    if (auto* view = SceneComponent::findFor (*this); view != nullptr && problem.isNotEmpty())
        view->reportProblem (*this, SceneNodeComponentIds::node, problem);

    // The morph targets of the mesh of the entity the node has by now take its weights, which may tell what is wrong too
    if (! checker.shouldBailOut())
        applyMorphWeights();
}

void SceneNodeComponent::releaseEntity()
{
    // What shows the components inside the node was on the entity that goes
    if (surface != nullptr)
        surface->release();

    // The morph targets get back the weights they had before the node set them
    applyMorphWeights (true);

    if (entity.isValid())
    {
        if (isEntityOwned)
        {
            entity.destroy();
        }
        else
        {
            // The entity of a model gets back what the model gave it, for what the node drove
            entity.setVisible (boundVisible);

            if (auto* transform = entity.get<TransformNode>())
            {
                const auto given = boundTransform.value_or (Transform());

                if (drivesPosition)
                    transform->setPosition (given.position);

                if (drivesRotation)
                    transform->setRotation (given.rotation);

                if (drivesScale)
                    transform->setScale (given.scale);
            }
        }
    }

    entity = {};
    scene = nullptr;
    isEntityOwned = false;
    boundTransform.reset();
    boundVisible = true;
    drivesPosition = drivesRotation = drivesScale = false;
}

Entity SceneNodeComponent::findParentEntity()
{
    auto* parent = getParentComponent();

    if (const auto* node = dynamic_cast<const SceneNodeComponent*> (parent))
        return node->getEntity();

    if (auto* view = dynamic_cast<SceneComponent*> (parent))
        return view->getOrCreateScene().getRoot();

    return {};
}

Entity SceneNodeComponent::findModelEntity (String& problem)
{
    // The model is the closest Model3D above the node, reached through the nodes bound to it
    auto* parent = dynamic_cast<SceneNodeComponent*> (getParentComponent());

    while (parent != nullptr && dynamic_cast<SceneModelComponent*> (parent) == nullptr)
        parent = parent->isBound() ? dynamic_cast<SceneNodeComponent*> (parent->getParentComponent()) : nullptr;

    const auto* model = dynamic_cast<SceneModelComponent*> (parent);

    if (model == nullptr)
    {
        problem = "a node is bound to an entity of the Model3D it is in, and this one is in none";
        return {};
    }

    // A model that could not be loaded says so itself
    const auto instance = model->getInstance();
    if (! instance.isValid())
        return {};

    const auto found = findSceneModelEntity (instance, nodeName);

    if (! found.isValid())
        problem = "the model has no entity '" + nodeName + "'";

    return found;
}

void SceneNodeComponent::placeEntity (Entity parentEntity)
{
    // Before the entity of the next component that has one there, or else last: the entities follow the order of their components
    auto* parent = getParentComponent();
    int index = -1;

    for (int i = parent->getIndexOfChildComponent (this) + 1; i < parent->getNumChildComponents() && index < 0; ++i)
    {
        const auto* sibling = dynamic_cast<const SceneNodeComponent*> (parent->getChildComponent (i));

        if (sibling == nullptr || ! sibling->isEntityOwned || ! sibling->entity.isValid() || sibling->entity.getParent() != parentEntity)
            continue;

        index = 0;

        for (const auto child : parentEntity.getChildren())
        {
            if (child == sibling->entity)
                break;

            if (child != entity)
                ++index;
        }
    }

    // Where a new entity is, and where one that did not move stays
    if (parentEntity.getChild (index < 0 ? parentEntity.getNumChildren() - 1 : index) == entity)
        return;

    parentEntity.addChild (entity, index);
}

//==============================================================================

void SceneNodeComponent::applyTransform()
{
    if (! entity.isValid())
        return;

    const auto target = readSceneVector (lookAt);
    const auto isPositionSet = ! (position.isVoid() && travel.isVoid());
    const auto isRotationSet = ! (rotation.isVoid() && lookAt.isVoid() && turn.isVoid());
    const auto isSet = isPositionSet || isRotationSet || ! scale.isVoid();

    auto* transform = entity.get<TransformNode>();

    if (isSet && transform == nullptr)
        transform = &entity.attach<TransformNode>();

    if (transform != nullptr && ! isSet && isEntityOwned)
    {
        // Nothing places it anymore: it is a plain group again
        entity.detach<TransformNode>();
    }
    else if (transform != nullptr)
    {
        // What is not set is the identity for an entity of its own. The entity of a model is left what it
        // has, for what else moves it, and gets back what the model gave it once the node stops setting it
        const auto given = boundTransform.value_or (Transform());

        const auto drives = [this] (bool isValueSet, bool& wasDriven)
        {
            const auto wasSet = std::exchange (wasDriven, isValueSet);
            return isEntityOwned || isValueSet || wasSet;
        };

        // Along its travel and its turn, from where it is without them. The turn is in its own space: around the
        // axes its rotation gave it
        const auto along = static_cast<float> (currentValue);

        if (drives (isPositionSet, drivesPosition))
            transform->setPosition (getRestPosition() + readSceneVector (travel).value_or (Vector3<float>()) * along);

        if (drives (isRotationSet, drivesRotation))
        {
            const auto angles = readSceneVector (rotation);
            const auto turned = readSceneVector (turn);
            const auto rest = angles.has_value() ? makeSceneRotation (*angles) : given.rotation;

            transform->setRotation (turned.has_value() ? rest * makeSceneRotation (*turned * along) : rest);
        }

        if (drives (! scale.isVoid(), drivesScale))
            transform->setScale (readSceneScale (scale).value_or (given.scale));

        if (target.has_value())
            transform->lookAt (*target);
    }

    repaintScene();
}

var SceneNodeComponent::getTransformValue (const var& value, const var& identity) const
{
    // A copy: the numbers of an array are shared by the values made from it, and changing those of the one
    // handed out would change what the node keeps, or what every node reads when the value is not set
    if (! value.isVoid())
        return value.clone();

    // Not set: the identity for an entity of its own, nothing for the entity of a model, which keeps
    // what the model gave it, and nothing outside a scene. Setting nothing back is not setting it
    return isEntityOwned && entity.isValid() ? identity.clone() : var();
}

void SceneNodeComponent::setTransformValue (var SceneNodeComponent::* member, const var& newValue, const Identifier& property)
{
    auto& value = this->*member;

    // Of the same type: nothing, which unsets the value, is not the number 0
    if (value.equalsWithSameType (newValue))
        return;

    // Its own copy: an array given back with other numbers in it is then another value
    value = newValue.clone();

    const auto isScale = member == &SceneNodeComponent::scale;
    const auto isUsable = value.isVoid() || (isScale ? readSceneScale (value) : readSceneVector (value)).has_value();

    if (auto* view = SceneComponent::findFor (*this))
        view->clearProblems (*this, property);

    applyTransform();

    // Told last, what is wrong then the change: who hears about either may destroy the node
    const BailOutChecker checker (this);

    if (auto* view = SceneComponent::findFor (*this); view != nullptr && ! isUsable)
        view->reportProblem (*this, property, isScale ? "expected a number, or the 3 numbers of a scale along x, y and z" : "expected 3 numbers");

    if (! checker.shouldBailOut())
        notifyPropertyChanged (property);
}

Vector3<float> SceneNodeComponent::getRestPosition() const
{
    // Where the node puts itself, or else where the model put the entity it is bound to
    return readSceneVector (position).value_or (boundTransform.value_or (Transform()).position);
}

void SceneNodeComponent::setValue (double newValue)
{
    const auto limited = jlimit (0.0, 1.0, newValue);

    if (currentValue == limited)
        return;

    currentValue = limited;

    // Only a node that travels or turns is placed by it
    if (! (travel.isVoid() && turn.isVoid()))
        applyTransform();

    // Told last: who hears about the change may destroy the node
    notifyPropertyChanged (SceneNodeComponentIds::value);
}

std::optional<double> SceneNodeComponent::getDraggedValue (Point<float> pointerPosition)
{
    if (! press.has_value())
        return std::nullopt;

    const auto dragged = pointerPosition - press->position;

    if (const auto distance = readSceneVector (travel))
    {
        auto* view = SceneComponent::findFor (*this);

        if (view == nullptr || ! entity.isValid())
            return std::nullopt;

        // The line the node travels on, as the scene shows it: the node follows the pointer along it, whatever the camera
        const auto parent = entity.getParent();
        const auto toClipSpace = (parent.isValid() ? parent.getWorldMatrix() : Matrix4()).followedBy (view->getViewProjection());
        const auto area = view->getLocalBounds();
        const auto rest = getRestPosition();

        const auto from = findScenePointInArea (toClipSpace, area, rest);
        const auto to = findScenePointInArea (toClipSpace, area, rest + *distance);

        if (! from.has_value() || ! to.has_value())
            return std::nullopt;

        const auto line = *to - *from;

        if (line.magnitude() < sceneShortestDraggedTravel)
            return std::nullopt;

        return jlimit (0.0, 1.0, press->value + static_cast<double> (dragged.dotProduct (line) / line.dotProduct (line)));
    }

    // A node that only turns follows the pointer up and down, as a rotary slider does
    if (readSceneVector (turn).has_value())
        return jlimit (0.0, 1.0, press->value - static_cast<double> (dragged.getY() / sceneTurnDragDistance));

    return std::nullopt;
}

void SceneNodeComponent::applyMorphWeights (bool isLeaving)
{
    // A node that sets no weights, and has none to give back, has nothing to do
    if (morphWeights.isVoid() && ! modelWeights.has_value())
        return;

    auto* deform = entity.isValid() ? entity.get<DeformNode>() : nullptr;
    const auto hasTargets = deform != nullptr && ! deform->getWeights().empty();

    const auto* values = isLeaving ? nullptr : morphWeights.getArray();
    const auto isUsable = values != nullptr && std::all_of (values->begin(), values->end(), isSceneNumber);

    // What the mesh had when the node first set its weights: a target the node gives none keeps its own, and all get
    // theirs back once the node sets none or lets go of the entity
    if (isUsable && hasTargets && ! modelWeights.has_value())
        modelWeights.emplace (deform->getWeights().begin(), deform->getWeights().end());

    if (modelWeights.has_value())
    {
        auto weights = *modelWeights;

        if (isUsable && hasTargets)
        {
            const auto count = jmin (weights.size(), static_cast<size_t> (values->size()));

            for (size_t index = 0; index < count; ++index)
                weights[index] = static_cast<float> (values->getReference (static_cast<int> (index)));
        }
        else
        {
            modelWeights.reset();
        }

        const auto current = hasTargets ? deform->getWeights() : Span<const float>();

        if (hasTargets && ! std::equal (weights.begin(), weights.end(), current.begin(), current.end()))
        {
            // The mesh follows at once: nothing else updates a scene that plays no clip
            deform->setWeights (weights);
            deform->update (0.0);
            repaintScene();
        }
    }

    auto* view = isLeaving ? nullptr : SceneComponent::findFor (*this);
    if (view == nullptr)
        return;

    // Told last, once the mesh has its weights: who hears about it may destroy the node
    view->clearProblems (*this, SceneNodeComponentIds::morphWeights);

    if (! morphWeights.isVoid() && ! isUsable)
        view->reportProblem (*this, SceneNodeComponentIds::morphWeights, "expected numbers");
    else if (isUsable && entity.isValid() && ! hasTargets)
        view->reportProblem (*this, SceneNodeComponentIds::morphWeights, "the entity of the node draws no mesh with morph targets");
}

void SceneNodeComponent::setMorphWeights (const var& newWeights)
{
    // Of the same type: nothing, which sets no weights, is not the number 0
    if (morphWeights.equalsWithSameType (newWeights))
        return;

    // Its own copy: an array given back with other numbers in it is then another value
    morphWeights = newWeights.clone();

    if (auto* view = SceneComponent::findFor (*this))
        view->clearProblems (*this, SceneNodeComponentIds::morphWeights);

    // What is wrong is told once the mesh has its weights, then the change: who hears about either may destroy the node
    const BailOutChecker checker (this);

    applyMorphWeights();

    if (! checker.shouldBailOut())
        notifyPropertyChanged (SceneNodeComponentIds::morphWeights);
}

void SceneNodeComponent::repaintScene()
{
    if (auto* view = SceneComponent::findFor (*this))
        view->sceneContentChanged();
}

//==============================================================================

void SceneNodeComponent::setSurfaceSlot (SurfaceSlot newSlot)
{
    if (surfaceSlot == newSlot)
        return;

    // The material gets back what the surface took from the slot it leaves
    if (surface != nullptr)
        surface->restoreMaterial();

    surfaceSlot = newSlot;

    // Told last: who hears about the change may destroy the node
    repaintScene();
    notifyPropertyChanged (SceneNodeComponentIds::surface);
}

void SceneNodeComponent::childrenChanged()
{
    std::vector<Component*> shown;

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* child = getChildComponent (i); isSceneSurfaceComponent (*child))
            shown.push_back (child);
    }

    if (shown.empty() && surface == nullptr)
        return;

    if (shown.empty())
    {
        surface.reset();
    }
    else
    {
        if (surface == nullptr)
            surface = std::make_unique<SceneSurface> (*this);

        surface->setComponents (shown);
    }

    // The scene finds the mesh and the material that show them at its next frame, and once its document has loaded
    if (auto* view = SceneComponent::findFor (*this))
        view->deferSurfaceUpdate();
}

std::optional<Point<float>> SceneNodeComponent::getChildPointFromLocal (const Component& child, Point<float> localPoint) const
{
    if (surface == nullptr || ! isSceneSurfaceComponent (child))
        return Component::getChildPointFromLocal (child, localPoint);

    // Where hitTest() found the pointer over a node inside this one, the mesh of this one is behind that node: the
    // components it shows are not under the pointer, wherever they are written among the children. A component that
    // holds the pointer is not looked for under it, and so keeps being told where the mesh shows it
    if (coveredPosition == localPoint)
        return std::nullopt;

    // The point of the scene is the one of the surface the mesh shows there, which the component is in as in any parent
    if (const auto surfacePoint = surface->getSurfacePoint (localPoint))
        return Component::getChildPointFromLocal (child, *surfacePoint);

    return std::nullopt;
}

std::optional<Point<float>> SceneNodeComponent::getLocalPointFromChild (const Component& child, Point<float> childPoint) const
{
    const auto point = Component::getLocalPointFromChild (child, childPoint);

    if (surface == nullptr || ! point.has_value() || ! isSceneSurfaceComponent (child))
        return point;

    return surface->getScenePoint (*point);
}

//==============================================================================

const ComponentTypeInfo& SceneNodeComponent::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;

    // A value of the transform is kept as it is given, so that it reads back the same
    const auto transformProperty = [] (const Identifier& name, Type type, var SceneNodeComponent::* member, const var& identity, const char* description)
    {
        return ComponentProperty {
            .name = name,
            .type = type,
            .elementType = type == Type::array ? Type::number : Type::any,
            .defaultValue = identity,
            .getter = [member, identity] (const Component& component)
            {
                const auto& node = static_cast<const SceneNodeComponent&> (component);
                return node.getTransformValue (node.*member, identity);
            },
            .setter = [member, name] (Component& component, const var& value)
            {
                static_cast<SceneNodeComponent&> (component).setTransformValue (member, value, name);
            },
            .description = description
        };
    };

    static const auto info = ComponentTypeBuilder<SceneNodeComponent> (Identifier ("Node3D"))
                                 .property (SceneNodeComponentIds::node, &SceneNodeComponent::getNodeName, &SceneNodeComponent::setNodeName, String(), "Binds the node to an entity of the Model3D it is in, instead of making one: the name of the entity, or the names leading to it from the root of the model separated by '/'.")
                                 .enumeration<SurfaceSlot> (
                                     SceneNodeComponentIds::surface,
                                     { { "emissive", SurfaceSlot::emissive },
                                       { "baseColor", SurfaceSlot::baseColor } },
                                     &SceneNodeComponent::getSurfaceSlot,
                                     &SceneNodeComponent::setSurfaceSlot,
                                     "How the components inside the node are shown on its mesh: glowing like a screen whatever the light (emissive), or lit as the rest of the object (baseColor).")
                                 .signal ("onClick", &SceneNodeComponent::onClick, "Emitted when the node is clicked: pressed, then released over it.")
                                 .addProperty (transformProperty (SceneNodeComponentIds::position, Type::array, &SceneNodeComponent::position, makeSceneVector (0.0, 0.0, 0.0), "The position of the node relative to its parent, as [x, y, z]."))
                                 .addProperty (transformProperty (SceneNodeComponentIds::rotation, Type::array, &SceneNodeComponent::rotation, makeSceneVector (0.0, 0.0, 0.0), "The rotation of the node relative to its parent, as Euler angles in radians [x, y, z]: the turn around z is applied first, then the one around x, then the one around y."))
                                 .addProperty (transformProperty (SceneNodeComponentIds::scale, Type::any, &SceneNodeComponent::scale, var (1.0), "The scale of the node relative to its parent: a number for every axis, or [x, y, z]."))
                                 .addProperty (transformProperty (SceneNodeComponentIds::lookAt, Type::array, &SceneNodeComponent::lookAt, var(), "A point the node turns its -z axis to, the way cameras and lights look, as [x, y, z] relative to its parent. It replaces the rotation."))
                                 .addProperty (transformProperty (SceneNodeComponentIds::travel, Type::array, &SceneNodeComponent::travel, var(), "How far the node moves over its range, as [x, y, z] relative to its parent, from where it is without it. Set, the node is dragged along that line as the handle of a slider is."))
                                 .addProperty (transformProperty (SceneNodeComponentIds::turn, Type::array, &SceneNodeComponent::turn, var(), "The Euler angles in radians [x, y, z], as those of the rotation, the node turns by over its range, in its own space. Set without a travel, the node is dragged up and down as a knob is."))
                                 .styleableProperty (SceneNodeComponentIds::value, &SceneNodeComponent::getValue, &SceneNodeComponent::setValue, 0.0, "Where the node is along its travel and its turn, from 0 to 1.")
                                 .addProperty (ComponentProperty {
                                     .name = SceneNodeComponentIds::morphWeights,
                                     .type = Type::array,
                                     .elementType = Type::number,
                                     .getter = [] (const Component& component)
                                     {
                                         return static_cast<const SceneNodeComponent&> (component).morphWeights.clone();
                                     },
                                     .setter = [] (Component& component, const var& value)
                                     {
                                         static_cast<SceneNodeComponent&> (component).setMorphWeights (value);
                                     },
                                     .description = "The weight of each morph target of the mesh the entity of the node deforms, in the order of the targets. Not set, the weights stay as the model, or a clip animating them, has them." })
                                 .build();

    return info;
}

//==============================================================================

void SceneNodeComponent::setVisible (bool shouldBeVisible)
{
    if (entity.isValid() && entity.isVisible() != shouldBeVisible)
    {
        entity.setVisible (shouldBeVisible);
        repaintScene();
    }

    Component::setVisible (shouldBeVisible);
}

bool SceneNodeComponent::hitTest (float x, float y)
{
    auto* view = SceneComponent::findFor (*this);

    coveredPosition.reset();

    if (view == nullptr)
        return false;

    // The node has the area of its scene, as the nodes it is in have: its points are those of the scene. It
    // answers for what it shows and for what the nodes inside it show, which the pointer reaches through it
    const Point<float> position (x, y);
    const auto* picked = view->findNodeAt (position);

    // Where the pointer is looked for is where it is, for the components the mesh shows: see SceneSurface::getSurfacePoint()
    if (surface != nullptr)
        surface->setPointerPosition (position);

    for (const Component* owner = picked; owner != nullptr && owner != view; owner = owner->getParentComponent())
    {
        if (owner != this)
            continue;

        // A node inside this one is in front of its mesh there: remembered for the components the mesh shows, which
        // are asked for the pointer too and are not under it
        if (picked != this)
            coveredPosition = position;

        return true;
    }

    return false;
}

void SceneNodeComponent::mouseDown (const MouseEvent& event)
{
    press = Press { event.getPosition(), currentValue, false };
}

void SceneNodeComponent::mouseDrag (const MouseEvent& event)
{
    const auto dragged = getDraggedValue (event.getPosition());

    if (! dragged.has_value())
        return;

    // A press that moved the value dragged the node, and is not a click
    if (*dragged != currentValue)
        press->hasMovedValue = true;

    // Last: who hears about the change may destroy the node
    setValue (*dragged);
}

void SceneNodeComponent::mouseUp (const MouseEvent& event)
{
    // As a Button: a node that is not enabled is given the release of a press it was not given
    const auto released = std::exchange (press, std::nullopt);

    if (released.has_value() && ! released->hasMovedValue && isEnabled() && onClick != nullptr && hitTest (event.getPosition().getX(), event.getPosition().getY()))
        onClick();
}

void SceneNodeComponent::visibilityChanged()
{
    // What the components inside the node asked to be painted while they were not showing was dropped: they are
    // rendered again now that they are
    if (surface != nullptr)
        surface->repaintComponents();
}

void SceneNodeComponent::enablementChanged()
{
    // The components inside the node are enabled with it and paint themselves accordingly, but only the component
    // that was enabled or not is asked to repaint
    if (surface != nullptr)
        surface->repaintComponents();
}

void SceneNodeComponent::parentHierarchyChanged()
{
    updateBounds();
    updateEntity();
}

void SceneNodeComponent::parentSizeChanged()
{
    updateBounds();
}

void SceneNodeComponent::resized()
{
    updateBounds();
}

void SceneNodeComponent::moved()
{
    updateBounds();
}

void SceneNodeComponent::updateBounds()
{
    // The area of its scene, where the pointer is to be tested against what the node shows
    if (const auto* parent = getParentComponent())
        setBounds (parent->getLocalBounds());
}

void SceneNodeComponent::updateEntitiesBelow (Component& parent)
{
    // Found before any of them is updated, a node before the ones inside it: one that can't have its entity says
    // so, and who hears about it may destroy the others
    std::vector<WeakReference<Component>> nodes;

    forEachSceneNode (parent, [&nodes] (SceneNodeComponent& node)
    {
        nodes.emplace_back (&node);
    });

    for (const auto& reference : nodes)
    {
        if (auto* node = static_cast<SceneNodeComponent*> (reference.get()))
            node->updateEntity();
    }
}

} // namespace yup
