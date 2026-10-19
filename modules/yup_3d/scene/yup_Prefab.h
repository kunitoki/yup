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
/** A template of an entity tree, which builds a fresh copy of it each time it is instantiated.

    A prefab is made of parts: each part becomes an entity, named like the part, with the nodes
    the part attaches and the children the part holds. Nodes are described by how to make them,
    so every instance gets its own nodes, while shared resources like meshes and materials
    passed to them are shared by every instance, also across scenes.

    @code
    auto lamp = yup::Prefab::Ptr (new yup::Prefab ("lamp"));
    lamp->getRoot()
        .attach<yup::TransformNode> (yup::Vector3<float> (0.0f, 2.0f, 0.0f))
        .attach<yup::MeshNode> (lampMesh);

    lamp->getRoot().addChild ("bulb")
        .attachWith<yup::LightNode> ([] (yup::LightNode& light) { light.intensity = 4.0f; });

    yup::Entity first = lamp->instantiate (*scene);
    yup::Entity second = lamp->instantiate (*scene, first);
    @endcode

    Build a prefab completely before instantiating or sharing it: instances don't follow later
    changes. GltfModel builds one prefab per glTF scene.

    @see GltfModel, Scene, Entity
*/
class YUP_API Prefab : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<Prefab>;

    //==============================================================================
    /** One entity of a prefab: its name, the nodes to attach to it and its children. */
    class YUP_API Part
    {
    public:
        /** Creates a part making an entity named @p name. */
        explicit Part (const String& name = {});

        /** Returns the name of the entities this part makes. */
        const String& getName() const noexcept { return name; }

        //==============================================================================
        /** Adds a node of type T, constructed from copies of @p args on every instantiation.

            The arguments are copied into the prefab, so they must be copyable: pass shared
            resources by their Ptr. A later node of the same type replaces this one, like
            Entity::attach() does.

            @return This part, to chain calls.
        */
        template <class T, class... Args>
        Part& attach (Args&&... args)
        {
            static_assert (std::is_base_of_v<Node, T>, "Only Node types can be attached");

            factories.push_back ([... args = std::forward<Args> (args)] (Entity& entity)
            {
                entity.attach<T> (args...);
            });

            return *this;
        }

        /** Adds a default-constructed node of type T that @p setup configures on every instantiation.

            @param setup A copyable callable taking a T&, called once per instance right after
                         the node is attached.

            @return This part, to chain calls.
        */
        template <class T, class Setup>
        Part& attachWith (Setup setup)
        {
            static_assert (std::is_base_of_v<Node, T>, "Only Node types can be attached");
            static_assert (std::is_invocable_v<const Setup&, T&>, "The setup must take a T&");

            factories.push_back ([setup = std::move (setup)] (Entity& entity)
            {
                setup (entity.attach<T>());
            });

            return *this;
        }

        //==============================================================================
        /** Adds a child part, made after the existing ones, and returns it.

            The reference stays valid as long as the prefab.
        */
        Part& addChild (const String& childName = {});

        /** Returns the number of child parts. */
        int getNumChildren() const noexcept { return static_cast<int> (children.size()); }

        /** Returns a child part, @p index must be in range. */
        Part& getChild (int index) noexcept;

        /** Returns a child part, @p index must be in range. */
        const Part& getChild (int index) const noexcept;

    private:
        friend class Prefab;

        String name;
        std::vector<std::function<void (Entity&)>> factories;
        std::vector<std::unique_ptr<Part>> children;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Part)
    };

    //==============================================================================
    /** Creates a prefab whose root part makes an entity named @p rootName. */
    explicit Prefab (const String& rootName = {});

    /** Returns the root part. */
    Part& getRoot() noexcept { return root; }

    /** Returns the root part. */
    const Part& getRoot() const noexcept { return root; }

    //==============================================================================
    /** Builds a new entity tree from the prefab.

        Parts become entities in order, parents before children, so attach callbacks and
        signals see the parents already built. Deep hierarchies can't overflow the stack.

        @param scene  The scene to create the entities in.
        @param parent The entity to add the tree to, or an invalid entity for the root of the scene.

        @return The entity made by the root part.
    */
    Entity instantiate (Scene& scene, Entity parent = {}) const;

private:
    Part root;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Prefab)
};

} // namespace yup
