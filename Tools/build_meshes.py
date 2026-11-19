"""Generates the game's own static meshes under /Game/Meshes with Geometry Script. Run in the editor:
Tools/py.sh Tools/build_meshes.py   (the meshes are build output; this script is the source).

The putter is built in the club pivot's space: X runs from the butt of the grip down the shaft, Y is to the
player's right, Z is up. The game attaches each part to the pivot with no offset.
"""

import math

import unreal

P = unreal.GeometryScript_Primitives
CENTER = unreal.GeometryScriptPrimitiveOriginMode.CENTER
BASE = unreal.GeometryScriptPrimitiveOriginMode.BASE
ALONG_X = unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0)  # turns a primitive's Z axis onto +X
DEFAULT_MATERIAL = unreal.load_asset("/Game/Materials/M_GG_Surface")
NOTES = []


def options(material=0):
    o = unreal.GeometryScriptPrimitiveOptions()
    o.material_id = material
    return o


def at(x, y, z, rotation=None):
    return unreal.Transform(location=unreal.Vector(x, y, z), rotation=rotation or unreal.Rotator(0, 0, 0))


def beveled_box(target, lo, hi, bevel, material=0):
    """An axis-aligned box from corner lo to corner hi with its edges rounded off, appended to target."""
    piece = unreal.DynamicMesh()
    size = [hi[i] - lo[i] for i in range(3)]
    P.append_box(piece, options(material), at(*[(lo[i] + hi[i]) / 2 for i in range(3)]), size[0], size[1], size[2],
                 0, 0, 0, CENTER)
    if bevel > 0:
        b = unreal.GeometryScriptMeshBevelOptions()
        b.bevel_distance = bevel
        b.subdivisions = 2
        unreal.GeometryScript_MeshModeling.apply_mesh_polygroup_bevel(piece, b)
    unreal.GeometryScript_MeshEdits.append_mesh(target, piece, unreal.Transform())
    return target


def taper(target, x0, x1, r0, r1, sides=28, material=0, y=0.0, z=0.0):
    """A round bar along X from x0 (radius r0) to x1 (radius r1)."""
    P.append_cone(target, options(material), at(x0, y, z, ALONG_X), r0, r1, x1 - x0, sides, 1, True, BASE)


def bake(mesh, name, slots):
    """Write a dynamic mesh to /Game/Meshes/<name> with a number of material slots."""
    split = unreal.GeometryScriptSplitNormalsOptions()
    split.opening_angle_deg = 35.0
    unreal.GeometryScript_Normals.compute_split_normals(mesh, split, unreal.GeometryScriptCalculateNormalsOptions())
    path = f"/Game/Meshes/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
    else:
        unreal.EditorAssetLibrary.make_directory("/Game/Meshes")
        asset, _ = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(
            mesh, path, unreal.GeometryScriptCreateNewStaticMeshAssetOptions())
    copy = unreal.GeometryScriptCopyMeshToAssetOptions()
    copy.replace_materials = True
    copy.new_materials = [DEFAULT_MATERIAL] * slots
    copy.enable_recompute_normals = False
    copy.enable_recompute_tangents = True
    unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(mesh, asset, copy, unreal.GeometryScriptMeshWriteLOD(), False)
    unreal.EditorAssetLibrary.save_asset(path)
    box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(mesh)
    return f"{name}: {box.min.x:.1f}..{box.max.x:.1f}, {box.min.y:.1f}..{box.max.y:.1f}, {box.min.z:.1f}..{box.max.z:.1f}"


