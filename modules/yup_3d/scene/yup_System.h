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

class Scene;
class System;

//==============================================================================
/** Where a System runs in Scene::update(), built with chained calls.

    A system runs in a phase, before the node updates (the default) or after them, and may be
    ordered against other System and Node types with before<T>() and after<T>():

    @code
    scene->addSystem<Gravity> (yup::SystemOrder()
                                   .beforeNodes()
                                   .before<Physics>()
                                   .after<Wind>());
    @endcode

    Constraints are rules about types, see Scene::addSystemRule().

    @see Scene::addSystem, Scene::addSystemRule
*/
class SystemOrder
{
public:
    //==============================================================================
    /** The phase of a system, relative to the Node::update() calls. */
    enum class Phase
    {
        beforeNodes, ///< Before the nodes are updated.
        afterNodes   ///< After the nodes are updated.
    };

    /** A constraint against another type, made by before() and after(). */
    struct Constraint
    {
        uint64 typeId = 0;
        bool runsBefore = true;
    };

    //==============================================================================
    /** Creates an order running before the node updates, without constraints. */
    SystemOrder() = default;

    //==============================================================================
    /** Runs the system before the nodes are updated. */
    SystemOrder& beforeNodes() noexcept
    {
        phase = Phase::beforeNodes;
        return *this;
    }

    /** Runs the system after the nodes are updated. */
    SystemOrder& afterNodes() noexcept
    {
        phase = Phase::afterNodes;
        return *this;
    }

    /** Runs the system before the systems of type T, or before the updates of the nodes of type T. */
    template <class T>
    SystemOrder& before()
    {
        static_assert (std::is_base_of_v<System, T> || std::is_base_of_v<Node, T>, "Systems are ordered against yup::System or yup::Node types");
        constraints.push_back ({ NodeTypeId::get<T>(), true });
        return *this;
    }

    /** Runs the system after the systems of type T, or after the updates of the nodes of type T. */
    template <class T>
    SystemOrder& after()
    {
        static_assert (std::is_base_of_v<System, T> || std::is_base_of_v<Node, T>, "Systems are ordered against yup::System or yup::Node types");
        constraints.push_back ({ NodeTypeId::get<T>(), false });
        return *this;
    }

    //==============================================================================
    /** Returns the phase. */
    Phase getPhase() const noexcept { return phase; }

    /** Returns the constraints, in the order they were added. */
    const std::vector<Constraint>& getConstraints() const noexcept { return constraints; }

private:
    Phase phase = Phase::beforeNodes;
    std::vector<Constraint> constraints;
};

//==============================================================================
/** Logic that runs on the whole scene once per Scene::update().

    Systems suit work across many nodes, for example moving every node of a type through a
    view, while Node::update() suits simple per-node scripting. Systems run before or after
    the node updates, in the order required by their SystemOrder constraints and
    Scene::addSystemRule(), and otherwise in the order they were added.

    @code
    class Gravity : public yup::System
    {
    public:
        void update (yup::Scene& scene, double deltaSeconds) override
        {
            for (auto [entity, body] : scene.view<RigidBody>())
                body.velocity += gravity * static_cast<float> (deltaSeconds);
        }

        yup::Vector3<float> gravity { 0.0f, -9.81f, 0.0f };
    };

    scene->addSystem<Gravity> (yup::SystemOrder().beforeNodes());
    @endcode

    @see Scene::addSystem
*/
class YUP_API System
{
public:
    /** Destructor. */
    virtual ~System() = default;

    /** Called once per Scene::update().

        @param scene         The scene that owns the system.
        @param deltaSeconds  The time elapsed since the previous update.
    */
    virtual void update (Scene& scene, double deltaSeconds) = 0;
};

} // namespace yup
