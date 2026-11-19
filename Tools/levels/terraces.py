"""Hole 1, The Terraces: limestone terraces stepping down to a sea at sunrise, under a broken halo.

Collision geometry comes from Content/Courses/terraces.json (written by Tools/courses/terraces.py), so the
scenery matches what the ball hits. Everything else here is dressing.

Run inside the editor (MCP execute_python):  runpy.run_path('<repo>/Tools/levels/terraces.py')
"""

import importlib
import json
import math
import random
import sys

import unreal

sys.path.insert(0, "/Users/ken/Code/gravitygolf/Tools/levels")
import common as c  # noqa: E402

importlib.reload(c)  # the editor's interpreter persists between runs

LEVEL = "/Game/Levels/Terraces"
COURSE = "/Users/ken/Code/gravitygolf/Content/Courses/terraces.json"
SEA_LEVEL = -3200
DROP = 320

LIMESTONE = "/Game/Fab/Roman_Limestone_Wall/thzldg2qx_tier_1/Textures/T_thzldg2qx_4K_"
CLIFF = "/Game/Fab/Quarry_Cliff/uchoab3fa_tier_1/Textures/T_uchoab3fa_4K_"
LAWN = "/Game/Fab/Grass_Lawn/sfklehwa_tier_1/Textures/T_sfklehwa_4K_"
CLIFF_MESH = "/Game/Fab/Quarry_Cliff/uchoab3fa_tier_1/StaticMeshes/uchoab3fa_tier_1"
PILLARS = "/Game/Ancient_Pillars/Meshes/Joined/SM_Pillars_0"
PINES = "/Game/Megaplant_Library/Tree_Aleppo_Pine/Tree_Aleppo_Pine_01/Tree_Aleppo_Pine_01_"

rng = random.Random(7)


def scan(name, textures, tile, tint=(1, 1, 1, 1), roughness=1.0):
    instance = c.material_instance("/Game/Materials/M_GG_Scan.M_GG_Scan", name, {"Tile": tile, "RoughnessScale": roughness},
                                   {"Tint": tint})
    mel = unreal.MaterialEditingLibrary
    for param, suffix in (("BaseTex", "B"), ("NormalTex", "N"), ("OrmTex", "ORM")):
        mel.set_material_instance_texture_parameter_value(instance, param, unreal.load_asset(textures + suffix))
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


def materials():
    glow = "/Game/Materials/M_GG_Glow.M_GG_Glow"
    return {
        "grass": scan("MI_Terraces_Grass", LAWN, 260, (0.8, 0.95, 0.7, 1)),
        "green": scan("MI_Terraces_Green", LAWN, 180, (0.62, 1.0, 0.6, 1)),
        "stone": scan("MI_Terraces_Limestone", LIMESTONE, 320, (1.0, 0.95, 0.86, 1)),
        "rock": scan("MI_Terraces_Rock", CLIFF, 900, (1.0, 0.94, 0.85, 1)),
        "ocean": unreal.load_asset("/Game/Materials/M_GG_Ocean.M_GG_Ocean"),
        "glyph": c.material_instance(glow, "MI_Terraces_Glyph", {"Strength": 18.0}, {"Color": (0.25, 0.6, 1.0, 1)}),
    }


def course_geometry(m):
    with open(COURSE) as f:
        course = json.load(f)
    for b in course["boxes"]:
        if b["shape"] == "cylinder":
            # Scanned pillars stand in for collision cylinders: tall ones intact, short ones broken.
            tall = b["size"][2] > 400
            mesh = PILLARS + ("3" if tall else "6")
            base = (b["center"][0], b["center"][1], b["center"][2] - b["size"][2] / 2)
            actor = c.spawn(unreal.StaticMeshActor, base, (0, rng.uniform(0, 360), 0), b["name"], "Ruins")
            actor.static_mesh_component.set_static_mesh(unreal.load_asset(mesh))
            scale = b["size"][2] / (636.0 if tall else 228.0)
            actor.set_actor_scale3d(unreal.Vector(0.75, 0.75, scale))
        else:
            c.box(b["center"], b["size"], m[b["look"]], b["name"])
    return course


