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

#include "yup_YuiCommands.h"
#include "yup_YuiToolSupport.h"

#include <iostream>
#include <map>
#include <set>

using namespace yup;

namespace
{

// Bumped when a field is removed or changes meaning, not when one is added
constexpr int schemaVersion = 1;

const char* getTypeName (ComponentProperty::Type type)
{
    switch (type)
    {
        case ComponentProperty::Type::number:
            return "number";

        case ComponentProperty::Type::integer:
            return "integer";

        case ComponentProperty::Type::boolean:
            return "boolean";

        case ComponentProperty::Type::string:
            return "string";

        case ComponentProperty::Type::color:
            return "color";

        case ComponentProperty::Type::point:
            return "point";

        case ComponentProperty::Type::rectangle:
            return "rectangle";

        case ComponentProperty::Type::transform:
            return "transform";

        case ComponentProperty::Type::enumeration:
            return "enumeration";

        case ComponentProperty::Type::font:
            return "font";

        case ComponentProperty::Type::drawing:
            return "drawing";

        case ComponentProperty::Type::array:
            return "array";

        case ComponentProperty::Type::any:
            break;
    }

    return "any";
}

// Only the defaults a `.yui` file can spell as a literal are listed
var describeDefault (const ComponentProperty& property)
{
    const auto& value = property.defaultValue;

    if (property.type == ComponentProperty::Type::color && (value.isInt() || value.isInt64()))
        return VariantConverter<Color>::fromVar (value).toString();

    if (value.isBool() || value.isInt() || value.isInt64() || value.isDouble() || value.isString())
        return value;

    return {};
}

void describeType (DynamicObject& record, ComponentProperty::Type type, ComponentProperty::Type elementType)
{
    record.setProperty ("type", getTypeName (type));

    if (type == ComponentProperty::Type::array && elementType != ComponentProperty::Type::any)
        record.setProperty ("elementType", getTypeName (elementType));
}

var describeProperty (const ComponentProperty& property)
{
    auto* record = new DynamicObject();
    record->setProperty ("name", property.name.toString());
    describeType (*record, property.type, property.elementType);

    if (const auto defaultValue = describeDefault (property); ! defaultValue.isVoid())
        record->setProperty ("default", defaultValue);

    if (! property.enumValues.isEmpty())
        record->setProperty ("values", var (property.enumValues));

    record->setProperty ("state", property.isState);
    record->setProperty ("styleable", property.isStyleable);
    record->setProperty ("inherited", property.isInherited);
    record->setProperty ("readOnly", property.isReadOnly());

    // An array a drawing or a shader reads where the component keeps it, with no copy
    if (property.isSeries())
        record->setProperty ("series", true);

    if (property.description.isNotEmpty())
        record->setProperty ("description", property.description);

    return var (record);
}

var describePart (const ComponentPart& part)
{
    auto* record = new DynamicObject();
    record->setProperty ("name", part.name.toString());
    record->setProperty ("placeable", part.isPlaceable);

    if (part.description.isNotEmpty())
        record->setProperty ("description", part.description);

    return var (record);
}

var describeSignal (const ComponentSignal& signal)
{
    Array<var> arguments;

    for (const auto& argument : signal.arguments)
    {
        auto* record = new DynamicObject();
        record->setProperty ("name", argument.name.toString());
        describeType (*record, argument.type, argument.elementType);

        arguments.add (var (record));
    }

    auto* record = new DynamicObject();
    record->setProperty ("name", signal.name.toString());

    if (signal.description.isNotEmpty())
        record->setProperty ("description", signal.description);

    record->setProperty ("arguments", var (arguments));

    return var (record);
}

} // namespace

