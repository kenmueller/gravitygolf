"""Hole 4, Lantern Court: a marble courtyard at night, lit by thousands of lanterns that have forgotten to fall.

Collision geometry comes from Content/Courses/lantern.json (written by Tools/courses/lantern.py).

Run inside the editor:  Tools/regen.sh lantern
"""

import importlib
import json
import math
import random
import sys

import unreal

sys.path.insert(0, "/Users/ken/Code/gravitygolf/Tools/levels")
import common as c  # noqa: E402

importlib.reload(c)

LEVEL = "/Game/Levels/Lantern"
COURSE = "/Users/ken/Code/gravitygolf/Content/Courses/lantern.json"
SURFACE = "/Game/Materials/M_GG_Surface.M_GG_Surface"
GLOW = "/Game/Materials/M_GG_Glow.M_GG_Glow"
PINES = "/Game/Megaplant_Library/Tree_Aleppo_Pine/Tree_Aleppo_Pine_01/Tree_Aleppo_Pine_01_"
WARM = (1.0, 0.55, 0.2)
rng = random.Random(41)


def materials():
    return {
        "paving": c.scan_material("MI_Lantern_Paving", "Beige_Marble_Tiles", 300, (0.55, 0.55, 0.6, 1), roughness=0.55),
        "wall": c.scan_material("MI_Lantern_Wall", "Roman_Limestone_Wall", 320, (0.8, 0.8, 0.85, 1)),
        "hedge": c.scan_material("MI_Lantern_Hedge", "Creeping_Fig_Wall", 220, (0.6, 0.8, 0.55, 1)),
        "pad": c.scan_material("MI_Lantern_Pad", "Red_Desert_Sand", 120, (0.35, 0.3, 0.3, 1)),
        "glyph": c.material_instance(GLOW, "MI_Lantern_Glyph", {"Strength": 10.0}, {"Color": (WARM[0], WARM[1], WARM[2], 1)}),
        "lantern": c.material_instance(GLOW, "MI_Lantern_Paper", {"Strength": 30.0}, {"Color": (1.0, 0.6, 0.25, 1)}),
        "farLantern": c.material_instance(GLOW, "MI_Lantern_Far", {"Strength": 60.0}, {"Color": (1.0, 0.62, 0.28, 1)}),
        "moon": c.material_instance(GLOW, "MI_Lantern_Moon", {"Strength": 2.2}, {"Color": (0.8, 0.88, 1.0, 1)}),
        "bronze": c.material_instance(SURFACE, "MI_Lantern_Bronze", {"Roughness": 0.4, "Metallic": 1.0},
                                      {"BaseColor": (0.25, 0.15, 0.06, 1)}),
        "black": c.material_instance(SURFACE, "MI_Cup", {"Roughness": 1.0}, {"BaseColor": (0.004, 0.004, 0.004, 1)}),
    }


def dressed_wall(box, m):
    """Limestone walls with a plinth, a cap and a seam of light, like the wall on hole 1 but lower."""
    cx, cy, cz = box["center"]
    sx, sy, sz = box["size"]
    base = cz - sz / 2
    c.box((cx, cy, cz), (sx, sy, sz), m["wall"], box["name"])
    c.box((cx, cy, base + 14), (sx + 24, sy + 24, 28), m["wall"], box["name"] + "_Plinth", folder="Ruins/Walls")
    c.box((cx, cy, base + sz + 10), (sx + 20, sy + 20, 20), m["wall"], box["name"] + "_Cap", folder="Ruins/Walls")
    along_x = sx >= sy
    for side in (-1, 1):
        if along_x:
            c.box((cx, cy + side * (sy / 2 + 1), base + sz * 0.7), (sx - 60, 2, 4), m["glyph"], box["name"] + "_Seam",
                  folder="Ruins/Walls")
        else:
            c.box((cx + side * (sx / 2 + 1), cy, base + sz * 0.7), (2, sy - 60, 4), m["glyph"], box["name"] + "_Seam",
                  folder="Ruins/Walls")


def lantern(location, size, m, lit=None, far=False):
    """A paper lantern: a glowing body with a dark cap and foot. A few carry real light."""
    x, y, z = location
    c.box((x, y, z), (size, size, size * 1.35), m["farLantern" if far else "lantern"], "Lantern", folder="Fantastical/Lanterns",
          solid=False)
    if not far:
        c.box((x, y, z + size * 0.72), (size * 1.12, size * 1.12, size * 0.1), m["bronze"], "LanternCap",
              folder="Fantastical/Lanterns", solid=False)
        c.box((x, y, z - size * 0.72), (size * 0.8, size * 0.8, size * 0.1), m["bronze"], "LanternFoot",
              folder="Fantastical/Lanterns", solid=False)
    if lit:
        c.point_light((x, y, z), WARM, lit[0], lit[1], "LanternLight")