def putter():
    report = []

    # Grip: a tapered rubber handle with a rounded butt and a ferrule where the steel begins.
    grip = unreal.DynamicMesh()
    taper(grip, -0.5, 0.0, 1.15, 1.45)
    taper(grip, 0.0, 26.0, 1.45, 1.08)
    taper(grip, 26.0, 26.8, 1.08, 0.7)
    report.append(bake(grip, "SM_Putter_Grip", 1))

    # Shaft: tapered steel, then a short neck that turns down into the head.
    shaft = unreal.DynamicMesh()
    taper(shaft, 24.0, 71.6, 0.62, 0.45)
    P.append_sphere_lat_long(shaft, options(), at(71.6, 0, 0), 0.47, 12, 20, CENTER)
    P.append_cylinder(shaft, options(), at(71.6, 0, -2.1), 0.47, 2.1, 20, 0, True, BASE)
    report.append(bake(shaft, "SM_Putter_Shaft", 1))

    # Head: a blade with a low flange behind the face, weights at heel and toe, a sight line and a face insert.
    # Slot 0 steel, slot 1 the white sight line, slot 2 bronze (the weights and the face insert).
    head = unreal.DynamicMesh()
    beveled_box(head, (71.9, -7.5, -4.5), (73.0, 4.0, -1.9), 0.14)   # face bar
    beveled_box(head, (69.3, -6.9, -4.5), (72.0, 3.4, -3.5), 0.10)   # flange
    beveled_box(head, (69.3, -7.5, -4.5), (72.0, -5.9, -2.5), 0.16, material=2)  # toe weight, bronze
    beveled_box(head, (69.3, 2.4, -4.5), (72.0, 4.0, -2.5), 0.16, material=2)    # heel weight, bronze
    beveled_box(head, (69.6, -1.86, -3.53), (71.9, -1.64, -3.45), 0.0, material=1)  # sight line on the flange
    beveled_box(head, (72.0, -1.86, -1.93), (72.9, -1.64, -1.86), 0.0, material=1)  # and on the top edge
    beveled_box(head, (72.94, -5.4, -4.15), (73.05, 1.9, -2.25), 0.0, material=2)   # face insert
    report.append(bake(head, "SM_Putter_Head", 3))
    return report


def bar(target, a, b, r0, r1, sides=16, material=0):
    """A round bar from point a (radius r0) to point b (radius r1), in any direction."""
    axis = unreal.Vector(b[0] - a[0], b[1] - a[1], b[2] - a[2])
    P.append_cone(target, options(material), at(a[0], a[1], a[2], unreal.MathLibrary.make_rot_from_z(axis)), r0, r1,
                  axis.length(), sides, 1, True, BASE)


def around(radius, degrees, z):
    return (radius * math.cos(math.radians(degrees)), radius * math.sin(math.radians(degrees)), z)


def machine():
    """The gravity machine: a turned bronze drum on three iron legs, a cradle of claws and two rings around the
    place where the core floats (0, 0, 45). Slot 0 iron, slot 1 bronze, slot 2 light (coloured per machine)."""
    IRON, BRONZE, LIGHT = 0, 1, 2
    core = 45.0
    mesh = unreal.DynamicMesh()

    for leg in range(3):
        yaw = 120.0 * leg + 30.0
        foot, knee, top = around(34.0, yaw, 1.6), around(22.0, yaw, 10.4), around(7.5, yaw, 24.0)
        bar(mesh, foot, top, 1.05, 1.75, material=IRON)
        P.append_cylinder(mesh, options(BRONZE), at(foot[0], foot[1], 0.0), 3.8, 1.1, 24, 0, True, BASE)   # foot pad
        P.append_sphere_lat_long(mesh, options(BRONZE), at(*foot), 1.9, 10, 16, CENTER)                    # ankle
        collar_a, collar_b = around(23.2, yaw, 9.5), around(20.8, yaw, 11.3)
        bar(mesh, collar_a, collar_b, 1.9, 1.9, material=BRONZE)                                           # knee collar

    # The drum, turned on a lathe: a chamfered base, a waist groove, a lip, and a recess that holds the ring of light.
    profile = [(0.0, 21.5), (8.2, 21.5), (10.0, 23.2), (10.0, 26.6), (9.1, 27.2), (9.1, 28.0), (10.8, 28.8),
               (10.8, 30.8), (9.9, 31.6), (8.0, 31.6), (8.0, 30.6), (0.0, 30.6)]
    drum = unreal.DynamicMesh()
    P.append_revolve_polygon(drum, options(), unreal.Transform(), [unreal.Vector2D(r, z) for r, z in profile],
                             unreal.GeometryScriptRevolveOptions(), 0.0, 48)
    _, volume = unreal.GeometryScript_MeshQueries.get_mesh_volume_area(drum)
    if volume < 0:  # the lathe came out inside-out
        unreal.GeometryScript_Normals.flip_normals(drum)
    # The lathe ignores the material in its options, so set it afterwards.
    unreal.GeometryScript_Materials.remap_material_i_ds(drum, 0, BRONZE)
    unreal.GeometryScript_MeshEdits.append_mesh(mesh, drum, unreal.Transform())
    NOTES.append(f"drum volume {volume:.0f}")
    P.append_cylinder(mesh, options(IRON), at(0, 0, 30.6), 5.2, 1.6, 32, 0, True, BASE)                    # emitter cap
    P.append_torus(mesh, options(LIGHT), at(0, 0, 31.0), unreal.GeometryScriptRevolveOptions(), 6.9, 0.75, 48, 10, CENTER)

    # Three claws cradle the core from below.
    for claw in range(3):
        yaw = 120.0 * claw + 90.0
        a, b, c3 = around(9.6, yaw, 31.4), around(13.2, yaw, 37.5), around(11.2, yaw, 44.0)
        bar(mesh, a, b, 1.0, 0.8, material=BRONZE)
        bar(mesh, b, c3, 0.8, 0.5, material=BRONZE)
        P.append_sphere_lat_long(mesh, options(BRONZE), at(*b), 0.95, 8, 12, CENTER)
        P.append_sphere_lat_long(mesh, options(BRONZE), at(*c3), 0.7, 8, 12, CENTER)

    # Two thin rings around the core, tilted like an armillary sphere.
    P.append_torus(mesh, options(IRON), at(0, 0, core, unreal.Rotator(roll=72.0, pitch=0.0, yaw=20.0)),
                   unreal.GeometryScriptRevolveOptions(), 9.6, 0.3, 64, 8, CENTER)
    P.append_torus(mesh, options(BRONZE), at(0, 0, core, unreal.Rotator(roll=18.0, pitch=14.0, yaw=0.0)),
                   unreal.GeometryScriptRevolveOptions(), 10.4, 0.36, 64, 8, CENTER)
    return [bake(mesh, "SM_Machine", 3)]