def cliffs(m):
    """Scanned cliff faces wrap the terraces' flanks and the drop to the sea."""
    mesh = unreal.load_asset(CLIFF_MESH)
    nanite = mesh.get_editor_property("nanite_settings")
    if not nanite.enabled:
        nanite.enabled = True
        mesh.set_editor_property("nanite_settings", nanite)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)

    def cliff(location, yaw, scale, pitch=0.0, roll=0.0):
        actor = c.spawn(unreal.StaticMeshActor, location, (pitch, yaw, roll), "Cliff", "Landscape/Cliffs")
        actor.static_mesh_component.set_static_mesh(mesh)
        actor.static_mesh_component.set_material(0, m["rock"])
        actor.set_actor_scale3d(unreal.Vector(*scale))

    # The mesh is about 13 m long, 3 m deep and 5 m tall. Rows of large copies form the faces.
    for side in (-1, 1):
        for row, top in enumerate((-760, -1900, -3000)):
            for i in range(5):
                x = -600 + i * 2500 + rng.uniform(-300, 300)
                y = side * (2750 + row * 420 + rng.uniform(-120, 120))
                tall = 3.0 + rng.uniform(-0.3, 0.5)
                cliff((x, y, top - 508 * tall + rng.uniform(-150, 0)), (0 if side > 0 else 180) + rng.uniform(-12, 12),
                      (2.6, 3.2, tall))
    for row, top in enumerate((-700, -1900, -3000)):
        for i in range(4):
            y = -2000 + i * 1350 + rng.uniform(-200, 200)
            cliff((9500 + row * 380, y, top - 1626), 90 + rng.uniform(-10, 10), (2.4, 3.0, 3.2))
            cliff((-900 - row * 380, y, top + 500 - 1727), -90 + rng.uniform(-10, 10), (2.4, 3.0, 3.4))

    # Rocky shelves beside the terraces carry the pines.
    for index in range(3):
        top = -DROP * index
        x = 3000 * index + 1500
        for side in (-1, 1):
            c.box((x, side * 2050, top - 420), (3000, 1300, 600), m["rock"], f"Shelf{index}{'L' if side < 0 else 'R'}",
                  folder="Landscape")
            c.box((x, side * 2050, top - 135), (3000, 1300, 30), m["grass"], f"ShelfTop{index}{'L' if side < 0 else 'R'}",
                  folder="Landscape")
    c.box((4500, 0, -2400), (10400, 5600, 3000), m["rock"], "Cliff_Core", folder="Landscape")


def pines():
    trees = [unreal.load_asset(PINES + letter) for letter in "ABCD"]
    spots = []
    for index in range(3):
        for side in (-1, 1):
            for _ in range(3):
                spots.append((3000 * index + rng.uniform(250, 2750), side * rng.uniform(1700, 2500), -DROP * index - 122))
    for i, spot in enumerate(spots):
        actor = c.spawn(unreal.SkeletalMeshActor, spot, (0, rng.uniform(0, 360), 0), f"Pine{i}", "Landscape/Pines")
        actor.skeletal_mesh_component.set_skinned_asset_and_update(rng.choice(trees))
        size = rng.uniform(0.8, 1.25)
        actor.set_actor_scale3d(unreal.Vector(size, size, size))


