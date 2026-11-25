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

struct SceneMeshComponentIds
{
    static inline const Identifier vertices { "vertices" };
    static inline const Identifier normals { "normals" };
    static inline const Identifier uvs { "uvs" };
    static inline const Identifier colors { "colors" };
    static inline const Identifier indices { "indices" };
};

/* One of the arrays a mesh is made from, with the numbers read from it. */
struct SceneMeshInput
{
    const var& value;
    const Identifier& property;
    size_t numbersPerItem;
    std::vector<double> numbers;

    // False when the value holds something else than numbers: one that is not set holds none
    bool read()
    {
        if (value.isVoid())
            return true;

        const auto* values = value.getArray();
        if (values == nullptr || ! std::all_of (values->begin(), values->end(), isSceneNumber))
            return false;

        numbers.reserve (static_cast<size_t> (values->size()));

        for (const auto& number : *values)
            numbers.push_back (static_cast<double> (number));

        return true;
    }

    float get (size_t index) const noexcept
    {
        return static_cast<float> (numbers[index]);
    }
};

} // namespace

//==============================================================================

SceneMeshComponent::SceneMeshComponent (StringRef componentID)
    : SceneNodeComponent (componentID)
{
}

//==============================================================================

void SceneMeshComponent::setGeometry (var SceneMeshComponent::* member, const var& newValue, const Identifier& property)
{
    auto& value = this->*member;

    if (value.equalsWithSameType (newValue))
        return;

    // Its own copy: an array given back with other numbers in it is then another value
    value = newValue.clone();

    geometryChanged();
    notifyPropertyChanged (property);
}

void SceneMeshComponent::geometryChanged()
{
    // The arrays set together make one mesh: it is built when the document has applied its values,
    // before the next frame of the scene, or when it is asked for
    if (auto* view = SceneComponent::findFor (*this); view != nullptr && ! isMeshOutdated)
        view->deferMeshUpdate (*this);

    isMeshOutdated = true;
    repaintScene();
}

Mesh::Ptr SceneMeshComponent::getMesh()
{
    if (! std::exchange (isMeshOutdated, false))
        return mesh;

    Identifier problemProperty;
    String problem;

    const auto built = buildMesh (problemProperty, problem);
    mesh = built;

    if (auto entity = getEntity(); entity.isValid())
        entity.getOrAttach<MeshNode>().mesh = mesh;

    // The scene is painted with the mesh it now has: the one that asked for it may have been painted already
    repaintScene();

    // What is wrong with the arrays is told last: who hears about it may destroy the mesh
    if (auto* view = SceneComponent::findFor (*this); view != nullptr && problem.isNotEmpty())
        view->reportProblem (*this, problemProperty, problem);

    return built;
}

void SceneMeshComponent::entityChanged()
{
    if (! getEntity().isValid())
        return;

    // Built again for the scene it is now in, which is the one told what is wrong with the arrays
    isMeshOutdated = false;
    geometryChanged();
}

