"""Hole 1, The Terraces: a walled lawn on a limestone headland above the sea, under a broken halo.

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
DROP = 0
LENGTH = 6400

LIMESTONE = "/Game/Fab/Roman_Limestone_Wall/thzldg2qx_tier_1/Textures/T_thzldg2qx_4K_"
CLIFF = "/Game/Fab/Quarry_Cliff/uchoab3fa_tier_1/Textures/T_uchoab3fa_4K_"
LAWN = "/Game/Fab/Grass_Lawn/sfklehwa_tier_1/Textures/T_sfklehwa_4K_"
CLIFF_MESH = "/Game/Fab/Quarry_Cliff/uchoab3fa_tier_1/StaticMeshes/uchoab3fa_tier_1"
PILLARS = "/Game/Ancient_Pillars/Meshes/Joined/SM_Pillars_0"
STONES = "/Game/Fab/Ancient_Temple_Stones/tixjegefa_tier_1/StaticMeshes/tixjegefa_tier_1"
HEATHER = "/Game/Megaplant_Library/Shrub_Heather/Shrub_Heather_01/Shrub_Heather_01_"
ROSE = "/Game/Megaplant_Library/Shrub_Beach_Rose/Shrub_Beach_Rose_01/Shrub_Beach_Rose_01_"
PINES = "/Game/Megaplant_Library/Tree_Aleppo_Pine/Tree_Aleppo_Pine_01/Tree_Aleppo_Pine_01_"

rng = random.Random(7)


def scan(name, textures, tile, tint=(1, 1, 1, 1), roughness=1.0, stripes=(0.0, 0.0)):
    instance = c.material_instance("/Game/Materials/M_GG_Scan.M_GG_Scan", name,
                                   {"Tile": tile, "RoughnessScale": roughness, "StripeWidth": stripes[0],
                                    "StripeStrength": stripes[1]}, {"Tint": tint})
    mel = unreal.MaterialEditingLibrary
    for param, suffix in (("BaseTex", "B"), ("NormalTex", "N"), ("OrmTex", "ORM")):
        mel.set_material_instance_texture_parameter_value(instance, param, unreal.load_asset(textures + suffix))
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


def materials():
    glow = "/Game/Materials/M_GG_Glow.M_GG_Glow"
    return {
        "grass": scan("MI_Terraces_Grass", LAWN, 260, (0.8, 0.95, 0.7, 1)),
        # The lawn is most of what the player sees on the first hole. Mown in bold stripes and tinted cool, so the low
        # warm sun reads as light on green grass. (At 0.34 and a yellower tint it was one flat olive plane.)
        "green": scan("MI_Terraces_Green", LAWN, 170, (0.42, 0.95, 0.5, 1), stripes=(300.0, 0.6)),
        "stone": scan("MI_Terraces_Limestone", LIMESTONE, 320, (1.0, 0.95, 0.86, 1)),
        # The hedges wear the lawn's own scan, tiled small and tinted deep: clipped box, close-leaved. The fig-wall
        # scan they had is so dark that a hedge in shade was a black wall, however it was tinted.
        "hedge": scan("MI_Terraces_Hedge", LAWN, 110, (0.3, 0.72, 0.34, 1)),
        "rock": scan("MI_Terraces_Rock", CLIFF, 900, (1.35, 1.3, 1.2, 1)),
        "farRock": scan("MI_Terraces_FarRock", CLIFF, 5200, (1.5, 1.42, 1.3, 1)),
        "ocean": unreal.load_asset("/Game/Materials/M_GG_Ocean.M_GG_Ocean"),
        "glyph": c.material_instance(glow, "MI_Terraces_Glyph", {"Strength": 18.0}, {"Color": (0.25, 0.6, 1.0, 1)}),
    }


def hedge_row(box, shrubs, m):
    """A hedge is a soft barrier, so it should look like one: clipped and leafy, with heather at its foot."""
    cx, cy, cz = box["center"]
    sx, sy, sz = box["size"]
    c.box((cx, cy, cz), (sx, sy, sz), m["hedge"], box["name"])
    along_x = sx > sy
    # A clipped crown a little narrower than the body, and a limestone kerb at its foot: a hedge someone keeps, not a
    # green box.
    top = cz + sz / 2
    base = cz - sz / 2
    crown = (sx, sy - 34, 22) if along_x else (sx - 34, sy, 22)
    c.box((cx, cy, top + 11), crown, m["hedge"], box["name"] + "_Crown", folder="Garden/Hedges", solid=False)
    kerb = (sx, sy + 26, 12) if along_x else (sx + 26, sy, 12)
    c.box((cx, cy, base + 6), kerb, m["stone"], box["name"] + "_Kerb", folder="Garden/Hedges", solid=False)
    length = sx if along_x else sy
    inward = -1 if (cy if along_x else cx - LENGTH / 2) > 0 else 1
    count = int(length / 170)
    for i in range(count):
        if rng.random() < 0.3:
            continue
        t = (i + 0.5) / count - 0.5
        offset = inward * ((sy if along_x else sx) / 2 + rng.uniform(15, 45))
        x = cx + (t * length if along_x else offset)
        y = cy + (offset if along_x else t * length)
        yaw = rng.uniform(0, 360)
        asset = rng.choice(shrubs)
        size = rng.uniform(0.9, 1.5)
        # (No shadows from the heather on the hedges: 134 small shadows redrawn for nothing anyone notices.)
        c.plant(asset, (x, y, cz - sz / 2), yaw, size, "heather", cast_shadow=False)


def great_wall(box, m):
    """The wall that hides the cup: plinth, pilasters, a capping course and a line of glyphs."""
    cx, cy, cz = box["center"]
    sx, sy, sz = box["size"]
    base = cz - sz / 2
    c.box((cx, cy, cz), (sx, sy, sz), m["stone"], box["name"])
    c.box((cx, cy, base + 18), (sx + 36, sy + 36, 36), m["stone"], "Wall_Plinth", folder="Ruins/Wall")
    c.box((cx, cy, base + sz + 14), (sx + 30, sy + 30, 28), m["stone"], "Wall_Cap", folder="Ruins/Wall")
    c.box((cx, cy, base + sz + 38), (sx + 6, sy + 10, 20), m["stone"], "Wall_Cap2", folder="Ruins/Wall")
    for i, t in enumerate(range(int(-sy / 2) + 90, int(sy / 2), 330)):
        c.box((cx, cy + t, cz), (sx + 28, 62, sz), m["stone"], f"Wall_Pilaster{i}", folder="Ruins/Wall")
    for side in (-1, 1):
        c.box((cx + side * (sx / 2 + 1), cy, base + sz * 0.72), (3, sy - 120, 5), m["glyph"], f"Wall_Glyph{side}",
              folder="Ruins/Wall")
    # The free end is finished with a heavier pier.
    c.box((cx, cy + sy / 2 - 30, cz + 20), (sx + 60, 110, sz + 40), m["stone"], "Wall_Pier", folder="Ruins/Wall")
    c.box((cx, cy + sy / 2 - 30, base + sz + 62), (sx + 84, 134, 26), m["stone"], "Wall_PierCap", folder="Ruins/Wall")


def course_geometry(m):
    with open(COURSE) as f:
        course = json.load(f)
    heather = [unreal.load_asset(HEATHER + letter) for letter in "ABCDEF"]
    for b in course["boxes"]:
        if b["look"] == "hedge":
            hedge_row(b, heather, m)
        elif b["look"] == "wall":
            great_wall(b, m)
        elif b["shape"] == "cylinder":
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
                x = -600 + i * 1900 + rng.uniform(-300, 300)
                y = side * (2750 + row * 420 + rng.uniform(-120, 120))
                tall = 3.0 + rng.uniform(-0.3, 0.5)
                cliff((x, y, top - 508 * tall + rng.uniform(-150, 0)), (0 if side > 0 else 180) + rng.uniform(-12, 12),
                      (2.6, 3.2, tall))
    for row, top in enumerate((-700, -1900, -3000)):
        for i in range(4):
            y = -2000 + i * 1350 + rng.uniform(-200, 200)
            cliff((LENGTH + 500 + row * 380, y, top - 1626), 90 + rng.uniform(-10, 10), (2.4, 3.0, 3.2))
            cliff((-900 - row * 380, y, top + 500 - 1727), -90 + rng.uniform(-10, 10), (2.4, 3.0, 3.4))

    # Rocky shelves beside the lawn carry the pines.
    for side in (-1, 1):
        c.box((LENGTH / 2, side * 2050, -420), (LENGTH, 1300, 600), m["rock"], f"Shelf{'L' if side < 0 else 'R'}",
              folder="Landscape")
        c.box((LENGTH / 2, side * 2050, -135), (LENGTH, 1300, 30), m["grass"], f"ShelfTop{'L' if side < 0 else 'R'}",
              folder="Landscape")
    c.box((LENGTH / 2, 0, -2400), (LENGTH + 1000, 5600, 3000), m["rock"], "Cliff_Core", folder="Landscape")


def pines():
    trees = [unreal.load_asset(PINES + letter) for letter in "ABCD"]
    spots = []
    for side in (-1, 1):
        for _ in range(8):
            spots.append((rng.uniform(250, LENGTH - 250), side * rng.uniform(1700, 2500), -122))
    for i, spot in enumerate(spots):
        yaw = rng.uniform(0, 360)
        asset = rng.choice(trees)
        size = rng.uniform(0.8, 1.25)
        # The sun shines toward +Y, so only the pines on the -Y side throw shadows onto the lawn. The others' shadows
        # fall off the terrace where nobody sees them, and a swaying tree redraws its shadow every frame (2 ms for all).
        c.plant(asset, spot, yaw, size, "pine", cast_shadow=spot[1] < 0)


def halo(m, course):
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
        roll = 90 - math.degrees(angle)
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
        c.box((LENGTH + 500 + i * 1050, 1500 - i * 260, -200 + t * t * 1900), (620 - i * 25, 880 - i * 30, 90), m["stone"],
              f"FloatingSlab{i}", folder="Fantastical/Slabs", rotation=(rng.uniform(-6, 6), rng.uniform(-14, 14), 0))

    # A monolith hovers where the machine waits, lit from within.
    machine = course["attractors"][0]["position"]
    x, y = machine[0], machine[1]
    c.box((x, y, 640), (120, 120, 520), m["stone"], "Monolith", folder="Fantastical")
    c.box((x, y, 640), (126, 30, 480), m["glyph"], "MonolithGlyphA", folder="Fantastical")
    c.box((x, y, 640), (30, 126, 480), m["glyph"], "MonolithGlyphB", folder="Fantastical")
    c.dais((x, y), 240, m["stone"], m["glyph"], folder="Fantastical")


def floating_islands(m):
    """Pieces of the headland that gravity forgot: rocks hanging in the air with pines still growing on them."""
    mesh = unreal.load_asset(CLIFF_MESH)
    trees = [unreal.load_asset(PINES + letter) for letter in "BCD"]
    for i, (x, y, z, size) in enumerate([(5200, 2300, 620, 1.0), (2700, -2500, 980, 0.8), (7400, -900, 1500, 1.3)]):
        for part, (yaw, flip) in enumerate([(rng.uniform(0, 360), 0), (rng.uniform(0, 360), 180)]):
            actor = c.spawn(unreal.StaticMeshActor, (x, y, z - (0 if flip == 0 else -60)), (flip, yaw, 0), f"Island{i}_{part}",
                            "Fantastical/Islands")
            actor.static_mesh_component.set_static_mesh(mesh)
            actor.static_mesh_component.set_material(0, m["rock"])
            actor.set_actor_scale3d(unreal.Vector(0.42 * size, 1.5 * size, 0.55 * size))
        top = z + 250 * size
        c.mesh(c.CYLINDER, (x, y, top - 10), (520 * size, 520 * size, 40), material=m["grass"], label=f"Island{i}_Turf",
               folder="Fantastical/Islands")
        spot = (x + rng.uniform(-60, 60), y + rng.uniform(-60, 60), top)
        yaw = rng.uniform(0, 360)
        c.plant(rng.choice(trees), spot, yaw, 0.8 * size, "pine")


def shrubs():
    """Heather and beach rose soften every edge where stone meets grass."""
    heather = [unreal.load_asset(HEATHER + letter) for letter in "ABCDEF"]
    rose = [unreal.load_asset(ROSE + letter) for letter in "ABCD"]
    count = 0
    for side in (-1, 1):
        for _ in range(44):
            x = rng.uniform(80, LENGTH - 80)
            y = side * rng.uniform(1440, 2650)
            kind = rose if rng.random() < 0.35 else heather
            yaw = rng.uniform(0, 360)
            asset = rng.choice(kind)
            size = rng.uniform(0.9, 1.6)
            c.plant(asset, (x, y, -122), yaw, size, "rose" if kind is rose else "heather")
            count += 1


def rubble(m):
    """Fallen temple stones lie where the ruin shed them, off the line of play."""
    mesh = unreal.load_asset(STONES)
    spots = [(2620, -1050, 0), (2250, -900, 0), (2600, 620, 0), (5900, -1000, 0), (5800, 950, 0), (300, 1000, 0)]
    for i, spot in enumerate(spots):
        actor = c.spawn(unreal.StaticMeshActor, spot, (0, rng.uniform(0, 360), 0), f"Rubble{i}", "Ruins/Rubble")
        actor.static_mesh_component.set_static_mesh(mesh)
        size = rng.uniform(0.8, 1.4)
        actor.set_actor_scale3d(unreal.Vector(size, size, size))


def sea_and_horizon(m):
    # Sea stacks far out on the horizon, softened by haze.
    mesh = unreal.load_asset(CLIFF_MESH)
    for i, (x, y, size) in enumerate([(52000, -30000, 7), (70000, 26000, 10), (38000, 44000, 6), (90000, -6000, 12)]):
        for part in range(2):
            actor = c.spawn(unreal.StaticMeshActor, (x + rng.uniform(-900, 900) * part, y + rng.uniform(-900, 900) * part,
                            SEA_LEVEL - 800), (0, rng.uniform(0, 360), 0), f"SeaStack{i}_{part}", "Landscape/SeaStacks")
            actor.static_mesh_component.set_static_mesh(mesh)
            actor.static_mesh_component.set_material(0, m["farRock"])
            actor.set_actor_scale3d(unreal.Vector(size * 0.8, size * 1.6, size * rng.uniform(1.5, 2.4)))
    c.mesh(c.PLANE, (0, 0, SEA_LEVEL), (1200000, 1200000, 1), material=m["ocean"], label="Sea", folder="Landscape",
           cast_shadow=False)


def markers(course, m):
    tee = course["tee"]["position"]
    c.mesh(c.CYLINDER, (tee[0], tee[1], 0.5), (200, 200, 1), material=m["stone"], label="TeeStone", folder="Course/Markers",
           cast_shadow=False)
    cup = course["cup"]
    x, y, z = cup["position"]
    # The paved court where machines can't stand, with a glyph ring marking its edge.
    for zone in course["noMachine"]:
        zx, zy, _ = zone["position"]
        size = zone["radius"] * 2
        c.mesh(c.CYLINDER, (zx, zy, 0.3), (size, size, 0.6), material=m["stone"], label="Court", folder="Course/Markers",
               cast_shadow=False)
        c.mesh(c.CYLINDER, (zx, zy, 0.2), (size + 24, size + 24, 0.4), material=m["glyph"], label="CourtEdge",
               folder="Course/Markers", cast_shadow=False)
    black = c.material_instance("/Game/Materials/M_GG_Surface.M_GG_Surface", "MI_Cup", {"Roughness": 1.0},
                                {"BaseColor": (0.004, 0.004, 0.004, 1)})
    c.mesh(c.CYLINDER, (x, y, z + 0.9), (cup["radius"] * 2, cup["radius"] * 2, 1), material=black, label="Cup",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 120), (2.5, 2.5, 240), material=m["stone"], label="FlagPole", folder="Course/Markers")


c.fresh_level(LEVEL)
mats = materials()
c.sky_and_sun(sun_pitch=-19.0, sun_yaw=28.0, fog_density=0.03, sun_kelvin=4800.0, sky_intensity=1.2)
course_data = course_geometry(mats)
cliffs(mats)
pines()
shrubs()
rubble(mats)
halo(mats, course_data)
floating_islands(mats)
sea_and_horizon(mats)
markers(course_data, mats)
c.player_start(course_data)
c.write_plants(LEVEL, web_ambient=2.6, web_tint=(1.0, 0.97, 0.9))
c.LES.save_current_level()