def blob(target, center, radii, material):
    """An ellipsoid: a ball of leaves."""
    transform = unreal.Transform(location=unreal.Vector(*center), rotation=unreal.Rotator(0, 0, 0),
                                 scale=unreal.Vector(radii[0] / 50.0, radii[1] / 50.0, radii[2] / 50.0))
    P.append_sphere_lat_long(target, options(material), transform, 50.0, 9, 14, CENTER)


def web_plants():
    """Stand-ins for the scanned plants, for renderers that can't draw them (the web, phones). Each is built in a
    box 200 across and 200 tall with its foot at the origin; the game stretches it to the real plant's size.
    Slot 0 bark, slot 1 leaves."""
    report = []

    # An umbrella pine: a leaning trunk, two boughs, and a wide flat crown of five clumps.
    pine = unreal.DynamicMesh()
    bar(pine, (0, 0, 0), (8, 4, 98), 9.0, 5.0, sides=10, material=0)
    bar(pine, (8, 4, 96), (34, -14, 140), 4.5, 2.5, sides=8, material=0)
    bar(pine, (8, 4, 96), (-28, 18, 144), 4.5, 2.5, sides=8, material=0)
    for center, radii in [((2, 0, 164), (78, 74, 32)), ((42, -26, 146), (54, 48, 26)), ((-44, 30, 152), (52, 50, 25)),
                          ((12, 46, 174), (46, 42, 22)), ((-16, -46, 170), (44, 46, 21))]:
        blob(pine, center, radii, 1)
    report.append(bake(pine, "SM_Web_Pine", 2))

    # A shrub: a low mound of clumps on a few stems.
    shrub = unreal.DynamicMesh()
    for tip in [(30, 10, 90), (-35, 20, 80), (5, -40, 85)]:
        bar(shrub, (0, 0, 0), tip, 5.0, 2.5, sides=6, material=0)
    for center, radii in [((0, 0, 118), (72, 70, 62)), ((48, 18, 92), (50, 46, 46)), ((-52, 24, 86), (46, 48, 44)),
                          ((8, -54, 90), (48, 44, 46)), ((-20, 40, 140), (40, 40, 40))]:
        blob(shrub, center, radii, 1)
    report.append(bake(shrub, "SM_Web_Shrub", 2))
    return report