Mesh::Ptr SceneMeshComponent::buildMesh (Identifier& problemProperty, String& problem)
{
    SceneMeshInput inputs[] = { { vertices, SceneMeshComponentIds::vertices, 3, {} },
                                { normals, SceneMeshComponentIds::normals, 3, {} },
                                { uvs, SceneMeshComponentIds::uvs, 2, {} },
                                { colors, SceneMeshComponentIds::colors, 4, {} },
                                { indices, SceneMeshComponentIds::indices, 3, {} } };

    auto& [positions, vertexNormals, vertexUvs, vertexColors, triangles] = inputs;
    auto* view = SceneComponent::findFor (*this);

    const auto count = [] (size_t number) { return String (static_cast<int> (number)); };

    const auto fail = [&] (const SceneMeshInput& input, const String& message) -> Mesh::Ptr
    {
        problemProperty = input.property;
        problem = message;

        return nullptr;
    };

    for (auto& input : inputs)
    {
        if (view != nullptr)
            view->clearProblems (*this, input.property);
    }

    for (auto& input : inputs)
    {
        if (! input.read())
            return fail (input, "expected numbers");
    }

    if (positions.numbers.size() % positions.numbersPerItem != 0)
        return fail (positions, "expected an x, y and z for each vertex: " + count (positions.numbers.size()) + " numbers are not a multiple of 3");

    const auto numVertices = positions.numbers.size() / positions.numbersPerItem;

    for (const auto* input : { &vertexNormals, &vertexUvs, &vertexColors })
    {
        if (! input->numbers.empty() && input->numbers.size() != numVertices * input->numbersPerItem)
            return fail (*input, "expected " + count (numVertices * input->numbersPerItem) + " numbers, " + count (input->numbersPerItem) + " for each of the " + count (numVertices) + " vertices, not " + count (input->numbers.size()));
    }

    std::vector<uint32> meshIndices;

    if (triangles.numbers.empty())
    {
        if (numVertices % 3 != 0)
            return fail (positions, "without indices the vertices are taken three by three: " + count (numVertices) + " vertices are not a multiple of 3");
    }
    else
    {
        if (triangles.numbers.size() % triangles.numbersPerItem != 0)
            return fail (triangles, "expected three indices for each triangle: " + count (triangles.numbers.size()) + " indices are not a multiple of 3");

        meshIndices.reserve (triangles.numbers.size());

        for (const auto index : triangles.numbers)
        {
            if (! (index >= 0.0 && index < static_cast<double> (numVertices)))
                return fail (triangles, "the index " + String (index, 0) + " is not the one of a vertex, there are " + count (numVertices));

            meshIndices.push_back (static_cast<uint32> (index));
        }
    }

    if (numVertices == 0)
        return nullptr;

    if (numVertices < 3)
        return fail (positions, "a triangle takes 3 vertices, there are " + count (numVertices));

    std::vector<Mesh::Vertex> meshVertices (numVertices);

    for (size_t i = 0; i < numVertices; ++i)
    {
        auto& vertex = meshVertices[i];
        vertex.position = { positions.get (i * 3), positions.get (i * 3 + 1), positions.get (i * 3 + 2) };

        if (! vertexNormals.numbers.empty())
            vertex.normal = { vertexNormals.get (i * 3), vertexNormals.get (i * 3 + 1), vertexNormals.get (i * 3 + 2) };

        if (! vertexUvs.numbers.empty())
            vertex.uv = { vertexUvs.get (i * 2), vertexUvs.get (i * 2 + 1) };

        if (! vertexColors.numbers.empty())
            vertex.color = { vertexColors.get (i * 4), vertexColors.get (i * 4 + 1), vertexColors.get (i * 4 + 2), vertexColors.get (i * 4 + 3) };
    }

    auto built = Mesh::Ptr (new Mesh (getComponentID()));
    built->addPrimitive (std::move (meshVertices), std::move (meshIndices));
    return built;
}

//==============================================================================

const ComponentTypeInfo& SceneMeshComponent::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;

    // An array is kept as it is given, and read into the mesh when it is built
    const auto geometryProperty = [] (const Identifier& name, Type elementType, var SceneMeshComponent::* member, const char* description)
    {
        return ComponentProperty {
            .name = name,
            .type = Type::array,
            .elementType = elementType,
            .defaultValue = var (Array<var>()),
            .getter = [member] (const Component& component)
            {
                // A copy: the numbers of an array are shared by the values made from it
                const auto& value = static_cast<const SceneMeshComponent&> (component).*member;
                return value.isVoid() ? var (Array<var>()) : value.clone();
            },
            .setter = [member, name] (Component& component, const var& value)
            {
                static_cast<SceneMeshComponent&> (component).setGeometry (member, value, name);
            },
            .description = description
        };
    };

    static const auto info = ComponentTypeBuilder<SceneMeshComponent> (Identifier ("Mesh3D"), SceneNodeComponent::getClassTypeInfo())
                                 .addProperty (geometryProperty (SceneMeshComponentIds::vertices, Type::number, &SceneMeshComponent::vertices, "The x, y, z of each vertex, one after the other."))
                                 .addProperty (geometryProperty (SceneMeshComponentIds::normals, Type::number, &SceneMeshComponent::normals, "The x, y, z of the normal of each vertex. Without them each triangle gets flat normals."))
                                 .addProperty (geometryProperty (SceneMeshComponentIds::uvs, Type::number, &SceneMeshComponent::uvs, "The u, v texture coordinates of each vertex, v pointing down."))
                                 .addProperty (geometryProperty (SceneMeshComponentIds::colors, Type::number, &SceneMeshComponent::colors, "The linear r, g, b, a of each vertex, multiplied with the base color of the material."))
                                 .addProperty (geometryProperty (SceneMeshComponentIds::indices, Type::integer, &SceneMeshComponent::indices, "Three vertex indices for each triangle, counter-clockwise when front facing. Without them the vertices are taken three by three."))
                                 .build();

    return info;
}

} // namespace yup