// The properties, signals and parts are the ones the type declares: the inherited ones are those of its base,
// listed here too when asked, each with the type it comes from
var describeYuiToolComponentType (const ComponentTypeInfo& info, bool includeInherited)
{
    Array<var> properties;
    Array<var> signals;
    Array<var> parts;
    std::set<String> listedProperties;
    std::set<String> listedSignals;
    std::set<String> listedParts;

    for (const auto* type = &info; type != nullptr; type = includeInherited ? type->getBaseTypeInfo() : nullptr)
    {
        for (const auto& property : type->getOwnProperties())
        {
            if (! listedProperties.insert (property.name.toString()).second)
                continue;

            auto described = describeProperty (property);

            if (type != &info)
                described.getDynamicObject()->setProperty ("from", type->getTypeName().toString());

            properties.add (described);
        }

        for (const auto& signal : type->getOwnSignals())
        {
            if (! listedSignals.insert (signal.name.toString()).second)
                continue;

            auto described = describeSignal (signal);

            if (type != &info)
                described.getDynamicObject()->setProperty ("from", type->getTypeName().toString());

            signals.add (described);
        }

        for (const auto& part : type->getOwnParts())
        {
            if (! listedParts.insert (part.name.toString()).second)
                continue;

            auto described = describePart (part);

            if (type != &info)
                described.getDynamicObject()->setProperty ("from", type->getTypeName().toString());

            parts.add (described);
        }
    }

    auto* record = new DynamicObject();
    record->setProperty ("name", info.getTypeName().toString());

    if (const auto* base = info.getBaseTypeInfo())
        record->setProperty ("base", base->getTypeName().toString());

    record->setProperty ("instantiable", info.hasFactory());
    record->setProperty ("properties", var (properties));
    record->setProperty ("signals", var (signals));

    // What a drawing places of the component with a `part` element
    if (! parts.isEmpty())
        record->setProperty ("parts", var (parts));

    return var (record);
}