def halo(m):
    """The fantastical: a colossal broken ring of stone standing in the sea, and slabs that ignore gravity."""
    center = (26000.0, -3000.0, 2600.0)
    radius = 9000.0
    segments = 72
    for i in range(segments):
        if i in (11, 12, 13, 14, 41, 42):  # the breaks
            continue
        angle = 2 * math.pi * i / segments
        y = center[1] + math.cos(angle) * radius
        z = center[2] + math.sin(angle) * radius
        length = 2 * math.pi * radius / segments * 1.03
        roll = math.degrees(angle) + 90
        c.box((center[0], y, z), (900, length, 700), m["stone"], f"Halo{i}", folder="Fantastical/Halo",
              rotation=(0, 0, roll))
        c.box((center[0] - 460, y, z), (30, length * 0.9, 60), m["glyph"], f"HaloGlyph{i}", folder="Fantastical/Halo",
              rotation=(0, 0, roll))

    # Broken pieces of the ring hang where they fell out of it.
    for i, (dy, dz, yaw) in enumerate([(7400, 6900, 20), (8600, 6000, -35), (9100, 4600, 60), (6600, 7900, 10)]):
        c.box((center[0] + rng.uniform(-600, 600), center[1] + dy, center[2] + dz), (800, 1100, 650), m["stone"],
              f"HaloShard{i}", folder="Fantastical/Halo", rotation=(rng.uniform(-25, 25), yaw, rng.uniform(-30, 30)))

    # A stair of floating slabs climbs from the cliff edge toward the ring.
    for i in range(9):
        t = i / 8.0
        c.box((9900 + i * 1050, 1500 - i * 260, -520 + t * t * 1700), (620 - i * 25, 880 - i * 30, 90), m["stone"],
              f"FloatingSlab{i}", folder="Fantastical/Slabs", rotation=(rng.uniform(-6, 6), rng.uniform(-14, 14), 0))

    # A monolith hovers over the first terrace, lit from within, marking where the attractor waits.
    c.box((2300, 700, 620), (120, 120, 520), m["stone"], "Monolith", folder="Fantastical")
    c.box((2300, 700, 620), (126, 30, 480), m["glyph"], "MonolithGlyphA", folder="Fantastical")
    c.box((2300, 700, 620), (30, 126, 480), m["glyph"], "MonolithGlyphB", folder="Fantastical")
    c.mesh(c.CYLINDER, (2300, 700, 4), (300, 300, 8), material=m["stone"], label="AttractorDais", folder="Fantastical")
    c.mesh(c.CYLINDER, (2300, 700, 9), (240, 240, 2), material=m["glyph"], label="AttractorDaisGlyph", folder="Fantastical")


def sea_and_horizon(m):
    for i, (x, y, size) in enumerate([(52000, -42000, 9000), (90000, 30000, 16000), (38000, 52000, 7000)]):
        c.mesh(c.SPHERE, (x, y, SEA_LEVEL - size * 0.25), (size * 2.4, size * 1.7, size), material=m["rock"],
               label=f"Headland{i}", folder="Landscape")
    c.mesh(c.PLANE, (0, 0, SEA_LEVEL), (600000, 600000, 1), material=m["ocean"], label="Sea", folder="Landscape",
           cast_shadow=False)


def markers(course, m):
    tee = course["tee"]["position"]
    c.box((tee[0], tee[1], 1.5), (260, 260, 3), m["stone"], "TeeBox", folder="Course/Markers")
    cup = course["cup"]["position"]
    c.mesh(c.CYLINDER, (cup[0], cup[1], cup[2] + 0.6), (36, 36, 1), material=c.material_instance(
        "/Game/Materials/M_GG_Surface.M_GG_Surface", "MI_Cup", {"Roughness": 1.0}, {"BaseColor": (0.004, 0.004, 0.004, 1)}),
        label="Cup", folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (cup[0], cup[1], cup[2] + 120), (2.5, 2.5, 240), material=m["stone"], label="FlagPole",
           folder="Course/Markers")


c.fresh_level(LEVEL)
mats = materials()
c.sky_and_sun(sun_pitch=-22.0, sun_yaw=112.0, fog_density=0.012)
course_data = course_geometry(mats)
cliffs(mats)
pines()
halo(mats)
sea_and_horizon(mats)
markers(course_data, mats)
c.LES.save_current_level()
