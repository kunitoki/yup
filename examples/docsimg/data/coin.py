"""Builds the coin of the documentation figures: a coin engraved with the YUP logo.

Writes coin.glb, the model the 3D figures show, and coin.png, a picture of it for the
figures drawing images.

    blender --background --python examples/docsimg/data/coin.py
"""

import math
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

folder = Path(__file__).resolve().parent
logo_file = folder.parents[2] / "logos" / "yup_black.svg"

radius, thickness = 1.0, 0.16
rim_width, rim_slope, recess, engraving = 0.07, 0.02, 0.025, 0.03
logo_size = 1.1


def make_material(name, color, metallic, roughness):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    shader = material.node_tree.nodes["Principled BSDF"]
    shader.inputs["Base Color"].default_value = (*color, 1.0)
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    return material


def make_coin(material):
    bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=radius, depth=thickness)
    coin = bpy.context.object
    coin.name = "Coin"
    coin.data.materials.append(material)

    mesh = bmesh.new()
    mesh.from_mesh(coin.data)

    # A flat rim around each face, sloping down to the field the logo is engraved in
    for cap in [face for face in mesh.faces if len(face.verts) > 4]:
        normal = cap.normal.copy()
        bmesh.ops.inset_region(mesh, faces=[cap], thickness=rim_width)
        bmesh.ops.inset_region(mesh, faces=[cap], thickness=rim_slope)
        bmesh.ops.translate(mesh, verts=cap.verts, vec=-normal * recess)

    def is_outer(vertex):
        return abs(vertex.co.z) > thickness / 2 - 1e-5 and vertex.co.xy.length > radius - 1e-5

    outer_edges = [edge for edge in mesh.edges if all(is_outer(vertex) for vertex in edge.verts)]
    bmesh.ops.bevel(mesh, geom=outer_edges, offset=0.015, segments=3, affect="EDGES", profile=0.5)

    mesh.to_mesh(coin.data)
    mesh.free()
    return coin


def import_logo():
    """Returns the paths of the logo as flat meshes, fitted to the field of the coin."""
    bpy.ops.import_curve.svg(filepath=str(logo_file))
    shapes = [shape for shape in bpy.data.objects if shape.type == "CURVE"]

    bpy.ops.object.select_all(action="DESELECT")
    for shape in shapes:
        shape.data.dimensions = "2D"
        shape.data.fill_mode = "BOTH"
        shape.data.resolution_u = 6
        shape.select_set(True)

    bpy.context.view_layer.objects.active = shapes[0]
    bpy.ops.object.convert(target="MESH")

    corners = [shape.matrix_world @ Vector(corner) for shape in shapes for corner in shape.bound_box]
    low = Vector([min(corner[axis] for corner in corners) for axis in range(3)])
    high = Vector([max(corner[axis] for corner in corners) for axis in range(3)])
    fit = Matrix.Scale(logo_size / max(high.x - low.x, high.y - low.y), 4) @ Matrix.Translation(-(low + high) / 2)

    for shape in shapes:
        shape.data.transform(fit @ shape.matrix_world)
        shape.matrix_world = Matrix.Identity(4)

    return shapes


def engrave(coin, shape, material, flipped):
    """Cuts a flat shape into one face of the coin, lining the cut with a material."""
    floor = thickness / 2 - recess - engraving

    mesh = bmesh.new()
    mesh.from_mesh(shape.data)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=1e-5)
    bmesh.ops.translate(mesh, verts=mesh.verts, vec=(0, 0, floor))
    walls = bmesh.ops.extrude_face_region(mesh, geom=mesh.faces[:])
    top = [element for element in walls["geom"] if isinstance(element, bmesh.types.BMVert)]
    bmesh.ops.translate(mesh, verts=top, vec=(0, 0, thickness))
    bmesh.ops.recalc_face_normals(mesh, faces=mesh.faces[:])

    cutter_mesh = bpy.data.meshes.new("Cutter")
    mesh.to_mesh(cutter_mesh)
    mesh.free()

    # Turned around the vertical axis, so the logo reads the right way on the back too
    if flipped:
        cutter_mesh.transform(Matrix.Rotation(math.pi, 4, "Y"))

    cutter_mesh.materials.append(material)
    cutter = bpy.data.objects.new("Cutter", cutter_mesh)
    bpy.context.collection.objects.link(cutter)

    modifier = coin.modifiers.new("Engrave", "BOOLEAN")
    modifier.operation = "DIFFERENCE"
    modifier.solver = "EXACT"
    modifier.material_mode = "TRANSFER"
    modifier.object = cutter

    bpy.context.view_layer.objects.active = coin
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.data.objects.remove(cutter)