def lanterns(course, m):
    """The fantastical: lanterns hang in the air over the court, low ones lighting the way, a column of them
    rising from the cup's court into the sky."""
    cup = course["cup"]["position"]
    # Low lanterns that light the play: along the walls and over the key places.
    key = [(course["tee"]["position"][0], course["tee"]["position"][1] + 260, 300),
           (cup[0], cup[1], 430), (-1300, 350, 330), (-2300, 0, 330), (660, 1300, 360), (1500, 1200, 340),
           (2000, -300, 320), (-600, -500, 320), (3300, -300, 320), (-1900, 1200, 340), (300, 700, 320)]
    for x, y, z in key:
        lantern((x, y, z), 46, m, lit=(520.0, 1700.0))
    for _ in range(70):
        x = rng.uniform(-2700, 4100)
        y = rng.uniform(-850, 1850)
        lantern((x, y, rng.uniform(420, 1500)), rng.uniform(26, 44), m)

    # The column: a slow spiral of lanterns climbing out of the court.
    for i in range(260):
        t = i / 260.0
        angle = t * 26.0
        radius = 240 + 2600 * t ** 1.4
        height = 520 + 26000 * t ** 1.25
        size = 36 + 240 * t
        lantern((cup[0] + math.cos(angle) * radius, cup[1] + math.sin(angle) * radius, height), size, m, far=t > 0.12)

    # Far lanterns over the dark land beyond the walls, so the night has depth.
    for _ in range(180):
        angle = rng.uniform(0, 2 * math.pi)
        distance = rng.uniform(7000, 60000)
        lantern((700 + math.cos(angle) * distance, 500 + math.sin(angle) * distance, rng.uniform(300, 0.25 * distance)),
                rng.uniform(0.006, 0.012) * distance, m, far=True)


def course_geometry(course, m):
    for b in course["boxes"]:
        look = b["look"]
        if look == "wall":
            dressed_wall(b, m)
        elif look == "pad":
            side = max(b["size"][0], b["size"][1])
            c.mesh(c.CYLINDER, (b["center"][0], b["center"][1], 0.4), (side * 1.25, side * 1.25, 0.8), material=m["pad"],
                   label=b["name"], folder="Course/Markers", cast_shadow=False)
        else:
            c.box(b["center"], b["size"], m[look], b["name"])


def markers(course, m):
    tee = course["tee"]["position"]
    c.mesh(c.CYLINDER, (tee[0], tee[1], 0.5), (200, 200, 1), material=m["wall"], label="TeeStone", folder="Course/Markers",
           cast_shadow=False)
    cup = course["cup"]
    x, y, z = cup["position"]
    c.mesh(c.CYLINDER, (x, y, z + 0.4), (cup["radius"] * 2 + 16, cup["radius"] * 2 + 16, 0.8), material=m["glyph"],
           label="CupRim", folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 0.9), (cup["radius"] * 2, cup["radius"] * 2, 1), material=m["black"], label="Cup",
           folder="Course/Markers", cast_shadow=False)
    c.mesh(c.CYLINDER, (x, y, z + 130), (2.5, 2.5, 260), material=m["bronze"], label="FlagPole", folder="Course/Markers")
    for machine in course["attractors"]:
        mx, my, _ = machine["position"]
        c.mesh(c.CYLINDER, (mx, my, 0.6), (130, 130, 1), material=m["glyph"], label="MachineDais", folder="Course/Markers",
               cast_shadow=False)


def surroundings(m):
    # Dark ground beyond the hedges, pines against the stars, and the moon.
    c.box((700, 500, -40), (60000, 60000, 40), m["hedge"], "Outer_Ground", folder="Landscape")
    trees = [unreal.load_asset(PINES + letter) for letter in "ABCD"]
    for i in range(26):
        angle = rng.uniform(0, 2 * math.pi)
        distance = rng.uniform(4300, 7500)
        x, y = 700 + math.cos(angle) * distance * 1.25, 500 + math.sin(angle) * distance * 0.75
        if -3000 < x < 4400 and -1100 < y < 2100:
            continue
        actor = c.spawn(unreal.SkeletalMeshActor, (x, y, -20), (0, rng.uniform(0, 360), 0), f"Pine{i}", "Landscape/Pines")
        actor.skeletal_mesh_component.set_skinned_asset_and_update(rng.choice(trees))
        size = rng.uniform(1.0, 1.7)
        actor.set_actor_scale3d(unreal.Vector(size, size, size))
    c.mesh(c.SPHERE, (-900000, -700000, 620000), (150000, 150000, 150000), material=m["moon"], label="Moon",
           folder="Lighting", cast_shadow=False, solid=False)


c.fresh_level(LEVEL)
mats = materials()
# The moon stands behind the tee and to the left; its light comes over the player's shoulder.
c.night_sky(moon_pitch=-28.0, moon_yaw=38.0)
with open(COURSE) as f:
    course_data = json.load(f)
course_geometry(course_data, mats)
markers(course_data, mats)
c.player_start(course_data)
lanterns(course_data, mats)
surroundings(mats)
c.LES.save_current_level()