def lathe(target, profile, material, steps=40):
    """Turn a profile of (radius, height) points round the vertical axis and add it to target."""
    piece = unreal.DynamicMesh()
    P.append_revolve_polygon(piece, options(), unreal.Transform(), [unreal.Vector2D(r, z) for r, z in profile],
                             unreal.GeometryScriptRevolveOptions(), 0.0, steps)
    _, volume = unreal.GeometryScript_MeshQueries.get_mesh_volume_area(piece)
    if volume < 0:
        unreal.GeometryScript_Normals.flip_normals(piece)
    if material:
        unreal.GeometryScript_Materials.remap_material_i_ds(piece, 0, material)
    unreal.GeometryScript_MeshEdits.append_mesh(target, piece, unreal.Transform())


def avatar():
    """What other players look like: a caddie that hovers. A turned ivory body that tapers to a ring of light instead
    of legs, bronze shoulders and collar, and a separate head with a visor of light, so it can look where its player
    looks. Built with its feet (the air under it) at the origin, facing +X. Slot 0 shell, slot 1 bronze, slot 2 light
    (the player's colour)."""
    SHELL, BRONZE, LIGHT = 0, 1, 2
    report = []

    body = unreal.DynamicMesh()
    # The hull: narrow at the hover ring, swelling to the chest, closing at the neck.
    # A flared hem, a pinched waist and a broad chest, so it reads as a figure and not a skittle.
    lathe(body, [(0.0, 38), (15, 38), (18.5, 41), (17, 47), (13, 62), (11.5, 82), (13.5, 96), (20, 114), (23.5, 126), (23, 136),
                 (18, 145), (9, 150), (0.0, 150)], SHELL)
    # A pack on its back with a strip of light, so players can be told apart from behind.
    beveled_box(body, (-27.0, -9.0, 104.0), (-17.0, 9.0, 138.0), 2.0, material=BRONZE)
    beveled_box(body, (-27.6, -2.2, 109.0), (-26.6, 2.2, 133.0), 0.0, material=LIGHT)
    # Collar and belt in bronze.
    lathe(body, [(7.5, 148), (11, 148), (11.5, 154), (8, 156), (7.5, 156)], BRONZE)
    lathe(body, [(11.0, 80), (12.6, 80), (13.2, 85), (12.2, 88), (11.0, 88)], BRONZE)
    # The hover ring under it, and a faint second one.
    P.append_torus(body, options(LIGHT), at(0, 0, 30), unreal.GeometryScriptRevolveOptions(), 12.0, 1.6, 40, 10, CENTER)
    P.append_torus(body, options(LIGHT), at(0, 0, 20), unreal.GeometryScriptRevolveOptions(), 7.0, 0.9, 32, 8, CENTER)
    # Shoulders, and arms that reach forward and down to where a putter is held.
    for side in (-1, 1):
        shoulder = (0, side * 26, 130)
        elbow = (12, side * 28, 106)
        hand = (30, side * 12, 92)
        P.append_sphere_lat_long(body, options(BRONZE), at(*shoulder), 6.5, 10, 16, CENTER)
        bar(body, shoulder, elbow, 4.2, 3.4, sides=12, material=SHELL)
        P.append_sphere_lat_long(body, options(BRONZE), at(*elbow), 3.9, 8, 12, CENTER)
        bar(body, elbow, hand, 3.4, 2.6, sides=12, material=SHELL)
        P.append_sphere_lat_long(body, options(BRONZE), at(*hand), 3.4, 8, 12, CENTER)
    report.append(bake(body, "SM_Avatar_Body", 3))

    # The head, built round its own pivot at the neck so it can nod: a smooth ivory shape with a band of light.
    head = unreal.DynamicMesh()
    transform = unreal.Transform(location=unreal.Vector(2, 0, 15), rotation=unreal.Rotator(0, 0, 0), scale=unreal.Vector(0.30, 0.26, 0.31))
    P.append_sphere_lat_long(head, options(SHELL), transform, 50.0, 16, 24, CENTER)
    lathe(head, [(4.5, 0), (6.5, 0), (6.5, 3), (4.5, 3)], BRONZE, steps=24)
    # The visor: a slab of light across the front of the face.
    beveled_box(head, (10.5, -10.5, 12.5), (17.4, 10.5, 20.0), 1.2, material=LIGHT)
    beveled_box(head, (-9.0, -1.4, 27.5), (9.0, 1.4, 31.5), 0.6, material=BRONZE)
    report.append(bake(head, "SM_Avatar_Head", 3))
    return report


"\n".join(putter() + machine() + web_plants() + avatar() + NOTES)