def render(coin, file):
    """Renders a picture of the coin on a transparent background, in a studio fading from light to dark."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = scene.render.resolution_y = 256
    scene.render.film_transparent = True
    scene.render.filepath = str(file)
    scene.view_settings.view_transform = "Standard"

    scene.world = bpy.data.worlds.new("Studio")
    scene.world.use_nodes = True
    nodes, links = scene.world.node_tree.nodes, scene.world.node_tree.links

    height = nodes.new("ShaderNodeSeparateXYZ")
    unit = nodes.new("ShaderNodeMapRange")
    unit.inputs["From Min"].default_value = -1.0
    fade = nodes.new("ShaderNodeValToRGB")
    fade.color_ramp.elements[0].position = 0.3
    fade.color_ramp.elements[0].color = (0.06, 0.045, 0.03, 1.0)
    fade.color_ramp.elements[1].position = 0.8
    fade.color_ramp.elements[1].color = (1.0, 0.96, 0.9, 1.0)

    links.new(nodes.new("ShaderNodeTexCoord").outputs["Generated"], height.inputs["Vector"])
    links.new(height.outputs["Z"], unit.inputs["Value"])
    links.new(unit.outputs["Result"], fade.inputs["Fac"])
    links.new(fade.outputs["Color"], nodes["Background"].inputs["Color"])

    def add(name, data, location):
        placed = bpy.data.objects.new(name, data)
        placed.location = location
        placed.rotation_euler = (-placed.location).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(placed)
        return placed

    sun = add("Sun", bpy.data.lights.new("Sun", "SUN"), (-3.0, -4.0, 5.0))
    sun.data.energy = 3.0

    scene.camera = add("Camera", bpy.data.cameras.new("Camera"), (1.9, -6.2, 1.55))
    scene.camera.data.lens = 85

    bpy.ops.render.render(write_still=True)

    bpy.data.objects.remove(sun)
    bpy.data.objects.remove(scene.camera)


def finish(coin):
    """Shades the coin smooth but for its creases, and stands it up facing the default camera."""
    mesh = bmesh.new()
    mesh.from_mesh(coin.data)

    for face in mesh.faces:
        face.smooth = True
    for edge in mesh.edges:
        edge.smooth = len(edge.link_faces) == 2 and edge.calc_face_angle() < math.radians(35)

    mesh.to_mesh(coin.data)
    mesh.free()

    coin.data.transform(Matrix.Rotation(math.radians(90), 4, "X"))


bpy.ops.wm.read_factory_settings(use_empty=True)

gold = make_material("Gold", (1.0, 0.77, 0.34), 1.0, 0.28)
patina = make_material("Patina", (0.42, 0.29, 0.1), 1.0, 0.55)
enamel = make_material("Enamel", (0.0, 0.35, 1.0), 0.0, 0.25)

coin = make_coin(gold)
shapes = import_logo()
dot = min(shapes, key=lambda shape: shape.dimensions.length)

for shape in shapes:
    for flipped in (False, True):
        engrave(coin, shape, enamel if shape is dot else patina, flipped)
    bpy.data.objects.remove(shape)

finish(coin)
render(coin, folder / "coin.png")

bpy.ops.object.select_all(action="DESELECT")
coin.select_set(True)
bpy.ops.export_scene.gltf(
    filepath=str(folder / "coin.glb"),
    export_format="GLB",
    use_selection=True,
    export_texcoords=False,
    export_animations=False,
    export_yup=True,
)