namespace
{

var describeDrawingTopic()
{
    const auto grammar = YuiDocument::createSchema();
    Array<var> elements;

    for (const auto& element : DrawingStyle::getElementDescriptions())
    {
        auto* record = new DynamicObject();
        record->setProperty ("type", element.type);
        record->setProperty ("role", element.role);

        if (! element.geometry.isEmpty())
            record->setProperty ("geometry", var (element.geometry));

        record->setProperty ("properties", var (element.properties));

        // What it holds is told by the grammar, which a document is read with
        if (const Identifier type (element.type); grammar != nullptr && grammar->hasNodeType (type))
            if (const auto children = grammar->getChildConstraints (type); children.allowsChildren() && ! children.allowedTypes.isEmpty())
                record->setProperty ("holds", var (children.allowedTypes));

        elements.add (var (record));
    }

    auto* topic = new DynamicObject();
    topic->setProperty ("elements", var (elements));
    topic->setProperty ("shapePaint", var (DrawingStyle::getShapePaintProperties()));
    topic->setProperty ("notes", "A shape takes its geometry, its properties and those of shapePaint. In geometry, a name followed by '=' has that default. "
                                 "A 'use' also takes the properties its drawing declares. Every value is an expression. "
                                 "A 'vertex' is one point of a line and only goes among the shapes of a 'repeat'. "
                                 "A 'part' names one of the parts the type of the component lists in the schema.");
    topic->setProperty ("docs", "docs/ui/yui/drawing.md");

    return var (topic);
}

var describeStylesTopic()
{
    auto* topic = new DynamicObject();
    topic->setProperty ("declarations", var (StyleSheet::getDeclarationNames()));
    topic->setProperty ("pseudoClasses", var (StyleSelector::getPseudoClassNames()));
    topic->setProperty ("paletteRoles", var (ThemePalette::getRoleNames()));
    topic->setProperty ("notes", "A rule sets the declarations listed here and the styleable properties of the components it selects, which the "
                                 "components topic lists for a type. A selector is made of a type, #id, .class, :pseudoClass and [property=value], "
                                 "joined by a space for a descendant or '>' for a child. A state property of a type, such as 'checked', is a pseudo-class too.");
    topic->setProperty ("docs", "docs/ui/yui/styles.md");

    return var (topic);
}

var describeExpressionsTopic()
{
    const auto context = PropertyExpression::createContext();
    const auto known = context.getFunctions();
    std::set<String> signatures;
    StringArray functions;
    std::map<String, StringArray> members;

    for (const auto& function : known)
        signatures.insert (function.getSignature());

    for (const auto& function : known)
    {
        const auto& name = function.name;
        const auto signature = function.getSignature();

        // A function of doubles that is also one of floats says nothing more than that one
        if (signature.contains ("double") && signatures.contains (signature.replace ("double", "float")))
            continue;

        // Operators, conversions and what the library keeps for itself are not called by name
        if (name.startsWith ("operator") || name.startsWithChar ('@') || name.startsWithChar ('_') || name.substring (1).startsWithChar ('_'))
            continue;

        if (! name.startsWithChar ('.'))
        {
            functions.add (signature);
            continue;
        }

        if (function.parameterTypes.empty())
            continue;

        // A member or a method is told for the type it belongs to, which is its first parameter
        StringArray parameters;

        for (size_t index = 1; index < function.parameterTypes.size(); ++index)
            parameters.add (function.parameterTypes[index]->getName());

        const auto call = parameters.isEmpty() ? String() : " (" + parameters.joinIntoString (", ") + ")";

        members[function.parameterTypes.front()->getName()].add (name.substring (1) + call + " -> " + function.resultType->getName());
    }

    functions.sort (false);

    auto* memberRecord = new DynamicObject();

    for (auto& [type, signatures] : members)
    {
        signatures.sort (false);
        memberRecord->setProperty (type, var (signatures));
    }

    auto* enumerationRecord = new DynamicObject();

    for (const auto& [type, values] : context.getEnumerations())
        enumerationRecord->setProperty (type, var (values));

    // The named colors are too many to list: they are those of CSS
    StringArray constants;

    for (const auto& constant : context.getConstantNames())
        if (! constant.startsWith ("colors."))
            constants.add (constant);

    auto* topic = new DynamicObject();
    topic->setProperty ("functions", var (functions));
    topic->setProperty ("members", var (memberRecord));
    topic->setProperty ("enumerations", var (enumerationRecord));
    topic->setProperty ("constants", var (constants));
    topic->setProperty ("notes", "A member is read as value.name and a method called as value.name (arguments), on a value of the type they are listed for. "
                                 "The value of an enumeration is written by its name where one is expected. colors.<name> is a named color of CSS, such as "
                                 "colors.orange. An expression also reads the properties of the component it is in, those of the components and models "
                                 "it names as id.property, palette.<role>, and tokens.");
    topic->setProperty ("docs", "docs/ui/yui/language.md");

    return var (topic);
}

var describeShadersTopic()
{
    // The types a property of a shader is declared with: those a uniform takes, and the arrays of numbers
    StringArray propertyTypes;

    for (const auto& name : ComponentProperty::getDeclaredTypeNames())
        if (ShaderEffect::getNumUniformComponents (ComponentProperty::getTypeFromName (name)) > 0)
            propertyTypes.add (name);

    propertyTypes.add ("number[]");

    String notes ("A shader is a block at the top of a file. It names its GLSL file with 'fragment' or with 'material', by a path relative to the "
                  "document, and can't have the name of a drawing. Each property fills the member of its name in the one uniform block of the "
                  "shader, and is set where the shader is used: in the rule, the 'use' or the Material3D naming it, where it also takes bindings "
                  "and transitions. A fragment shader reads up to two texture2D, each through a sampler declared apart: an effect and a drawing "
                  "give it one. A member named resolution (vec2, in pixels), scale (float) or time (float, in seconds, which keeps what shows it "
                  "repainting) is filled in when no property has its name. A material is one function with no #version and no main(): it colors "
                  "a mesh unlit and declares no texture, and of these members only gets time. Both are compiled for every graphics API, so they "
                  "keep to the GLSL its WGSL target takes.");

#if YUP_ENABLE_SHADER_TRANSPILER
    notes << " yui_check compiles every shader of a file, used or not, and reports the errors of the compiler.";
#endif

    auto* topic = new DynamicObject();
    topic->setProperty ("block", "shader Name { fragment: \"fx/name.frag\"; property intensity: number = 0.5; property tint: color = palette.accent; }");
    topic->setProperty ("uses", var (StringArray { "style { .display { effect: Name; intensity: 0.8; } } runs a fragment shader over what a component paints",
                                                   "drawing Screen { use Name { bounds: bounds; intensity: 0.8; } } paints a fragment shader in a drawing",
                                                   "layer { filter { use Name { intensity: 0.8; } } ... } runs a fragment shader over a layer of a drawing",
                                                   "Material3D { shader: Name; rim: #00ffff; } colors the meshes of a scene with a material shader" }));
    topic->setProperty ("propertyTypes", var (propertyTypes));
    topic->setProperty ("fragment", String (R"glsl(#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;          // property tint: color, sRGB encoded, alpha not premultiplied
    float intensity;    // property intensity: number
} params;

layout(set = 0, binding = 1) uniform texture2D source;         // the picture, premultiplied by alpha
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;                               // (0, 0) at the top left
layout(location = 0) out vec4 color;                           // premultiplied by alpha

void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
    vec4 tint = vec4 (params.tint.rgb * params.tint.a, params.tint.a);

    color = mix (pixel, pixel * tint, params.intensity);
}
)glsl"));
    topic->setProperty ("material", String (R"glsl(YUP_MATERIAL_PARAMS
{
    vec4 rim;           // property rim: color, linear, alpha not premultiplied
} params;

vec4 yupMaterial (YupSurface surface)
{
    float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);

    return vec4 (params.rim.rgb * fresnel, fresnel);            // linear, alpha not premultiplied
}
)glsl"));
    topic->setProperty ("surface", var (StringArray { "vec3 position: where the fragment is, in the space of the scene",
                                                      "vec3 normal: the unit normal the surface is shaded with",
                                                      "vec3 view: the unit vector from the fragment to the camera",
                                                      "vec2 uv: the texture coordinates of the mesh",
                                                      "vec4 color: the color of the vertices of the mesh",
                                                      "vec4 baseColor: the base color of the Material3D, its factor by its texture, linear",
                                                      "vec3 emissive: the light the Material3D emits, linear",
                                                      "float metallic, float roughness, float occlusion: those of the Material3D, its factors by its textures" }));
    topic->setProperty ("notes", notes);
    topic->setProperty ("docs", "docs/ui/yui/shaders.md");

    return var (topic);
}

#if YUP_MODULE_AVAILABLE_yup_renderer
var describeScenesTopic()
{
    static const std::pair<const char*, const char*> types[] {
        { "Scene3D", "The component showing a scene, laid out as any other. It holds the 3D types, and paints what else it holds over the scene." },
        { "Node3D", "A place of the scene, moving what it holds: position, rotation, scale and lookAt. With 'node' it is the part of that name of the Model3D it is in." },
        { "Camera3D", "A Node3D seeing down its -z axis, y up. The scene is seen from the first one that is active." },
        { "Light3D", "A Node3D that lights: directional down its -z axis, point, or spot." },
        { "Mesh3D", "A Node3D drawing triangles: the x, y and z of each of its 'vertices', three 'indices' a triangle, counter-clockwise seen from its front." },
        { "Model3D", "A Node3D drawing the model of a .glb or .gltf file, its 'source'." },
        { "Material3D", "The material of the meshes of what it is written in, and of what that holds: the factors and textures of a PBR material, or a 'shader'. It is no Node3D." }
    };

    Array<var> described;

    for (const auto& [name, what] : types)
    {
        auto* record = new DynamicObject();
        record->setProperty ("name", name);
        record->setProperty ("what", what);

        described.add (var (record));
    }

    auto* topic = new DynamicObject();
    topic->setProperty ("types", var (described));
    topic->setProperty ("example", String (R"yui(Scene3D {
    background: #101014; flexGrow: 1;

    Camera3D { position: [0, 0.5, 4]; lookAt: [0, 0, 0]; fov: 45deg; }
    Light3D { type: directional; rotation: [-45deg, 30deg, 0]; intensity: 3; }

    Mesh3D {
        position: [-1.5, 0, 0];
        vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
        indices: [0, 1, 2,  0, 2, 3];

        Material3D { baseColor: #ff8800; metallic: 0; roughness: 0.6; }
    }

    Model3D {
        source: "models/device.glb";

        Node3D { node: "Knob"; rotation: [0, cutoff.value * 270deg, 0]; }
    }

    Slider { id: cutoff; x: 16; y: 16; width: 160; height: 24; }
}
)yui"));
    topic->setProperty ("notes", "The 3D types are components: the components topic lists the properties of each, which take bindings, style rules and transitions "
                                 "as those of a widget do. A Scene3D needs a size as any component does, a flexGrow or a width and a height. A position, a "
                                 "scale and a lookAt are [x, y, z], and a rotation is the angle around each axis in radians, written 45deg. A component that "
                                 "is no 3D type is painted over the scene when it is written in the Scene3D, and on the surface of a node when it is written "
                                 "in one. A node answers the pointer as a widget does, with ':hover', ':pressed' and 'onClick', and a Model3D plays the clips "
                                 "of its file with 'clip' and 'playing'. The 'source' of a Model3D and the textures of a Material3D are paths relative to the "
                                 "document, read with it: a .glb, or a .gltf holding its buffers as data URIs, and PNG, JPEG or WebP images. yui_check "
                                 "reports the files that can't be read or loaded, and yui_render draws the scene once it has settled.");
    topic->setProperty ("docs", "docs/ui/yui/scenes.md");

    return var (topic);
}
#endif

} // namespace

StringArray getYuiToolSchemaTopics()
{
    StringArray topics { "drawing", "styles", "expressions", "shaders" };

#if YUP_MODULE_AVAILABLE_yup_renderer
    topics.add ("scenes");
#endif

    return topics;
}

var describeYuiToolSchemaTopic (const String& topic)
{
    if (topic == "drawing")
        return describeDrawingTopic();

    if (topic == "styles")
        return describeStylesTopic();

    if (topic == "expressions")
        return describeExpressionsTopic();

    if (topic == "shaders")
        return describeShadersTopic();

#if YUP_MODULE_AVAILABLE_yup_renderer
    if (topic == "scenes")
        return describeScenesTopic();
#endif

    return {};
}

int runYuiSchemaCommand (int argc, char** argv)
{
    File output;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--output" && i + 1 < argc)
        {
            output = File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
            continue;
        }

        std::cerr << "invalid argument: " << argv[i] << "\n";
        return 2;
    }

    registerYuiComponentTypes();

    Array<var> types;

    for (const auto* info : ComponentTypeRegistry::getInstance().getRegisteredTypes())
        types.add (describeYuiToolComponentType (*info, false));

    auto* schema = new DynamicObject();
    schema->setProperty ("version", schemaVersion);
    schema->setProperty ("types", var (types));
    schema->setProperty ("paletteRoles", var (ThemePalette::getRoleNames()));

    Array<var> namedColors;

    for (const auto& named : Colors::getNamedColors())
        namedColors.add (String (named.name));

    schema->setProperty ("namedColors", var (namedColors));

    const auto text = JSON::toString (var (schema)) + "\n";

    if (output == File())
    {
        std::cout << text.toStdString();
        return 0;
    }

    if (! output.replaceWithText (text, false, false, "\n"))
    {
        std::cerr << "can't write '" << output.getFullPathName() << "'\n";
        return 2;
    }

    return 0;
}
